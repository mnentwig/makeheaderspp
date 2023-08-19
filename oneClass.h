#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#ifndef MHPP
#define MHPP(arg)  // arg is for makeheaderspp.exe, to be ignored by compiler
#endif

// holds information for one class subject to MHPP-autogeneration
class oneClass {
    MHPP("begin oneClass")  // === autogenerated code. Do not edit ===
   public:
    oneClass();
    const ::std::string getPublicText(const ::std::string& indent) const;
    const ::std::string getProtectedText(const ::std::string& indent) const;
    const ::std::string getPrivateText(const ::std::string& indent) const;
    void addTextByKeyword(const ::std::string& keyword, const ::std::vector<::std::string>& text, const ::std::string& errorObjName);
    static ::std::string indentStringVec(const ::std::vector<::std::string>& vec, const ::std::string& indent);
    MHPP("end oneClass")
   protected:
    ::std::vector<::std::string> publicText;
    ::std::vector<::std::string> protectedText;
    ::std::vector<::std::string> privateText;
};
