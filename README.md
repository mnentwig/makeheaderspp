# makeheaderspp
Automatic header generation for C++ classes

### Motivation
The tool auto-generates class method declarations in C++ headers from their implementation. 
It modifies only user-defined parts of a header, while the remaining lines are edited conventionally. 

### "Inspiration"
For routine programming work in C++, maintaining class header files seems an unproductive routine task that could be easily automated.

The idea is far from new - [makeheaders](https://fossil-scm.org/home/doc/trunk/tools/makeheaders.html) is a decades-old tool to generate complete headers from C code.

For C++ the task is more difficult, although IDE-specific plugins exist.

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

A static/virtual function would be tagged e.g. 
```
MHPP("public static virtual") 
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

### Comments
Comments (single- or multiline) between `MHPP("begin classname")` and the definition will carry over to the declaration.

If this is not desired, place before the `MHPP` tag.

### Caveats
A "waterproof" C++ parser is a nontrivial task, and it wouldn't be complete without preprocessor, which requires setup of environmental and command line defines.

`makeheaderspp` aims to be a simple tool (e.g. no library dependencies, just STL), stays clear of that rabbit hole and uses fairly simple regular expressions that work most of the time. 

If some construct cannot be autogenerated, just add it manually to the regular section of its header.

Whitespace characters are tolerated in typical places but not all legal variants are supported (e.g. around ::).

### Notes:
The tool attempts to be failsafe and rewrites files only when all checks have passed (note: the redundant class name in `MHPP("begin myClass") ... MHPP("end myClass")`is one of them). 

However, modifying files in-place is inherently somewhat risky. Use e.g. git.