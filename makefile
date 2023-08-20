CXXFLAGS := -O -static -std=c++17 -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -fmax-errors=1
# remove -D_GLIBCXX_DEBUG for performance, add -DNDEBUG
all: makeheaderspp.exe
makeheaderspp.exe: makeheaderspp.cpp myRegexBase.cpp myRegexBase.h myAppRegex.cpp myAppRegex.h codeGen.cpp codeGen.h oneClass.cpp oneClass.h
	g++ -o $@ makeheaderspp.cpp myRegexBase.cpp myAppRegex.cpp codeGen.cpp oneClass.cpp ${CXXFLAGS}

# run own code generation (only needed after code changes that change generated declarations)
# (don't add dependency on makeheaderspp.exe, rather use the last working binary) 
gen: 
	./makeheaderspp.exe myRegexBase.* myAppRegex.* oneClass.* codeGen.*

# run on sample program, compile, runtime checks
test:
# testcase code generation
	./makeheaderspp.exe test.cpp
# compile test case
	g++ ${CXXFLAGS} -o test.exe test.cpp
# runtime checks
	./test.exe
	@echo "test passed!"

clean: 
	rm -f makeheaderspp.exe test.exe
.PHONY: clean test gen