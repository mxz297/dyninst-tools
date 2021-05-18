import sys
import os
import argparse
from subprocess import *
import tempfile

def getParameters():
    parser = argparse.ArgumentParser(description='Collect a list of binaries in a given directory')
    parser.add_argument("--srcdir", help ="The directory to search ", required=True)
    parser.add_argument("--out", help ="The output file list", required=True)
    args = parser.parse_args()
    return args

def isELFFormat(msg):
    if msg.find("ASCII") != -1:
        return False
    if msg.find("core file") != -1:
        return False
    if msg.find("ELF") == -1:
        return False
    return True
   
args = getParameters()
tmpfile = tempfile.NamedTemporaryFile(mode="w")
for root, dirs, files in os.walk(args.srcdir):
    for filename in files:
        srcPath = os.path.join(root, filename)
        tmpfile.write(srcPath + "\n")

cmd = "file -f {0}".format(tmpfile.name)
p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
msg, err = p.communicate()

with open(args.out, "w") as f:
    for line in msg.decode().split("\n")[:-1]:
        colon = line.find(":")
        path = line[:colon]
        result = line[colon+1:]
        if isELFFormat(result):
            f.write(path + "\n")
tmpfile.close()            
