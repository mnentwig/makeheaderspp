#pragma once
#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

#define MHPP(arg)  // arg is for makeheaderspp.exe, to be ignored by compiler

// Regex wrapper with named captures using C++17 STL regex only.
// Note that code inside a derived class has direct access to members, without requiring any qualifying text input, which allows for concise and readable code.
class myRegexBase {
   public:
    // minimum priority of the included regex. See https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_04_06
    typedef enum {
        // assume no priority, always enclose expr in (?: ... )
        PRIO_UNKNOWN = 0,
        // expr is an alternation e.g. one|two|three. As a special rule, further alternations may be simply appended
        PRIO_OR,
        // expr results from concatenation e.g. abc
        PRIO_CONCAT,
        // ends with + * + * {a,} {a,b} and their non-greedy '...?' variants
        PRIO_REP,
        // expression is enclosed in (...) or (?: ...)
        PRIO_GRP
    } prio_e;

    // captured range reference to the original string, supports line-/character count in the input e.g. for error messages
    class range {
       public:
        MHPP("begin myRegexBase::range") // === autogenerated code. Do not edit ===
        public:
        	// construct begin-end range with complete text (e.g. file contents) starting at istart
        	range(::std::string::const_iterator istart, ::std::string::const_iterator ibegin, ::std::string::const_iterator iend);
        	// e.g. l100c3 for character 3 in line 100 (base 1, e.g. for messages)
        	::std::string getLcAnnotString() const;
        	// extracts range into new string
        	::std::string str() const;
        	// gets line and character count relative to beginning of string for start of region
        	void getBeginLineCharBase1(size_t& ixLineBase1, size_t& ixCharBase1) const;
        	// gets line and character count relative to beginning of string for end of region
        	void getEndLineCharBase1(size_t& ixLineBase1, size_t& ixCharBase1) const;
        	// start of string that contains the range (e.g. to report line / character count in error messages)
        	::std::string::const_iterator start() const;
        	// start of range in a string beginning at start()
        	::std::string::const_iterator begin() const;
        	// end of range in a string beginning at start()
        	::std::string::const_iterator end() const;
        protected:
        	// gets line and character count for a given iterator
        	void getLineCharBase1(::std::string::const_iterator itDest, size_t& ixLineBase1, size_t& ixCharBase1) const;
        MHPP("end myRegexBase::range")

        ::std::string::const_iterator istart;
        ::std::string::const_iterator ibegin;
        ::std::string::const_iterator iend;
    };

    MHPP("begin myRegexBase") // === autogenerated code. Do not edit ===
    public:
    	// match a literal text (escaping regex metacharacters)
    	static myRegexBase txt(const ::std::string& text);
    	// create arbitrary regex. Most generic variant, assumes reuse needs to wrap in (?: ...)
    	static myRegexBase rx(const ::std::string& re);
    	// create regex, special case for an alternation e.g. one|two|three at toplevel
    	static myRegexBase rx_alt(const ::std::string& re);
    	// create regex, special case for a group e.g. (...), (?:...) at toplevel
    	static myRegexBase rx_grp(const ::std::string& re);
    	static myRegexBase zeroOrMore(const myRegexBase& arg);
    	static myRegexBase zeroOrMore_lazy(const myRegexBase& arg);
    	static myRegexBase oneOrMore(const myRegexBase& arg);
    	static myRegexBase oneOrMore_lazy(const myRegexBase& arg);
    	static myRegexBase zeroOrOne(const myRegexBase& arg);
    	static myRegexBase zeroOrOne_lazy(const myRegexBase& arg);
    	static myRegexBase capture(const ::std::string& captName, const myRegexBase& arg);
    	::std::string getNamedCapture(const ::std::string& name, const ::std::smatch& m) const;
    	myRegexBase operator+(const myRegexBase& arg) const;
    	myRegexBase operator|(const myRegexBase& arg) const;
    	// returns content as regex string
    	::std::string getExpr() const;
    	// applies std::regex_match and returns captures by name. Failure to match returns false.
    	bool match(const ::std::string& text, ::std::map<::std::string, myRegexBase::range>& captures);
    	void allMatches(const ::std::string& text, ::std::vector<myRegexBase::range>& nonMatch, ::std::vector<::std::map<::std::string, myRegexBase::range>>& captures);
    	static std::string namedCaptAsString(const std::string& name, std::map<std::string, myRegexBase::range> capt);
    	static myRegexBase::range namedCaptAsRange(const std::string& name, std::map<std::string, myRegexBase::range> capt);
    	static myRegexBase makeGrp(const myRegexBase& arg);
    	myRegexBase makeGrp() const;
    protected:
    	myRegexBase(const ::std::string& expr, prio_e prio);
    	// replaces internal regex, keeps named captures
    	myRegexBase changeExpr(const ::std::string& newExpr, prio_e newPrio) const;
    	// converts STL regex smatch result to named captures
    	std::map<std::string, myRegexBase::range> smatch2named(const std::smatch& m, const std::string::const_iterator start);
    MHPP("end myRegexBase")
   public:
    operator ::std::regex();

   protected:
    // expression in human-readable regex format
    ::std::string expr;
    // decides whether or not expr needs to be enclosed in (?: ... ) before adding to it
    prio_e prio;
    // named captures in order of appearance
    ::std::vector<::std::string> captureNames;
};