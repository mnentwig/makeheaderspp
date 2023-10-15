#include "common.h"

using std::to_string;
MHPP("public static")
std::string common::errmsg(const regionizedText& body, csit_t iBegin, csit_t iEnd, const string& filename, const string& msg) {
    size_t lineBegin;
    size_t charBegin;
    size_t lineEnd;
    size_t charEnd;
    body.regionInSource(iBegin, iEnd, /*base1*/ true, lineBegin, charBegin, lineEnd, charEnd);
    string ret;
    if (filename.size() > 0)
        ret += filename + string(":");
    ret += "l" + to_string(lineBegin) + "c" + to_string(charBegin);
    if ((lineEnd != lineBegin) || (charEnd != charBegin)) {
        ret += "..l" + to_string(lineEnd) + "c" + to_string(charEnd);
    }

    const size_t nCharMax = 200;
    if (msg.size() > 0)
        ret += ":" + msg;
    if (iEnd > iBegin + nCharMax)
        ret += "\nSource:\n" + string(iBegin, iBegin + nCharMax) + "...";
    else
        ret += "\nSource:\n" + string(iBegin, iEnd);
    return ret;
}

MHPP("public static")
// construct error message relating to region r of source file 'filename'
std::string common::errmsg(const stringRegion& r, const string& filename, const string& msg) {
    size_t lineBegin;
    size_t charBegin;
    size_t lineEnd;
    size_t charEnd;
    r.regionInSource(/*base1*/ true, lineBegin, charBegin, lineEnd, charEnd);
    string ret;
    if (filename.size() > 0)
        ret += filename + string(":");
    ret += "l" + to_string(lineBegin) + "c" + to_string(charBegin);
    if ((lineEnd != lineBegin) || (charEnd != charBegin)) {
        ret += "..l" + to_string(lineEnd) + "c" + to_string(charEnd);
    }

    const size_t nCharMax = 200;
    if (msg.size() > 0)
        ret += ":" + msg;
    if (r.size() >= nCharMax)
        ret += "\nSource:\n" + r.str().substr(0, nCharMax) + "...";
    else
        ret += "\nSource:\n" + r.str();
    return ret;
}
