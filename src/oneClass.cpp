#include "oneClass.h"

#include <regex>
#include <stdexcept>
using std::string, std::vector, std::runtime_error;

MHPP("public")
oneClass::oneClass() : publicText(), protectedText(), privateText(), rawText() {}

MHPP("public")
const std::string oneClass::getPublicText(const std::string& indent) const { return indentStringVec(publicText, indent); }

MHPP("public")
const std::string oneClass::getProtectedText(const std::string& indent) const { return indentStringVec(protectedText, indent); }

MHPP("public")
const std::string oneClass::getPrivateText(const std::string& indent) const { return indentStringVec(privateText, indent); }

MHPP("public")
const std::string oneClass::getRawText(const std::string& indent) const { return indentStringVec(rawText, indent); }

MHPP("public")
void oneClass::addPublicText(const std::vector<std::string>& txt) {
    vector<string> text = splitMultilineAndIndent(txt);
    publicText.insert(publicText.end(), text.cbegin(), text.cend());
}

MHPP("public")
void oneClass::addPublicText(const std::string& txt) {
    vector<string> text = splitMultilineAndIndent(vector<string>({txt}));
    publicText.insert(publicText.end(), text.cbegin(), text.cend());
}

MHPP("public")
void oneClass::addProtectedText(const std::string& txt) {
    vector<string> text = splitMultilineAndIndent(vector<string>({txt}));
    protectedText.insert(protectedText.end(), text.cbegin(), text.cend());
}

MHPP("public")
void oneClass::addRawText(const std::vector<std::string>& txt) {
    vector<string> text = splitMultilineAndIndent(txt);
    rawText.insert(rawText.end(), text.cbegin(), text.cend());
}

MHPP("public")
void oneClass::addRawText(const std::string& txt) {
    vector<string> text = splitMultiline(vector<string>({txt}));
    rawText.insert(rawText.end(), text.cbegin(), text.cend());
}

MHPP("public")
void oneClass::addTextByKeyword(const std::string& keyword, const std::vector<std::string>& txt, const std::string& errorObjName) {
    vector<string> text = splitMultilineAndIndent(txt);
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

MHPP("public static")
std::string oneClass::indentStringVec(const std::vector<std::string>& vec, const std::string& indent) {
    string r;
    for (const string& v : vec)
        r += indent + v + "\n";
    return r;
}

MHPP("protected static")
// splits a string item containing newlines into multiple items with added indentation after the first one
std::vector<std::string> oneClass::splitMultilineAndIndent(const std::vector<std::string>& arg) {
    vector<string> r;
    const std::regex rsplit("\\r*\\n");
    for (const string& line : arg) {
        std::sregex_token_iterator it(line.cbegin(),
                                      line.cend(),
                                      rsplit,
                                      -1);
        std::sregex_token_iterator itEnd;
        bool isFirst = true;
        for (; it != itEnd; ++it) {
            if (isFirst)
                r.push_back(it->str());
            else
                r.push_back(string("\t") + it->str());
        }
    }
    return r;
}

MHPP("protected static")
// splits a string item containing newlines into multiple items
std::vector<std::string> oneClass::splitMultiline(const std::vector<std::string>& arg) {
    vector<string> r;
    const std::regex rsplit("\\r*\\n");
    for (const string& line : arg) {
        std::sregex_token_iterator it(line.cbegin(),
                                      line.cend(),
                                      rsplit,
                                      -1);
        std::sregex_token_iterator itEnd;
        for (; it != itEnd; ++it)
            r.push_back(it->str());
    }
    return r;
}