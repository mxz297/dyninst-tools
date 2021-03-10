This is a simple tool built on ParseAPI and InstructionAPI of Dyninst to produce 
the control flow graph of a binary file.

Usage: ./CFG [options] binFile
-i: Output the instructions in each basic blocks
-n patterns: filter functions by matching the function names using the specified pattern
Example: -n "foo bar" means only output CFG for functions with function names containing substring "foo" or "bar"
-o outFileName: Specify the output file name
-p: Print inter-procedural edges

