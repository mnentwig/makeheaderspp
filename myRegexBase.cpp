#include "myRegexBase.h"

#include <cassert>
#include <cstring>   // strchr
#include <iostream>  // debug
#include <set>
#include <stdexcept>
using std::string, std::map, std::to_string, std::runtime_error, std::vector, std::smatch, std::ssub_match, std::pair, std::cout, std::endl, ::std::set;

// ==========================
// === myRegexBase public ===
// ==========================
// generic regex wrapper with named captures

MHPP("public static")
// match a literal text (escaping regex metacharacters)
myRegexBase myRegexBase::txt(const ::std::string& text) {
    static const char metacharacters[] = R"(\.^$-+()[]{}|?*)";
    std::string out;
    out.reserve(text.size());
    for (auto ch : text) {
        if (std::strchr(metacharacters, ch))
            out.push_back('\\');
        out.push_back(ch);
    }
    return myRegexBase(out, prio_e::PRIO_CONCAT);
}

MHPP("public static")
// create arbitrary regex. Most generic variant, assumes reuse needs to wrap in (?: ...)
myRegexBase myRegexBase::rx(const ::std::string& re) {  // TODO strip default arguments
    return myRegexBase(re, prio_e::PRIO_UNKNOWN);
}

MHPP("public static")
// create regex, special case for an alternation e.g. one|two|three at toplevel
myRegexBase myRegexBase::rx_alt(const ::std::string& re) {
    return myRegexBase(re, prio_e::PRIO_OR);
}

MHPP("public static")
// create regex, special case for a group e.g. (...), (?:...) at toplevel
myRegexBase myRegexBase::rx_grp(const ::std::string& re) {
    return myRegexBase(re, prio_e::PRIO_GRP);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrMore(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "*", prio_e::PRIO_REP);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrMore_lazy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "*?", prio_e::PRIO_REP);
}

MHPP("public static")
myRegexBase myRegexBase::oneOrMore(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "+", prio_e::PRIO_REP);
}

MHPP("public static")
myRegexBase myRegexBase::oneOrMore_lazy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "+?", prio_e::PRIO_REP);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrOne(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "?", prio_e::PRIO_REP);
}

MHPP("public static")
myRegexBase myRegexBase::zeroOrOne_lazy(const myRegexBase& arg) {
    myRegexBase r = arg.makeGrp();
    return r.changeExpr(r.expr + "??", prio_e::PRIO_REP);
}

MHPP("public static")
myRegexBase myRegexBase::capture(const ::std::string& captName, const myRegexBase& arg) {
    myRegexBase r = arg.changeExpr("(" + arg.expr + ")", prio_e::PRIO_GRP);
    r.captureNames.insert(r.captureNames.begin(), captName);  // insert at head as we're wrapping the expression
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

//MHPP("public") // TODO support
myRegexBase myRegexBase::operator+(const myRegexBase& arg) const {
    myRegexBase a = (prio >= prio_e::PRIO_CONCAT) ? *this : makeGrp();
    myRegexBase b = (arg.prio >= prio_e::PRIO_CONCAT) ? arg : arg.makeGrp();
    myRegexBase r = myRegexBase(a.expr + b.expr, prio_e::PRIO_CONCAT);
    for (auto v : a.captureNames) r.captureNames.push_back(v);
    for (auto v : b.captureNames) r.captureNames.push_back(v);
    return r;
}

// M HPP("public") // TODO support
myRegexBase myRegexBase::operator|(const myRegexBase& arg) const {
    // special case: if an argument is an alternation at toplevel, it can be appended without need for grouping
    myRegexBase a = (prio == prio_e::PRIO_OR) ? *this : makeGrp();
    myRegexBase b = (arg.prio == prio_e::PRIO_OR) ? arg : arg.makeGrp();
    myRegexBase r = myRegexBase(a.expr + "|" + b.expr, prio_e::PRIO_OR);
    for (auto v : a.captureNames) r.captureNames.push_back(v);
    for (auto v : b.captureNames) r.captureNames.push_back(v);
    return r;
}

// converts to STL regex
myRegexBase::operator ::std::regex() {  // FIXME support this
    const set<string> captNamesUnique(captureNames.begin(), captureNames.end());
    assert(captNamesUnique.size() == captureNames.size() && "duplicate capture names");
    return ::std::regex(getExpr());
}

MHPP("public")
// returns content as regex string
::std::string myRegexBase::getExpr() const {
    return expr;
}

MHPP("public")
// applies std::regex_match and returns captures by name. Failure to match returns false.
bool myRegexBase::match(const ::std::string& text, ::std::map<::std::string, myRegexBase::range>& captures) {
    std::smatch m;
    if (!std::regex_match(text, m, (std::regex) * this))
        return false;
    captures = smatch2named(m, /*start*/ text.cbegin());
    return true;
}

MHPP("public")
void myRegexBase::allMatches(const ::std::string& text, ::std::vector<myRegexBase::range>& nonMatch, ::std::vector<::std::map<::std::string, myRegexBase::range>>& captures) {
    assert(0 == nonMatch.size());
    assert(0 == captures.size());
    const ::std::regex r = *this;
    // cout << "REGEX: " << getExpr() << endl;
    const string::const_iterator start = text.cbegin();
    std::sregex_iterator it(text.cbegin(), text.cend(), r);
    // cout << "got iterator " << endl; // this part can be slow
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

        map<string, range> oneMatchResult = smatch2named(oneMatch, start);
        trailer = oneMatch[0].second;

        captures.push_back(oneMatchResult);
        ++it;
    }
    nonMatch.push_back(range(start, trailer, text.cend()));
    assert(nonMatch.size() == captures.size() + 1);  // unmatched|match|unmatched|match|...|unmatched
    // cout << "iterator done " << endl;
}

