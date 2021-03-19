#include "Snippet.h"

class CoverageSnippet : public Dyninst::PatchAPI::Snippet {
public:
    bool generateNOPs(Dyninst::Buffer& buf);
};


class GlobalMemCoverageSnippet : public CoverageSnippet {

public:    
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override;
};

class ThreadLocalMemCoverageSnippet : public CoverageSnippet {

public:    
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer& buf) override;
};