#include "myRegexBase.h"

#include <cassert>
#include <cstring>   // strchr
#include <iostream>  // debug
#include <iterator>
#include <set>
#include <stdexcept>
using std::string, std::map, std::to_string, std::runtime_error, std::vector, std::smatch, std::ssub_match, std::pair, std::cout, std::endl, std::set;

// ==========================
// === myRegexBase public ===
// ==========================
// generic regex wrapper with named captures

MHPP("public static")
// match a literal text (escaping regex metacharacters)
myRegexBase myRegexBase::txt(const std::string& text) {
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
myRegexBase myRegexBase::rx(const std::string& re) {  // TODO strip default arguments
    return myRegexBase(re, prio_e::PRIO_UNKNOWN);
}

MHPP("public static")
// create regex, special case for an alternation e.g. one|two|three at toplevel
myRegexBase myRegexBase::rx_alt(const std::string& re) {
    return myRegexBase(re, prio_e::PRIO_OR);
}

MHPP("public static")
// create regex, special case for a group e.g. (...), (?:...) at toplevel
myRegexBase myRegexBase::rx_grp(const std::string& re) {
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
myRegexBase myRegexBase::capture(const std::string& captName, const myRegexBase& arg) {
    myRegexBase r = arg.changeExpr("(" + arg.expr + ")", prio_e::PRIO_GRP);
    r.captureNames.insert(r.captureNames.begin(), captName);  // insert at head as we're wrapping the expression
    return r;
}

MHPP("public static")
std::string myRegexBase::replaceAll(std::string text, const std::regex r, const std::string& repl) {
    while (true) {
        string tmp = std::regex_replace(text, r, repl);
        if (tmp == text) return text;
        text = tmp;
    }
}

MHPP("public static")
// splits a string item using regex
std::vector<std::string> myRegexBase::split(const std::string& arg, const std::regex& rx) {
    vector<string> r;
    std::sregex_token_iterator it(arg.cbegin(),
                                  arg.cend(),
                                  rx,
                                  -1);
    std::sregex_token_iterator itEnd;
    bool isFirst = true;
    for (; it != itEnd; ++it) {
        if (isFirst)
            r.push_back(it->str());
        else
            r.push_back(string("\t") + it->str());
    }
    return r;
}

MHPP("public")
const std::vector<std::string> myRegexBase::getNames() const { return captureNames; }

MHPP("public")
std::string myRegexBase::getNamedCapture(const std::string& name, const std::smatch& m) const {
    const size_t nNames = captureNames.size();
    if (m.size() != nNames + 1) throw runtime_error("unexpected number of regex matches (" + to_string(m.size()) + ") expecting " + to_string(nNames) + "+1 captures");
    for (size_t ix = 0; ix < nNames; ++ix)
        if (captureNames[ix] == name)
            return m[ix];
    throw runtime_error("Named match '" + name + "' not found");
}

MHPP("public")
myRegexBase myRegexBase::operator+(const myRegexBase& arg) const {
    myRegexBase a = (prio >= prio_e::PRIO_CONCAT) ? *this : makeGrp();
    myRegexBase b = (arg.prio >= prio_e::PRIO_CONCAT) ? arg : arg.makeGrp();
    myRegexBase r = myRegexBase(a.expr + b.expr, prio_e::PRIO_CONCAT);
    for (auto v : a.captureNames) r.captureNames.push_back(v);
    for (auto v : b.captureNames) r.captureNames.push_back(v);
    return r;
}

MHPP("public")
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
myRegexBase::operator std::regex() const {  // FIXME support this
    const set<string> captNamesUnique(captureNames.begin(), captureNames.end());
    assert(captNamesUnique.size() == captureNames.size() && "duplicate capture names");
    return std::regex(getExpr());
}

MHPP("public")
// returns content as regex string
std::string myRegexBase::getExpr() const {
    return expr;
}

MHPP("public")
// applies std::regex_match and returns captures by name. Failure to match returns false.
bool myRegexBase::match(const std::string& text, std::map<std::string, myRegexBase::range>& captures, const fileLoc& ref) {
    std::smatch m;
    if (!std::regex_match(text, m, (std::regex) * this))
        return false;
    captures = smatch2named(m, ref);
    return true;
}

MHPP("public")
// applies std::regex_match and returns captures by name. Failure to match returns false.
bool myRegexBase::match(const myRegexBase::range& rText, std::map<std::string, myRegexBase::range>& captures) {
    std::smatch m;
    const string& text = rText.str();
    if (!std::regex_match(text, m, (std::regex) * this))
        return false;
    captures = smatch2named(m, rText.getRef());
    return true;
}

MHPP("public")
void myRegexBase::allMatches(const myRegexBase::range& rText, std::vector<myRegexBase::range>& nonMatch, std::vector<std::map<std::string, myRegexBase::range>>& captures) {
    const string& text = rText.str();
    const fileLoc& ref = rText.getRef();
    assert(0 == nonMatch.size());
    assert(0 == captures.size());
    const std::regex r = *this;
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
        nonMatch.push_back(range(text.cbegin(), /* noncaptured section (=begin) */ trailer, /*captured section start (=end)*/ oneMatch[0].first, ref));

        map<string, range> oneMatchResult = smatch2named(oneMatch, ref);
        trailer = oneMatch[0].second;

        captures.push_back(oneMatchResult);
        ++it;
    }
    nonMatch.push_back(range(text.cbegin(), /*range.begin*/ trailer, /*range.end*/ text.cend(), rText.getRef()));
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
myRegexBase::myRegexBase(const std::string& expr, prio_e prio) : expr(expr), prio(prio) {}

MHPP("protected")
// replaces internal regex, keeps named captures
myRegexBase myRegexBase::changeExpr(const std::string& newExpr, prio_e newPrio) const {
    myRegexBase r = myRegexBase(newExpr, newPrio);
    for (auto v : captureNames) r.captureNames.push_back(v);
    return r;
}

MHPP("protected")
// converts STL regex smatch result to named captures
std::map<std::string, myRegexBase::range> myRegexBase::smatch2named(const std::smatch& m, const myRegexBase::fileLoc ref) {
    map<string, range> r;

    const size_t nMatch = captureNames.size();
    assert(nMatch + 1 == m.size());

    // insert full match at position 0
    ssub_match m0 = m[0];

    // ref pointed at input text = m0.
    // exprLoc points at match.
    fileLoc exprLoc = ref.locFromMatch(/*start of string*/ m0.first, /*start of match*/ m0.first, /*end of match*/ m0.second);
    auto q = r.insert({string("all"), range(m0.first, m0.first, m0.second, ref)});
    assert(q.second);

    for (size_t ixMatch = 0; ixMatch < nMatch; ++ixMatch) {
        const string& n = captureNames[ixMatch];
        ssub_match ms = m[ixMatch /*skip full capture*/ + 1];
        auto res = r.insert({n, range(m0.first, ms.first, ms.second, ref)});
        assert(/* insertion may never fail (e.g. from duplicate name)*/ res.second);
    }
    return r;
}

#if false
// =====================================================
// myRegexBase::range
// =====================================================
MHPP("public")
myRegexBase::range::range(const std::string& text, const fileLoc& ref) : text(text), ref(ref) {}

MHPP("public")
myRegexBase::range::range(const std::string& text, const std::string& fname) : text(text), ref(fileLoc(fname, text)) {}

MHPP("public")
// extracts range into new string
std::string myRegexBase::range::str() const {
    return text;
}

MHPP("public")
myRegexBase::fileLoc myRegexBase::range::getRef() const {
    return ref;
}

MHPP("public")
// creates range for iBegin..iEnd when original string starts at iStart and is identified by ref in a source file
myRegexBase::range::range(const std::string::const_iterator iStart, const std::string::const_iterator iBegin, const std::string::const_iterator iEnd, const myRegexBase::fileLoc& aref) : text(iBegin, iEnd), ref(aref) {
    ref = ref.locFromMatch(/*start of string*/ iStart, /*start of match*/ iBegin, /*end of match*/ iEnd);
}

// =====================================================
// myRegexBase::fileLoc
// =====================================================
MHPP("public")
myRegexBase::fileLoc::fileLoc(const string& filename, const string& body) : filename(filename), body(body), offset(0), length(0) {}

MHPP("public")
// get file contents
const std::string& myRegexBase::fileLoc::str() const { return body; }

MHPP("public")
myRegexBase::fileLoc myRegexBase::fileLoc::locFromMatch(const std::string::const_iterator& iStart, const std::string::const_iterator iMatchBegin, const std::string::const_iterator iMatchEnd) const {
    fileLoc r = fileLoc(filename, body);

    // === convert iterators to offset / len ===
    auto d1 = std::distance(iMatchBegin, iStart);
    assert(d1 >= 0);
    size_t delta = (size_t)d1;
    auto d2 = std::distance(iMatchEnd, iMatchBegin);
    assert(d2 >= 0);
    size_t len = (size_t)d2;

    assert(offset + delta < length && "fileLoc startpoint beyond body end");
    assert(offset + delta + len <= length && "fileLoc endpoint beyond body end");
    r.offset += delta;
    r.length = len;
    return r;
}
#endif