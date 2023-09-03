//  g++ -O0 -g src/bracketizer.cpp -Wall -fmax-errors=1 -static -Wextra -Weffc++ -D_GLIBCXX_DEBUG
#include <cassert>
#include <iostream>  // debug
#include <iterator>
#include <memory>
#include <regex>
#include <set>
#include <tuple>
#include <vector>
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
class regionized {
   public:
    class region;
    typedef std::string::const_iterator csit_t;
    typedef enum { INVALID,
                   TOPLEVEL,
                   BRK_ANG,
                   BRK_RND,
                   BRK_SQU,
                   BRK_CRL,
                   REM_CPP,
                   REM_C,
                   SQUOTE,
                   DQUOTE,
                   DQUOTE_BODY,
                   TOP } rType_e;

    regionized(const csit_t begin, const csit_t end) : regions() {
        auto it = begin;
        it = cursor(it, it, end, /*level*/ 0, regions, /*tExit*/ "", TOPLEVEL);
        assert(it == end);
    }
    // returns all regions (overlapping, in order of parsing, insertion at end of region)
    vector<region> getRegions() const { return regions; }
    class region {
        friend regionizedText;  // to expose iterators

       public:
        region(csit_t begin, csit_t end, size_t level, regionized::rType_e rType) : begin(begin), end(end), level(level), rType(rType) {}
        string str() const { return string(begin, end); }
        size_t getLevel() const { return level; }
        rType_e getRType() const { return rType; }

       protected:
        const csit_t begin;
        const csit_t end;
        const size_t level;
        rType_e rType;
    };

   private:
    bool tokenFoundAtIt(const csit_t begin, const csit_t end, const string token) {
        csit_t itText = begin;
        csit_t itToken = token.cbegin();
        while (itToken != token.cend()) {
            if (itText == end) return false;
            if (*(itText++) != *(itToken++)) return false;
        }
        return true;
    }

