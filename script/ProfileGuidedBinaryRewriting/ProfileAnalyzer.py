import sys
from HPCToolkitDatabaseReader import HPCToolkitReader

class InstrumentDataAnalyzer:
    """ Analyze HPCToolkit database to find instrumentation bottleneck
    """

    def __init__(self, readers):
        self.readers = readers
        self.instrumentationFrameName = "dyninst_instrumentation_op"

        # Key is the function address
        self.functionOverhead = {}
        self.loopOverhead = {}
        self.functionCallChainOverhead = {}
        self.addressOverhead = {}
        self.unaccounted = 0

    def bottomUpViewForInstrumentation(self, resultType):
        for reader in self.readers:
            self.reader = reader
            self.metric_id = None
            for metric_id, metric_name in self.reader.metric_names.items():
                if metric_name.find("(E)") != -1:
                    self.metric_id = metric_id
            if self.metric_id is None:
                for metric_id, metric_name in self.reader.metric_names.items():
                    self.metric_id = metric_id
                    break
            if resultType == "function":
                self.summarizeFunction()
            elif resultType == "loop":
                self.summarizeLoop()
            elif resultType == "callpair":
                self.summarizeCallerCallee()
            elif resultType == "callsite":
                self.summarizeCallSite()
            elif resultType == "address":
                self.summarizeAddress()
            else:
                print ("Unsupported analysis type", resultType)
                return
        if resultType == "function":
            self.printFunction()
        elif resultType == "loop":
            self.printLoop()
        elif resultType == "callpair":
            self.printCallerCallee()
        elif resultType == "callsite":
            self.printCallSite()
        elif resultType == "address":
            self.printAddress()

    def summarizeFunction(self):
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            funcAddr, containingNode = self.findParentFunction(cctNode)
            if funcAddr == "0":
            #    print ("callee addr 0",containingNode.node_dict)
                continue

            val = self.findChildMetric(cctNode)
            if funcAddr not in self.functionOverhead:
                self.functionOverhead[funcAddr] = 0
            self.functionOverhead[funcAddr] += val

    def printFunction(self):
        funcList = []
        for k , v in self.functionOverhead.items():
            funcList.append((v, k ))
        funcList.sort(reverse=True)
        for m, addr in funcList:
            print(addr[2:], m)

    def summarizeLoop(self):
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            loopAddr, containingNode = self.findParentLoop(cctNode)
            if loopAddr == "0":
                continue

            val = self.findChildMetric(cctNode)
            if loopAddr not in self.loopOverhead:
                self.loopOverhead[loopAddr] = 0
            self.loopOverhead[loopAddr] += val

    def printLoop(self):
        loopList = []
        for k , v in self.loopOverhead.items():
            loopList.append((v, k))
        loopList.sort(reverse=True)
        for m, addr in loopList:
            print(addr[2:], m)

    def summarizeCallerCallee(self):
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            funcAddr, containingNode = self.findParentFunction(cctNode)
            if funcAddr == "0":
            #    print ("callee addr 0",containingNode.node_dict)
                continue

            callerAddr, callerNode = self.findParentFunction(containingNode)
            if callerAddr == "0":
            #    print ("caller addr 0", callerNode.node_dict)
                continue

            callPairKey = (callerAddr, funcAddr)
            val = self.findChildMetric(cctNode)
            if callPairKey not in self.functionCallChainOverhead:
                self.functionCallChainOverhead[callPairKey] = 0
            self.functionCallChainOverhead[callPairKey] += val

    def printCallerCallee(self):
        funcCallList = []
        for k , v in self.functionCallChainOverhead.items():
            funcCallList.append((v, k))
        funcCallList.sort(reverse=True)
        for m, funcPair in funcCallList:
            #print("{0}({1}) --> {2}({3}) : {4}".format(
            #    self.functionAddrNamesMap[funcPair[0]], funcPair[0],
            #    self.functionAddrNamesMap[funcPair[1]], funcPair[1],
            #    m))

            print ("{0} {1} {2}".format(funcPair[0][2:], funcPair[1][2:], m))

    def summarizeCallSite(self):
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            val = self.findChildMetric(cctNode)
            funcAddr, containingNode = self.findParentFunction(cctNode)
            if funcAddr == "0":
            #    print ("callee addr 0",containingNode.node_dict)
                self.unaccounted += val
                continue

            callerAddr, callerNode = self.findParentCallSite(containingNode)
            if callerAddr == "0":
            #    print ("caller addr 0", callerNode.node_dict)
                self.unaccounted += val
                continue

            callPairKey = (callerAddr, funcAddr)
            if callPairKey not in self.functionCallChainOverhead:
                self.functionCallChainOverhead[callPairKey] = 0
            self.functionCallChainOverhead[callPairKey] += val

    def printCallSite(self):
        funcCallList = []
        for k , v in self.functionCallChainOverhead.items():
            funcCallList.append((v, k))
        funcCallList.sort(reverse=True)
        for m, funcPair in funcCallList:
            #print("{0}({1}) --> {2}({3}) : {4}".format(
            #    self.functionAddrNamesMap[funcPair[0]], funcPair[0],
            #    self.functionAddrNamesMap[funcPair[1]], funcPair[1],
            #    m))

            print ("{0} {1} {2}".format(funcPair[0][2:], funcPair[1][2:], m))

    def summarizeAddress(self):
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            val = self.findChildMetric(cctNode)
            callerAddr, callerNode = self.findParentCallSite(cctNode)
            if callerAddr == "0":
            #    print ("caller addr 0", callerNode.node_dict)
                self.unaccounted += val
                continue

            if callerAddr not in self.addressOverhead:
                self.addressOverhead[callerAddr] = 0
            self.addressOverhead[callerAddr] += val

    def printAddress(self):
        addressList = []
        for k , v in self.addressOverhead.items():
            addressList.append((v, k))
        addressList.sort(reverse=True)
        for m, addr in addressList:            
            print ("{0} {1}".format(addr[2:], m))

    def findParentFunction(self, cctNode):
        assert(len(cctNode.parents) == 1)
        curNode = cctNode.parents[0]
        while curNode.node_dict['type'] != 'PF':
            assert(len(curNode.parents) == 1)
            curNode = curNode.parents[0]
        return curNode.node_dict['addr'], curNode

    def findParentCallSite(self, cctNode):
        assert(len(cctNode.parents) == 1)
        curNode = cctNode.parents[0]
        while curNode.node_dict['type'] != 'C' and len(curNode.parents) > 0:
            assert(len(curNode.parents) == 1)
            curNode = curNode.parents[0]
        if curNode.node_dict['type'] != 'C':
            return '0' , curNode
        return curNode.node_dict['addr'], curNode

    def findParentLoop(self, cctNode):
        assert(len(cctNode.parents) == 1)
        curNode = cctNode.parents[0]
        while curNode.node_dict['type'] != 'PF' and curNode.node_dict['type'] != 'L':
            assert(len(curNode.parents) == 1)
            curNode = curNode.parents[0]
        if curNode.node_dict['type'] == 'PF':
            return '0', curNode
        else:
            return curNode.node_dict['addr'], curNode


    def findChildMetric(self, cctNode):
        #assert(len(cctNode.children) == 1)
        curNode = cctNode.children[0]
        while self.metric_id not in curNode.metrics:
            #assert(len(curNode.children) == 1)
            curNode = curNode.children[0]
        return curNode.metrics[self.metric_id]

def main():
    sys.setrecursionlimit(0x10000)
    if len(sys.argv) > 2:
        resultType = sys.argv[-1]
        readers = []

        for path in sys.argv[1:-1]:
            reader = HPCToolkitReader(path)
            reader.create_graphframe()
            readers.append(reader)
        pgo = InstrumentDataAnalyzer(readers)
        pgo.bottomUpViewForInstrumentation(resultType)


    else:
        print ("Syntax: ", sys.argv[0], " <database_dir>")

if __name__ == "__main__":
    main()
