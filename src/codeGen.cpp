#include "codegen.h"

using std::vector, std::string, std::runtime_error, std::map, std::cout, std::endl, std::regex, std::to_string;
MHPP("public")
codeGen::codeGen(bool annotate) : annotate(annotate) {}

MHPP("public")
void codeGen::pass1(const std::string& fname, bool clean) {
    // === read file contents ===
    string all = readFile(fname);
    myRegexRange rall = myRegexRange(all, fname);
    auto r = filebodyByFilename.insert({fname, rall});
    if (!r.second) throw runtime_error("duplicate filename: '" + fname + "'");

    if (!clean) {
        // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
        myAppRegex rx = myAppRegex::comment().makeGrp() | myAppRegex::MHPP_classfun().makeGrp() | myAppRegex::MHPP_classvar().makeGrp();

        vector<myRegexRange> nonCapt;
        vector<map<string, myRegexRange>> capt;
        rall.splitByMatches(rx, nonCapt, capt);

        for (const auto& a : capt)
            MHPP_classitem(a);
    }
}

MHPP("public")
void codeGen::pass2(const std::string& fname, bool clean) {
    // === retrieve original file contents ===
    auto it = filebodyByFilename.find(fname);
    assert(it != filebodyByFilename.end());
    const myRegexRange& all = it->second;
    // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
    myAppRegex rx = myAppRegex::MHPP_begin();
    vector<myRegexRange> nonCapt;
    vector<map<string, myRegexRange>> capt;
    all.splitByMatches(rx, nonCapt, capt);

    // === replace old file content between MHPP ("begin classname")...MHPP ("end classname") with respective classname's declarations ===
    const size_t nCapt = capt.size();
    if (nCapt == 0)
        return;
    assert(nonCapt.size() == nCapt + 1);  // implies > 0
    string res;
    for (size_t ixMatch = 0; ixMatch < nCapt; ++ixMatch) {
        res += nonCapt[ixMatch].str();
        res += MHPP_begin(capt[ixMatch], clean);
    }
    res += nonCapt[nCapt].str();

    // === replace in-memory file contents (but don't write yet) ===
    if (all.str() != res) {
        // contents did change
        auto r2 = fileRewriteByName.find(fname);
        assert(r2 == fileRewriteByName.end());
        auto r3 = fileRewriteByName.insert({fname, res});
        assert(/*insertion may not fail*/ r3.second);
    }
}

MHPP("public")
void codeGen::pass3(const std::string& fname) {
    auto it = fileRewriteByName.find(fname);
    if (it == fileRewriteByName.end())
        return;
    string all = it->second;
#if false
    std::ofstream(fname, std::ios::binary) << all;
#else
    cout << "===" << fname << "===" << endl
         << all;
#endif
}

MHPP("private static")
std::string codeGen::namedCaptAsString(const std::string& name, const std::map<std::string, myRegexRange> capt) {
    return namedCaptAsRange(name, capt).str();
}

MHPP("private static")
myRegexRange codeGen::namedCaptAsRange(const std::string& name, const std::map<std::string, myRegexRange> capt) {
    auto it = capt.find(name);
    if (it == capt.end())
        throw runtime_error("regex result does not contain named capture '" + name + "'");
    return it->second;
}

MHPP("public")
// called on declaration regex capture declaration
void codeGen::MHPP_classitem(const std::map<std::string, myRegexRange> capt) {
    const string leadingComment = namedCaptAsString("leadingComment", capt);
    bool isComment = leadingComment.size() > 0;
    if (isComment) {
        return;
    }
#if false
    cout << "=== non-comment classitem ===" << endl;
    for (auto x : capt) cout << x.first << "\t>>>" << x.second.str() << "<<<" << endl;
#endif

    const string fun_keyword = namedCaptAsString("fun_MHPP_keyword", capt);
    const string var_keyword = namedCaptAsString("var_MHPP_keyword", capt);
    bool isFun = fun_keyword.size() > 0;
    bool isVar = var_keyword.size() > 0;

    if (isFun && !isVar)
        MHPP_classfun(capt);
    else if (!isFun && isVar)
        MHPP_classvar(capt);
    else
        throw runtime_error("?? neither var nor fun (or both) ??");
}

