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
    MHPP("begin myClass")
    myClass();
    MHPP("end myClass")
    class myNestedClass{
    MHPP("begin myClass::myNestedClass")
    MHPP("end myClass::myNestedClass")
    };
};

MHPP("public")
myClass::myClass() {}

MHPP("public")
myClass::myNestedClass::myNestedClass(int arg) {}

MHPP("public")
myClass::myNestedClass::~myNestedClass() {}