    string getRawStringTerminatorOrDoubleQuote(const csit_t start, const csit_t end) {
        csit_t it = start;
        const string dchar = string(R"--(!"#%&'*+,-./0123456789:;=?ABCDEFGHIJKLMNOPQRSTUVWXYZ^_abcdefghijklmnopqrstuvwxyz|~$@`)--");
        // skip u8R in u8R" (advance it to double quote)
        for (size_t ix = 0; ix < 3; ++ix) {
            if (*it == '\"') break;
            assert(it < end);
            ++it;
        }
        assert(*it == '\"');
        csit_t itDblQuote = it;
        for (size_t ix = 0; ix < 16; ++ix) {
            if (it == end) goto fail;
            if (*it == '(') return ")" + string(itDblQuote + 1, it) + "\"";
            if (dchar.find(*it) == string::npos) goto fail;
            ++it;
        }
        // fallthrough: maximum length exceeded
    fail:
        // an invalid raw string gets treated as plain double quoted string
        return "\"";
    }

    csit_t cursor(csit_t begin, csit_t beginSearch, csit_t end, size_t level, vector<region>& result, const string tExit, rType_e rType) {
        //    cout << "..." << tExit << endl;
        //    cout << "cursor" << level << "'" << string(begin, end) << "' " << tExit << endl;
        assert(beginSearch >= begin);
        assert(beginSearch <= end);
        bool noRecurse = (rType == DQUOTE) || (rType == SQUOTE) || (rType == REM_C) || (rType == REM_CPP);  // strings and comments are lowest hierarchy level

        const vector<std::tuple<string, string, rType_e>>
            bracketpairs({{"<", ">", BRK_ANG},
                          {"(", ")", BRK_RND},
                          {"[", "]", BRK_SQU},
                          {"{", "}", BRK_CRL},
                          {"/*", "*/", REM_CPP},
                          {"//", "\n", REM_C},
                          {"'", "'", SQUOTE},
                          {"\"", "\"", DQUOTE},
                          {"L\"", "\"", DQUOTE},
                          {"u8\"", "\"", DQUOTE},
                          {"u\"", "\"", DQUOTE},
                          {"U\"", "\"", DQUOTE}});

        const vector<string> rawTokens({"R\"", "LR\"", "u8R\"", "uR\"", "UR\""});

        csit_t it = beginSearch;
        size_t ntExit = tExit.size();
        bool stringBackslashEscape = false;
        while (true) {
            assert(it <= end);

            if (it == end) {
                result.push_back({begin, end, level, rType});
                return it;
            }

            // backslash-escaped character: Skipping the next char for end detection
            if (stringBackslashEscape) {
                stringBackslashEscape = false;
                ++it;
                goto continueMainLoop;
            }

            // backslash-escaped next character (disabled in raw mode)
            if (*it == '\\' && ((rType == SQUOTE) || ((rType == DQUOTE) && (tExit.size() == 1)))) {
                stringBackslashEscape = true;
                ++it;
                goto continueMainLoop;
            }

            // check for exit token
            if (ntExit > 0)                            // empty tExit flags toplevel: run to end of string
                if (tokenFoundAtIt(it, end, tExit)) {  // exit token at it
                    if (rType == DQUOTE)
                        result.push_back({beginSearch, it, level + 1, DQUOTE_BODY});

                    it += tExit.size();  // include exit token in extracted region
                    // a C-style comment is terminated by \n or \r\n, identified by \n as last char in tExit.
                    // Move back to leave \n or \r\n as unprocessed text for caller.
                    if (tExit.back() == '\n') {
                        --it;
                        if ((it > beginSearch) && (*(it - 1) == '\r'))
                            --it;
                    }
                    assert(it <= end);
                    result.push_back({begin, it, level, rType});
                    return it;
                }

            if (noRecurse)
                goto skipRecursion;

            // Skip << operator e.g. "cout << endl" to disambiguate from template angle brackets (which can open only one at a time)
            if (tokenFoundAtIt(it, end, string("<<"))) {
                it += 2;
                goto continueMainLoop;
            }

            // search for raw string
            for (const string& rawToken : rawTokens) {
                if (tokenFoundAtIt(it, end, rawToken)) {
                    const string rawTerm = getRawStringTerminatorOrDoubleQuote(it, end);
                    cout << "XXXXXX rawTerm:" << rawTerm << endl;
                    it = cursor(it, it + rawToken.size(), end, level + 1, result, rawTerm, DQUOTE);
                    goto continueMainLoop;
                }
            }

            // search for hierarchic subexpressions
            for (const auto& [left, right, br_rType] : bracketpairs) {
                if (tokenFoundAtIt(it, end, left)) {
                    it = cursor(it, it + left.size(), end, level + 1, result, right, br_rType);
                    goto continueMainLoop;
                }
            }

        skipRecursion:
            ++it;
        continueMainLoop:;
        }  // while true
    }

    vector<region> regions;
};

class regionizedText {
   public:
    typedef std::string::const_iterator csit_t;
    typedef regionized::rType_e rType_e;
    regionizedText(const string& text) : text(std::make_shared<string>(text)), regs(this->text->cbegin(), this->text->cend()) {}
    vector<regionized::region> getRegions() const { return regs.getRegions(); }
    regionized::region getRegion(size_t ixRegion) const {
        auto v = regs.getRegions();
        assert(v.size() > ixRegion);
        return v[ixRegion];
    }
    csit_t begin() const { return text->cbegin(); }
    csit_t end() const { return text->cend(); }
    string str() const { return string(text->cbegin(), text->cend()); }
    void mask(string& data, const regionized::region& reg, char maskChar = ' ') {
        mask(data, reg.begin, reg.end, maskChar);
    }
    void mask(string& data, const vector<regionized::region>& regions, regionized::rType_e rType, char maskChar = ' ') {
        for (const regionized::region r : regions)
            if (r.getRType() == rType) {
                mask(data, r, maskChar);
            }
    }

    // returns all regions fully included in iBegin..iEnd, optionally filtered by rType
    std::vector<regionized::region> getRegions(csit_t iBegin, csit_t iEnd, rType_e rType = rType_e::INVALID) {
        std::vector<regionized::region> ret;
        for (const regionized::region r : regs.getRegions())
            if ((rType == rType_e::INVALID) || (rType == r.getRType()))
                if ((r.begin >= iBegin) && (r.end <= iEnd))
                    ret.push_back(r);
        return ret;
    }

