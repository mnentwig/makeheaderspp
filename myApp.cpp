#include "myRegexBase.h"
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
    static myAppRegex doubleQuote;
    static myAppRegex Cidentifier;

    static myAppRegex MHPP_begin() {
        myAppRegex classname = Cidentifier;// +
//                        zeroOrMore_greedy() +
//txt("::") + Cidentifier
            // destructor tilde and method name
  //          "::" + capture(zeroOrOne("~")) + capture(CPP_methodName) + wsOpt +

            myAppRegex r = txt("MHPP") + wsOpt + openRoundBracket + doubleQuote + txt("begin") + wsSep + capture();
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
myAppRegex myAppRegex::wsOpt = myAppRegex::rx("\\s*", /*isGroup*/ false);
myAppRegex myAppRegex::doubleQuote = myAppRegex::txt("\"");
myAppRegex myAppRegex::Cidentifier = myAppRegex::rx("[_a-zA-Z][_a-zA-Z0-9]*");
int main() {
    myRegexBase r = myRegexBase::txt("Hello World");
    return 0;
}
