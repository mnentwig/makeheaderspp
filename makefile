CXXFLAGS := -O0 -g -static -std=c++17 -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -fmax-errors=1
# remove -D_GLIBCXX_DEBUG for performance, add -DNDEBUG
all: makeheaderspp.exe
makeheaderspp.exe: src/makeheaderspp.cpp src/myRegexBase.cpp src/myRegexBase.h src/myAppRegex.cpp src/myAppRegex.h src/codeGen.cpp src/codeGen.h src/oneClass.cpp src/oneClass.h src/myRegexRange.cpp src/myRegexRange.h
	g++ -Isrc -o $@ src/makeheaderspp.cpp src/myRegexBase.cpp src/myAppRegex.cpp src/codeGen.cpp src/oneClass.cpp src/myRegexRange.cpp ${CXXFLAGS}

# run own code generation (only needed after code changes that change generated declarations)
# (don't add dependency on makeheaderspp.exe, rather use the last working binary) 
gen: 
	./makeheaderspp.exe src/myRegexBase.* src/myAppRegex.* src/oneClass.* src/codeGen.* src/myRegexRange.*
	@echo classes of makeheaderspp were successfully updated after code change.
	@echo Now run "make makeheaderspp.exe"

# run on sample program, compile, runtime checks
test: makeheaderspp.exe
# testcase code generation
	./makeheaderspp.exe tests/test.cpp
# compile test case
	g++ ${CXXFLAGS} -o tests/test.exe tests/test.cpp
# runtime checks
	tests/test.exe

	./makeheaderspp.exe tests/testPImpl.cpp
	g++ ${CXXFLAGS} -o tests/testPImpl.exe tests/testPImpl.cpp
	tests/testPImpl.exe
	@echo "success: test passed!"

reftest: makeheaderspp.exe
	cp tests/test.cpp tests/testBasic.cpp
	./makeheaderspp.exe tests/testBasic.cpp
	diff tests/testBasic.cpp tests/testBasicRef.cpp
	rm -f tests/testBasic.cpp

	cp tests/test.cpp tests/testAnnotate.cpp
	./makeheaderspp.exe -annotate tests/testAnnotate.cpp
	diff tests/testAnnotate.cpp tests/testAnnotateRef.cpp
	rm -f tests/testAnnotate.cpp

	cp tests/test.cpp tests/testClean.cpp
	./makeheaderspp.exe -clean tests/testClean.cpp
	diff tests/testClean.cpp tests/testCleanRef.cpp
	rm -f tests/testClean.cpp

	cp tests/testPImpl.cpp tests/testPImplCopy.cpp
	./makeheaderspp.exe tests/testPImplCopy.cpp
	diff tests/testPImplCopy.cpp tests/testPImplRef.cpp
	rm -f tests/testPImplCopy.cpp

	@echo "success: all test results are identical to reference results"

clean: 
	rm -f makeheaderspp.exe test.exe
.PHONY: clean test gen