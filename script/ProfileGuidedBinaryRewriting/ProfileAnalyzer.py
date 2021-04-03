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
        self.functionNames = {}


    def bottomUpViewForInstrumentation(self):
        self.instOverhead = {}
        for cctNodeDict in self.reader.node_dicts:
            if cctNodeDict['name'] != self.instrumentationFrameName: continue
            cctNode = cctNodeDict['node']
            funcName, funcAddr = self.findParentFunction(cctNode)
            val = self.findChildMetric(cctNode)
            if funcAddr not in self.functionOverhead:
                self.functionOverhead[funcAddr] = 0
                self.functionNames[funcAddr] = funcName
            self.functionOverhead[funcAddr] += val

        funcList = []
        for k , v in self.functionOverhead.items():
            funcList.append((v, k , self.functionNames[k]))
        funcList.sort(reverse=True)
        for m, addr, name in funcList:
            print(name, addr, m)                            

    def findParentFunction(self, cctNode):
        assert(len(cctNode.parents) == 1)
        curNode = cctNode.parents[0]
        while curNode.node_dict['type'] != 'PF' and curNode.node_dict['type'] != 'Pr':
            assert(len(curNode.parents) == 1)
            curNode = curNode.parents[0]
        return curNode.node_dict['name'], self.reader.procedure_addr[curNode.node_dict['name']]

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
