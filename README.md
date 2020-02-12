# dyninst-tools
A collection of command-line tools based on Dyninst (https://github.com/dyninst/dyninst).

These tools can be used as examples of showing how to use Dyninst and as testing and debugging tools for Dyninst.

## Shadow Stack

SimpleShadowStack.cpp: A mutator for implementing a shadow stack. It illustrates the basic usage of using Dyninst to rewrite an executable

MemoryAccessShadowStack.cpp: A mutator that analyze each function to decide whether or not to insert instrumentation. It represents a more advanced use of Dyninst by combining static binary analysis with instrumentation

shadowstack.c: A shared library for shadow stack instrumentation 

## Insert an code buffer as instrumentation

The BPatch interface in Dyninst provides a snippet based interface to construct instrumentation via a set of `BPatch_snippet` classes. However, when the user wants to experiment with new instructions in a new ISA, the BPatch interface does not provide a way to do that.

In this example, we use PatchAPI, a component library in Dyninst to show how to insert a code buffer as instrumentation. The user will be responsible to emit machine instructions to the code buffer. Two useful software tools for encoding instructions are asmjit (https://github.com/asmjit/asmjit) and Intel Xed (https://github.com/intelxed/xed).

## ParseAPI tools

CFGConsistency: This tool checks whether the blocks acquired through traversing the CFG match the ones acquired through looking up by addresses

CodeDump: This tool iterates every function and basic block and prints the instructions in each basic block. This tool can be used to check whether there are missing entries in instruction decoding tables inside Dyninst.
