CXXFLAGS := -O -static -std=c++17 -D _GLIBCXX_DEBUG
all: makeheaderspp.exe
makeheaderspp.exe: makeheaderspp.cpp
	g++ -o $@ makeheaderspp.cpp ${CXXFLAGS}
demo: makeheaderspp.exe
	./makeheaderspp.exe myRegexBase.cpp myRegexBase.h
clean: 
	rm -f makeheaderspp.exe
.PHONY: clean demo