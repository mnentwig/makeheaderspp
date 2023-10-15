#include "stringRegion.h"

#include <cassert>

// note: Using std::reference_wrapper<string> prevents temporaries (caller must guarantee lifetime of string)

MHPP("public")
// construct empty
stringRegion::stringRegion() : sBegin(nullptr), sEnd(nullptr), offsetBegin(0), offsetEnd(0) {}

MHPP("public")
// construct spanning whole s
stringRegion::stringRegion(std::reference_wrapper<const string> s) : sBegin(s.get().cbegin()), sEnd(s.get().cend()), offsetBegin(0), offsetEnd(s.get().size()) {}

MHPP("public")
// construct spanning begin..end in s
stringRegion::stringRegion(std::reference_wrapper<const string> s, string::const_iterator begin, string::const_iterator end) : sBegin(s.get().begin()), sEnd(s.get().end()), offsetBegin(begin - s.get().begin()), offsetEnd(end - s.get().begin()) {
    assert(begin >= sBegin && "given begin iterator is outside string");
    assert(end <= sEnd && "given end iterator is outside string");
    assert(end >= begin && "invalid iterators: end < begin");
}

MHPP("public")
// construct spanning offsetBegin..offsetEnd in s
stringRegion::stringRegion(std::reference_wrapper<const string> s, size_t offsetBegin, size_t offsetEnd) : sBegin(s.get().cbegin()), sEnd(s.get().cend()), offsetBegin(offsetBegin), offsetEnd(offsetEnd) {}

MHPP("public")
// construct spanning offsetBegin..offsetEnd in s
stringRegion::stringRegion(const std::shared_ptr<const string> s, size_t offsetBegin, size_t offsetEnd) : sBegin(s->cbegin()), sEnd(s->cend()), offsetBegin(offsetBegin), offsetEnd(offsetEnd) {}

MHPP("public")
// construct offsetBegin..offsetEnd in sBegin..sEnd
stringRegion::stringRegion(const string::const_iterator sBegin, const string::const_iterator sEnd, size_t offsetBegin, size_t offsetEnd) : sBegin(sBegin), sEnd(sEnd), offsetBegin(offsetBegin), offsetEnd(offsetEnd) {}

MHPP("public")
// construct region of regex submatch in s
stringRegion::stringRegion(std::reference_wrapper<const string> s, const std::ssub_match& subMatch)
    : sBegin(s.get().cbegin()),
      sEnd(s.get().cend()),
      offsetBegin(subMatch.first - s.get().begin()),
      offsetEnd(subMatch.second - s.get().begin()) {
    assert(subMatch.first >= s.get().begin() && "given submatch.first is outside string");
    assert(subMatch.second <= s.get().end() && "given submatch.second is outside string");
    assert(subMatch.second >= subMatch.first && "??? reverse submatch ???");
}

MHPP("private")
// construct region of regex submatch in sBegin..sEnd
stringRegion::stringRegion(const string::const_iterator sBegin, const string::const_iterator sEnd, const std::ssub_match& subMatch) : sBegin(sBegin), sEnd(sEnd), offsetBegin(subMatch.first - sBegin), offsetEnd(subMatch.second - sBegin) {
    assert(subMatch.first >= sBegin && "given submatch.first is outside string");
    assert(subMatch.second <= sEnd && "given submatch.second is outside string");
    assert(subMatch.second >= subMatch.first && "??? reverse submatch ???");
}

MHPP("public")
// construct stringRegion for relative position from src on newS (which must have same size as src)
stringRegion::stringRegion(const stringRegion& src, std::reference_wrapper<const string> newS) : sBegin(newS.get().cbegin()), sEnd(newS.get().cend()), offsetBegin(src.offsetBegin), offsetEnd(src.offsetEnd) {
    assert(src.size() == size() && "attempt to transfer stringRegion to a string of different length");
}

MHPP("public")
// replace underlying string with newS (which must hve same size)
void stringRegion::rebase(std::reference_wrapper<const string> newS) {
    assert(sEnd >= sBegin);
    const size_t baseSize = sEnd - sBegin;
    assert(baseSize == newS.get().size() && "stringRegion rebase() to different length input");
    sBegin = newS.get().cbegin();
    sEnd = newS.get().cend();
}

MHPP("public")
// returns length
size_t stringRegion::size() const {
    assert(offsetEnd >= offsetBegin);
    return offsetEnd - offsetBegin;
}

MHPP("public")
// creates new string with content of referenced region
string stringRegion::str() const {
    return string(sBegin + offsetBegin, sBegin + offsetEnd);
}

MHPP("public")
// std::regex_match on content. Returns matches (matches.size()==0 if fail, otherwise full match plus captures)
vector<stringRegion> stringRegion::regex_match(std::regex r) const {
    std::smatch m;
    bool mres = std::regex_match(sBegin + offsetBegin, sBegin + offsetEnd, m, r);

    vector<stringRegion> match;
    if (mres)
        for (const auto& mm : m)
            match.push_back(stringRegion(sBegin, sEnd, mm));

    return match;
}

