#include "myRegex.h"

#include <cstring>
using std::string;

MHPP("public static")
myRegex myRegex::txt(const ::std::string& text) {
    static const char metacharacters[] = R"(\.^$-+()[]{}|?*)";
    std::string out;
    out.reserve(text.size());
    for (auto ch : text) {
        if (std::strchr(metacharacters, ch))
            out.push_back('\\');
        out.push_back(ch);
    }
    return myRegex(out, /*isGroup*/ false);
}

MHPP("public")
myRegex myRegex::operator+(const myRegex& arg) const {
    myRegex a = this->makeGrp();
    myRegex b = arg.makeGrp();
    myRegex r = myRegex(a.expr + b.expr, false);
    for (auto v : a.captureNames) r.captureNames.push_back(v);
    for (auto v : b.captureNames) r.captureNames.push_back(v);
    return r;
}

MHPP("public")
const void* myRegex::test(){}

MHPP("public static")
myRegex myRegex::regex(const ::std::string& re, bool isGroup) {
    return myRegex(re, isGroup);
}

MHPP("public")
myRegex myRegex::capture(const ::std::string& name) {
    myRegex r = myRegex("(" + expr + ")", true);
    r.captureNames.push_back(name);
    for (auto v : captureNames) r.captureNames.push_back(v);
    return r;
}

// ==================
// === non-public ===
// ==================
MHPP("protected")
myRegex::myRegex(const ::std::string& expr, bool isGroup) : expr(expr), isGroup(isGroup) {}

MHPP("protected")
myRegex myRegex::makeGrp() const {
    if (this->isGroup) return *this;
    return myRegex("(:?" + expr + ")", true);
}

int main() {
    myRegex r = myRegex::txt("Hello World");
    return 0;
}
