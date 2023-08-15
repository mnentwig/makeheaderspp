#include <cassert>
#include <fstream>  // ifstream
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>
#include <map>

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

    static myAppRegex MHPP_classitem() {
        return txt("MHPP") + wsOpt +
               openRoundBracket + wsOpt +
               doubleQuote + wsOpt +
               capture("MHPP_keyword",
                       makeGrp(txt("public") |
                               txt("protected") | txt("private")) +
                           wsOpt + zeroOrMore_greedy(rx("[^\"]", false)) + wsOpt) +
               doubleQuote + wsOpt + closingRoundBracket + wsOpt +
               capture("comment", zeroOrMore_greedy(CComment | CppComment)) + wsOpt +
               // return type (optional, free form for templates, may include constexpr, const separated by whitespace)
               zeroOrMore_greedy(capture("returntype", rx("[^\\(]*", false)) + wsSep) +
               // class name
               oneOrMore_greedy(capture("classname", Cidentifier + doubleColon + zeroOrMore_greedy(Cidentifier + doubleColon))) +
               // method name
               capture("methodname", zeroOrOne_greedy(txt("~")) + Cidentifier) + wsOpt +
               capture("arglist", openRoundBracket + rx("[^\\)]*", false) + closingRoundBracket) + wsOpt +
               capture("postArgQual", zeroOrMore_greedy(txt("noexcept") | txt("const"))) + wsOpt +
               txt("{");
    }

    static myAppRegex MHPP_begin() {
        myAppRegex classname = zeroOrMore_greedy(doubleColon) + Cidentifier + zeroOrMore_greedy(doubleColon + Cidentifier);
        myAppRegex methodname = zeroOrOne_greedy(txt("~")) + Cidentifier;

        myAppRegex r = txt("MHPP") + wsOpt + openRoundBracket + wsOpt + doubleQuote + wsOpt + txt("begin") + wsSep +
                       capture("classname", classname) + doubleColon +
                       capture("methodname", methodname) + wsOpt +
                       closingRoundBracket;
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
myAppRegex myAppRegex::doubleColon = wsOpt + txt("::") + wsOpt;

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

#if 0
    void pass2(const string& fname) {
        // === retrieve original file contents ===
        auto it = filebodyByFilename.find(fname);
        assert(it != filebodyByFilename.end());
        string all = it->second;
        // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
        regex r = regex(myRegexGen::getMHPP_beginEnd());
        vector<string> unmatched;
        vector<vector<string>> captures;
        regexGen::process(all, r, unmatched, captures);
        // === replace AHBEGIN(classname)...AHEND with respective classname's declarations ===
        const size_t nCapt = captures.size();
        if (nCapt == 0)
            return;
        assert(unmatched.size() == nCapt + 1);  // implies > 0
        string res;
        for (size_t ixMatch = 0; ixMatch < nCapt; ++ixMatch) {
            res += unmatched[0];
            res += AHBEGIN(captures[ixMatch]);
        }
        res += unmatched[nCapt];

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
        std::ofstream(fname, std::ios::binary) << all;
    }

    void checkAllClassesDone() {
        for (auto it : classDone)
            if (!it.second)
                throw runtime_error("no MHPP(\"begin " + it.first + "\") ... MHPP(\"end " + it.first + "\") anywhere in files");
    }
#endif

   protected:
    static string range2string(const myRegexBase::range_t& r) {
        return string(r.first, r.second);
    }

    void MHPPMETHOD(const std::map<string, myAppRegex::range> capt) {
        auto it = capt.find("MHPP_keyword");
        assert(it != capt.end());
        string keyword = it->second.str();

        it = capt.find("comment");
        assert(it != capt.end());
        string comment = it->second.str();

        it = capt.find("returntype");
        assert(it != capt.end());
        string returntype = it->second.str();

        it = capt.find("classname");
        assert(it != capt.end());
        string classname = it->second.str();

        it = capt.find("methodname");
        assert(it != capt.end());
        string methodname = it->second.str();

        it = capt.find("arglist");
        assert(it != capt.end());
        string arglist = it->second.str();

        it = capt.find("postArgQual");
        assert(it != capt.end());
        string postArgQual = it->second.str();

        string destText;
        if (keyword.find("virtual") != string::npos)
            destText += "virtual ";
        if (keyword.find("static") != string::npos)
            destText += "static ";

        destText += returntype + string((returntype.size() > 0) ? " " : "");
        destText += methodname;
        destText += arglist;
        destText += string(postArgQual.size() > 0 ? " " : "");
        destText += postArgQual;
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

    string AHBEGIN(const vector<string>& captures) {
        assert(captures.size() == 4);
        string indent = captures[1];
        string classname = captures[2];
        string classnameEnd = captures[3];
        if (classname != classnameEnd) throw runtime_error("MHPP(\"begin " + classname + "\") terminated by MHPP(\"end " + classnameEnd + ")\"");

        auto it = classesByName.find(classname);
        if (it == classesByName.end()) throw runtime_error("no data for AHBEGIN(" + classname + ")");

        // === sanity check that each class has only one AHBEGIN(classname)...AHEND section ===
        auto r2 = classDone.find(classname);
        assert(r2 != classDone.end());
        if (r2->second) throw runtime_error("duplicate AHBEGIN(" + classname + ")");
        r2->second = true;

        string indentp1 = indent + "\t";

        const oneClass& c = it->second;
        const string pubTxt = c.getPublicText(indentp1);
        const string protTxt = c.getProtectedText(indentp1);
        const string privTxt = c.getPrivateText(indentp1);
        string res;
        res += indent + "MHPP(\"begin " + classname + "\") // === autogenerated code. Do not edit ===\n";
        if (pubTxt.size() > 0)
            res += indent + "public:\n" + pubTxt;
        if (protTxt.size() > 0)
            res += indent + "protected:\n" + protTxt;
        if (privTxt.size() > 0)
            res += indent + "private:\n" + privTxt;
        res += indent + "MHPP(\"end " + classname + "\")";  // no newline (pattern ends before it)
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
#if false
    for (const string& filename : filenames)
        cg.pass2(filename);
    cg.checkAllClassesDone();
    for (const string& filename : filenames)
        cg.pass3(filename);
#endif
    return 0;
}
