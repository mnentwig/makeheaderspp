#include "oneClass.h"
using std::string, std::runtime_error;

MHPP("public")
oneClass::oneClass() : publicText(), protectedText(), privateText() {}

MHPP("public")
const std::string oneClass::getPublicText(const std::string& indent) const { return indentStringVec(publicText, indent); }

MHPP("public")
const std::string oneClass::getProtectedText(const std::string& indent) const { return indentStringVec(protectedText, indent); }

MHPP("public")
const std::string oneClass::getPrivateText(const std::string& indent) const { return indentStringVec(privateText, indent); }

MHPP("public")
void oneClass::addTextByKeyword(const std::string& keyword, const std::vector<std::string>& text, const std::string& errorObjName) {
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