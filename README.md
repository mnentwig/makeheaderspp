# makeheaderspp
Automatic header generation for C++ classes. See source as example.

### Purpose
The tool auto-generates class method declarations in C++ headers from their implementation. 
It modifies only user-defined parts of a header, while the remaining lines are edited conventionally. 

As a typical command line invocation,
```
makeheaderspp myApplicationClass.cpp myApplicationClass.h
``` 
takes definitions from `myApplicationClass.cpp` and updates them in `myApplicationClass.h` (lazy variant: `makeheaderspp myApplicationClass.*` or even `makeheaderspp *.cpp *.h`)

For a smooth workflow, the IDE or text editor should ideally reload a file automatically on change (e.g. VSCODE does).

### Motivation
For routine programming work in C++, maintaining class header files seems an unproductive routine task that could be easily automated.

Also, having to manually keep headers in sync adds an extra work step that discourages from e.g. adding optional "const" qualifiers, reference arguments, experimenting with refactoring and generally non-mandatory improvements for code quality.

The idea is far from new - [makeheaders](https://fossil-scm.org/home/doc/trunk/tools/makeheaders.html) is a decades-old tool to generate complete headers from C code.

But for C++ the task is more difficult, although IDE-specific plugins exist.

Enter `makeheaderspp`, a self-contained command line tool to update class declarations from their definitions.

Definitions need to be explicitly tagged for autogeneration, providing some auxillary information e.g. "static", "public", "virtual" that C++ allows for declarations only.

Further, only marked file sections are autogenerated. Other code is left unchanged. The tool performs sanity checks before rewriting files (e.g. any resulting declaration must have exactly one destination section).

The tool is not needed to build a project that uses it, as the autogenerated code remains in place in the source files.

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

A private static function would be tagged 
```
MHPP("private static") 
int myClass::myStaticFunction(int myArg){}
```
and a protected virtual method 
```
MHPP("protected virtual") 
int myClass::myVirtualMethod(int myArg){}
```
respectively.

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
* public / protected / private (must be first)
* static
* virtual

The following types of class methods are supported:
* regular function
* static function
* constructor
* destructor
* operator (?? check ??)

### Supported qualifiers (after arguments list)
* const
* noexcept

### Templates (function / variable arguments, static variables)
Templates usually do work, also using whitespaces (e.g. after comma: `map<string, int>)`as long as they don't make use of round or curly brackets (which is unusual e.g. in a string argument).


### Comments
Comments (single- or multiline) between `MHPP("begin classname")` and the definition will carry over to the declaration.

If this is not desired, place before the `MHPP` tag.

### pImpl generator
Optionally, one or more pImpl wrappers can be generated around a std::shared_ptr of a given class. See tests/testPImpl.cpp (experimental feature).

### Caveats
A "waterproof" C++ parser is a nontrivial project, and it wouldn't be complete without preprocessor and ultimately knowledge of command line defines.

`makeheaderspp` stays clear of that rabbit hole as it aims to be simple and accessible (e.g. just STL from C++17, no library dependencies). It uses fairly simple regular expressions that work "most of the time". If some construct cannot be autogenerated, it can always be added manually to the regular section of its header.

Whitespace characters are tolerated in typical places but not all legal variants are supported (e.g. around ::).

### Notes:
Note, there is no autogeneration for pure virtual or deleted methods as by definition :-) they don't have a definition.  

Files are only rewritten once all checks have passed (e.g. the redundant class name in `MHPP("begin myClass") ... MHPP("end myClass")`). Still, modifying files in-place is by nature somewhat risky e.g. file server connectivity issues. Consider using e.g. git as a safeguard.