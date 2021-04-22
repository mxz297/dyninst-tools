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
public:
    GlobalMemCoverageSnippet();
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override;
    void print() override;
};

class ThreadLocalMemCoverageSnippet : public CoverageSnippet {
    int offset;
public:
    ThreadLocalMemCoverageSnippet();
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override;
    void print() override;
};

#endif