import argparse
from subprocess import *
import sys

def getParameters():
     parser = argparse.ArgumentParser(description='Repeatatively run parallel parsing to find non-deterministic results')
     parser.add_argument("--filelist", type=str, help="a list of binary for benchmarking")
     parser.add_argument("--binpath", type=str, help="the path to a binary for benchmarking")
     parser.add_argument("--exe", type=str, default="./micro-parse-func-level", help="the path to the microbenchmark executeable")
     parser.add_argument("--rep", type=int, default=100, help="the number of times of execution for calculating average run time")
     parser.add_argument("--thread", type=int, default=16, help="the number of threads")
     parser.add_argument("--cmp", type=int, default=1, help="Whether or not to compare results from single threaded run to multi-threaded runs")
     parser.add_argument("--valgrind", type=int, default=0, help="Whether or not run with valgrind for memory check")
     args = parser.parse_args()
     return args

def Run(filepath, thread, i):
    if args.valgrind > 0:
        cmd = "OMP_NUM_THREADS={1} valgrind {0} {2} > t{3}.txt 2>&1".format(args.exe, thread, filepath, i)
    else:
        cmd = "OMP_NUM_THREADS={1} {0} {2} > t{1}.txt".format(args.exe, thread, filepath)
    p = Popen(cmd, stdout=PIPE, stderr=PIPE,  shell=True)
    msg, err = p.communicate()
    if thread > 1 and args.cmp > 0:
        cmd = "diff t1.txt t{0}.txt".format(thread)
        p = Popen(cmd, stdout=PIPE, stderr=PIPE,  shell=True)
        msg, err = p.communicate()
        if len(msg) > 0:
            print "There are difference"
            sys.exit()
        if len(err) > 0:
            print err
            sys.exit()

args = getParameters()
if args.filelist != None:
    bins = []
    for line in open(args.filelist, "r"):
        bins.append(line[:-1])
else:
    bins = [args.binpath]

for filepath in bins:
    print "Experiment for", filepath
    if args.cmp > 0:
        Run(filepath, 1, 0)
    for i in range(args.rep):
        print i
        Run(filepath, args.thread, i)
