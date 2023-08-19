#include "myAppRegex.h"

#include <iostream>
MHPP("public")
myAppRegex::myAppRegex(const myRegexBase& arg) : myRegexBase(arg) {}

MHPP("public static")
myAppRegex myAppRegex::comment() {
    return capture("leadingComment", oneOrMore(CComment | CppComment)) + wsOpt;
}

MHPP("public static")
myAppRegex myAppRegex::MHPP_classfun() {
    return txt("MHPP(\"") +
           // public or private or protected (all start with "p")
           capture("fun_MHPP_keyword", txt("p") + zeroOrMore_lazy(rx("."))) + txt("\")") + wsOpt +
           capture("fun_comment", zeroOrMore(CComment | CppComment)) + wsOpt +
           // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
           zeroOrOne(
               capture("fun_returntype", CppTemplatedType) + wsSep) +
           // method name
           capture("fun_classmethodname", CppClassname + doubleColon + makeGrp(CppMethodname | CppOperator)) +

           // arguments list (may not contain a round bracket)
           capture("fun_arglist", openRoundBracket + rx("[^\\)\\{]*") + closingRoundBracket) + wsOpt +
           // constructor initializers
           // "constexpr", "const" qualifiers after arg list
           capture("fun_postArg", rx("[^\\{\\;]*")) + wsOpt +
           txt("{");
}

MHPP("public static")
myAppRegex myAppRegex::MHPP_classvar() {
    return txt("MHPP(\"") +
           // public or private or protected (all start with "p")
           capture("var_MHPP_keyword", txt("p") + zeroOrMore_lazy(rx("[a-zA-Z0-9_\\s]"))) + txt("\")") + wsOpt +
           capture("var_comment", zeroOrMore(CComment | CppComment)) + wsOpt +
           // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
           capture("var_returntype", rx("[_a-zA-Z0-9<>,:\\s]*?")) +

           // name
           capture("var_classvarname",
                   rx("["
                      "_"
                      ":"
                      "a-z"
                      "A-Z"
                      "0-9"
                      "]+")) +
           wsOpt + zeroOrOne(rx("="
                                ".*")) +
           txt(";");
}

MHPP("public static")
myAppRegex myAppRegex::MHPP_begin() {
    myAppRegex classname = Cidentifier + zeroOrMore(doubleColon + Cidentifier);

    myAppRegex r =
        // indent of MHPP(...)
        capture("indent", rx("[ \\t]*")) +

        // MHPP ("begin myClass::myMethod")
        txt("MHPP(\"begin ") +
        capture("classname1", classname) +
        txt("\")") +

        // existing definitions (to be replaced)
        capture("body", rx("[\\s\\S]*?")) +

        // MHPP ("end myClass::myMethod")
        txt("MHPP(\"end ") +
        capture("classname2", classname) +
        txt("\")");

    return r;
}

MHPP("public static")
myAppRegex myAppRegex::classMethodname() {
    myAppRegex r = capture("classname",
                           Cidentifier +
                               zeroOrMore(doubleColon + Cidentifier)) +
                   doubleColon +
                   capture("methodname",
                           makeGrp(zeroOrOne(txt("~")) +
                                   Cidentifier) |
                               CppOperator);
    std::cout << r.getExpr() << std::endl;
    return r;
}

MHPP("protected static")
myAppRegex myAppRegex::CppOperator = myAppRegex::rx("operator\\s*[\\+\\-\\*\\/\\(\\)<>|&~]+");

MHPP("protected static")
myAppRegex myAppRegex::wsSep = myAppRegex::rx("\\s+");

MHPP("protected static")
myAppRegex myAppRegex::openRoundBracket = myAppRegex::txt("(");

MHPP("protected static")
myAppRegex myAppRegex::closingRoundBracket = myAppRegex::txt(")");

MHPP("protected static")
myAppRegex myAppRegex::wsOpt = myAppRegex::rx("\\s*");

MHPP("protected static")
myAppRegex myAppRegex::doubleQuote = myAppRegex::txt("\"");

MHPP("protected static")
myAppRegex myAppRegex::Cidentifier = myAppRegex::rx("[_a-zA-Z][_a-zA-Z0-9]*");

MHPP("protected static")
myAppRegex myAppRegex::eol = myAppRegex::rx("\\r?\\n");

MHPP("protected static")
myAppRegex myAppRegex::CComment = myAppRegex::rx("//.*") + eol;

MHPP("protected static")
myAppRegex myAppRegex::CppComment = myAppRegex::rx("/\\*.*?\\*/") + eol;

MHPP("protected static")
myAppRegex myAppRegex::doubleColon = txt("::");

MHPP("protected static")
myAppRegex myAppRegex::CppIdentifierFirstChar = myAppRegex::rx("[_a-zA-Z]");

MHPP("protected static")
myAppRegex myAppRegex::CppTemplatedTypeFirstChar = myAppRegex::rx("[:_a-zA-Z]");

MHPP("protected static")
myAppRegex myAppRegex::CppTemplatedTypeBodyChar = myAppRegex::rx("[_a-zA-Z0-9<>,\\s:]");

MHPP("protected static")
myAppRegex myAppRegex::CppTemplatedTypeLastChar = myAppRegex::rx("[_a-zA-Z0-9>]");

MHPP("public static")
myAppRegex myAppRegex::CppTemplatedType = myAppRegex::rx("[a-zA-Z0-9_<>,\\s:\\*&]+");

MHPP("protected static")
myAppRegex myAppRegex::CppClassname = myAppRegex::rx("[_a-zA-Z0-9:]+");

MHPP("protected static")
myAppRegex myAppRegex::CppMethodname = myAppRegex::makeGrp(myAppRegex::zeroOrMore(myAppRegex::txt("~")) + Cidentifier);

MHPP("protected static")
myAppRegex myAppRegex::any = wsOpt + rx(".*");  // regex "." does not include newline