    // given an iterator it from sOrig, return an iterator to the same position in sDest (which must have the same length)
    static std::string::const_iterator remapIterator(const std::string& sSrc, const std::string& sDest, const std::string::const_iterator it) {
        assert(sSrc.size() == sDest.size());
        assert(it >= sSrc.cbegin());
        assert(it <= sSrc.cend());
        size_t delta = it - sSrc.cbegin();
        return sDest.cbegin() + delta;
    }

    // given an iterator it from external string sExt, return an iterator to the same position in object text
    std::string::const_iterator remapExtIteratorToInt(const std::string& sExt, const std::string::const_iterator it) {
        return remapIterator(/*from*/ sExt, /*to*/ *text, it);
    }

    // given iterators it1, it2 from external string sExt, return iterators to the same position in object text
    std::pair<std::string::const_iterator, std::string::const_iterator> remapExtIteratorsToInt(const std::string& sExt, const pair<std::string::const_iterator, std::string::const_iterator> it) {
        return {remapIterator(/*from*/ sExt, /*to*/ *text, it.first), remapIterator(/*from*/ sExt, /*to*/ *text, it.second)};
    }

    // given iterators it1, it2 from external string sExt, refer to the same position in object text and return as string
    std::string remapExtIteratorsToIntStr(const std::string& sExt, const pair<std::string::const_iterator, std::string::const_iterator> it) {
        return string(remapIterator(/*from*/ sExt, /*to*/ *text, it.first), remapIterator(/*from*/ sExt, /*to*/ *text, it.second));
    }

    // given an iterator it from object text, return an iterator to the same position in an external string sExt
    std::string::const_iterator remapIntIteratorToExt(const std::string& sExt, const std::string::const_iterator it) {
        return remapIterator(/*from*/ *text, /*to*/ sExt, it);
    }

    // returns line-/character position of substring in source
    void regionInSource(const regionized::region& r, bool base1, /*out*/ size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd) const {
        return regionInSource(r.begin, r.end, base1, lineBegin, charBegin, lineEnd, charEnd);
    }

    // returns line-/character position of substring in source
    void regionInSource(csit_t iBegin, csit_t iEnd, bool base1, /*out*/ size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd) const {
        assert(iEnd >= iBegin);
        assert(iBegin >= text->cbegin());
        assert(iEnd <= text->cend());
        size_t lcount = 0;
        size_t ccount = 0;
        const size_t offset = base1 ? 1 : 0;
        std::string::const_iterator it = text->cbegin();
        while (it != iBegin) {
            if (*it == '\n') {
                ++lcount;
                ccount = 0;
            } else {
                ++ccount;
            }
            ++it;
        }
        lineBegin = lcount + offset;
        charBegin = ccount + offset;
        while (it != iEnd) {
            if (*it == '\n') {
                ++lcount;
                ccount = 0;
            } else {
                ++ccount;
            }
            ++it;
        }
        lineEnd = lcount + offset;
        charEnd = ccount + offset;
    }

   private:
    void mask(string& data, csit_t maskBegin, csit_t maskEnd, char maskChar) const {
        assert(maskBegin <= maskEnd);
        assert(maskBegin >= text->cbegin());
        assert(maskEnd <= text->cend());
        const size_t offsetStart = maskBegin - text->cbegin();
        const size_t offsetEnd = maskEnd - text->cbegin();
        auto it = data.begin() + offsetStart;
        const auto itEnd = data.begin() + offsetEnd;
        while (it != itEnd)
            *(it++) = maskChar;
    }

    const shared_ptr<const string> text;  // may never change (would invalidate region iterators)
    regionized regs;
};

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

std::string errmsg(const regionizedText& body, regionized::csit_t iBegin, regionized::csit_t iEnd, const string& filename, const string& msg) {
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
    res.mask(blanked, res.getRegions(), regionized::rType_e::DQUOTE);
    res.mask(blanked, res.getRegions(), regionized::rType_e::REM_C);
    res.mask(blanked, res.getRegions(), regionized::rType_e::REM_CPP);
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
         pair <string::const_iterator, string::const_iterator> p(match_all);
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