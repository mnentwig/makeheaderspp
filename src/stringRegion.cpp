#include "stringRegion.h"

#include <cassert>
MHPP("public")
// empty region
stringRegion::stringRegion() : sBegin(nullptr), sEnd(nullptr), offsetBegin(0), offsetEnd(0) {}

MHPP("public")
// region spanning whole s
stringRegion::stringRegion(const string& s) : sBegin(s.cbegin()), sEnd(s.cend()), offsetBegin(0), offsetEnd(s.size()) {}

MHPP("public")
// region begin..end in s
stringRegion::stringRegion(const string& s, string::const_iterator begin, string::const_iterator end) : sBegin(s.begin()), sEnd(s.end()), offsetBegin(begin - s.begin()), offsetEnd(end - s.begin()) {
    assert(begin >= sBegin && "given begin iterator is outside string");
    assert(end <= sEnd && "given end iterator is outside string");
    assert(end >= begin && "invalid iterators: end < begin");
}

MHPP("public")
// region of regex submatch in s
stringRegion::stringRegion(const string& s, const std::ssub_match& subMatch) : sBegin(s.cbegin()), sEnd(s.cend()), offsetBegin(subMatch.first - s.begin()), offsetEnd(subMatch.second - s.begin()) {
    assert(subMatch.first >= s.begin() && "given submatch.first is outside string");
    assert(subMatch.second <= s.end() && "given submatch.second is outside string");
    assert(subMatch.second >= subMatch.first && "??? reverse submatch ???");
}

MHPP("private")
// region of regex submatch in s
stringRegion::stringRegion(const string::const_iterator sBegin, const string::const_iterator sEnd, const std::ssub_match& subMatch) : sBegin(sBegin), sEnd(sEnd), offsetBegin(subMatch.first - sBegin), offsetEnd(subMatch.second - sBegin) {
    assert(subMatch.first >= sBegin && "given submatch.first is outside string");
    assert(subMatch.second <= sEnd && "given submatch.second is outside string");
    assert(subMatch.second >= subMatch.first && "??? reverse submatch ???");
}

MHPP("public")
// returns length
size_t stringRegion::size() const {
    return sEnd - sBegin;
}

MHPP("public")
// creates region with same relative position in new string of same size
stringRegion::stringRegion(const stringRegion& src, const string& newS) : sBegin(newS.cbegin()), sEnd(newS.cend()), offsetBegin(src.offsetBegin), offsetEnd(src.offsetEnd) {
    assert(src.size() == size() && "attempt to transfer stringRegion to a string of different length");
}

MHPP("public")
// creates new string with content of referenced region
string stringRegion::str() const {
    return string(sBegin + offsetBegin, sBegin + offsetEnd);
}

MHPP("public")
// std::regex_match on content. Returns matches (matches.size()==0 if fail, otherwise full match plus captures)
vector<stringRegion> stringRegion::regex_match(std::regex r) {
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
std::tuple<stringRegion, vector<stringRegion>, stringRegion> stringRegion::regex_search(const std::regex& r) {
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
std::tuple<vector<stringRegion>, vector<vector<stringRegion>>> stringRegion::regexMatchNonMatch(const std::regex& r) {
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

#define TEST_STRINGREGION
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