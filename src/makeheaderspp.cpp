#include <cassert>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>

#include "codegen.h"
#include "myAppRegex.h"
//
using std::string, std::runtime_error, std::vector, std::set, std::map, std::cout;

int main(int argc, const char** argv) {
    std::map<std::string, myRegexBase::range> captures;
    assert(myAppRegex::CppTemplatedType.match("std::vector<int>", captures));

    // === copy command line args as filenames ===
    vector<string> filenames;
    set<string> uniqueFilenames;
    bool annotate = false;
    bool clean = false;

    if (argc <= 1) {
        cout << "usage: " << argv[0] <<  //
            " myfile1.cpp myfile2.h ...\n"
            "-annotate: add comment with declaration file and line\n"
            "-clean: remove all generated code\n";
        exit(0);
    }

    for (size_t ix = 1; ix < (size_t)argc; ++ix) {
        const string f = argv[ix];
        if (f == "-annotate")
            annotate = true;
        else if (f == "-clean")
            clean = true;
        else {
            filenames.push_back(f);
            if (!uniqueFilenames.insert(f).second)
                throw runtime_error("duplicate filename: '" + f + "'");
        }
    }

    if (annotate && clean) throw runtime_error("-annotate and -clean are mutually exclusive");

    codeGen cg(annotate);

    // === parse all files for declarations ===
    for (const string& filename : filenames)
        cg.pass1(filename, clean);

    // === fill in declarations ===
    for (const string& filename : filenames)
        cg.pass2(filename, clean);

    // === sanity check: all declarations referenced? ===
    if (!clean)
        cg.checkAllClassesDone();

    // === write output ===
    for (const string& filename : filenames)
        cg.pass3(filename);

    return 0;
}