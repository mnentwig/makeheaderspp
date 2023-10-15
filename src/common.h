#pragma once
#include "regionizedText.h"
#include "stringRegion.h"
class common {
    MHPP("begin common") // === autogenerated code. Do not edit ===
    public:
    	static std::string errmsg(const regionizedText& body, csit_t iBegin, csit_t iEnd, const string& filename, const string& msg);
    	// construct error message relating to region r of source file 'filename'
    	static std::string errmsg(const stringRegion& r, const string& filename, const string& msg);
    MHPP("end common")
};