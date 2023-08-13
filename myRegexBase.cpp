#include "myRegexBase.h"

#include <cstring>  // strchr
#include <iostream> // debug
#include <stdexcept>
using std::string, std::map, std::to_string, std::runtime_error;

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
myRegexBase myRegexBase::rx(const ::std::string& re, bool isGroup) {
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
myRegexBase myRegexBase::capture(const myRegexBase& arg, const ::std::string& captName) {
    myRegexBase r = arg.changeExpr("(" + arg.expr + ")", true);
    r.captureNames.insert(r.captureNames.begin(), captName);
    return r;
}

MHPP("public")
::std::string myRegexBase::getNamedCapture(const ::std::string& name, const ::std::smatch& m) const {
    const size_t nNames = captureNames.size();
    for (auto n : captureNames) ::std::cout << n << ::std::endl;
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

myRegexBase::operator ::std::regex() { // FIXME support this
    return ::std::regex(getExpr());
}

MHPP("public")
::std::string myRegexBase::getExpr() const {
    return expr;
}

//MHPP("public")
//bool matchAny

// ==============================
// === myRegexBase non-public ===
// ==============================
MHPP("protected")
myRegexBase::myRegexBase(const ::std::string& expr, bool isGroup) : expr(expr), isGroup(isGroup) {}

MHPP("protected")
myRegexBase myRegexBase::makeGrp() const {
    if (this->isGroup) return *this;
    return myRegexBase("(:?" + expr + ")", true);
}

MHPP("protected")
myRegexBase myRegexBase::changeExpr(const ::std::string& newExpr, bool isGroup) const {
    myRegexBase r = myRegexBase(newExpr, isGroup);
    for (auto v : captureNames) r.captureNames.push_back(v);
    return r;
}