MHPP("public")
std::string codeGen::MHPP_begin(const std::map<std::string, myRegexRange>& capt, bool clean) {
    const string indent = namedCaptAsString("indent", capt);
    const string classname1 = namedCaptAsString("classname1", capt);
    const string classname2 = namedCaptAsString("classname2", capt);
    if (classname1 != classname2) throw runtime_error("MHPP(\"begin " + classname1 + "\") terminated by MHPP(\"end " + classname2 + ")\"");

    string indentp1 = indent + "\t";

    string res = indent + "MHPP(\"begin " + classname1 + "\") // === autogenerated code. Do not edit ===\n";
    if (!clean) {
        auto itc = classesByName.find(classname1);
        if (itc == classesByName.end()) throw runtime_error("no data for MHPP(\"begin " + classname1 + "\")");

        // === sanity check that each class has only one AHBEGIN(classname)...AHEND section ===
        auto r2 = classDone.find(classname1);
        assert(r2 != classDone.end());
        if (r2->second) throw runtime_error("duplicate MHPP(\"begin...end " + classname1 + "\")");
        r2->second = true;

        const oneClass& c = itc->second;
        const string pubTxt = c.getPublicText(indentp1);
        const string protTxt = c.getProtectedText(indentp1);
        const string privTxt = c.getPrivateText(indentp1);
        const string rawTxt = c.getRawText(indentp1);
        if (pubTxt.size() > 0)
            res += indent + "public:\n" + pubTxt;
        if (protTxt.size() > 0)
            res += indent + "protected:\n" + protTxt;
        if (privTxt.size() > 0)
            res += indent + "private:\n" + privTxt;
        if (rawTxt.size() > 0)
            res += rawTxt;
    }
    res += indent + "MHPP(\"end " + classname1 + "\")";  // no newline (pattern ends before it)
    return res;
}

MHPP("public")
void codeGen::checkAllClassesDone() {
    for (auto it : classDone)
        if (!it.second)
            throw runtime_error("no MHPP(\"begin " + it.first + "\") ... MHPP(\"end " + it.first + "\") anywhere in files");
}

MHPP("protected")
bool codeGen::hasClass(const std::string& classname) {
    auto itc = classesByName.find(classname);
    return itc != classesByName.end();
}

MHPP("protected")
oneClass& codeGen::getClass(const std::string& classname) {
    auto itc = classesByName.find(classname);
    if (itc == classesByName.end()) {
        cout << "getClass inserts " << classname << endl;
        // === create new oneClass for classnames ===
        auto r = classesByName.insert({classname, oneClass()});
        itc = r.first;
        assert(r.second);
        // === flag as class that is waiting to be collected by an MHPP ("begin classname")... MHPP("end classname") section ===
        auto r2 = classDone.insert({classname, false});
        assert(r2.second);
    }
    return itc->second;
}

MHPP("protected static")
std::string codeGen::trimNewline(std::string& text) {
    while (true) {
        if (text.size() == 0) break;
        char lastChar = text[text.size() - 1];
        if ((lastChar != '\n') && (lastChar != '\r')) break;
        text = text.substr(0, text.size() - 1);
    }
    return text;
}

MHPP("private static")
string getAnnot(const myRegexRange& r) {
    size_t nLineBeginBase1;
    size_t nCharBeginBase1;
    size_t nLineEndBase1;
    size_t nCharEndBase1;
    string fname;
    r.regionInSource(nLineBeginBase1, nCharBeginBase1, nLineEndBase1, nCharEndBase1, fname, /*base 1*/ true);
    return fname + " l" + to_string(nLineBeginBase1) + "c" + to_string(nCharBeginBase1) + "..l" + to_string(nLineEndBase1) + "c" + to_string(nCharEndBase1);
}

