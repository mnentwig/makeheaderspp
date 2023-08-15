#include <cassert>
#include <cstring>  // strchr
#include <fstream>  // ifstream
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using std::cout, std::endl, std::smatch, std::regex_match, std::regex, std::string, std::runtime_error, std::vector, std::map, std::set;

class regexGen {
   public:
    static string nonCaptGroup(string expr) { return "(?:" + expr + ")"; }
    static string capture(string expr) { return "(" + expr + ")"; }
    static string zeroOrMore_greedy(string expr) { return "(?:" + expr + ")*"; }
    static string zeroOrMore_nonGreedy(string expr) { return "(?:" + expr + ")*?"; }
    static string oneOrMore_greedy(string expr) { return "(?:" + expr + ")+"; }
    static string oneOrMore_nonGreedy(string expr) { return "(?:" + expr + ")+?"; }
    static string zeroOrOne(string expr) { return "(?:" + expr + ")?"; }
    static string literal(string expr) {
        static const char metacharacters[] = R"(\.^$-+()[]{}|?*)";
        std::string out;
        out.reserve(expr.size());
        for (auto ch : expr) {
            if (std::strchr(metacharacters, ch))
                out.push_back('\\');
            out.push_back(ch);
        }
        return out;
    }
#if 0
    for (const string& s : vector<string>({".", "?", "+", "*", "\\", "[", "]", "(", ")", "{", "}", "|", "^", "$"}))
        expr = replaceString(expr, s, "\\" + s);
    return expr;
#endif
    static string oneOf(const vector<string>& arg) {
        string r;
        for (const auto& v : arg) {
            if (r.size() > 0)
                r += "|";
            r += nonCaptGroup(v);
        }
        return nonCaptGroup(r);
    }
    inline static const string Cidentifier = "[_a-zA-Z][_a-zA-Z0-9]*";
    inline static const string wsSep = "\\s+";
    inline static const string wsOpt = "\\s*";
    inline static const string any = "[\\s\\S]";  // includes newline
    static void process(const string& text, const regex& r, vector<string>& nonMatch, vector<vector<string>>& captures) {
        assert(0 == nonMatch.size() + captures.size());
        std::sregex_iterator it(text.cbegin(), text.cend(), r);
        std::sregex_iterator itEnd;
        auto trailer = text.cbegin();

        while (it != itEnd) {
            nonMatch.push_back(it->prefix());
            vector<string> oneMatch;
            size_t nMatch = it->size();
            for (auto it2 = it->begin(); it2 != it->end(); ++it2)
                oneMatch.push_back(it2->str());
            captures.push_back(oneMatch);
            trailer = it->suffix().first;
            ++it;
        }
        nonMatch.push_back(string(trailer, text.cend()));
        assert(nonMatch.size() == captures.size() + 1);  // unmatched|match|unmatched|match|...|unmatched
    }

   protected:
    static std::string replaceString(std::string subject, const std::string& search,
                                     const std::string& replace) {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
};

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
    class myRegexGen : protected regexGen {
       public:
        static string getAHMETHOD() {
            const string oneSpecifier = oneOf({"noexcept", "const"});
            //const string CPP_return_type = zeroOrOne("const" + wsSep) + zeroOrMore_greedy("::" + Cidentifier) + Cidentifier + "[\\&\\*]*";
            const string CPP_methodName = oneOf({Cidentifier, "operator\\s*[^\\s\\(]+"});

            return "MHPP" + wsOpt + literal("(") + wsOpt + capture(zeroOrMore_nonGreedy(any)) + "\"" + wsOpt + "\\)" + wsOpt +

                   zeroOrOne(capture("constexpr") + wsSep) +

                   // method return type (empty for a constructor)
                   capture(
                       zeroOrOne("const" + wsSep) +  // "const" is optional (if found, enforce separator)
                       zeroOrOne(oneOrMore_nonGreedy("[^\\(]") + wsSep)) +

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

        static string getMHPP_beginEnd() {
            return
                // indent of MHPP
                capture(zeroOrMore_greedy("[ \t]"))
                // keyword itself
                + "MHPP" + wsOpt +
                literal("(") + wsOpt + literal("\"") + wsOpt + "begin" + wsSep + capture(Cidentifier + zeroOrMore_greedy("::" + Cidentifier)) + wsOpt + literal("\"") + wsOpt + literal(")") +
                // old definitions (to be replaced)
                zeroOrMore_nonGreedy(any) +
                "MHPP" + wsOpt +
                literal("(") + wsOpt + literal("\"") + wsOpt + "end" + wsSep + capture(Cidentifier + zeroOrMore_greedy("::" + Cidentifier)) + wsOpt + literal("\"") + wsOpt + literal(")");
        }
    };

