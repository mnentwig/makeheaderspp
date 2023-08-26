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
    	/* tests/testAnnotate.cpp l71c1..l72c21 */
    	myClass();
    	/* tests/testAnnotate.cpp l75c1..l76c20 */
    	static int myInt;
    	/* tests/testAnnotate.cpp l83c1..l84c39 */
    	float myPublicMethod(int x);
    	/* tests/testAnnotate.cpp l95c1..l96c36 */
    	virtual int myVirtualFun() const;
    	/* tests/testAnnotate.cpp l101c1..l102c39 */
    	int myNonvirtualFun() const;
    	/* tests/testAnnotate.cpp l110c1..l111c88 */
    	int myFunWithCommentsInArgs(/*first arg*/ int x, /*second arg*/ int y) const;
    	/* tests/testAnnotate.cpp l113c1..l116c34 */
    	int myFunWithCommentsInArgsMultiline(
    	    /*first arg*/ int x,
    	    /*second arg*/ int y) const;
    	/* tests/testAnnotate.cpp l128c1..l130c101 */
    	/* static variable with templated type and initializer */
    	static std::map<std::string, int> myMap1;
    	/* tests/testAnnotate.cpp l132c1..l134c44 */
    	/* static variable without initializer */
    	static std::map<std::string, int> myMap2;
    	/* tests/testAnnotate.cpp l136c1..l137c51 */
    	std::vector<int> myTemplateReturnType();
    	/* tests/testAnnotate.cpp l139c1..l140c67 */
    	std::map<int, int> myTemplateReturnTypeWithCommaSpace();
    	/* tests/testAnnotate.cpp l142c1..l143c55 */
    	myClass operator+(const myClass& arg) const;
    	/* tests/testAnnotate.cpp l145c1..l146c56 */
    	myClass operator||(const myClass& arg) const;
    protected:
    	/* tests/testAnnotate.cpp l79c1..l80c30 */
    	static int myFloat;
    	/* tests/testAnnotate.cpp l87c1..l88c42 */
    	float myProtectedMethod(int x);
    private:
    	/* tests/testAnnotate.cpp l91c1..l92c40 */
    	float myPrivateMethod(int x);
    MHPP("end myClass")
    class myNestedClass {
        MHPP("begin myClass::myNestedClass") // === autogenerated code. Do not edit ===
        public:
        	/* tests/testAnnotate.cpp l107c1..l108c45 */
        	myNestedClass(int);
        	/* tests/testAnnotate.cpp l120c1..l122c43 */
        	// a comment that carries over to the declaration
        	~myNestedClass();
        	/* tests/testAnnotate.cpp l124c1..l126c40 */
        	// a static variable. Its initial value is stripped for the declaration
        	static int myVar;
        MHPP("end myClass::myNestedClass")
    };
};

class myDerivedClass : public myClass {
    MHPP("begin myDerivedClass") // === autogenerated code. Do not edit ===
    public:
    	/* tests/testAnnotate.cpp l98c1..l99c43 */
    	int myVirtualFun() const;
    	/* tests/testAnnotate.cpp l104c1..l105c46 */
    	int myNonvirtualFun() const;
    MHPP("end myDerivedClass")
};

// testing constructor
MHPP("public")
myClass::myClass() {}

// testing static var without initializer
MHPP("public static")
int myClass::myInt;

// testing static var with initializer
MHPP("protected static")
int myClass::myFloat = 3.14f;

// testing public method
MHPP("public")
float myClass::myPublicMethod(int x) { return (float)x + myInt; }

// testing protected method
MHPP("protected")
float myClass::myProtectedMethod(int x) { return (float)x + myInt + 1.0f; }

// testing private method
MHPP("private")
float myClass::myPrivateMethod(int x) { return (float)x + myInt + 2.0f; }

// === runtime test that virtual behaves as expected ===
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
int myClass::myFunWithCommentsInArgs(/*first arg*/ int x, /*second arg*/ int y) const { return x + y + 2; }

MHPP("public")
int myClass::myFunWithCommentsInArgsMultiline(
    /*first arg*/ int x,
    /*second arg*/ int y) const {
    return x + y + 2;
}

MHPP("public")
// a comment that carries over to the declaration
myClass::myNestedClass::~myNestedClass() {}

MHPP("public static")
// a static variable. Its initial value is stripped for the declaration
int myClass::myNestedClass::myVar = 33;

MHPP("public static")
/* static variable with templated type and initializer */
std::map<std::string, int> myClass::myMap1 = std::map<std::string, int>({{"theAnswerIs", 84 >> 1}});

MHPP("public static")
/* static variable without initializer */
std::map<std::string, int> myClass::myMap2;

MHPP("public")
std::vector<int> myClass::myTemplateReturnType() { return std::vector<int>(); }

MHPP("public")
std::map<int, int> myClass::myTemplateReturnTypeWithCommaSpace() { return std::map<int, int>(); }

MHPP("public")
myClass myClass::operator+(const myClass& arg) const { return myClass(arg); }

MHPP("public")
myClass myClass::operator||(const myClass& arg) const { return myClass(arg); }

int main(void) {
    myDerivedClass o;
    assert(o.myVirtualFun() == 2);
    assert(o.myNonvirtualFun() == 2);

    myClass* po = &o;
    assert(po->myVirtualFun() == 2);     // "virtual" calls overloaded fun even after casting to baseclass
    assert(po->myNonvirtualFun() == 1);  // default (nonvirtual) calls base class method
    return 0;
}