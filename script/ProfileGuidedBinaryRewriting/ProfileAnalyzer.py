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
        self.loopOverhead = {}        
        self.functionCallChainOverhead = {}

        self.metric_id = None
        for metric_id, metric_name in self.reader.metric_names.items():
            if metric_name.find("(E)") != -1:
                self.metric_id = metric_id
        if self.metric_id is None:
            for metric_id, metric_name in self.reader.metric_names.items():
                self.metric_id = metric_id
                break

        self.metric_count_set = set()

    def bottomUpViewForInstrumentation(self, topk, resultType):
        if resultType == "function":
            self.summarizeFunction(topk)
        elif resultType == "loop":
            self.summarizeLoop(topk)
        elif resultType == "callpair":
            self.summarizeCallerCallee(topk)
        elif resultType == "callsite":
            self.summarizeCallSite(topk)
        else:
            print ("Unsupported analysis type", resultType)

    def summarizeFunction(self, topk):                
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

        funcList = []
        for k , v in self.functionOverhead.items():
            funcList.append((v, k ))
        funcList.sort(reverse=True)
        K = 0
        for m, addr in funcList:
            K += 1
            print(addr[2:])
            if K == topk: break

    def summarizeLoop(self, topk):
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

        loopList = []
        for k , v in self.loopOverhead.items():
            loopList.append((v, k))
        loopList.sort(reverse=True)
        K = 0
        for m, addr in loopList:
            K += 1
            print(addr[2:])
            if K == topk: break

    def summarizeCallerCallee(self, topk):
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

        funcCallList = []
        for k , v in self.functionCallChainOverhead.items():
            funcCallList.append((v, k))
        funcCallList.sort(reverse=True)
        K = 0
        for m, funcPair in funcCallList:
            K += 1 
            #print("{0}({1}) --> {2}({3}) : {4}".format(
            #    self.functionAddrNamesMap[funcPair[0]], funcPair[0],
            #    self.functionAddrNamesMap[funcPair[1]], funcPair[1],
            #    m))

            print ("{0} {1}".format(funcPair[0][2:], funcPair[1][2:]))
            if K == topk: break

    def summarizeCallSite(self, topk):
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            funcAddr, containingNode = self.findParentFunction(cctNode)
            if funcAddr == "0":                
            #    print ("callee addr 0",containingNode.node_dict)
                continue

            callerAddr, callerNode = self.findParentCallSite(containingNode)
            if callerAddr == "0":                
            #    print ("caller addr 0", callerNode.node_dict)
                continue

            callPairKey = (callerAddr, funcAddr)
            val = self.findChildMetric(cctNode)
            if callPairKey not in self.functionCallChainOverhead:
                self.functionCallChainOverhead[callPairKey] = 0
            self.functionCallChainOverhead[callPairKey] += val

        funcCallList = []
        for k , v in self.functionCallChainOverhead.items():
            funcCallList.append((v, k))
        funcCallList.sort(reverse=True)
        K = 0
        for m, funcPair in funcCallList:
            K += 1 
            #print("{0}({1}) --> {2}({3}) : {4}".format(
            #    self.functionAddrNamesMap[funcPair[0]], funcPair[0],
            #    self.functionAddrNamesMap[funcPair[1]], funcPair[1],
            #    m))

            print ("{0} {1}".format(funcPair[0][2:], funcPair[1][2:]))
            if K == topk: break

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
    if len(sys.argv) > 1:
        #print ("reading: ", sys.argv[1])
        reader = HPCToolkitReader(sys.argv[1])
        if len(sys.argv) > 2:
            topk = int(sys.argv[2])
        else:
            topk = 10
        if len(sys.argv) > 3:
            resultType = sys.argv[3]
        else:
            resultType = "callpair"
        reader.create_graphframe()

        pgo = InstrumentDataAnalyzer(reader)
        pgo.bottomUpViewForInstrumentation(topk, resultType)


    else:
        print ("Syntax: ", sys.argv[0], " <database_dir>")

if __name__ == "__main__":
    main()
