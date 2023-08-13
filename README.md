# makeheaderspp
Automatic header generation for C++ classes

### Motivation
The tool auto-generates class method declarations in C++ headers from their implementation. 
It modifies only user-defined parts of a header, while the remaining lines are edited conventionally. 

### "Inspiration"
I use (and try to use more) C++17 for everyday tasks, replacing quick-and-dirty tools like perl, Octave, Excel, shell script.

After a steep and segfault-littered learning curve, it started to pay off: The compiler truly becomes "my best friend", now catching the vast majority of issues at compile time (hint: -D_GLIBCXX_DEBUG has become "my second best friend", largely eliminating the dreaded segfaults).

The fastest way to write a small-ish program is arguably to put method declarations and definitions into the same file (.hpp headerless style). 
This workmode largely avoids the need to maintain headers, but gets clumsy with increasing project size (e.g. 20-page error messages from a missing-bracket typo; compile time), discourages two-way links between data structures (which requires to separate declaration and definition) and feels like a hack for non-lightweight code.

In principle, class header files seem "the right way" for many reasons - if only as a compiler-checked summary of my class APIs - but maintaining them manually feels redundant.

Enter `makeheaderspp`, inspired by [makeheaders](https://fossil-scm.org/home/doc/trunk/tools/makeheaders.html)

In comparison, we're less ambitious - instead of writing the complete header file, `makeheaderspp` output integrates into an existing header, for selected items only.

### Use model
A single dummy macro
```
#define MHPP(arg)
``` 
tags relevant sections of the code in various ways. As the macro itself is empty, it has no side effects on compiler or IDE.

The implementation of a new C++ function `myMethod` in a class `myClass` is marked as follows:
```
// e.g. located in myClass.cpp or some other .cpp file
MHPP("public") 
int myClass::myMethod(int myArg){}
```

A conventional header file is set up, and a section is marked for autogeneration as follows:
```
// this could be located in myClass.h 
class myClass{ // this could be in myClass.h
    // here: any contents outside autogeneration
    MHPP("begin myClass") // === autogenerated code. Do not edit === 
    MHPP("end myClass")
    // here: more content outside autogeneration
};
```

Running 
```
makeheaderpp.exe myClass.cpp myClass.h
```
fills in the declaration:
```
class myClass{
    // here: any contents outside autogeneration
    MHPP("begin myClass") // === autogenerated code. Do not edit === 
    public:
        myMethod(int myArg);
    MHPP("end myClass")
    // here: more content outside autogeneration
};
```

Multiple classes in any number of .cpp and .h files may be processed at the same time. 
File names, extensions, file order do not affect functionality.

### Method variations
The following keywords are recognized in MHPP("..."):
* public
* protected
* private
* static
* virtual

The following types of class methods are supported:
* regular function
* static function
* constructor
* destructor
* operator

### Supported qualifiers (after arguments list)
* const
* noexcept

### Notes:
The tool attempts to be failsafe and rewrites files only when all checks have passed (note: the redundant class name in `MHPP("begin myClass") ... MHPP("end myClass")`is one of them). 

However, modifying files in-place is inherently somewhat risky. Use git, commit often.

In case a required construct cannot be autogenerated for whatever reason, please add it manually to the non-generated section of its respective header. 