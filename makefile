CXXFLAGS := -O -static -std=c++17 -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -fmax-errors=1
#-DNDEBUG
all: makeheaderspp.exe
makeheaderspp.exe: makeheaderspp.cpp myRegexBase.cpp myRegexBase.h myAppRegex.cpp myAppRegex.h
	g++ -o $@ makeheaderspp.cpp myRegexBase.cpp myAppRegex.cpp codeGen.cpp oneClass.cpp ${CXXFLAGS}

# run own code generation (no need if building makeheaderspp.exe - run only after editing the respective classes)
gen: 
	./makeheaderspp.exe myRegexBase.* myAppRegex.* oneClass.* codeGen.*

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