#include <cassert>
#include <fstream>  // ifstream
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>

#include "myRegexBase.h"
using std::cout, std::endl, std::string, std::runtime_error, std::vector, std::set, std::map;
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

    static myAppRegex MHPP_classitem() {
        return txt("MHPP(\"") +
               // public or private or protected (all start with "p")
               capture("MHPP_keyword", txt("p") + zeroOrMore_lazy(rx(".", false))) + txt("\")") + wsOpt +
               capture("comment", zeroOrMore_greedy(CComment | CppComment)) + wsOpt +
               // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
               capture("returntype", rx(".*?", false)) +

               // class name (final :: not captured)
               // oneOrMore_greedy(capture("classname", Cidentifier + zeroOrMore_greedy(doubleColon + Cidentifier))) + doubleColon +

               // method name
               // capture("methodname", zeroOrOne_greedy(txt("~")) + Cidentifier) + wsOpt +
               capture("classmethodname", rx("["
                                             "_"
                                             "~"
                                             ":"
                                             "a-z"
                                             "A-Z"
                                             "0-9"
                                             "]+",
                                             false)) +

               // arguments list (may not contain a round bracket)
               capture("arglist", openRoundBracket + rx("[^\\)]*", false) + closingRoundBracket) + wsOpt +
               // qualifiers after arg list
               capture("postArg", rx("[^\\{]+", false)) + wsOpt +
               txt("{");
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
            capture("body", rx("[\\s\\S]*", false)) +

            // MHPP ("end myClass::myMethod")
            txt("MHPP(\"end ") +
            capture("classname2", classname) +
            txt("\")");

        return r;
    }

    static myAppRegex classMethodname() {
        myAppRegex r = capture("classname", Cidentifier + zeroOrMore_greedy(doubleColon + Cidentifier)) + doubleColon + capture("methodname", Cidentifier);
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
    void addPublicText(const string& text) { publicText.push_back(text); }
    void addProtectedText(const string& text) { protectedText.push_back(text); }
    void addPrivateText(const string& text) { privateText.push_back(text); }
    const string getPublicText(const string& indent) const { return indentStringVec(publicText, indent); }
    const string getProtectedText(const string& indent) const { return indentStringVec(protectedText, indent); }
    const string getPrivateText(const string& indent) const { return indentStringVec(privateText, indent); }

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
    codeGen() {}
    void pass1(const string& fname) {
        // === read file contents ===
        string all = readFile(fname);
        auto r = filebodyByFilename.insert({fname, all});
        if (!r.second) throw runtime_error("duplicate filename: '" + fname + "'");

        // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
        myAppRegex rx = myAppRegex::MHPP_classitem();
        std::vector<myAppRegex::range> nonCapt;
        std::vector<std::map<string, myAppRegex::range>> capt;
        rx.allMatches(all, nonCapt, capt);
        for (const auto& a : capt)
            MHPPMETHOD(a);
    }

    void pass2(const string& fname) {
        // === retrieve original file contents ===
        auto it = filebodyByFilename.find(fname);
        assert(it != filebodyByFilename.end());
        string all = it->second;
        // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
        myAppRegex rx = myAppRegex::MHPP_begin();
        std::vector<myAppRegex::range> nonCapt;
        std::vector<std::map<string, myAppRegex::range>> capt;
        rx.allMatches(all, nonCapt, capt);
        cout << "XXXXXXX" << capt.size() << endl;

        // === replace AHBEGIN(classname)...AHEND with respective classname's declarations ===
        const size_t nCapt = capt.size();
        if (nCapt == 0)
            return;
        assert(nonCapt.size() == nCapt + 1);  // implies > 0
        string res;
        for (size_t ixMatch = 0; ixMatch < nCapt; ++ixMatch) {
            res += nonCapt[ixMatch].str();
            res += AHBEGIN(capt[ixMatch]);
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
        cout << "===" << fname << "===\n"
             << res;
    }

    void pass3(const string& fname) {
        if (filesNeedRewrite.find(fname) == filesNeedRewrite.end())
            return;
        auto it = filebodyByFilename.find(fname);
        assert(it != filebodyByFilename.end());
        string all = it->second;
        //      std::ofstream(fname, std::ios::binary) << all;
        cout << fname << endl
             << all;
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

    void MHPPMETHOD(const std::map<string, myAppRegex::range> capt) {
        const string keyword = myAppRegex::namedCaptAsString("MHPP_keyword", capt);
        const string comment = myAppRegex::namedCaptAsString("comment", capt);
        const string returntype = myAppRegex::namedCaptAsString("returntype", capt);
        const string classmethodname = myAppRegex::namedCaptAsString("classmethodname", capt);
        const string arglist = myAppRegex::namedCaptAsString("arglist", capt);
        const string postArg = myAppRegex::namedCaptAsString("postArg", capt);

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
        string destText;
        if (keyword.find("virtual") != string::npos)
            destText += "virtual ";
        if (keyword.find("static") != string::npos)
            destText += "static ";

        destText += returntype; // includes separating whitespace + string((returntype.size() > 0) ? " " : "");
        destText += methodname;
        destText += arglist;
        if (postArg.find("const") != string::npos)
            destText += " const";
        if (postArg.find("noexcept") != string::npos)
            destText += " noexcept";

        destText += ";";
        auto itc = classesByName.find(classname);
        if (itc == classesByName.end()) {
            // === create new oneClass for classnames ===
            auto r = classesByName.insert({classname, oneClass()});
            itc = r.first;
            assert(r.second);
            // === flag as class that is waiting for an AHBEGIN(classname)...AHEND section ===
            auto r2 = classDone.insert({classname, false});
            assert(r2.second);
        }
        oneClass& c = itc->second;
        size_t isPublic = keyword.find("public") != string::npos ? 1 : 0;
        size_t isProtected = keyword.find("protected") != string::npos ? 1 : 0;
        size_t isPrivate = keyword.find("private") != string::npos ? 1 : 0;
        if (isPublic + isProtected + isPrivate < 1) throw runtime_error(classname + "::" + methodname + " needs AH: public|private|protected (got '" + keyword + "')");
        if (isPublic + isProtected + isPrivate > 1) throw runtime_error(classname + "::" + methodname + " has more than one choice of AH: public|private|protected (got '" + keyword + "')");
        if (isPublic)
            c.addPublicText(destText);
        if (isProtected)
            c.addProtectedText(destText);
        if (isPrivate)
            c.addPrivateText(destText);
        cout << destText << endl;
    }

    string AHBEGIN(const std::map<string, myAppRegex::range> capt) {
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

        auto itc = classesByName.find(classname1);
        if (itc == classesByName.end()) throw runtime_error("no data for AHBEGIN(" + classname1 + ")");

        // === sanity check that each class has only one AHBEGIN(classname)...AHEND section ===
        auto r2 = classDone.find(classname1);
        assert(r2 != classDone.end());
        if (r2->second) throw runtime_error("duplicate AHBEGIN(" + classname1 + ")");
        r2->second = true;

        string indentp1 = indent + "\t";

        const oneClass& c = itc->second;
        const string pubTxt = c.getPublicText(indentp1);
        const string protTxt = c.getProtectedText(indentp1);
        const string privTxt = c.getPrivateText(indentp1);
        string res;
        res += indent + "MHPP(\"begin " + classname1 + "\") // === autogenerated code. Do not edit ===\n";
        if (pubTxt.size() > 0)
            res += indent + "public:\n" + pubTxt;
        if (protTxt.size() > 0)
            res += indent + "protected:\n" + protTxt;
        if (privTxt.size() > 0)
            res += indent + "private:\n" + privTxt;
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
};

int main(int argc, const char** argv) {
#if 0
    myAppRegex ra = myAppRegex::MHPP_classitem();

    std::vector<myRegexBase::range_t> nonCapt;
    std::vector<std::map<string, myAppRegex::range_t>> capt;
    string all = readFile("myRegexBase.cpp");
    ra.allMatches(all, nonCapt, capt);

    for (const auto& a : capt)
        for (const auto& b : a)
            cout << b.first << "\t" << string(b.second.first, b.second.second) << endl;
    cout << myAppRegex::MHPP_classitem().getExpr() << endl;
    return 0;
#endif
    // === copy command line args as filenames ===
    vector<string> filenames;
    set<string> uniqueFilenames;
    for (size_t ix = 1; ix < (size_t)argc; ++ix) {
        const string f = argv[ix];
        filenames.push_back(f);
        if (!uniqueFilenames.insert(f).second)
            throw runtime_error("duplicate filename: '" + f + "'");
    }

    codeGen cg;
    for (const string& filename : filenames)
        cg.pass1(filename);
    for (const string& filename : filenames)
        cg.pass2(filename);
    cg.checkAllClassesDone();
    for (const string& filename : filenames)
        cg.pass3(filename);
    return 0;
}
