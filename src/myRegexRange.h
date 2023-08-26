#pragma once
#include <iterator>
#include <memory>
#include <string>
#include <regex>
#include <vector>
#ifndef MHPP
#define MHPP(arg)  // see https://github.com/mnentwig/makeheaderspp
#endif
class myRegexBase;
// source text for work with substrings that can be referenced to the original test
// as line x character y for error/annotation/etc messages.
class myRegexRange {
    MHPP("begin myRegexRange") // === autogenerated code. Do not edit ===
    public:
    	myRegexRange(const std::string& text, const std::string& filename);
    	std::string::const_iterator begin() const;
    	std::string::const_iterator end() const;
    	std::string str() const;
    	myRegexRange substr(std::string::const_iterator iBegin, std::string::const_iterator iEnd) const;
    	// applies std::regex_match and returns captures by position as myRegexRange. Failure to match returns false.
    	bool match(const std::regex& rx, std::vector<myRegexRange>& captures) const;
    	// applies std::regex_match and returns captures by name as myRegexRange. Failure to match returns false.
    	bool match(const std::regex& rx, const std::vector<std::string>& names, std::map<std::string, myRegexRange>& captures) const;
    	// applies std::regex_match and returns captures by name as myRegexRange. Failure to match returns false.
    	bool match(const myRegexBase& rx, std::map<std::string, myRegexRange>& captures) const;
    	// split into unmatched|match|unmatched|match|...|unmatched
    	void splitByMatches(const std::regex& rx, std::vector<myRegexRange>& nonMatch, std::vector<std::vector<myRegexRange>>& captures) const;
    	// split into unmatched|match|unmatched|match|...|unmatched
    	void splitByMatches(const std::regex& rx, const std::vector<std::string>& names, std::vector<myRegexRange>& nonMatch, std::vector<std::map<std::string, myRegexRange>>& captures) const;
    	// split into unmatched|match|unmatched|match|...|unmatched
    	void splitByMatches(const myRegexBase& rx, std::vector<myRegexRange>& nonMatch, std::vector<std::map<std::string, myRegexRange>>& captures) const;
    	// returns line-/character count for substring
    	void regionInSource(size_t& lineBegin, size_t& charBegin, size_t& lineEnd, size_t& charEnd, std::string& fname, bool base1) const;
    private:
    	myRegexRange(const myRegexRange& src, std::string::const_iterator iBegin, std::string::const_iterator iEnd);
    MHPP("end myRegexRange")
   private:
    // original string e.g. source file contents, of which this represents a substring
    std::shared_ptr<const std::string> body;
    // filename where body was read from
    const std::string filename;
    // start of substring in body
    std::string::const_iterator iBegin;
    // end of substring in body
    std::string::const_iterator iEnd;
};