   public:
    codeGen() {}
    void pass1(const string& fname) {
        // === read file contents ===
        string all = readFile(fname);
        auto r = filebodyByFilename.insert({fname, all});
        if (!r.second) throw runtime_error("duplicate filename: '" + fname + "'");

        // === break into nonmatch|match|nonmatch|...|nonmatch stream ===
        const regex rx = regex(myRegexGen::getAHMETHOD());
        vector<string> unmatched;
        vector<vector<string>> captures;
        regexGen::process(all, rx, unmatched, captures);

        for (const auto& a : captures)
            AHMETHOD(a);
    }

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
            res += unmatched[ixMatch];
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

   protected:
    void AHMETHOD(const vector<string>& captures) {
        assert(captures.size() == 9);
        const string& AH_keyword = captures[1];
        const string& maybeConstexpr = captures[2];
        const string& maybeReturnType = captures[3];
        const string& classnames = captures[4];
        const string& maybeDestructorTilde = captures[5];
        const string& methodname = captures[6];
        const string& args = captures[7];
        const string& maybeNoexceptConst = captures[8];

cout << "XXX" << maybeReturnType << endl;

        string destText;
        if (AH_keyword.find("virtual") != string::npos)
            destText += "virtual ";
        if (AH_keyword.find("static") != string::npos)
            destText += "static ";
        if (maybeConstexpr.find("constexpr") != string::npos)
            destText += "constexpr ";

        destText += maybeReturnType + string((maybeReturnType.size() > 0) ? " " : "");
        destText += maybeDestructorTilde;
        destText += methodname;
        destText += args;
        destText += string(maybeNoexceptConst.size() > 0 ? " " : "");
        destText += maybeNoexceptConst;
        destText += ";";
        auto it = classesByName.find(classnames);
        if (it == classesByName.end()) {
            // === create new oneClass for classnames ===
            auto r = classesByName.insert({classnames, oneClass()});
            it = r.first;
            assert(r.second);
            // === flag as class that is waiting for an AHBEGIN(classname)...AHEND section ===
            auto r2 = classDone.insert({classnames, false});
            assert(r2.second);
        }
        oneClass& c = it->second;
        size_t isPublic = AH_keyword.find("public") != string::npos ? 1 : 0;
        size_t isProtected = AH_keyword.find("protected") != string::npos ? 1 : 0;
        size_t isPrivate = AH_keyword.find("private") != string::npos ? 1 : 0;
        if (isPublic + isProtected + isPrivate < 1) throw runtime_error(classnames + "::" + maybeDestructorTilde + methodname + " needs AH: public|private|protected (got '" + AH_keyword + "')");
        if (isPublic + isProtected + isPrivate > 1) throw runtime_error(classnames + "::" + maybeDestructorTilde + methodname + " has more than one choice of AH: public|private|protected (got '" + AH_keyword + "')");
        if (isPublic)
            c.addPublicText(destText);
        if (isProtected)
            c.addProtectedText(destText);
        if (isPrivate)
            c.addPrivateText(destText);
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
    // === copy command line args as filenames ===
    vector<string> filenames;
    set<string> uniqueFilenames;
    for (size_t ix = 1; ix < argc; ++ix) {
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