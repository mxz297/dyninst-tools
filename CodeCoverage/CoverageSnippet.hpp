#ifndef COVERAGE_SNIPPET_HPP
#define COVERAGE_SNIPPET_HPP

#include "Snippet.h"

class CoverageSnippet : public Dyninst::PatchAPI::Snippet {
public:
    using Ptr = boost::shared_ptr<CoverageSnippet>;
    bool generateNOPs(Dyninst::Buffer& buf);
    void virtual print();
};


class GlobalMemCoverageSnippet : public CoverageSnippet {
    Dyninst::Address memLoc;
    static std::map<Dyninst::Address, Dyninst::Address> locMap;
public:
    GlobalMemCoverageSnippet(Dyninst::Address);
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override;
    void print() override;
};

class ThreadLocalMemCoverageSnippet : public CoverageSnippet {
    int offset;
    static std::map<Dyninst::Address, int> locMap;    
public:
    static int gsOffset;
    static void printCoverage(std::string&);
    ThreadLocalMemCoverageSnippet(Dyninst::Address);
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override;
    void print() override;
};

#endif