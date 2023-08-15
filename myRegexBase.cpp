#include "myRegexBase.h"

#include <cassert>
#include <cstring>   // strchr
#include <iostream>  // debug
#include <stdexcept>
using std::string, std::map, std::to_string, std::runtime_error, std::vector, std::smatch, std::ssub_match, std::pair, std::cout, std::endl;

// ==========================
// === myRegexBase public ===
// ==========================
// generic regex wrapper with named captures

MHPP("public static")
myRegexBase myRegexBase::txt(const ::std::string& text) {
    static const char metacharacters[] = R"(\.^$-+()[]{}|?*)";
    std::string out;
    out.reserve(text.size());
    for (auto ch : text) {
        if (std::strchr(metacharacters, ch))
            out.push_back('\\');
        out.push_back(ch);
    }
    return myRegexBase(out, /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::rx(const ::std::string& re, bool isGroup) {  // TODO strip default arguments
    return myRegexBase(re, isGroup);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrMore_greedy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "*", /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrMore_lazy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "*?", /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::oneOrMore_greedy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "+", /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::oneOrMore_lazy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "+?", /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrOne_greedy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "?", /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrOne_lazy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "??", /*isGroup*/ false);
}

MHPP("public static")
myRegexBase myRegexBase::capture(const ::std::string& captName, const myRegexBase& arg) {
    myRegexBase r = arg.changeExpr("(" + arg.expr + ")", true);
    r.captureNames.insert(r.captureNames.begin(), captName);
    return r;
}

MHPP("public")
::std::string myRegexBase::getNamedCapture(const ::std::string& name, const ::std::smatch& m) const {
    const size_t nNames = captureNames.size();
    if (m.size() != nNames + 1) throw runtime_error("unexpected number of regex matches (" + to_string(m.size()) + ") expecting " + to_string(nNames) + "+1 captures");
    for (size_t ix = 0; ix < nNames; ++ix)
        if (captureNames[ix] == name)
            return m[ix];
    throw runtime_error("Named match '" + name + "' not found");
}

MHPP("public")
myRegexBase myRegexBase::operator+(const myRegexBase& arg) const {
    const myRegexBase& a = *this;
    myRegexBase b = arg;
    myRegexBase r = myRegexBase(a.expr + b.expr, false);
    for (auto v : a.captureNames) r.captureNames.push_back(v);
    for (auto v : b.captureNames) r.captureNames.push_back(v);
    return r;
}

MHPP("public")
myRegexBase myRegexBase::operator|(const myRegexBase& arg) const {
    myRegexBase a = this->makeGrp();
    myRegexBase b = arg.makeGrp();
    myRegexBase r = myRegexBase(a.expr + "|" + b.expr, false);
    for (auto v : a.captureNames) r.captureNames.push_back(v);
    for (auto v : b.captureNames) r.captureNames.push_back(v);
    return r;
}

myRegexBase::operator ::std::regex() {  // FIXME support this
    return ::std::regex(getExpr());
}

MHPP("public")
::std::string myRegexBase::getExpr() const {
    return expr;
}

MHPP("public")
void myRegexBase::allMatches(const ::std::string& text, ::std::vector<myRegexBase::range>& nonMatch, ::std::vector<::std::map<::std::string, myRegexBase::range>>& captures) {
    assert(0 == nonMatch.size());
    assert(0 == captures.size());
    const ::std::regex r = *this;
    const string::const_iterator start = text.cbegin();
    std::sregex_iterator it(text.cbegin(), text.cend(), r);
    std::sregex_iterator itEnd;
    auto trailer = text.cbegin();
    while (it != itEnd) {
        const smatch& oneMatch = *it;
        size_t nMatch = captureNames.size();  // count does not include "all" capture

        if (nMatch + 1 != oneMatch.size()) {
            string e = "error for regex " + expr + "\ngot " + to_string(oneMatch.size()) + " matches, expecting " + to_string(nMatch) + "+1 matches\nMatches:";
            for (size_t ix = 0; ix < oneMatch.size(); ++ix)
                e += "|" + oneMatch[ix].str() + "|\n";
            e += "capture names:\n";
            for (size_t ix = 0; ix < captureNames.size(); ++ix)
                e += "|" + captureNames[ix] + "|\n";
            throw runtime_error(e);
        }
        nonMatch.push_back(range(/* head of file (origin)*/ start, /* noncaptured section (=begin) */ trailer, /*captured section start (=end)*/ oneMatch[0].first));

        // insert full match at position 0
        ssub_match m0 = oneMatch[0];
        map<string, range> oneMatchResult;
        auto q = oneMatchResult.insert({string("all"), range(start, m0.first, m0.second)});
        assert(q.second);

        trailer = m0.second;
        for (size_t ixMatch = 0; ixMatch < nMatch; ++ixMatch) {
            const string& n = captureNames[ixMatch];
            ssub_match m = oneMatch[ixMatch /*skip full capture*/ + 1];
            auto r = oneMatchResult.insert({n, range(/*origin*/ start, /*begin*/ m.first, /*end*/ m.second)});
            assert(/* insertion may never fail (e.g. from duplicate name)*/ r.second);
        }
        captures.push_back(oneMatchResult);
        ++it;
    }
    nonMatch.push_back(range(start, trailer, text.cend()));
    assert(nonMatch.size() == captures.size() + 1);  // unmatched|match|unmatched|match|...|unmatched
}

// ==============================
// === myRegexBase non-public ===
// ==============================
MHPP("protected")
myRegexBase::myRegexBase(const ::std::string& expr, bool isGroup) : expr(expr), isGroup(isGroup) {}

MHPP("protected")
myRegexBase myRegexBase::makeGrp() const {
    if (this->isGroup) return *this;
    return changeExpr("(?:" + expr + ")", true);
}

MHPP("protected static")
myRegexBase myRegexBase::makeGrp(const myRegexBase& arg) {
    if (arg.isGroup) return arg;
    return arg.changeExpr("(?:" + arg.expr + ")", true);
}

MHPP("protected")
myRegexBase myRegexBase::changeExpr(const ::std::string& newExpr, bool isGroup) const {
    myRegexBase r = myRegexBase(newExpr, isGroup);
    for (auto v : captureNames) r.captureNames.push_back(v);
    return r;
}
