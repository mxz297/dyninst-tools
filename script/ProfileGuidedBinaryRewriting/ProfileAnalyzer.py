import sys
from HPCToolkitDatabaseReader import HPCToolkitReader

class InstrumentDataAnalyzer:
    """ Analyze HPCToolkit database to find instrumentation bottleneck
    """

    def __init__(self, reader):
        self.reader = reader
        self.instrumentationFrameName = "dyninst_instrumentation_op"

        # Key is the function address
        self.functionOverhead = {}
        self.functionAddrNamesMap = {}

        for name, addr in self.reader.procedure_addr.items():
            if addr == "0": continue 
            self.functionAddrNamesMap[addr] = name

        self.functionCallChainOverhead = {}


    def bottomUpViewForInstrumentation(self):
        self.instOverhead = {}
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            funcAddr, containingNode = self.findParentFunction(cctNode)
            if funcAddr == "0":
                print ("callee addr 0",containingNode.node_dict)            
            val = self.findChildMetric(cctNode)
            if funcAddr not in self.functionOverhead:
                self.functionOverhead[funcAddr] = 0
            self.functionOverhead[funcAddr] += val

            callerAddr, callerNode = self.findParentFunction(containingNode)
            if callerAddr == "0":
                print ("caller addr 0", callerNode.node_dict)

            callPairKey = (callerAddr, funcAddr)
            if callPairKey not in self.functionCallChainOverhead:
                self.functionCallChainOverhead[callPairKey] = 0
            self.functionCallChainOverhead[callPairKey] += val

        """
        funcList = []
        for k , v in self.functionOverhead.items():
            funcList.append((v, k , self.functionNames[k]))
        funcList.sort(reverse=True)
        for m, addr, name in funcList:
            print(name, addr, m)                            
        """
        funcCallList = []
        for k , v in self.functionCallChainOverhead.items():
            funcCallList.append((v, k))
        funcCallList.sort(reverse=True)
        K = 0
        for m, funcPair in funcCallList:
            K += 1
            print("{0}({1}) --> {2}({3}) : {4}".format(
                self.functionAddrNamesMap[funcPair[0]], funcPair[0],
                self.functionAddrNamesMap[funcPair[1]], funcPair[1],
                m))
            if K == 10: break



    def findParentFunction(self, cctNode):
        assert(len(cctNode.parents) == 1)
        curNode = cctNode.parents[0]
        while curNode.node_dict['type'] != 'PF':
            assert(len(curNode.parents) == 1)
            curNode = curNode.parents[0]
        return self.reader.procedure_addr[curNode.node_dict['name']], curNode

    def findChildMetric(self, cctNode):
        assert(len(cctNode.children) == 1)
        curNode = cctNode.children[0]
        while len(curNode.metrics) == 0:
            assert(len(curNode.children) == 1)
            curNode = curNode.children[0]
        return curNode.metrics[0]




def main():
    if len(sys.argv) > 1:
        print ("reading: ", sys.argv[1])
        reader = HPCToolkitReader(sys.argv[1])
        reader.create_graphframe()

        pgo = InstrumentDataAnalyzer(reader)
        pgo.bottomUpViewForInstrumentation()


    else:
        print ("Syntax: ", sys.argv[0], " <database_dir>")

if __name__ == "__main__":
    main()
