#include "regionized.h"

#include <cassert>
#include <iostream>  // debug
#include <iterator>
#include <memory>
#include <tuple>
#include <vector>
using std::cout, std::endl;  // debug
using std::runtime_error;
using std::string, std::vector, std::tuple, std::shared_ptr;
using std::to_string;

MHPP("public")
regionized::regionized(const csit_t begin, const csit_t end) : regions() {
    auto it = begin;
    it = cursor(it, it, end, /*level*/ 0, regions, /*tExit*/ "", TOPLEVEL);
    assert(it == end);
}

MHPP("public")
// returns all regions (overlapping, in order of parsing, insertion at end of region)
std::vector<regionized::region> regionized::getRegions() const { return regions; }

MHPP("public")
regionized::region::region() : begin(nullptr), end(nullptr), level(0), rType(regionized::rType_e::INVALID) {}
MHPP("public")
regionized::region::region(csit_t begin, csit_t end, size_t level, regionized::rType_e rType) : begin(begin), end(end), level(level), rType(rType) {}
MHPP("public")
std::string regionized::region::str() const { return string(begin, end); }
MHPP("public")
size_t regionized::region::getLevel() const { return level; }
MHPP("public")
csit_t regionized::region::getBegin() const { return begin; }
MHPP("public")
csit_t regionized::region::getEnd() const { return end; }
MHPP("public")
regionized::rType_e regionized::region::getRType() const { return rType; }
MHPP("public")
bool regionized::region::isValid() const { return rType != regionized::rType_e::INVALID; }
MHPP("public")
bool regionized::region::startsAt(csit_t it) const { return it == begin; }
MHPP("public")
bool regionized::region::contains(const regionized::region& arg) const { return (arg.begin >= begin) && (arg.end <= end); }

MHPP("private")
bool regionized::tokenFoundAtIt(const csit_t begin, const csit_t end, const std::string token) {
    csit_t itText = begin;
    csit_t itToken = token.cbegin();
    while (itToken != token.cend()) {
        if (itText == end) return false;
        if (*(itText++) != *(itToken++)) return false;
    }
    return true;
}

MHPP("private")
std::string regionized::getRawStringTerminatorOrDoubleQuote(const csit_t start, const csit_t end) {
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

MHPP("private")
csit_t regionized::cursor(csit_t begin, csit_t beginSearch, csit_t end, size_t level, std::vector<region>& result, const std::string tExit, rType_e rType) {
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
            result.push_back(regionized::region(begin, end, level, rType));
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
                    result.push_back(regionized::region({beginSearch, it, level + 1, DQUOTE_BODY}));

                it += tExit.size();  // include exit token in extracted region
                // a C-style comment is terminated by \n or \r\n, identified by \n as last char in tExit.
                // Move back to leave \n or \r\n as unprocessed text for caller.
                if (tExit.back() == '\n') {
                    --it;
                    if ((it > beginSearch) && (*(it - 1) == '\r'))
                        --it;
                }
                assert(it <= end);
                result.push_back(regionized::region({begin, it, level, rType}));
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
