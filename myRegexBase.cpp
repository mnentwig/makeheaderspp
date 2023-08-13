#include "myRegexBase.h"

#include <cstring>
using std::string;

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

MHPP("public")
myRegexBase myRegexBase::operator+(const myRegexBase& arg) const {
    myRegexBase a = this->makeGrp();
    myRegexBase b = arg.makeGrp();
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
