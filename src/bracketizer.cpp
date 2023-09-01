#include <cassert>
#include <iostream>  // debug
#include <iterator>
#include <memory>
#include <tuple>
#include <vector>
using std::cout, std::endl;  // debug
using std::string, std::vector, std::tuple, std::shared_ptr;
class regionizedText;
class regionized {
   public:
    class region;
    typedef std::string::const_iterator csit_t;
    typedef enum { TOPLEVEL,
                   BRK_ANG,
                   BRK_RND,
                   BRK_SQU,
                   BRK_CRL,
                   REM_CPP,
                   REM_C,
                   SQUOTE,
                   DQUOTE,
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
    bool stringFound(const csit_t begin, const csit_t end, const string token) {
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
            if ((ntExit > 0) && stringFound(it, end, tExit)) {  // empty string (toplevel): run to end of string
                // skip newline as part of terminator (don't absorb as part of C-comment but leave as whitespace)
                if (tExit.back() == '\n')
                    it += tExit.size() - 1;
                else
                    it += tExit.size();
                assert(it <= end);
                result.push_back({begin, it, level, rType});
                return it;
            }

            if (noRecurse)
                goto skipRecursion;

            // Skip << operator e.g. "cout << endl" to disambiguate from template angle brackets (which can open only one at a time)
            if (stringFound(it, end, string("<<"))) {
                it += 2;
                goto continueMainLoop;
            }

            // search for raw string
            for (const string& rawToken : rawTokens) {
                if (stringFound(it, end, rawToken)) {
                    const string rawTerm = getRawStringTerminatorOrDoubleQuote(it, end);
                    cout << "XXXXXX rawTerm:" << rawTerm << endl;
                    it = cursor(it, it + rawToken.size(), end, level + 1, result, rawTerm, DQUOTE);
                    goto continueMainLoop;
                }
            }

            // search for hierarchic subexpressions
            for (const auto& [left, right, br_rType] : bracketpairs) {
                if (stringFound(it, end, left)) {
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
                //                cout << r.str() << "$" << endl;
                mask(data, r, maskChar);
            }
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

void testcases() {
    assert(regionizedText(R"---(hello('\"'))---").getRegion(2).str() == string("hello('\\\"')"));
    assert(regionizedText("hello('a')").getRegion(0).str() == "'a'");
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
    })---");
    //   text = R"ZZZ(u8R"xxx(blabla)xxx")ZZZ";

    regionizedText res = regionizedText(text);
    for (auto r : res.getRegions())
        cout << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;

    //    for (auto r : regionizedText("hello('a')").getRegions())
    //        cout << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;
    cout << endl;
    for (auto r : regionizedText(R"---(hello('\"'))---").getRegions())
        cout << r.getLevel() << "\t" << r.getRType() << "\t" << r.str() << endl;
    cout << regionizedText(R"---(hello('\"'))---").getRegion(2).str() << endl;
    string a = regionizedText(R"---(hello('\"'))---").getRegion(2).str();
    string b = string("hello('\"')");

    string blanked = res.str();
    res.mask(blanked, res.getRegions(), regionized::rType_e::DQUOTE, 'd');
    res.mask(blanked, res.getRegions(), regionized::rType_e::REM_C, 'c');
    res.mask(blanked, res.getRegions(), regionized::rType_e::REM_CPP, 'p');
    //   res.mask(blanked, res.getRegions()[0], '-');
    cout << "-----------------\n"
         << blanked << endl;

    return 0;
}