CXXFLAGS := -O -static -std=c++17 -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG 
#-DNDEBUG
all: makeheaderspp.exe
makeheaderspp.exe: makeheaderspp.cpp myRegexBase.cpp myRegexBase.h
	g++ -o $@ makeheaderspp.cpp ${CXXFLAGS}

# run own code generation (no need if building makeheaderspp.exe - run only after editing the respective classes)
gen: makeheaderspp.exe
	./makeheaderspp.exe myRegexBase.cpp myRegexBase.h

test: makeheaderspp.exe
# testcase code generation
	./makeheaderspp.exe test.cpp
# compile test case
	g++ ${CXXFLAGS} -o test.exe test.cpp
# runtime checks
	./test.exe

clean: 
	rm -f makeheaderspp.exe test.exe
.PHONY: clean test gen