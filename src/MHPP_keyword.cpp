#include "MHPP_keyword.h"

#include <cassert>
#include <iostream>  // debug
#include <stdexcept>
#include <tuple>

#include "common.h"

using std::smatch, std::to_string, std::runtime_error, std::pair;
#include "regionizedText.h"

namespace regexTooling {
string grp(string arg) { return "(?:" + arg + ")"; };
string grp(string qual, string arg) { return "(?:" + arg + ")" + qual; }
string optGrp(string arg) { return "(?:" + arg + ")?"; };
string capture(string arg) { return "(" + arg + ")"; };
const string CIdentifier = string("[a-zA-Z_][a-zA-Z0-9_]*");
// C++ class path separator, allowing whitespace
const string doubleColon = string("(?:\\s*::\\s*)");
pair<size_t, size_t> match2offset(const string& s, const std::ssub_match& m) {
    assert(m.first >= s.cbegin());
    assert(m.second <= s.cend());
    return {m.first - s.cbegin(), m.second - s.cbegin()};
}
string::iterator offset2it(string& s, size_t offset) {
    string::iterator it = s.begin() + offset;
    assert(it <= s.end());
    return it;
}
pair<string::iterator, string::iterator> offset2it(string& s, pair<size_t, size_t> offset) {
    return {offset2it(s, offset.first), offset2it(s, offset.second)};
}
};  // namespace regexTooling

MHPP("public static")
vector<MHPP_keyword> MHPP_keyword::parse(const regionizedText& t, const string& filenameForError) {
    vector<MHPP_keyword> ret;

    // all regions of input file
    std::vector<regionized::region> regs = t.getRegions();

    // === mask irrelevant regions for command regex search (literal strings, comments) ===
    string masked = t.str();
    t.mask(masked, regs, regionized::rType_e::SQUOTE, ' ');
    t.mask(masked, regs, regionized::rType_e::DQUOTE, ' ');
    t.mask(masked, regs, regionized::rType_e::REM_C, ' ');
    t.mask(masked, regs, regionized::rType_e::REM_CPP, ' ');
    string masked2 = masked;
    t.mask(masked2, regs, regionized::rType_e::BRK_ANG, 't', 'T', 'T');  // template parameters <int, int> => TttttttttT for debug only
    t.mask(masked2, regs, regionized::rType_e::BRK_RND, ' ', '(', ')');  // remove content of round brackets

    // mask all #define preprocessor directives via regex ^#\s*define(?:[^\n]*\\\s*\n)*[^\n]*?$
    std::regex dropDefines(R"---(^#\s*define(?:[^\n]*\\\s*\n)*[^\n]*?$)---", std::regex_constants::multiline);
    std::sregex_iterator itB(masked.begin(), masked.end(), dropDefines);
    const std::sregex_iterator itEnd1;  // content-independent end marker
    while (itB != itEnd1) {
        std::cout << (*itB)[0].str() << std::endl;
        auto o = regexTooling::match2offset(masked, (*itB)[0]);
        auto itBegEnd = regexTooling::offset2it(masked, o);
        for (string::iterator it = itBegEnd.first; it != itBegEnd.second; ++it) {
            *it = ' ';
        }
        ++itB;
    }

    // === collect matches for MHPP( ===
    std::sregex_iterator it(masked.begin(), masked.end(), rMHPP);
    const std::sregex_iterator itEnd;  // content-independent end marker
    vector<smatch> MHPP_matches;
    while (it != itEnd) {
        MHPP_matches.push_back(*it);
        ++it;
    }

    // foreach MHPP(
    for (const auto& s : MHPP_matches) {
        assert(s.size() == 1);  // zero additional captures

        // locate the opening round bracket
        csit_t matchLastCharInBlanked = s[0].second - 1;

        // map back to unmasked original file
        csit_t matchLastCharInOrig = t.remapExtIteratorToInt(masked, matchLastCharInBlanked);

        // expect exactly one round bracket region starting at opening round bracket
        vector<regionized::region> tmp;
        for (const auto& reg : regs)
            if (reg.startsAt(matchLastCharInOrig) && reg.getRType() == regionized::rType_e::BRK_RND)
                tmp.push_back(reg);
        assert(tmp.size() <= 1 && "multiple round-bracket regions cannot start at the same character position");
        if (tmp.size() != 1)
            throw runtime_error(common::errmsg(t, t.remapExtIteratorToInt(masked, s[0].first), t.remapExtIteratorToInt(masked, s[0].second), filenameForError, "MHPP( failed to locate closing bracket"));
        regionized::region rndBrkReg = tmp.back();

        // expect exactly one double-quoted region within the round brackets
        tmp.clear();
        for (const auto& reg : regs)
            if ((reg.getRType() == regionized::rType_e::DQUOTE_BODY) && rndBrkReg.contains(reg))
                tmp.push_back(reg);
        if (tmp.size() != 1)
            throw runtime_error(common::errmsg(t, t.remapExtIteratorToInt(masked, s[0].first), t.remapExtIteratorToInt(masked, s[0].second), filenameForError, "MHPP(...) requires single double-quoted argument (got " + to_string(tmp.size()) + ")"));
        regionized::region dblQuotArg = tmp.back();
        std::cout << dblQuotArg.str() << std::endl;

        // start parsing for a C++ declaration after the closing round bracket.
        // There can be a comment which will be copied to the header, to be extracted later
        size_t offsetDeclBody = t.endOffset(rndBrkReg);

        namespace rt = regexTooling;
        const string cvQualifierEatWs =
            rt::grp("*",
                    rt::grp("const\\s+") + "|" +
                        rt::grp("volatile\\s+") + "|" +
                        rt::grp("\\s+"));
        using std::regex_match, std::regex;
        assert(regex_match("  const volatile ", regex(cvQualifierEatWs)));
        assert(regex_match("const const volatile const ", regex(cvQualifierEatWs)));
        assert(!regex_match("const spam volatile ", regex(cvQualifierEatWs)));

        const string retType = "[a-zA-Z_:][a-zA-Z0-9_:\\s]*";

        const string srDecl = string(
            string("\\s*") +  // possible masked comment
            rt::capture(retType + "\\s+") /*+ "?"*/ +
            rt::capture(rt::doubleColon + "?" + rt::grp(rt::CIdentifier + rt::doubleColon) + "*") /*+ "?"*/ +
            rt::capture("~?" + rt::CIdentifier) +
            rt::capture("\\(\\s*\\)") + "?" +
            "[\\s\\S]*"  // tail
        );

        //        const string srDecl = string(
        //            "\\s*" +  // possible masked comment
        //            rt::capture(retType + "\\s+") /*+ "?"*/ +
        //            rt::capture(rt::doubleColon + rt::grp(rt::CIdentifier + rt::doubleColon) + "*") /*+ "?"*/ +
        //            rt::capture("~?" + rt::CIdentifier) +
        //            ".*"  // tail
        //        );
        std::regex rDecl = std::regex(cvQualifierEatWs + srDecl);
        std::cout << srDecl << std::endl;
        string st = string(masked2.cbegin() + offsetDeclBody, masked2.cend());
        std::cout << "trying to match: " << std::endl
                  << st << std::endl;

        std::smatch m;
        if (std::regex_match(st, m, rDecl)) {
            std::cout << "match!" << std::endl;
            for (size_t ix = 0; ix < m.size(); ++ix)
                std::cout << ix << "\t" << m[ix].str() << std::endl;
        }
    }

    return ret;
}