MHPP("protected")
// called on declaration regex capture that is a function
void codeGen::MHPP_classfun(const std::map<std::string, myRegexRange> capt) {
    auto all = namedCaptAsRange("all", capt);
    auto rkeyword = namedCaptAsRange("fun_MHPP_keyword", capt);
    const string keyword = rkeyword.str();
    auto comment = namedCaptAsString("fun_comment", capt);
    auto returntype = namedCaptAsString("fun_returntype", capt);
    auto rclassmethodname = namedCaptAsRange("fun_classmethodname", capt);
    const string classmethodname = rclassmethodname.str();
    auto arglist = namedCaptAsString("fun_arglist", capt);
    auto postArg = namedCaptAsString("fun_postArg", capt);
    assert(keyword.size() > 0);

    // parse classname::methodname
    myAppRegex rcm = myAppRegex::classMethodname();
    map<string, myRegexRange> cm;
    if (!rclassmethodname.match(rcm, cm)) {
        for (auto x : capt) cout << x.first << "\t>>>" << x.second.str() << "<<<" << endl;
        throw runtime_error("'" + classmethodname + "' is not of the expected format classname::(classname...)::methodname");
    }

    const string classname = namedCaptAsString("classname", cm);
    const string methodname = namedCaptAsString("methodname", cm);
    // build output line
    vector<string> destText;
    if (annotate) {
        destText.push_back("/* " + getAnnot(all) + " */");
    }

    comment = trimNewline(comment);  // newline is required terminator for multiple comments. Remove last newline only here.
    if (comment.size() > 0)
        destText.push_back(comment);
    string line;
    bool isVirtual = keyword.find("virtual") != string::npos;
    bool isStatic = keyword.find("static") != string::npos;
    if (isVirtual && isStatic) throw runtime_error(getAnnot(all) + ": C++ doesn't allow virtual and static at the same time");
    if (isVirtual)
        line += "virtual ";
    if (isStatic)
        line += "static ";

    if (returntype.size() > 0)
        line += returntype + " ";
    line += methodname;
    line += arglist;
    if (postArg.find("const") != string::npos)
        line += " const";
    if (postArg.find("noexcept") != string::npos)
        line += " noexcept";

    line += ";";
    destText.push_back(line);
    oneClass& c = getClass(classname);
    c.addTextByKeyword(keyword, destText, /*for error message*/ classmethodname);

    // search for altclass=xyz
    myAppRegex rAltCapt = myAppRegex::rx("altclass=") + myAppRegex::capture("altclass", myAppRegex::rx("[a-zA-Z0-9_:]+"));
    vector<myRegexRange> altclassNonCapt;
    vector<map<string, myRegexRange>> altclassCapt;
    rkeyword.splitByMatches(rAltCapt, altclassNonCapt, altclassCapt);
    for (auto altclassMatch : altclassCapt) {
        if (isStatic) throw runtime_error(getAnnot(rkeyword) + " An altclass-tagged method cannot be static");
        if (!isVirtual) throw runtime_error(getAnnot(rkeyword) + " An altclass-tagged method needs to be virtual");
        const string altClass = namedCaptAsString("altclass", altclassMatch);
        oneClass& cAlt = getClass(altClass);
        cAlt.addTextByKeyword("public", destText, /*for error message*/ classmethodname);
    }

    auto rx = regex("pImpl=([_a-zA-Z0-9:]+)");
    std::sregex_iterator itp(keyword.cbegin(), keyword.cend(), rx);
    std::sregex_iterator itEnd;
    while (itp != itEnd) {
        std::smatch m = *itp;
        assert(m.size() == 2);
        string pImplClass = m[1];
        generatePImpl(classname, returntype, methodname, pImplClass, arglist, postArg);
        ++itp;
    }
}

MHPP("protected")
// called on declaration regex capture that is a static variable
void codeGen::MHPP_classvar(const std::map<std::string, myRegexRange> capt) {
    const auto all = namedCaptAsRange("all", capt);
    const auto keyword = namedCaptAsString("var_MHPP_keyword", capt);
    auto comment = namedCaptAsString("var_comment", capt);
    const auto returntype = namedCaptAsString("var_returntype", capt);
    const auto rclassvarname = namedCaptAsRange("var_classvarname", capt);
    const auto classvarname = rclassvarname.str();
    assert(keyword.size() > 0);

    // parse classname::varname
    myAppRegex rcm = myAppRegex::classMethodname();  // reusing regex
    map<string, myRegexRange> cm;
    if (!rclassvarname.match(rcm, cm)) {
        for (auto x : capt) cout << x.first << "\t>>>" << x.second.str() << "<<<" << endl;
        throw runtime_error("'" + classvarname + "' is not of the expected format classname::(classname...)::varname");
    }

    const auto classname = namedCaptAsString("classname", cm);
    const auto varname = namedCaptAsString("methodname", cm);

    // build output line
    vector<string> destText;
    if (annotate)
        destText.push_back("/* " + getAnnot(all) + " */");
    comment = trimNewline(comment);  // newline is required terminator for multiple comments. Remove last newline only here.
    if (comment.size() > 0)
        destText.push_back(comment);

    if (keyword.find("static") == string::npos)
        throw runtime_error("var " + varname + " must be static");
    string line;
    line += "static ";

    line += returntype;  // includes separating whitespace
    line += varname;
    line += ";";
    destText.push_back(line);

    oneClass& c = getClass(classname);
    c.addTextByKeyword(keyword, destText, classvarname);
}

