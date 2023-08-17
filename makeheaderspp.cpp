#include <cassert>
#include <fstream>  // ifstream
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>

#include "myRegexBase.h"
using std::cout, std::endl, std::string, std::runtime_error, std::vector, std::set, std::map, std::cout;
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
    static myAppRegex eol;
    static myAppRegex openRoundBracket;
    static myAppRegex closingRoundBracket;
    static myAppRegex doubleQuote;
    static myAppRegex Cidentifier;
    static myAppRegex CComment;
    static myAppRegex CppComment;
    static myAppRegex doubleColon;
    static myAppRegex any;

    static myAppRegex MHPP_classfun() {
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
                                                 false)) +

               // arguments list (may not contain a round bracket)
               capture("fun_arglist", openRoundBracket + rx("[^\\)]*", false) + closingRoundBracket) + wsOpt +
               // qualifiers after arg list
               capture("fun_postArg", rx("[^\\{]+", false)) + wsOpt +
               txt("{");
    }

    static myAppRegex MHPP_classvar() {
        return txt("MHPP(\"") +
               // public or private or protected (all start with "p")
               capture("var_MHPP_keyword", txt("p") + zeroOrMore_lazy(rx(".", false))) + txt("\")") + wsOpt +
               capture("var_comment", zeroOrMore_greedy(CComment | CppComment)) + wsOpt +
               // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
               capture("var_returntype", rx(".*?", false)) +

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

    static myAppRegex MHPP_begin() {
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

    static myAppRegex classMethodname() {
        myAppRegex r = capture("classname", Cidentifier + zeroOrMore_greedy(doubleColon + Cidentifier)) + doubleColon + capture("methodname", zeroOrOne_greedy(txt("~")) + Cidentifier);
        return r;
    }
};
myAppRegex myAppRegex::wsSep = myAppRegex::rx("\\s+", /*isGroup*/ false);
myAppRegex myAppRegex::openRoundBracket = myAppRegex::txt("(");
myAppRegex myAppRegex::closingRoundBracket = myAppRegex::txt(")");
myAppRegex myAppRegex::wsOpt = myAppRegex::rx("\\s*", /*isGroup*/ false);
myAppRegex myAppRegex::doubleQuote = myAppRegex::txt("\"");
myAppRegex myAppRegex::Cidentifier = myAppRegex::rx("[_a-zA-Z][_a-zA-Z0-9]*", false);
myAppRegex myAppRegex::eol = myAppRegex::rx("\\r?\\n", false);
myAppRegex myAppRegex::CComment = myAppRegex::rx("//.*", false) + eol;
myAppRegex myAppRegex::CppComment = myAppRegex::rx("/\\*.*?\\*/", false) + eol;
myAppRegex myAppRegex::doubleColon = txt("::");
myAppRegex myAppRegex::any = wsOpt + rx(".*", false);  // regex "." does not include newline

string readFile(const string& fname) {
    std::ostringstream oss;
    auto s = std::ifstream(fname, std::ios::binary);
    if (!s) throw runtime_error("failed to read '" + fname + "'");
    oss << std::ifstream(fname, std::ios::binary).rdbuf();
    return oss.str();
}

class oneClass {
   public:
    oneClass() : publicText(), protectedText(), privateText() {}
    const string getPublicText(const string& indent) const { return indentStringVec(publicText, indent); }
    const string getProtectedText(const string& indent) const { return indentStringVec(protectedText, indent); }
    const string getPrivateText(const string& indent) const { return indentStringVec(privateText, indent); }
    void addTextByKeyword(const string& keyword, const vector<string>& text, const string& errorObjName) {
        size_t isPublic = keyword.find("public") != string::npos ? 1 : 0;
        size_t isProtected = keyword.find("protected") != string::npos ? 1 : 0;
        size_t isPrivate = keyword.find("private") != string::npos ? 1 : 0;
        if (isPublic + isProtected + isPrivate < 1) throw runtime_error(errorObjName + " needs AH: public|private|protected (got '" + keyword + "')");
        if (isPublic + isProtected + isPrivate > 1) throw runtime_error(errorObjName + " has more than one choice of AH: public|private|protected (got '" + keyword + "')");
        if (isPublic)
            publicText.insert(publicText.end(), text.cbegin(), text.cend());
        if (isProtected)
            protectedText.insert(protectedText.end(), text.cbegin(), text.cend());
        if (isPrivate)
            privateText.insert(privateText.end(), text.cbegin(), text.cend());
    }

   protected:
    vector<string> publicText;
    vector<string> protectedText;
    vector<string> privateText;
    static string indentStringVec(const vector<string>& vec, const string& indent) {
        string r;
        for (const string& v : vec)
            r += indent + v + "\n";
        return r;
    }
};

class codeGen {
   public:
    codeGen(bool annotate) : annotate(annotate) {}
    void pass1(const string& fname, bool clean) {
        // === read file contents ===
        string all = readFile(fname);
        auto r = filebodyByFilename.insert({fname, all});
        if (!r.second) throw runtime_error("duplicate filename: '" + fname + "'");

        if (!clean) {
            // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
            myAppRegex rx = myAppRegex::MHPP_classfun().makeGrp() | myAppRegex::MHPP_classvar().makeGrp();
            std::vector<myAppRegex::range> nonCapt;
            std::vector<std::map<string, myAppRegex::range>> capt;
            rx.allMatches(all, nonCapt, capt);
            for (const auto& a : capt)
                MHPP_classitem(a, fname);
        }
    }

    void pass2(const string& fname, bool clean) {
        // === retrieve original file contents ===
        auto it = filebodyByFilename.find(fname);
        assert(it != filebodyByFilename.end());
        string all = it->second;
        // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
        myAppRegex rx = myAppRegex::MHPP_begin();
        std::vector<myAppRegex::range> nonCapt;
        std::vector<std::map<string, myAppRegex::range>> capt;
        rx.allMatches(all, nonCapt, capt);

        // === replace old file content between MHPP("begin classname")...MHPP("end classname") with respective classname's declarations ===
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
        auto r2 = filebodyByFilename.find(fname);
        assert(r2 != filebodyByFilename.end());
        if (r2->second != res) {
            auto r = filesNeedRewrite.insert(fname);
            assert(/*insertion may not fail*/ r.second);
        }
        r2->second = res;
    }

    void pass3(const string& fname) {
        if (filesNeedRewrite.find(fname) == filesNeedRewrite.end())
            return;
        auto it = filebodyByFilename.find(fname);
        assert(it != filebodyByFilename.end());
        string all = it->second;
#if true
        std::ofstream(fname, std::ios::binary) << all;
#else
        cout << fname << endl
             << all;
#endif
    }

    void checkAllClassesDone() {
        for (auto it : classDone)
            if (!it.second)
                throw runtime_error("no MHPP(\"begin " + it.first + "\") ... MHPP(\"end " + it.first + "\") anywhere in files");
    }

   protected:
    static string range2string(const myRegexBase::range_t& r) {
        return string(r.first, r.second);
    }

    oneClass& getClass(const string& classname) {
        auto itc = classesByName.find(classname);
        if (itc == classesByName.end()) {
            // === create new oneClass for classnames ===
            auto r = classesByName.insert({classname, oneClass()});
            itc = r.first;
            assert(r.second);
            // === flag as class that is waiting to be collected by an MHPP("begin classname")... MHPP("end classname") section ===
            auto r2 = classDone.insert({classname, false});
            assert(r2.second);
        }
        return itc->second;
    }

    static string trimNewline(string& text) {
        while (true) {
            if (text.size() == 0) break;
            char lastChar = text[text.size() - 1];
            if ((lastChar != '\n') && (lastChar != '\r')) break;
            text = text.substr(0, text.size() - 1);
        }
        return text;
    }

    void MHPP_classfun(const std::map<string, myAppRegex::range> capt, const string& fnameForAnnot) {
        const myAppRegex::range all = myAppRegex::namedCaptAsRange("all", capt);
        const string keyword = myAppRegex::namedCaptAsString("fun_MHPP_keyword", capt);
        string comment = myAppRegex::namedCaptAsString("fun_comment", capt);
        const string returntype = myAppRegex::namedCaptAsString("fun_returntype", capt);
        const string classmethodname = myAppRegex::namedCaptAsString("fun_classmethodname", capt);
        const string arglist = myAppRegex::namedCaptAsString("fun_arglist", capt);
        const string postArg = myAppRegex::namedCaptAsString("fun_postArg", capt);
        assert(keyword.size() > 0);

        // parse classname::methodname
        myAppRegex rcm = myAppRegex::classMethodname();
        map<string, myAppRegex::range> cm;
        if (!rcm.match(classmethodname, cm)) {
            for (auto x : capt) cout << x.first << "\t" << x.second.str() << endl;
            throw runtime_error("'" + classmethodname + "' is not of the expected format classname::(classname...)::methodname");
        }

        const string classname = myAppRegex::namedCaptAsString("classname", cm);
        const string methodname = myAppRegex::namedCaptAsString("methodname", cm);

        // build output line
        vector<string> destText;
        if (annotate)
            destText.push_back("/* " + fnameForAnnot + " " + all.getLcAnnotString() + " */");

        comment = trimNewline(comment);  // newline is required terminator for multiple comments. Remove last newline only here.
        if (comment.size() > 0)
            destText.push_back(comment);
        string line;
        if (keyword.find("virtual") != string::npos)
            line += "virtual ";
        if (keyword.find("static") != string::npos)
            line += "static ";

        line += returntype;  // includes separating whitespace
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
    }

    void MHPP_classvar(const std::map<string, myAppRegex::range> capt, const string& fnameForAnnot) {
        const myAppRegex::range all = myAppRegex::namedCaptAsRange("all", capt);
        const string keyword = myAppRegex::namedCaptAsString("var_MHPP_keyword", capt);
        string comment = myAppRegex::namedCaptAsString("var_comment", capt);
        const string returntype = myAppRegex::namedCaptAsString("var_returntype", capt);
        const string classvarname = myAppRegex::namedCaptAsString("var_classvarname", capt);
        assert(keyword.size() > 0);

        // parse classname::varname
        myAppRegex rcm = myAppRegex::classMethodname();  // reusing regex
        map<string, myAppRegex::range> cm;
        if (!rcm.match(classvarname, cm)) {
            for (auto x : capt) cout << x.first << "\t" << x.second.str() << endl;
            throw runtime_error("'" + classvarname + "' is not of the expected format classname::(classname...)::varname");
        }

        const string classname = myAppRegex::namedCaptAsString("classname", cm);
        const string varname = myAppRegex::namedCaptAsString("methodname", cm);

        // build output line
        vector<string> destText;
        if (annotate)
            destText.push_back("/* " + fnameForAnnot + " " + all.getLcAnnotString() + " */");
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

    void MHPP_classitem(const std::map<string, myAppRegex::range> capt, const string& fnameForErrMsg) {
        const string fun_keyword = myAppRegex::namedCaptAsString("fun_MHPP_keyword", capt);
        const string var_keyword = myAppRegex::namedCaptAsString("var_MHPP_keyword", capt);
        bool isFun = fun_keyword.size() > 0;
        bool isVar = var_keyword.size() > 0;
        if (isFun && !isVar)
            MHPP_classfun(capt, fnameForErrMsg);
        else if (!isFun && isVar)
            MHPP_classvar(capt, fnameForErrMsg);
        else
            throw runtime_error("?? neither var nor fun (or both) ??");
    }

    string MHPP_begin(const std::map<string, myAppRegex::range>& capt, bool clean) {
        auto it = capt.find("indent");
        assert(it != capt.end());
        string indent = it->second.str();

        it = capt.find("classname1");
        assert(it != capt.end());
        string classname1 = it->second.str();

        it = capt.find("classname2");
        assert(it != capt.end());
        string classname2 = it->second.str();

        if (classname1 != classname2) throw runtime_error("MHPP(\"begin " + classname1 + "\") terminated by MHPP(\"end " + classname2 + ")\"");

        string indentp1 = indent + "\t";

        string res;
        res += indent + "MHPP(\"begin " + classname1 + "\") // === autogenerated code. Do not edit ===\n";
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
            if (pubTxt.size() > 0)
                res += indent + "public:\n" + pubTxt;
            if (protTxt.size() > 0)
                res += indent + "protected:\n" + protTxt;
            if (privTxt.size() > 0)
                res += indent + "private:\n" + privTxt;
        }
        res += indent + "MHPP(\"end " + classname1 + "\")";  // no newline (pattern ends before it)
        return res;
    }

    static string readFile(const string& fname) {
        std::ostringstream oss;
        auto s = std::ifstream(fname, std::ios::binary);
        if (!s) throw runtime_error("failed to read '" + fname + "'");
        oss << std::ifstream(fname, std::ios::binary).rdbuf();
        return oss.str();
    }

   protected:
    map<string, oneClass> classesByName;
    map<string, bool> classDone;
    map<string, string> filebodyByFilename;
    set<string> filesNeedRewrite;
    bool annotate;
};

int main(int argc, const char** argv) {
    // === copy command line args as filenames ===
    vector<string> filenames;
    set<string> uniqueFilenames;
    bool annotate = false;
    bool clean = false;

    if (argc <= 1) {
        cout << "usage: " << argv[0] <<  //
            " myfile1.cpp myfile2.h ...\n"
            "-annotate: add comment with declaration file and line\n"
            "-clean: remove all generated code\n";
        exit(0);
    }

    for (size_t ix = 1; ix < (size_t)argc; ++ix) {
        const string f = argv[ix];
        if (f == "-annotate")
            annotate = true;
        else if (f == "-clean")
            clean = true;
        else {
            filenames.push_back(f);
            if (!uniqueFilenames.insert(f).second)
                throw runtime_error("duplicate filename: '" + f + "'");
        }
    }

    if (annotate && clean) throw runtime_error("-annotate and -clean are mutually exclusive");

    codeGen cg(annotate);

    // === parse all files for declarations ===
    for (const string& filename : filenames)
        cg.pass1(filename, clean);

    // === fill in declarations ===
    for (const string& filename : filenames)
        cg.pass2(filename, clean);

    // === sanity check: all declarations referenced? ===
    if (!clean)
        cg.checkAllClassesDone();

    // === write output ===
    for (const string& filename : filenames)
        cg.pass3(filename);

    return 0;
}