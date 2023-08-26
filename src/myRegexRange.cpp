#include "myRegexRange.h"

#include <cassert>
#include <iterator>
#include <map>

#include "myRegexBase.h"

using std::string, std::map, std::to_string, std::runtime_error, std::vector, std::smatch, std::ssub_match, std::pair;
// ==========================
// === myRegexRange API
// ==========================

MHPP("public")
// creates root-level object with copy of the original text, managing ownership with substrings (shared_ptr internally)
myRegexRange::myRegexRange(const std::string& text, const std::string& filename)
    : body(std::make_shared<string>(text.cbegin(), text.cend())),
      filename(filename),
      iBegin(body->cbegin()),
      iEnd(body->cend()) {}

MHPP("public")
std::string myRegexRange::str() const { return string(iBegin, iEnd); }

MHPP("public")
// new myRegexRange with substring of source, using iBegin and iEnd from a regex match
myRegexRange myRegexRange::substr(const std::string::const_iterator iBegin, const std::string::const_iterator iEnd) const {
    assert((this->iEnd >= this->iBegin) && "this is reversed");
    assert((iEnd >= iBegin) && "arg is reversed");
    assert((iBegin >= this->iBegin) && "iBegin below string");
    assert((iBegin <= this->iEnd) && "iBegin above string");
    assert((iEnd >= this->iBegin) && "iEnd below string");
    assert((iEnd <= this->iEnd) && "iEnd above string");
    return myRegexRange(*this, iBegin, iEnd);
}

MHPP("public")
// applies std::regex_match and returns captures by position as myRegexRange. Failure to match returns false.
bool myRegexRange::match(const std::regex& rx, std::vector<myRegexRange>& captures) const {
    assert(captures.size() == 0);
    std::smatch m;
    if (!std::regex_match(iBegin, iEnd, m, rx))
        return false;
    for (const auto& it : m)
        captures.push_back(substr(it.first, it.second));
    return true;
}

MHPP("public")
// applies std::regex_match and returns captures by name from list as myRegexRange. Failure to match returns false.
bool myRegexRange::match(const std::regex& rx, const std::vector<std::string>& names, std::map<std::string, myRegexRange>& captures) const {
    assert(captures.size() == 0);
    std::smatch m;
    if (!std::regex_match(iBegin, iEnd, m, rx))
        return false;
    const size_t nCaptFromRegex = m.size();  // including "all" at pos 0
    assert(nCaptFromRegex == names.size() + 1);
    for (size_t ix = 0; ix < nCaptFromRegex; ++ix) {
        const string name = (ix == 0) ? string("all") : names[ix - 1];
        auto r = captures.insert({name, substr(m[ix].first, m[ix].second)});
        assert(r.second && "named match insertion failed. Duplicate name?");
    }
    return true;
}

MHPP("public")
// applies std::regex_match and returns captures by name as myRegexRange. Failure to match returns false.
bool myRegexRange::match(const myRegexBase& rx, std::map<std::string, myRegexRange>& captures) const {
    return match((std::regex)rx, rx.getNames(), captures);
}

MHPP("public")
// split into unmatched|match|unmatched|match|...|unmatched, returns matches (size n) with submatch lists and unmatched(size n+1)
void myRegexRange::splitByMatches(const std::regex& rx, std::vector<myRegexRange>& nonMatch, std::vector<std::vector<myRegexRange>>& captures) const {
    assert(0 == nonMatch.size());
    assert(0 == captures.size());
    std::sregex_iterator it(iBegin, iEnd, rx);
    // cout << "got iterator " << endl; // this part can be slow
    const std::sregex_iterator itEnd;
    auto cursor = iBegin;
    while (it != itEnd) {
        const smatch& oneMatch = *it;

        // uncaptured region up to beginning of match
        nonMatch.push_back(substr(cursor, oneMatch[0].first));

        // cursor moves to end of regex match
        cursor = oneMatch[0].second;

        // collect sub-matches (including all-match at position 0)
        vector<myRegexRange> capts;
        for (auto it2 : oneMatch)
            capts.push_back(substr(it2.first, it2.second));
        captures.push_back(capts);
        ++it;
    }
    nonMatch.push_back(substr(cursor, iEnd));
    // cout << "iterator done " << endl;
}

MHPP("public")
// split into unmatched|match|unmatched|match|...|unmatched, returns matches with named submatches in map(size n) and unmatched(size n+1)
void myRegexRange::splitByMatches(const std::regex& rx, const std::vector<std::string>& names, std::vector<myRegexRange>& nonMatch, std::vector<std::map<std::string, myRegexRange>>& captures) const {
    assert(captures.size() == 0);
    vector<vector<myRegexRange>> rawMatches;
    splitByMatches(rx, nonMatch, rawMatches);

    for (const auto& it : rawMatches) {
        const vector<myRegexRange>& oneRawMatch = it;
        assert(oneRawMatch.size() == names.size() + 1);
        map<string, myRegexRange> rInner;
        for (size_t ix = 0; ix < oneRawMatch.size(); ++ix) {
            if (ix == 0) {
                auto q = rInner.insert({"all", oneRawMatch[ix]});
                assert(q.second);
            } else {
                auto q = rInner.insert({names[ix - 1], oneRawMatch[ix]});
                assert(q.second);
            }
        }
        captures.push_back(rInner);
    }
}

MHPP("public")
// split into unmatched|match|unmatched|match|...|unmatched
void myRegexRange::splitByMatches(const myRegexBase& rx, std::vector<myRegexRange>& nonMatch, std::vector<std::map<std::string, myRegexRange>>& captures) const {
    const std::regex reg = (std::regex)rx;
    const vector<string> names = rx.getNames();
    splitByMatches(reg, names, nonMatch, captures);
}

MHPP("public")
// returns line-/character position of substring in source
void myRegexRange::regionInSource(size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd, std::string& fname, bool base1) const {
    fname = filename;
    size_t lcount = 0;
    size_t ccount = 0;
    size_t offset = base1 ? 1 : 0;
    std::string::const_iterator it = body->cbegin();
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

// ==========================
// === myRegexRange internal
// ==========================
MHPP("private")
myRegexRange::myRegexRange(const myRegexRange& src, std::string::const_iterator iBegin, std::string::const_iterator iEnd)
    : body(src.body),
      filename(src.filename),
      iBegin(iBegin),
      iEnd(iEnd) {}