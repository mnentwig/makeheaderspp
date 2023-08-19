#pragma once
#include "myRegexBase.h"

// rexex wrapper with application-specific "macros" e.g. wsSep for "\s+" or wsOpt for "\s*"
// Main objective is that code inside the class has direct access to members, without requiring any qualifying text input, to improve readability
class myAppRegex : public myRegexBase {
   public:
    MHPP("begin myAppRegex") // === autogenerated code. Do not edit ===
    public:
    	myAppRegex(const myRegexBase& arg);
    	static myAppRegex MHPP_classfun();
    	static myAppRegex MHPP_classvar();
    	static myAppRegex MHPP_begin();
    	static myAppRegex classMethodname();
    protected:
    	static myAppRegex classOp;
    	static myAppRegex wsSep;
    	static myAppRegex openRoundBracket;
    	static myAppRegex closingRoundBracket;
    	static myAppRegex wsOpt;
    	static myAppRegex doubleQuote;
    	static myAppRegex Cidentifier;
    	static myAppRegex eol;
    	static myAppRegex CComment;
    	static myAppRegex CppComment;
    	static myAppRegex doubleColon;
    	static myAppRegex any;
    MHPP("end myAppRegex")
};

