#include "MHPP_keyword.h"

#include <cassert>
#include <iostream>  // debug
#include <stdexcept>
#include <tuple>

#include "common.h"
#include "stringRegion.h"

using std::smatch, std::to_string, std::runtime_error, std::pair;

#include "regionizedText.h"
namespace regexTooling {
string grp(string arg) { return "(?:" + arg + ")"; };
string grp(string qual, string arg) { return "(?:" + arg + ")" + qual; }
string optGrp(string arg) { return "(?:" + arg + ")?"; };
string capture(string arg) { return "(" + arg + ")"; };
string capture(string qual, string arg) { return "(" + arg + ")" + qual; };
const string CIdentifier = string("[a-zA-Z_][a-zA-Z0-9_]*");
// C++ class path separator, allowing whitespace
const string doubleColon = string("(?:\\s*::\\s*)");
pair<size_t, size_t> match2offset(const string &s, const std::ssub_match &m) {
    assert(m.first >= s.cbegin());
    assert(m.second <= s.cend());
    return {m.first - s.cbegin(), m.second - s.cbegin()};
}
string::iterator offset2it(string &s, size_t offset) {
    string::iterator it = s.begin() + offset;
    assert(it <= s.end());
    return it;
}
pair<string::iterator, string::iterator> offset2it(string &s, pair<size_t, size_t> offset) {
    return {offset2it(s, offset.first), offset2it(s, offset.second)};
}
};  // namespace regexTooling

struct parseMethodResult {
    parseMethodResult() : comment(), retValQual(), retType(), methodClassName(), methodName(), methodQualifiers() {}  // -effC++
    stringRegion comment;
    stringRegion retValQual;
    stringRegion retType;
    stringRegion methodClassName;
    stringRegion methodName;
    stringRegion methodQualifiers;
};

static parseMethodResult parseMethod(const stringRegion &rMasked, const string &unmasked, const string &filenameForError) {
    namespace rt = regexTooling;
    using std::regex_match, std::regex;

    const string doubleColonSep = string("\\s*::\\s*");  // :: with possible whitespaces on either side
    const string cName = string("[a-zA-Z_][a-zA-Z0-9_]*");
    const string aType = rt::grp("?", doubleColonSep) + cName + rt::grp("*", doubleColonSep + cName);
    const string maybeTilde = "~?";
    const string cppOperator = "operator\\s*[\\+\\-\\*\\/\\|a-zA-Z0-9_\\^]+";
    const string srDecl =
        // [1] comment / whitespace
        rt::capture("\\s*") +
        // [2] return value qualifiers
        rt::capture("*",          // whole capture is optional
                    rt::grp("+",  // any combination (and repetitions)
                            rt::grp("const\\s+") + "|" +
                                rt::grp("volatile\\s+") + "|" +
                                rt::grp("constexpr\\s+"))) +
        // [3] return type (optional for destructor)
        rt::grp("?", rt::capture(aType) + "\\s+") +

        // [4] method class name
        rt::capture(aType) +

        // :: separating method class and name
        doubleColonSep +

        // [5] method name or operator
        rt::capture(rt::grp(maybeTilde + cName) + "|" +
                    rt::grp(cppOperator)) +

        // [6] method qualifiers (WS required on left side to separate from method name)
        rt::capture(rt::grp("?",          // optional
                            rt::grp("+",  // any combination (and repetitions)
                                    rt::grp("\\s+const") + "|" +
                                        rt::grp("\\s+volatile") + "|" +
                                        rt::grp("\\s+noexcept")))) +
        "\\s*";  // Ws before opening curly bracket of implementation

    std::regex rDecl = std::regex(srDecl);
    std::cout << srDecl << std::endl;
    std::cout << "trying to match: " << std::endl
              << rMasked.str() << std::endl;

    const auto capt = rMasked.regex_match(regex(srDecl));
    if (capt.size() == 0)
        throw runtime_error(common::errmsg(stringRegion(rMasked, unmasked), filenameForError, "failed to parse method (constructor, destructor, operator) declaration"));

    for (size_t ix = 0; ix < capt.size(); ++ix)
        std::cout << ix << "\t" << capt[ix].str() << std::endl;
    parseMethodResult r;
    r.comment = capt[1];
    r.retValQual = capt[2];
    r.retType = capt[3];
    r.methodClassName = capt[4];
    r.methodName = capt[5];
    r.methodQualifiers = capt[6];
    r.comment.rebase(unmasked);
    return r;
}

