#include "myAppRegex.h"

MHPP("public")
myAppRegex::myAppRegex(const myRegexBase& arg) : myRegexBase(arg) {}

MHPP("public static")
myAppRegex myAppRegex::comment() {
    return capture("leadingComment", oneOrMore_greedy(CComment | CppComment)) + wsOpt;
}

MHPP("public static")
myAppRegex myAppRegex::MHPP_classfun() {
    return txt("MHPP(\"") +
           // public or private or protected (all start with "p")
           capture("fun_MHPP_keyword", txt("p") + zeroOrMore_lazy(rx(".", false))) + txt("\")") + wsOpt +
           capture("fun_comment", zeroOrMore_greedy(CComment | CppComment)) + wsOpt +
           // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
           capture("fun_returntype", rx(".*?", false)) +

           // method name
           capture("fun_classmethodname", rx("["
                                             "_"
                                             "~"
                                             ":"
                                             "a-z"
                                             "A-Z"
                                             "0-9"
                                             "]+",
                                             false) |
                                              classOp) +

           // arguments list (may not contain a round bracket)
           capture("fun_arglist", openRoundBracket + rx("[^\\)\\{]*", false) + closingRoundBracket) + wsOpt +
           // constructor initializers
           // "constexpr", "const" qualifiers after arg list
           capture("fun_postArg", rx("[^\\{\\;]*", false)) + wsOpt +
           txt("{");
}

MHPP("public static")
myAppRegex myAppRegex::MHPP_classvar() {
    return txt("MHPP(\"") +
           // public or private or protected (all start with "p")
           capture("var_MHPP_keyword", txt("p") + zeroOrMore_lazy(rx("[a-zA-Z0-9_\\s]", false))) + txt("\")") + wsOpt +
           capture("var_comment", zeroOrMore_greedy(CComment | CppComment)) + wsOpt +
           // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
           capture("var_returntype", rx("[_a-zA-Z0-9<>,:\\s]*?", false)) +

           // name
           capture("var_classvarname",
                   rx("["
                      "_"
                      ":"
                      "a-z"
                      "A-Z"
                      "0-9"
                      "]+",
                      false)) +
           wsOpt + zeroOrOne_greedy(rx("="
                                       ".*",
                                       false)) +
           txt(";");
}

MHPP("public static")
myAppRegex myAppRegex::MHPP_begin() {
    myAppRegex classname = Cidentifier + zeroOrMore_greedy(doubleColon + Cidentifier);

    myAppRegex r =
        // indent of MHPP(...)
        capture("indent", rx("[ \\t]*", false)) +

        // MHPP ("begin myClass::myMethod")
        txt("MHPP(\"begin ") +
        capture("classname1", classname) +
        txt("\")") +

        // existing definitions (to be replaced)
        capture("body", rx("[\\s\\S]*?", false)) +

        // MHPP ("end myClass::myMethod")
        txt("MHPP(\"end ") +
        capture("classname2", classname) +
        txt("\")");

    return r;
}

MHPP("public static")
myAppRegex myAppRegex::classMethodname() {
    myAppRegex r = capture("classname", Cidentifier + zeroOrMore_greedy(doubleColon + Cidentifier)) + doubleColon + capture("methodname", zeroOrOne_greedy(txt("~")) + Cidentifier);
    return r;
}

MHPP("protected static")
myAppRegex myAppRegex::classOp = myAppRegex::rx("operator\\s*[\\+\\-\\*\\/\\(\\)<>|&~]+", false);

MHPP("protected static")
myAppRegex myAppRegex::wsSep = myAppRegex::rx("\\s+", /*isGroup*/ false);

MHPP("protected static")
myAppRegex myAppRegex::openRoundBracket = myAppRegex::txt("(");

MHPP("protected static")
myAppRegex myAppRegex::closingRoundBracket = myAppRegex::txt(")");

MHPP("protected static")
myAppRegex myAppRegex::wsOpt = myAppRegex::rx("\\s*", /*isGroup*/ false);

MHPP("protected static")
myAppRegex myAppRegex::doubleQuote = myAppRegex::txt("\"");

MHPP("protected static")
myAppRegex myAppRegex::Cidentifier = myAppRegex::rx("[_a-zA-Z][_a-zA-Z0-9]*", false);

MHPP("protected static")
myAppRegex myAppRegex::eol = myAppRegex::rx("\\r?\\n", false);

MHPP("protected static")
myAppRegex myAppRegex::CComment = myAppRegex::rx("//.*", false) + eol;

MHPP("protected static")
myAppRegex myAppRegex::CppComment = myAppRegex::rx("/\\*.*?\\*/", false) + eol;

MHPP("protected static")
myAppRegex myAppRegex::doubleColon = txt("::");

MHPP("protected static")
myAppRegex myAppRegex::any = wsOpt + rx(".*", false);  // regex "." does not include newline
