CXXFLAGS := -O -static -std=c++17 -D_GLIBCXX_DEBUG -Wall -Wextra -pedantic
all: makeheaderspp.exe
makeheaderspp.exe: makeheaderspp.cpp
	g++ -o $@ makeheaderspp.cpp ${CXXFLAGS}
demo: makeheaderspp.exe
	./makeheaderspp.exe myRegexBase.cpp myRegexBase.h
	g++ ${CXXFLAGS} -g myApp.cpp myRegexBase.cpp
	./a.exe myRegexBase.cpp myRegexBase.h
clean: 
	rm -f makeheaderspp.exe
.PHONY: clean demo