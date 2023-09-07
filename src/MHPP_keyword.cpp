#include "MHPP_keyword.h"

#include <cassert>
#include <iostream>  // debug

#include "common.h"

using std::smatch, std::to_string;
#include "regionizedText.h"
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
    t.mask(masked2, regs, regionized::rType_e::BRK_ANG, ' ');            // template parameters
    t.mask(masked2, regs, regionized::rType_e::BRK_RND, ' ', '(', ')');  // remove content of round brackets

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
            common::errmsg(t, s[0].first, s[0].second, filenameForError, "MHPP( failed to locate closing bracket");
        regionized::region rndBrkReg = tmp.back();

        tmp.clear();
        for (const auto& reg : regs)
            if ((reg.getRType() == regionized::rType_e::DQUOTE_BODY) && rndBrkReg.contains(reg))
                tmp.push_back(reg);
        if (tmp.size() != 1)
            common::errmsg(t, s[0].first, s[0].second, filenameForError, "MHPP(...) requires single double-quoted argument (got " + to_string(tmp.size()) + ")");
        regionized::region dblQuotArg = tmp.back();
        std::cout << dblQuotArg.str() << std::endl;
    }
    return ret;
}

MHPP("private static")
// match MHPP (
const std::regex MHPP_keyword::rMHPP = std::regex(R"===(MHPP)==="  // "MHPP"
                                                  R"===(\s*)==="   // maybe whitespace
                                                  R"===(\()==="    // "("
);