MHPP("private static")
// match MHPP (
const std::regex MHPP_keyword::rMHPP = std::regex(R"===(MHPP)==="  // "MHPP"
                                                  R"===(\s*)==="   // maybe whitespace
                                                  R"===(\()==="    // "("
);
#if false
int main(void) {
    namespace rt = regexTooling;
    string retType = "[a-zA-Z_:][a-zA-Z0-9_:\\s]*";

    const string cvQualifierEatWs =
        rt::grp("*",
                rt::grp("const\\s+") + "|" +
                    rt::grp("volatile\\s+") + "|" +
                    rt::grp("\\s+"));
    using std::regex_match, std::regex;
    assert(regex_match("  const volatile ", regex(cvQualifierEatWs)));
    assert(regex_match("const const volatile const ", regex(cvQualifierEatWs)));
    assert(!regex_match("const spam volatile ", regex(cvQualifierEatWs)));

    const string srDecl = string(
        string("\\s*") +  // possible masked comment
                          //        "([a-zA-Z_]+)" +
        rt::capture(retType + "\\s+") /*+ "?"*/ +
        rt::capture(rt::doubleColon + "?" + rt::grp(rt::CIdentifier + rt::doubleColon) + "*") /*+ "?"*/ +
        rt::capture("~?" + rt::CIdentifier) +
        rt::capture("\\(\\s*\\)") + "?" + cvQualifierEatsWs
//        "[\\s\\S]*"  // tail
    );
    std::regex rDecl = std::regex(srDecl /*, std::regex::extended*/);
    std::cout << srDecl << std::endl;
    string st = R"(
std::mapTttttttttT myClass::myTemplateReturnTypeWithCommaSpace(     ) { return std::mapTttttttttT(); }
)";
    //    st = R"(
    //        int bla
    //        )";
    std::cout << "trying to match: " << std::endl
              << st << std::endl;

    std::smatch m;
    if (std::regex_match(st, m, rDecl)) {
        std::cout << "match!" << std::endl;
        for (size_t ix = 0; ix < m.size(); ++ix)
            std::cout << ix << "\t" << m[ix].str() << std::endl;
    }
    return 0;
}
#endif