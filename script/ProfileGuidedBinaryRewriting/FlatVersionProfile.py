import sys
from HPCToolkitDatabaseReader import HPCToolkitReader
import bisect

class VersionDataAnalyzer:
    """ Analyze HPCToolkit database to find instrumentation bottleneck
    """

    def __init__(self, reader):
        self.reader = reader

    def loadVersionMap(self, mapFileName):
        self.relocAddrs = []
        self.origAddrs = []
        self.sizes = []
        self.versions = []
        self.strings = []
        with open(mapFileName, "r") as f:
            lines = f.read().split("\n")
            totalMapEntry = int(lines[0])
            for line in lines[1: totalMapEntry+1]:
                parts = line.split()
                self.relocAddrs.append(int(parts[0], 16))
                if parts[1][0] == "-":
                    self.origAddrs.append(int(parts[1]))
                else:
                    self.origAddrs.append(int(parts[1], 16))
                self.sizes.append(int(parts[2], 16))
                self.versions.append(parts[3])
            totalStringEntry = int(lines[totalMapEntry+1])
            for line in lines[totalMapEntry+2:totalStringEntry+totalMapEntry+2]:
                self.strings.append(line)

    def collectMetrics(self):
        self.metric_id = None
        for metric_id, metric_name in self.reader.metric_names.items():
            if metric_name.find("(E)") != -1:
                self.metric_id = metric_id
        if self.metric_id is None:
            for metric_id, metric_name in self.reader.metric_names.items():
                self.metric_id = metric_id
                break

        self.metricList = []
        self.totalMetric = 0.0
        for cctNodeDict in self.reader.node_dicts:
            cctNode = cctNodeDict['node']
            if self.metric_id not in cctNode.metrics: continue
            metricValue = float(cctNode.metrics[self.metric_id])
            if cctNode.node_dict['type'] != 'S': continue
            addr = int(cctNode.node_dict['addr'], 16)
            self.metricList.append( (addr, metricValue))
            self.totalMetric += metricValue
        print ("Total metric", self.totalMetric)

    def printInstrumentationProfile(self):
        self.metricMap = {}
        for s in self.strings:
            self.metricMap[s] = 0.0
        self.metricMap["other"] = 0.0
        self.metricMap["original"] = 0.0
        for addr, metric in self.metricList:
            s , _ = self.findAddrString(addr)
            self.metricMap[s] += metric
        print ("Instrumentation Profile:")
        for k, v in self.metricMap.items():
            print (k, v, v * 100.0 / self.totalMetric)

    def printVersionProfile(self):
        self.metricMap = {}
        for s in self.versions:
            self.metricMap[s] = 0.0
        self.metricMap["other"] = 0.0
        for addr, metric in self.metricList:
            _ , v = self.findAddrString(addr)
            self.metricMap[v] += metric
        print ("Version Profile:")
        profileList = []
        for k, v in self.metricMap.items():
            profileList.append( (v, k) )
        profileList.sort(reverse=True)
        for m, v in profileList:
            if m == 0.0: break
            print (v, m, m * 100.0 / self.totalMetric)


    def findAddrString(self, addr):
        index = bisect.bisect(self.relocAddrs, addr)
        if index == 0:
            return "other", "other"
        index -= 1
        if self.relocAddrs[index] <= addr and addr < self.relocAddrs[index] + self.sizes[index]:
            if self.origAddrs[index] > 0:
                return "original", self.versions[index]
            else:
                return self.strings[-self.origAddrs[index] - 1], self.versions[index]
        else:
            return "other", "other"

def main():
    sys.setrecursionlimit(0x10000)
    if len(sys.argv) > 2:
        reader = HPCToolkitReader(sys.argv[1])
        reader.create_graphframe()
        inst = VersionDataAnalyzer(reader)
        inst.loadVersionMap(sys.argv[2])
        inst.collectMetrics()
        inst.printInstrumentationProfile()
        inst.printVersionProfile()


    else:
        print ("Syntax: ", sys.argv[0], " <database_dir>")

if __name__ == "__main__":
    main()
