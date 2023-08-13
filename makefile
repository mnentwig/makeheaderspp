CXXFLAGS := -O -static -std=c++17 -D _GLIBCXX_DEBUG
all: makeheaderspp.exe
makeheaderspp.exe: makeheaderspp.cpp
	g++ -o $@ makeheaderspp.cpp ${CXXFLAGS}
demo: makeheaderspp.exe
	./makeheaderspp.exe myRegexBase.cpp myRegexBase.h
	g++ myApp.cpp myRegexBase.cpp
	./a.exe
clean: 
	rm -f makeheaderspp.exe
.PHONY: clean demo