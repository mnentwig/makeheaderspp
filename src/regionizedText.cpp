#include "regionizedText.h"

#include <cassert>
#include <set>
using std::set;
MHPP("public")
regionizedText::regionizedText(const std::string& text) : text(std::make_shared<std::string>(text)), regs(this->text->cbegin(), this->text->cend()) {}

MHPP("public")
vector<regionized::region> regionizedText::getRegions() const { return regs.getRegions(); }

MHPP("public")
regionized::region regionizedText::getRegion(size_t ixRegion) const {
    auto v = regs.getRegions();
    assert(v.size() > ixRegion);
    return v[ixRegion];
}
// begin() iterator into owned text
MHPP("public")
csit_t regionizedText::begin() const { return text->cbegin(); }

MHPP("public")
// end() iterator into owned text
csit_t regionizedText::end() const { return text->cend(); }

MHPP("public")
// returns complete owned text
string regionizedText::str() const { return *text; }

MHPP("public")
// maps region from internal text to "data" and fills with char.
void regionizedText::mask(string& data, const regionized::region& reg, char maskChar) const {
    assert(text->size() == data.size());
    mask(data, reg.begin, reg.end, maskChar);
}

MHPP("public")
// maps region from internal text to "data" and fills with char.
void regionizedText::mask(string& data, const regionized::region& reg, char maskChar, char startChar, char endChar) const {
    assert(text->size() == data.size());
    mask(data, reg.begin, reg.end, maskChar, startChar, endChar);
}

MHPP("public")
// maps regions filtered by rType from internal text to "data" and fills with char
void regionizedText::mask(string& data, const vector<regionized::region>& regions, regionized::rType_e rType, char maskChar) const {
    for (const regionized::region r : regions)
        if (r.getRType() == rType) {
            mask(data, r, maskChar);
        }
}

MHPP("public")
// maps regions filtered by rType from internal text to "data" and fills with char
void regionizedText::mask(string& data, const vector<regionized::region>& regions, regionized::rType_e rType, char maskChar, char startChar, char endChar) const {
    for (const regionized::region r : regions)
        if (r.getRType() == rType) {
            mask(data, r, maskChar, startChar, endChar);
        }
}

MHPP("public")
// returns all regions fully contained in iBegin..iEnd, filtered by rType (rType_e::INVALID selects all)
std::vector<regionized::region> regionizedText::getRegions(csit_t iBegin, csit_t iEnd, rType_e rType) const {
    std::vector<regionized::region> ret;
    for (const regionized::region r : regs.getRegions())
        if ((rType == rType_e::INVALID) || (rType == r.getRType()))
            if ((r.begin >= iBegin) && (r.end <= iEnd))
                ret.push_back(r);
    return ret;
}

MHPP("public static")
// given an iterator it from sOrig, return an iterator to the same position in (same-sized) sDest
csit_t regionizedText::remapIterator(const std::string& sSrc, const std::string& sDest, const csit_t it) {
    assert(sSrc.size() == sDest.size());
    assert(it >= sSrc.cbegin());
    assert(it <= sSrc.cend());
    size_t delta = it - sSrc.cbegin();
    return sDest.cbegin() + delta;
}

MHPP("public")
// given an iterator it from external string sExt, return an iterator to the same position in (same-sized) object text
csit_t regionizedText::remapExtIteratorToInt(const std::string& sExt, const csit_t it) const {
    return remapIterator(/*from*/ sExt, /*to*/ *text, it);
}

MHPP("public")
// given iterators it1, it2 from external string sExt, return iterators to the same position in object text
std::pair<csit_t, csit_t> regionizedText::remapExtIteratorsToInt(const std::string& sExt, const pair<csit_t, csit_t> it) const {
    return {remapIterator(/*from*/ sExt, /*to*/ *text, it.first), remapIterator(/*from*/ sExt, /*to*/ *text, it.second)};
}

MHPP("public")
// given iterators it1, it2 from external string sExt, refer to the same position in object text and return as string
std::string regionizedText::remapExtIteratorsToIntStr(const std::string& sExt, const pair<csit_t, csit_t> it) const {
    return string(remapIterator(/*from*/ sExt, /*to*/ *text, it.first), remapIterator(/*from*/ sExt, /*to*/ *text, it.second));
}

MHPP("public")
// given an iterator it from object text, return an iterator to the same position in an external string sExt
csit_t regionizedText::remapIntIteratorToExt(const std::string& sExt, const csit_t it) const {
    return remapIterator(/*from*/ *text, /*to*/ sExt, it);
}

MHPP("public")
// returns line-/character position of substring in source
void regionizedText::regionInSource(const regionized::region& r, bool base1, /*out*/ size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd) const {
    return regionInSource(r.begin, r.end, base1, lineBegin, charBegin, lineEnd, charEnd);
}

MHPP("public")
// returns line-/character position of substring in source
void regionizedText::regionInSource(csit_t iBegin, csit_t iEnd, bool base1, /*out*/ size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd) const {
    assert(iEnd >= iBegin);
    assert(iBegin >= text->cbegin());
    assert(iEnd <= text->cend());
    size_t lcount = 0;
    size_t ccount = 0;
    const size_t offset = base1 ? 1 : 0;
    csit_t it = text->cbegin();
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

MHPP("private")
void regionizedText::mask(string& data, csit_t maskBegin, csit_t maskEnd, char maskChar) const {
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

MHPP("private")
void regionizedText::mask(string& data, csit_t maskBegin, csit_t maskEnd, char maskChar, char startChar, char endChar) const {
    assert(maskBegin + 2 <= maskEnd);
    assert(maskBegin >= text->cbegin());
    assert(maskEnd <= text->cend());
    const size_t offsetStart = maskBegin - text->cbegin();
    const size_t offsetEnd = maskEnd - text->cbegin();
    auto it = data.begin() + offsetStart;
    const auto itEnd = data.begin() + offsetEnd;

    *(it++) = startChar;
    while (it != itEnd - 1)
        *(it++) = maskChar;
    *(it++) = endChar;
}

MHPP("public static")
void regionizedText::testcases() {
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