MHPP("public static")
vector<MHPP_keyword> MHPP_keyword::parse(const regionizedText &t, const string &filenameForError) {
    vector<MHPP_keyword> ret;

    // all regions of input file
    std::vector<regionized::region> regs = t.getRegions();

    // === mask irrelevant regions for command regex search (literal strings, comments) ===
    string unmasked = t.str();
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
        std::cout << (*it)[0].str() << ((*it)[0].first - masked.begin()) << std::endl;
        MHPP_matches.push_back(*it);
        ++it;
    }

    // foreach MHPP(
    for (const auto &s : MHPP_matches) {
        assert(s.size() == 1);  // zero additional captures

        // locate the opening round bracket
        csit_t matchLastCharInBlanked = s[0].second - 1;

        // map back to unmasked original file
        csit_t matchLastCharInOrig = t.remapExtIteratorToInt(masked, matchLastCharInBlanked);

        // expect exactly one round bracket region starting at opening round bracket
        vector<regionized::region> tmp;
        for (const auto &reg : regs)
            if (reg.startsAt(matchLastCharInOrig) && reg.getRType() == regionized::rType_e::BRK_RND)
                tmp.push_back(reg);
        assert(tmp.size() <= 1 && "multiple round-bracket regions cannot start at the same character position");
        if (tmp.size() != 1)
            throw runtime_error(common::errmsg(t, t.remapExtIteratorToInt(masked, s[0].first), t.remapExtIteratorToInt(masked, s[0].second), filenameForError, "MHPP( failed to locate closing bracket"));
        regionized::region rndBrkReg = tmp.back();

        // expect exactly one double-quoted region within the round brackets
        tmp.clear();
        for (const auto &reg : regs)
            if ((reg.getRType() == regionized::rType_e::DQUOTE_BODY) && rndBrkReg.contains(reg))
                tmp.push_back(reg);
        if (tmp.size() != 1)
            throw runtime_error(common::errmsg(t, t.remapExtIteratorToInt(masked, s[0].first), t.remapExtIteratorToInt(masked, s[0].second), filenameForError, "MHPP(...) requires single double-quoted argument (got " + to_string(tmp.size()) + ")"));
        regionized::region dblQuotArg = tmp.back();
        std::cout << dblQuotArg.str() << std::endl;

        // === end of MHPP("") start of C++ declaration ===
        // search for opening round bracket or semicolon

        size_t declBodyOffsetBegin = rndBrkReg.getEnd() - t.begin();
        size_t declBodyOffsetEnd = masked.find_first_of(";=(", declBodyOffsetBegin);
        if (declBodyOffsetEnd == string::npos)
            throw runtime_error(common::errmsg(t, t.remapExtIteratorToInt(masked, s[0].first), t.remapExtIteratorToInt(masked, s[0].second), filenameForError, "MHPP() failed to locate either open curly bracket or semicolon"));
        char cTerm = masked[declBodyOffsetEnd];
        enum { INVALID,
               FUNC,
               VAR } parseVariant =
            (cTerm == '(')   ? FUNC  // argument list: code block must be function
            : (cTerm == '=') ? FUNC  // init value assignment must be var
            : (cTerm == ';') ? VAR   // neither of above at semicolon => variable definition without init value
                             : INVALID;

        switch (parseVariant) {
            case FUNC: {
                stringRegion rDecl(masked2, declBodyOffsetBegin, declBodyOffsetEnd);
                std::cout << rDecl.str() << std::endl;
                parseMethod(rDecl, unmasked, filenameForError);
                // throw runtime_error("done");
                break;
            }
            case VAR:
                break;
            case INVALID:
            default:
                assert(false && "impossible");
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
