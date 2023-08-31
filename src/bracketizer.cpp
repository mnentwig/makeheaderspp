#include <cassert>
#include <iostream>  // debug
#include <iterator>
#include <tuple>
#include <vector>

using std::cout, std::endl, std::tuple;  // debug
using std::string, std::vector;
class regionized {
   public:
    class region;
    typedef std::string::const_iterator csit_t;
    regionized(const csit_t begin, const csit_t end) : regions() {
        auto it = begin;
        it = cursor(it, it, end, /*level*/ 0, regions, /*tExit*/ "", false);
        assert(it == end);
    }
    vector<region> getRegions() const { return regions; }
    class region {
       public:
        region(csit_t begin, csit_t end, size_t level) : begin(begin), end(end), level(level) {}
        string str() const { return string(begin, end); }
        size_t getLevel() const { return level; }

       protected:
        const csit_t begin;
        const csit_t end;
        const size_t level;
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
            if (*it == '(') return ")" + string(itDblQuote + 1, it);
            if (dchar.find(*it) == string::npos) goto fail;
            ++it;
        }
        // fallthrough: maximum length exceeded
    fail:
        // an invalid raw string gets treated as plain double quoted string
        return "\"";
    }

    csit_t cursor(csit_t begin, csit_t beginSearch, csit_t end, size_t level, vector<region>& result, const string tExit, bool rawStringMode) {
        //    cout << "..." << tExit << endl;
        //    cout << "cursor" << level << "'" << string(begin, end) << "' " << tExit << endl;
        assert(beginSearch >= begin);
        assert(beginSearch <= end);

        const vector<std::pair<string, string>>
            bracketpairs({
                {"<", ">"},
                {"(", ")"},
                {"[", "]"},
                {"{", "}"},
                {"/*", "*/"},
                {"//", "\n"},
                {"'", "'"},
                {"L\"", "\""},
                {"u8\"", "\""},
                {"u\"", "\""},
                {"U\"", "\""},
            });

        const vector<string> rawTokens({"R\"", "LR\"", "u8R\"", "uR\"", "UR\""});

        csit_t it = beginSearch;
        size_t ntExit = tExit.size();
        while (true) {
            assert(it <= end);
            if (it == end) {
                result.push_back({begin, end, level});
                return it;
            }

            // check for exit token
            if ((ntExit > 0) && stringFound(it, end, tExit)) {  // empty string (toplevel): run to end of string
                it += tExit.size();
                assert(it <= end);
                result.push_back({begin, it, level});
                return it;
            }

            if (rawStringMode)
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
                    cout << "XXXXXX" << rawTerm << endl;
                    it = cursor(it, it + rawToken.size(), end, level + 1, result, rawTerm, true);
                    goto continueMainLoop;
                }
            }

            // search for hierarchic subexpressions
            for (const auto& [left, right] : bracketpairs) {
                if (stringFound(it, end, left)) {
                    it = cursor(it, it + left.size(), end, level + 1, result, right, false);
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
    regionizedText(const string& text) : text(text), regs(this->text.cbegin(), this->text.cend()) {}
    vector<regionized::region> getRegions() const { return regs.getRegions(); }

   private:
    const string text;
    regionized regs;
};

// string raw(R"---(blabla)---");
int main(void) {
    const string text(R"---(#include <dummy.cpp>
    /* here it starts */
    int main(void){ // C comment
        while (true){
            cout << "hello" << endl;
        }
        map<string, vector<int>> result;
        string raw(u8R"xxx(blabla)xxx");
    })---");
    regionizedText res = regionizedText(text);
    for (auto r : res.getRegions()) {
        cout << r.getLevel() << "\t" << r.str() << endl;
    }

    return 0;
}