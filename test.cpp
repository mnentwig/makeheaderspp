// testcase source for MHPP (one file for .h and .cpp content)
#include <cassert>
#include <map>
#include <string>
#include <vector>
class x {
    constexpr int fun1();  // must duplicate
    virtual int fun2();    // may not duplicate
    inline int fun3();     // may duplicate
    explicit x();          // may not duplicate
    const int* fun5();     // must duplicate
    void fun6() const;     // must duplicate
};

constexpr int x::fun1() { return 33; }
int x::fun2() { return 33; }
inline int x::fun3() { return 33; }
x::x() {}
const int* x::fun5() { return nullptr; }
void x::fun6() const {}

#define MHPP(arg)

class myClass {
    MHPP("begin myClass") // === autogenerated code. Do not edit ===
    public:
    	myClass();
    	virtual int myVirtualFun() const;
    	int myNonvirtualFun() const;
    	/* static variable with templated type and initializer */
    	static ::std::map<::std::string, int> myMap1;
    	/* static variable without initializer */
    	static ::std::map<::std::string, int> myMap2;
    	::std::vector<int> myTemplateReturnType();
    MHPP("end myClass")
    class myNestedClass {
        MHPP("begin myClass::myNestedClass") // === autogenerated code. Do not edit ===
        public:
        	myNestedClass(int);
        	// a comment
        	~myNestedClass();
        	// a static variable. Its initial value is stripped for the declaration
        	static int myVar;
        MHPP("end myClass::myNestedClass")
    };
};

class myDerivedClass : public myClass {
    MHPP("begin myDerivedClass") // === autogenerated code. Do not edit ===
    public:
    	int myVirtualFun() const;
    	int myNonvirtualFun() const;
    MHPP("end myDerivedClass")
};

MHPP("public")
myClass::myClass() {}

MHPP("public virtual")
int myClass::myVirtualFun() const { return 1; }

MHPP("public")
int myDerivedClass::myVirtualFun() const { return 2; }

MHPP("public")
int myClass::myNonvirtualFun() const { return 1; }

MHPP("public")
int myDerivedClass::myNonvirtualFun() const { return 2; }

MHPP("public")
myClass::myNestedClass::myNestedClass(int) {}

MHPP("public")
// a comment
myClass::myNestedClass::~myNestedClass() {}

MHPP("public static")
// a static variable. Its initial value is stripped for the declaration
int myClass::myNestedClass::myVar = 33;

MHPP("public static")
/* static variable with templated type and initializer */
::std::map<::std::string, int> myClass::myMap1 = ::std::map<::std::string, int>({{"theAnswerIs", 84 >> 1}});

MHPP("public static")
/* static variable without initializer */
::std::map<::std::string, int> myClass::myMap2;

MHPP("public")
::std::vector<int> myClass::myTemplateReturnType() { return ::std::vector<int>(); }

int main(void) {
    myDerivedClass o;
    assert(o.myVirtualFun() == 2);
    assert(o.myNonvirtualFun() == 2);

    myClass* po = &o;
    assert(po->myVirtualFun() == 2);     // "virtual" calls overloaded fun even after casting to baseclass
    assert(po->myNonvirtualFun() == 1);  // default (nonvirtual) calls base class method
    return 0;
}