MHPP("protected static")
std::string codeGen::readFile(const std::string& fname) {
    std::ostringstream oss;
    auto s = std::ifstream(fname, std::ios::binary);
    if (!s) throw runtime_error("failed to read '" + fname + "'");
    oss << std::ifstream(fname, std::ios::binary).rdbuf();
    return oss.str();
}

MHPP("private static")
// converts "(int x, map<string, int>y)" to {"x", "y"}
std::vector<std::string> codeGen::arglist2names(const std::string& arglist) {
    vector<string> ret;
    std::smatch m;

    // remove outer round brackets, trim
    if (!std::regex_match(arglist, m, regex("^"
                                            "\\s*"
                                            "\\("
                                            "(.*)"
                                            "\\)"
                                            "\\s*"
                                            "$")))
        throw runtime_error("pimpl failed to match arglist brackets in '" + arglist + "'");
    assert(m.size() == 2);
    string arglistPImpl = m[1];
    if (arglistPImpl.size() == 0) return ret;  // split below will return nSep+1 results => empty string would cause "" capture

    arglistPImpl = myAppRegex::replaceAll(arglistPImpl, regex("/\\*"
                                                              "[^\\*]*"
                                                              "\\*/"),
                                          " ");  // replace C++ comments with whitespace
    arglistPImpl = myAppRegex::replaceAll(arglistPImpl, regex("<"
                                                              "[^<>]*"
                                                              ">"),
                                          " ");  // replace template <> with whitespace (as the name may follow immediately after >)

    // note: after removal of template args, remaining commas separate args
    vector<string> argsPImpl = myAppRegex::split(arglistPImpl, regex(","));
    for (const string& a : argsPImpl) {
        if (!std::regex_match(a, m, regex("^"
                                          ".*?"
                                          "("
                                          "[_a-zA-Z][_a-zA-Z0-9]*"
                                          ")"
                                          "\\s*"
                                          "$")))
            throw runtime_error("pimpl failed to match arg: '" + a + "' in '" + arglist + "'");
        assert(m.size() == 2);
        ret.push_back(m[1]);
    }
    // for (size_t ix = 0; ix < ret.size(); ++ix)
    //    cout << ix << "\t" << ret[ix] << endl;
    return ret;
}

MHPP("protected static")
std::string codeGen::join(const std::vector<std::string>& v, const std::string& delim) {
    string r;
    if (v.size() == 0)
        return r;
    r = v[0];
    for (size_t ix = 1; ix < v.size(); ++ix)
        r += delim + v[ix];
    return r;
}

MHPP("private")
void codeGen::generatePImpl(const std::string& classname,
                            const std::string& retType,
                            const std::string& methodname,
                            const std::string& pImplClass,
                            const std::string& fullArgsWithBrackets,
                            const std::string& postArgs) {
    const vector<string>& args = arglist2names(fullArgsWithBrackets);
    bool constFlag = postArgs.find("const") != string::npos;
    bool noexceptFlag = postArgs.find("noexcept") != string::npos;

    const string classnamePImplDecl = pImplClass + "_decl";
    const string classnamePImplImpl = pImplClass + "_impl";
    bool hasClasses = hasClass(classnamePImplDecl);
    bool hasClassesAlt = hasClass(classnamePImplImpl);
    assert(!hasClasses ^ hasClassesAlt);  // can't have only one

    oneClass& cDecl = getClass(classnamePImplDecl);
    oneClass& cImpl = getClass(classnamePImplImpl);

    string maybeConst = constFlag ? " const" : "";
    string maybeNoexcept = noexceptFlag ? " noexcept" : "";

    if (!hasClasses) {
        // constructor
        cDecl.addPublicText(pImplClass + "(std::shared_ptr<" + classname + "> pImpl);");
        cDecl.addProtectedText("std::shared_ptr<" + classname + "> pImpl;");
        cImpl.addRawText(pImplClass + "::" + pImplClass + "(std::shared_ptr<" + classname + "> pImpl):pImpl(pImpl){};");
    }

    cDecl.addPublicText(retType + " " + methodname + " " + fullArgsWithBrackets + maybeConst + maybeNoexcept + ";");

    string maybeReturn = retType.size() > 0 ? "return " : "";
    cImpl.addRawText(retType + " " + pImplClass + "::" + methodname + fullArgsWithBrackets + maybeConst + maybeNoexcept + "{" + "\n" +
                     "\t" + maybeReturn + "pImpl->" + methodname + "(" + join(args, ", ") + ");" + "\n" +
                     "}");
}