MHPP("public")
// regex_search on string region. Returns prefix, matches, postfix (matches.size()==0 if fail, otherwise full match plus captures)
std::tuple<stringRegion, vector<stringRegion>, stringRegion> stringRegion::regex_search(const std::regex& r) const {
    std::smatch m;
    bool mres = std::regex_search(sBegin + offsetBegin, sBegin + offsetEnd, m, r);

    stringRegion prefix(sBegin, sEnd, m.prefix());

    vector<stringRegion> match;

    if (mres)
        for (const auto& mm : m)
            match.push_back(stringRegion(sBegin, sEnd, mm));

    stringRegion suffix(sBegin, sEnd, m.suffix());

    return {prefix, match, suffix};
}

MHPP("public")
// splits input text into sequence of non-matches and matches. Returns {nonMatch, match} with nonMatch.size() == match.size()+1
std::tuple<vector<stringRegion>, vector<vector<stringRegion>>> stringRegion::regexMatchNonMatch(const std::regex& r) const {
    stringRegion cursor = *this;
    vector<stringRegion> retvalNonMatch;
    vector<vector<stringRegion>> retvalMatch;
    while (true) {
        const auto [prefix, match, suffix] = cursor.regex_search(r);
        if (!match.size()) break;

        retvalNonMatch.push_back(prefix);
        retvalMatch.push_back(match);
        cursor = suffix;
    }
    retvalNonMatch.push_back(cursor);
    return {retvalNonMatch, retvalMatch};
}

MHPP("public")
// remaps to new string (or non-const same string) and returns {begin, end} iterators. Note: Non-const variant, use for modifying
std::tuple<string::iterator, string::iterator> stringRegion::beginEnd(string& s) const {
    assert(s.size() == size() && "string size mismatch");
    return {s.begin() + offsetBegin, s.begin() + offsetEnd};
}

MHPP("public")
// returns line-/character position of substring in source
void stringRegion::regionInSource(bool base1, /*out*/ size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd) const {
    size_t lcount = 0;
    size_t ccount = 0;

    // === start ===
    const size_t offset = base1 ? 1 : 0;
    for (size_t ix = 0; ix < offsetBegin; ++ix) {
        const char c = *(sBegin + ix);
        if (c == '\n') {
            ++lcount;
            ccount = 0;
        } else {
            ++ccount;
        }
    }
    lineBegin = lcount + offset;
    charBegin = ccount + offset;

    // === end ===
    for (size_t ix = offsetBegin; ix < offsetEnd; ++ix) {
        const char c = *(sBegin + ix);
        if (c == '\n') {
            ++lcount;
            ccount = 0;
        } else {
            ++ccount;
        }
    }
    lineEnd = lcount + offset;
    charEnd = ccount + offset;
}

// #define TEST_STRINGREGION
#ifdef TEST_STRINGREGION
#include <iostream>
using std::regex, std::cout, std::endl;

void e1() {
    // perform regex match manipulation on input string
    string src("a quick  brown   fox  jumps over   the  lazy     dog");
    string re1("a_quick__brown___fox__jumps_over___the__lazy_____dog");  // result on src

    // perform same manipulation on alternative string
    string alt("                                                    ");  // same length as src
    string re2(" _     __     ___   __     _    ___   __    _____   ");  // result on alt

    stringRegion rSrc(src);
    regex r("\\s+");
    const auto [nonMatch, match] = rSrc.regexMatchNonMatch(r);
    for (const auto& m : match) {
        assert(m.size() == 1 /*only full expression, no capture groups*/);
        // get non-const iterators
        auto [iBegin, iEnd] = m[0].beginEnd(src);
        for (auto it = iBegin; it != iEnd; ++it)
            *it = '_';
        auto [iBegin2, iEnd2] = m[0].beginEnd(alt);
        for (auto it = iBegin2; it != iEnd2; ++it)
            *it = '_';
    }
    assert(src == re1);
    assert(alt == re2);
}

int main(void) {
    e1();
    string src("a quick brown fox jumps over the lazy dog");
    stringRegion rSrc(src);
    regex r1(".*\\s(\\S{5})\\s.*");
    vector<stringRegion> res = rSrc.regex_match(r1);
    for (size_t ix = 0; ix < res.size(); ++ix) cout << ix << "\t" << res[ix].str() << "\n";

    regex r2("\\S{5}");
    stringRegion cursor = rSrc;
    while (true) {
        const auto [prefix, match, suffix] = cursor.regex_search(r2);
        if (!match.size()) break;
        cout << "non-match:" << prefix.str() << endl;
        for (const auto& x : match)
            cout << "match:" << x.str() << endl;
        cursor = suffix;
    }
    cout << "trailing non-match:" << cursor.str() << endl;
    return 0;
}
#endif