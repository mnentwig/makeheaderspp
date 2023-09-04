//  g++ -O0 -g src/bracketizer.cpp -Wall -fmax-errors=1 -static -Wextra -Weffc++ -D_GLIBCXX_DEBUG
#include <cassert>
#include <iostream>  // debug
#include <iterator>
#include <memory>
#include <regex>
#include <set>
#include <tuple>
#include <vector>

#include "regionizedText.h"
using std::cout, std::endl;  // debug
// using std::map;
using std::pair;
using std::regex;
using std::runtime_error;
using std::set;
using std::smatch;
using std::string, std::vector, std::tuple, std::shared_ptr;
using std::to_string;
class regionizedText;
// Parses C(++) code recursively into list of bracketed-/quoted-/comment regions

void dumpRegions(const regionizedText rText) {
    auto reg = rText.getRegions();
    cout << "arrIx\tlevel\trType\rstr\n";
    for (size_t ix = 0; ix < reg.size(); ++ix) {
        regionized::region r = reg[ix];
        cout << ix << "\t" << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;
    }
    cout << endl;
}

void testcases() {
    assert(regionizedText(R"---(hello('\"'))---").getRegion(2).str() == string("hello('\\\"')"));  // escaped quote in char
    assert(regionizedText("hello('a')").getRegion(0).str() == "'a'");
    //    dumpRegions(regionizedText("print(\"she said \\\"hello\\\"\")"));
    auto r = regionizedText("\"she said \\\"hello\\\"\"");
    set<size_t> levels;
    for (auto rr : r.getRegions()) {
        levels.insert(rr.getLevel());
        switch (rr.getLevel()) {
            case 0:
                assert(rr.str() == "\"she said \\\"hello\\\"\"");
                assert(rr.getRType() == regionized::TOPLEVEL);
                break;
            case 1:
                assert(rr.str() == "\"she said \\\"hello\\\"\"");
                assert(rr.getRType() == regionized::DQUOTE);
                break;
            case 2:
                assert(rr.str() == "she said \\\"hello\\\"");
                assert(rr.getRType() == regionized::DQUOTE_BODY);
                break;
            default:
                assert(false);
        }
    }
    assert(levels.size() == 3);

    //   assert(.getRegion(0).str() == "\"she said \\\"hello\\\"\"");  // escaped quote in string
}

std::string errmsg(const regionizedText& body, csit_t iBegin, csit_t iEnd, const string& filename, const string& msg) {
    size_t lineBegin;
    size_t charBegin;
    size_t lineEnd;
    size_t charEnd;
    body.regionInSource(iBegin, iEnd, /*base1*/ true, lineBegin, charBegin, lineEnd, charEnd);
    string ret;
    if (filename.size() > 0)
        ret += filename + string(":");
    ret += "l" + to_string(lineBegin) + "c" + to_string(charBegin);
    if ((lineEnd != lineBegin) || (charEnd != charBegin)) {
        ret += "..l" + to_string(lineEnd) + "c" + to_string(charEnd);
    }

    const size_t nCharMax = 200;
    if (msg.size() > 0)
        ret += ":" + msg;
    if (iEnd > iBegin + nCharMax)
        ret += "\nSource:\n" + string(iBegin, iBegin + nCharMax) + "...";
    else
        ret += "\nSource:\n" + string(iBegin, iEnd);
    return ret;
}

// string raw(R"---(blabla)---");
int main(void) {
    testcases();
    string text(R"---(#include <dummy.cpp>
    /* here it starts */
    int main(void){ // C comment
        while (true){
            cout << "hello" << endl;
        }
        map<string, vector<int>> result;
        string raw(u8R"xxx(blabla)xxx");
    }
    class hello{
MHPP("begin hello")        
MHPP ("end hello")        
MHPP("bla")        
    };
    )---");
    //   text = R"ZZZ(u8R"xxx(blabla)xxx")ZZZ";
    text = R"ZZZ(MHPP("public")
std::map<int, int> myClass::myTemplateReturnTypeWithCommaSpace() { return std::map<int, int>(); })ZZZ";
    regionizedText res = regionizedText(text);
    for (auto r : res.getRegions())
        cout << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;

    //    for (auto r : regionizedText("hello('a')").getRegions())
    //        cout << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;
    cout << endl;
#if 0
    for (auto r : regionizedText(R"---(hello('\"'))---").getRegions())
        cout << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;
    cout << regionizedText(R"---(hello('\"'))---").getRegion(2).str() << endl;
    string a = regionizedText(R"---(hello('\"'))---").getRegion(2).str();
    dumpRegions(regionizedText(R"---(hello('\"'))---"));
#endif
    string blanked = res.str();
    res.mask(blanked, res.getRegions(), regionized::rType_e::DQUOTE, ' ');
    res.mask(blanked, res.getRegions(), regionized::rType_e::REM_C, ' ');
    res.mask(blanked, res.getRegions(), regionized::rType_e::REM_CPP, ' ');
    //   res.mask(blanked, res.getRegions()[0], '-');
    cout << "-----------------\n"
         << blanked << endl;

    // === regex to match MHPP("args") ===
    regex rMHPP(R"===(MHPP)==="      // "MHPP"
                R"===(\s*)==="       // maybe whitespace
                R"===(\()==="        // "("
                R"===(()==="         // capture 1 start
                R"===([^)]*)==="     // anything except literal closing bracket
                R"===())==="         // capture 1 end
                R"===(\))==="        // ")"
                R"===(([^;{]+))==="  // capture 2: anything except ; and {
                R"===(([;{]))==="    // capture 3: ; or {
    );

    // === collect matches for MHPP("args") ===
    std::sregex_iterator it(blanked.cbegin(), blanked.cend(), rMHPP);
    const std::sregex_iterator itEnd;  // content-independent end marker
    vector<smatch> matches;
    while (it != itEnd) {
        matches.push_back(*it);
        ++it;
    }
    for (const auto& s : matches) {
        cout << "MHPP match found:" << endl;
        assert(s.size() == 3 + 1);
        auto match_all = s[0];
        auto match_MHPP_arg = s[1];
        auto match_declaration = s[2];
        auto match_terminator = s[3];
        cout << "blanked:\n";
        cout << string(match_all.first, match_all.second) << endl;
        auto [sAllBegin, sAllEnd] = res.remapExtIteratorsToInt(blanked, match_all);
        cout << "original:\n";
        cout << string(sAllBegin, sAllEnd) << endl;
        pair<string::const_iterator, string::const_iterator> p(match_all);
        string q(p.first, p.second);
        auto [s3begin, s3end] = res.remapExtIteratorsToInt(blanked, match_MHPP_arg);
        vector<regionized::region> MHPP_argV = res.getRegions(s3begin, s3end, regionized::rType_e::DQUOTE_BODY);
        if (MHPP_argV.size() != 1)
            throw runtime_error(errmsg(res, sAllBegin, sAllEnd, "hardcoded", "expecting one double quoted argument in MHPP(...), got " + to_string(MHPP_argV.size()) + "."));
        cout << MHPP_argV[0].str() << endl;

        string term = res.remapExtIteratorsToIntStr(blanked, match_terminator);
        cout << term << endl;
        cout << match_declaration << endl;
        //            res.regionInSource
        //           if (MHPP_argV.size() != 1)throw runtime_error(res.
    }
    return 0;
}