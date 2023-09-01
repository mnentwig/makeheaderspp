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
myRegexBase::myRegexBase(const std::string& expr, prio_e prio) : expr(expr), prio(prio), captureNames() {}

MHPP("protected")
// replaces internal regex, keeps named captures
myRegexBase myRegexBase::changeExpr(const std::string& newExpr, prio_e newPrio) const {
    myRegexBase r = myRegexBase(newExpr, newPrio);
    for (auto v : captureNames) r.captureNames.push_back(v);
    return r;
}