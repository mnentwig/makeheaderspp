#include <iostream>
#include <regex>

#include "myRegexBase.h"
using std::cout, std::endl, std::string;
// =========================
// === myAppRegex public ===
// =========================

// rexex wrapper with application-specific "macros" e.g. wsSep for "\s+" or wsOpt for "\s*"
// Main objective is that code inside the class has direct access to members, without requiring any qualifying text input, to improve readability
class myAppRegex : public myRegexBase {
   public:
    myAppRegex(const myRegexBase& arg) : myRegexBase(arg) {}
    static myAppRegex wsSep;
    static myAppRegex wsOpt;
    static myAppRegex openRoundBracket;
    static myAppRegex closingRoundBracket;
    static myAppRegex doubleQuote;
    static myAppRegex Cidentifier;

    static myAppRegex MHPP_begin() {
        myAppRegex classname = zeroOrMore_greedy(txt("::")) + Cidentifier + zeroOrMore_greedy(txt("::") + Cidentifier);
        myAppRegex methodname = zeroOrOne_greedy(txt("~")) + Cidentifier;
        //          "::" + capture(zeroOrOne("~")) + capture(CPP_methodName) + wsOpt +

        myAppRegex r = txt("MHPP") + wsOpt + openRoundBracket + doubleQuote + txt("begin") + wsSep + capture(classname, "classname") + txt("::") + capture(methodname, "methodname") + wsOpt +
                       closingRoundBracket;
        return r;
    }
};
#if 0
           const string oneSpecifier = oneOf({"noexcept", "const"});
            const string CPP_return_type = zeroOrOne("const" + wsSep) + Cidentifier + "[\\&\\*]*";
            const string CPP_methodName = oneOf({Cidentifier, "operator\\s*[^\\s\\(]+"});

            return "MHPP" + wsOpt + literal("(") + wsOpt + capture(zeroOrMore_nonGreedy(any)) + "\"" + wsOpt + "\\)" + wsOpt +

                   zeroOrOne(capture("constexpr") + wsSep) +

                   // method return type (empty for a constructor)
                   zeroOrOne(capture(CPP_return_type) + wsSep) +

                   // class names
                   capture(Cidentifier +
                           zeroOrMore_greedy("::" + Cidentifier)) +

                   // destructor tilde and method name
                   "::" + capture(zeroOrOne("~")) + capture(CPP_methodName) + wsOpt +

                   // arguments list
                   capture(literal("(") +
                           zeroOrMore_nonGreedy(any) +
                           literal(")")) +
                   wsOpt +

                   // initializers (skip)
                   zeroOrOne(
                       // first initializer (separated by ":")
                       ":[^\\{]*?") +

                   // noexcept and const
                   capture(zeroOrOne(nonCaptGroup(oneSpecifier + zeroOrOne(wsSep + oneSpecifier)))) + wsOpt +

                   // opening bracket of definition
                   literal("{");
        }
#endif
myAppRegex myAppRegex::wsSep = myAppRegex::rx("\\s+", /*isGroup*/ false);
myAppRegex myAppRegex::openRoundBracket = myAppRegex::txt("(");
myAppRegex myAppRegex::closingRoundBracket = myAppRegex::txt(")");
myAppRegex myAppRegex::wsOpt = myAppRegex::rx("\\s*", /*isGroup*/ false);
myAppRegex myAppRegex::doubleQuote = myAppRegex::txt("\"");
myAppRegex myAppRegex::Cidentifier = myAppRegex::rx("[_a-zA-Z][_a-zA-Z0-9]*", false);
int main() {
    //myRegexBase r = myRegexBase::txt("Hello World");
    //myAppRegex r2 = myAppRegex::MHPP_begin();
    //cout << r2.getExpr() << endl;

    //::std::regex q(r2.getExpr());
    ::std::smatch m;
    myAppRegex ra = myAppRegex::capture(myAppRegex::rx("MHPP.*", false), "classname");
    ::std::regex regexa = ra;
    string s = "MHPP(\"begin myRegexBase\")";
    cout << ra.getExpr() << endl;
    if (::std::regex_match(s, m, regexa)) {
        cout << "xxx" << ra.getNamedCapture("classname", m) << endl;
    }

    return 0;
}
