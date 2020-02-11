# dyninst-tools
A collection of command-line tools based on Dyninst (https://github.com/dyninst/dyninst).

These tools can be used as examples of showing how to use Dyninst and as testing and debugging tools for Dyninst.

## Shadow Stack

SimpleShadowStack.cpp: A mutator for implementing a shadow stack. It illustrates the basic usage of using Dyninst to rewrite an executable

MemoryAccessShadowStack.cpp: A mutator that analyze each function to decide whether or not to insert instrumentation. It represents a more advanced use of Dyninst by combining static binary analysis with instrumentation

shadowstack.c: A shared library for shadow stack instrumentation 

## ParseAPI tools

CFGConsistency: This tool checks whether the blocks acquired through traversing the CFG match the ones acquired through looking up by addresses

CodeDump: This tool iterates every function and basic block and prints the instructions in each basic block. This tool can be used to check whether there are missing entries in instruction decoding tables inside Dyninst.
