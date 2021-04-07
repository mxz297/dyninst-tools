#  Copy from https://github.com/HPCToolkit/hpcviewer/blob/713dd1f1e6624756d6aaddc699b249b02ee154ab/edu.rice.cs.hpc.data/script/hpcread.py

#
#  Parsing HPCToolkit experiment.xml file
#
#  Code modified and taken from Hatchet tool:
#  https://github.com/LLNL/hatchet/blob/master/hatchet/hpctoolkit_reader.py

import sys

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

from functools import total_ordering

src_file = 0

@total_ordering
class Node:
    """ A node in the graph. The node only stores its callpath.
    """

    def __init__(self, nid, callpath_tuple, parent):
        self.nid = nid
        self.callpath = callpath_tuple

        self.parents = []
        if parent is not None:
            self.add_parent(parent)
        self.children = []
        self.metrics = {}
        self.node_dict = None

    def add_parent(self, node):
        """ Adds a parent to this node's list of parents.
        """
        assert isinstance(node, Node)
        self.parents.append(node)

    def add_child(self, node):
        """ Adds a child to this node's list of children.
        """
        assert isinstance(node, Node)
        self.children.append(node)

    def add_metric(self, metrid_id, val):
        self.metrics[metrid_id] = val

    def set_node_dict(self, node_dict):
        self.node_dict = node_dict

    def traverse(self, order='pre'):
        """ Traverse the tree depth-first and yield each node.
        """
        if order == 'pre':
            yield self

        for child in self.children:
            for item in child.traverse(order):
                yield item

        if order == 'post':
            yield self

    def traverse_bf(self):
        """ Traverse the tree breadth-first and yield each node.
        """
        yield self
        last = self

        for node in self.traverse_bf():
            for child in node.children:
                yield child
                last = child
            if last == node:
                return

    def set_callpath(self, callpath):
        self.callpath = callpath

    def __hash__(self):
        return hash(id(self))

    def __eq__(self, other):
        return (id(self) == id(other))

    def __lt__(self, other):
        return (self.callpath < other.callpath)

    def __str__(self):
        """ Returns a string representation of the node.
        """
        return self.callpath[-1]