MHPP("public static")
std::string myRegexBase::namedCaptAsString(const std::string& name, std::map<std::string, myRegexBase::range> capt) {
    const myRegexBase::range r = namedCaptAsRange(name, capt);
    return r.str();
}

MHPP("public static")
myRegexBase::range myRegexBase::namedCaptAsRange(const std::string& name, std::map<std::string, myRegexBase::range> capt) {
    auto it = capt.find(name);
    if (it == capt.end())
        throw runtime_error("regex result does not contain named capture '" + name + "'");
    return it->second;
}

MHPP("public static")
myRegexBase myRegexBase::makeGrp(const myRegexBase& arg) {
    return arg.makeGrp();
}

MHPP("public")
myRegexBase myRegexBase::makeGrp() const {
    if (prio >= prio_e::PRIO_GRP) return *this;
    return changeExpr("(?:" + expr + ")", prio_e::PRIO_GRP);
}

// ==============================
// === myRegexBase non-public ===
// ==============================
MHPP("protected")
myRegexBase::myRegexBase(const ::std::string& expr, prio_e prio) : expr(expr), prio(prio) {}

MHPP("protected")
// replaces internal regex, keeps named captures
myRegexBase myRegexBase::changeExpr(const ::std::string& newExpr, prio_e newPrio) const {
    myRegexBase r = myRegexBase(newExpr, newPrio);
    for (auto v : captureNames) r.captureNames.push_back(v);
    return r;
}

MHPP("protected")
// converts STL regex smatch result to named captures
std::map<std::string, myRegexBase::range> myRegexBase::smatch2named(const std::smatch& m, const std::string::const_iterator start) {
    map<string, range> r;

    const size_t nMatch = captureNames.size();

    assert(nMatch + 1 == m.size());

    // insert full match at position 0
    ssub_match m0 = m[0];
    auto q = r.insert({string("all"), range(start, m0.first, m0.second)});
    assert(q.second);

    for (size_t ixMatch = 0; ixMatch < nMatch; ++ixMatch) {
        const string& n = captureNames[ixMatch];
        ssub_match ms = m[ixMatch /*skip full capture*/ + 1];
        auto res = r.insert({n, range(/*origin*/ start, /*begin*/ ms.first, /*end*/ ms.second)});
        assert(/* insertion may never fail (e.g. from duplicate name)*/ res.second);
    }
    return r;
}

// =====================================================
// myRegexBase::range
// =====================================================
MHPP("public")
// construct begin-end range with complete text (e.g. file contents) starting at istart
myRegexBase::range::range(::std::string::const_iterator istart, ::std::string::const_iterator ibegin, ::std::string::const_iterator iend) : istart(istart), ibegin(ibegin), iend(iend) {}

MHPP("public")
// e.g. l100c3 for character 3 in line 100 (base 1, e.g. for messages)
::std::string myRegexBase::range::getLcAnnotString() const {
    size_t ixLineBase1;
    size_t ixCharBase1;
    std::string dest;
    getBeginLineCharBase1(ixLineBase1, ixCharBase1);
    dest += "l" + std::to_string(ixLineBase1) + "c" + std::to_string(ixCharBase1);
    dest += "..";
    getEndLineCharBase1(ixLineBase1, ixCharBase1);
    dest += "l" + std::to_string(ixLineBase1) + "c" + std::to_string(ixCharBase1);
    return dest;
}

MHPP("public")
// extracts range into new string
::std::string myRegexBase::range::str() const {
    return ::std::string(ibegin, iend);
}

MHPP("public")
// gets line and character count relative to beginning of string for start of region
void myRegexBase::range::getBeginLineCharBase1(size_t& ixLineBase1, size_t& ixCharBase1) const {
    getLineCharBase1(ibegin, ixLineBase1, ixCharBase1);
}

MHPP("public")
// gets line and character count relative to beginning of string for end of region
void myRegexBase::range::getEndLineCharBase1(size_t& ixLineBase1, size_t& ixCharBase1) const {
    getLineCharBase1(iend, ixLineBase1, ixCharBase1);
}

MHPP("public")
// start of string that contains the range (e.g. to report line / character count in error messages)
::std::string::const_iterator myRegexBase::range::start() const { return istart; }

MHPP("public")
// start of range in a string beginning at start()
::std::string::const_iterator myRegexBase::range::begin() const { return ibegin; }

MHPP("public")
// end of range in a string beginning at start()
::std::string::const_iterator myRegexBase::range::end() const { return iend; }

MHPP("protected")
// gets line and character count for a given iterator
void myRegexBase::range::getLineCharBase1(::std::string::const_iterator itDest, size_t& ixLineBase1, size_t& ixCharBase1) const {
    ixLineBase1 = 1;
    ixCharBase1 = 1;
    ::std::string::const_iterator it = istart;
    while (it != itDest) {
        char c = *(it++);
        if (c == '\n') {
            ++ixLineBase1;
            ixCharBase1 = 1;
        }
    }
}