class HPCToolkitReader:
    """ Read in the various sections of an HPCToolkit experiment.xml file
        and metric-db files.
    """

    def __init__(self, dir_name):
        # this is the name of the HPCToolkit database directory. The directory
        # contains an experiment.xml and some metric-db files
        self.dir_name = dir_name

        root = ET.parse(self.dir_name + '/experiment.xml').getroot()
        self.loadmodule_table = next(root.iter('LoadModuleTable'))
        self.file_table = next(root.iter('FileTable'))
        self.procedure_table = next(root.iter('ProcedureTable'))
        self.metric_table = next(root.iter('MetricTable'))
        self.metricdb_table = next(root.iter('MetricDBTable'))
        self.callpath_profile = next(root.iter('SecCallPathProfileData'))

        self.load_modules = {}
        self.src_files = {}
        self.procedure_names = {}
        self.metric_names = {}
        self.metricDB_names = {}

        self.procedure_addr = {}
        self.loop_addr = {}

        # this list of dicts will hold all the node information such as
        # procedure name, load module, filename etc. for all the nodes
        self.node_dicts = []


    def fill_tables(self):
        """ Read certain sections of the experiment.xml file to create dicts
            of load modules, src_files, procedure_names, and metric_names
        """
        for loadm in (self.loadmodule_table).iter('LoadModule'):
            self.load_modules[loadm.get('i')] = loadm.get('n')

        for filename in (self.file_table).iter('File'):
            self.src_files[filename.get('i')] = filename.get('n')

        for procedure in (self.procedure_table).iter('Procedure'):
            idx = procedure.get('i')
            self.procedure_names[idx] = procedure.get('n')
            self.procedure_addr[self.procedure_names[idx]] = procedure.get('v')

        for metric in (self.metric_table).iter('Metric'):
            self.metric_names[metric.get('i')] = metric.get('n')

        for metric in (self.metricdb_table).iter('MetricDB'):
            self.metricDB_names[metric.get('i')] = metric.get('n')

    def read_all_metricdb_files(self):
        """ Read all the metric-db files and create a dataframe with num_nodes
            X num_pes rows and num_metrics columns. Two additional columns
            store the node id and MPI process rank.
        """
        # all the metric data per node and per process is read into the metrics
        # array below. The two additional columns are for storing the implicit
        # node id (nid) and MPI process rank.

        # once all files have been read, create a dataframe of metrics
        metric_names = [self.metric_names[key] for key in sorted(self.metric_names.keys())]
        for idx, name in enumerate(metric_names):
            if name == 'CPUTIME (usec) (E)':
                metric_names[idx] = 'time'
            if name == 'CPUTIME (usec) (I)':
                metric_names[idx] = 'time (inc)'

        self.metric_columns = metric_names
        df_columns = self.metric_columns + ['nid', 'rank']

    def create_graphframe(self):
        """ Read the experiment.xml file to extract the calling context tree
            and create a dataframe out of it. Then merge the two dataframes to
            create the final dataframe.
        """
        self.fill_tables()

        self.read_all_metricdb_files()

        list_roots = []

        # parse the ElementTree to generate a calling context tree
        for root in self.callpath_profile.findall('PF'):
            nid = int(root.get('i'))

            # start with the root and create the callpath and node for the root
            # also a corresponding node_dict to be inserted into the dataframe
            node_callpath = []
            node_callpath.append(self.procedure_names[root.get('n')])
            graph_root = Node(nid, tuple(node_callpath), None)
            node_dict = self.create_node_dict(nid, graph_root,
                self.procedure_names[root.get('n')], 'PF',
                self.src_files[root.get('f')], root.get('l'),
                self.load_modules[root.get('lm')])

            self.node_dicts.append(node_dict)
            list_roots.append(graph_root)

            # start graph construction at the root
            self.parse_xml_children(root, graph_root, list(node_callpath))

        # set the index to be a MultiIndex
        indices = ['node', 'rank']

        # create list of exclusive and inclusive metric columns
        exc_metrics = []
        inc_metrics = []
        for column in self.metric_columns:
            if '(inc)' in column:
                inc_metrics.append(column)
            else:
                exc_metrics.append(column)

        return exc_metrics, inc_metrics

    def parse_xml_children(self, xml_node, hnode, callpath):
        """ Parses all children of an XML node.
        """
        for xml_child in xml_node.getchildren():
            if xml_child.tag != 'M':
                nid = int(xml_node.get('i'))
                line = xml_node.get('l')
                self.parse_xml_node(xml_child, nid, line, hnode, callpath)
            else:
                mid = xml_child.get('n')
                val = float(xml_child.get('v'))
                hnode.add_metric(mid, val)


    def parse_xml_node(self, xml_node, parent_nid, parent_line, hparent,
            parent_callpath):
        """ Parses an XML node and its children recursively.
        """
        nid = int(xml_node.get('i'))

        global src_file
        xml_tag = xml_node.tag

        if xml_tag == 'PF' or xml_tag == 'Pr':
            # procedure
            name = self.procedure_names[xml_node.get('n')]
            src_file = xml_node.get('f')
            line = xml_node.get('l')

            node_callpath = parent_callpath
            node_callpath.append(name)
            hnode = Node(nid, tuple(node_callpath), hparent)

            node_dict = self.create_node_dict(nid, hnode, name, xml_tag,
                self.src_files[src_file], line,
                self.load_modules[xml_node.get('lm')])

        elif xml_tag == 'L':
            # loop
            src_file = xml_node.get('f')
            line = xml_node.get('l')
            name = 'Loop@' + (self.src_files[src_file]).rpartition('/')[2] + ':' + line

            node_callpath = parent_callpath
            node_callpath.append(name)
            hnode = Node(nid, tuple(node_callpath), hparent)
            node_dict = self.create_node_dict(nid, hnode, name, xml_tag,
                self.src_files[src_file], line, None)

        elif xml_tag == 'S':
            # statement
            line = xml_node.get('l')
            # this might not be required for resolving conflicts
            name = "line " + line
            node_callpath = parent_callpath
            node_callpath.append(name)
            hnode = Node(nid, tuple(node_callpath), hparent)
            fname = src_file
            node_dict = self.create_node_dict(nid, hnode, name, xml_tag,
                fname, line, None)


        if xml_tag == 'C' or (xml_tag == 'Pr' and
                              self.procedure_names[xml_node.get('n')] == ''):
            # do not add a node to the graph if the xml_tag is a callsite
            # or if its a procedure with no name
            # for Prs, the preceding Pr has the calling line number and for
            # PFs, the preceding C has the line number
            line = xml_node.get('l')
            self.parse_xml_children(xml_node, hparent, list(parent_callpath))
        else:
            self.node_dicts.append(node_dict)
            hparent.add_child(hnode)
            self.parse_xml_children(xml_node, hnode, list(node_callpath))

    def create_node_dict(self, nid, hnode, name, node_type, src_file,
            line, module):
        """ Create a dict with all the node attributes.
        """
        node_dict = {'nid': nid, 'name': name, 'type': node_type, 'file': src_file, 'line': line, 'module': module, 'node': hnode}
        hnode.set_node_dict(node_dict)

        return node_dict


def main():
    if len(sys.argv) > 1:
        print ("reading: ", sys.argv[1])
        reader = HPCToolkitReader(sys.argv[1])
        reader.create_graphframe()

    else:
        print ("Syntax: ", sys.argv[0], " <database_dir>")

if __name__ == "__main__":
    main()
