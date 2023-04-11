# Copyright 2009-2013, 2019 Peter A. Bigot
#
# SPDX-License-Identifier: Apache-2.0

# This implementation is derived from the one in
# [PyXB](https://github.com/pabigot/pyxb), stripped down and modified
# specifically to manage edtlib Node instances.

import collections

class Graph:
    """
    Represent a directed graph with edtlib Node objects as nodes.

    This is used to determine order dependencies among nodes in a
    devicetree.  An edge from C{source} to C{target} indicates that
    some aspect of C{source} requires that some aspect of C{target}
    already be available.
    """

    def __init__(self, check_cyclic_dependencies, root=None):
        self.__roots = None
        self.__check_cyclic_dependencies = check_cyclic_dependencies
        if root is not None:
            self.__roots = {root}
        self.__edge_map = collections.defaultdict(set)
        self.__reverse_map = collections.defaultdict(set)
        self.__nodes = set()

    def add_edge(self, source, target):
        """
        Add a directed edge from the C{source} to the C{target}.

        The nodes are added to the graph if necessary.
        """
        self.__edge_map[source].add(target)
        if source != target:
            self.__reverse_map[target].add(source)
        self.__nodes.add(source)
        self.__nodes.add(target)

    def roots(self):
        """
        Return the set of nodes calculated to be roots (i.e., those
        that have no incoming edges).

        This caches the roots calculated in a previous invocation.

        @rtype: C{set}
        """
        if not self.__roots:
            self.__roots = set()
            for n in self.__nodes:
                if n not in self.__reverse_map:
                    self.__roots.add(n)
        return self.__roots

    def _tarjan(self):
        # Execute Tarjan's algorithm on the graph.
        #
        # Tarjan's algorithm
        # (http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm)
        # computes the strongly-connected components
        # (http://en.wikipedia.org/wiki/Strongly_connected_component)
        # of the graph: i.e., the sets of nodes that form a minimal
        # closed set under edge transition.  In essence, the loops.
        # We use this to detect groups of components that have a
        # dependency cycle, and to impose a total order on components
        # based on dependencies.

        self.__stack = []
        self.__scc_order = []
        self.__index = 0
        self.__tarjan_index = {}
        self.__tarjan_low_link = {}
        for v in self.__nodes:
            self.__tarjan_index[v] = None
        roots = sorted(self.roots(), key=node_key)
        if self.__nodes and not roots:
            raise Exception('TARJAN: No roots found in graph with {} nodes'.format(len(self.__nodes)))

        for r in roots:
            self._tarjan_root(r)

        # check for cyclic dependencies
        if self.__check_cyclic_dependencies:
            for scc in self.__scc_order:
                if len(scc) > 1:
                    raise Exception(f'found cyclic dependency in graph including the node {scc[0].name}')

        # Assign ordinals for edtlib
        ordinal = 0
        for scc in self.__scc_order:
            for node in scc:
                node.dep_ordinal = ordinal
                ordinal += 1

    def _tarjan_root(self, v):
        # Do the work of Tarjan's algorithm for a given root node.

        if self.__tarjan_index.get(v) is not None:
            # "Root" was already reached.
            return
        self.__tarjan_index[v] = self.__tarjan_low_link[v] = self.__index
        self.__index += 1
        self.__stack.append(v)
        source = v
        for target in sorted(self.__edge_map[source], key=node_key):
            if self.__tarjan_index[target] is None:
                self._tarjan_root(target)
                self.__tarjan_low_link[v] = min(self.__tarjan_low_link[v], self.__tarjan_low_link[target])
            elif target in self.__stack:
                self.__tarjan_low_link[v] = min(self.__tarjan_low_link[v], self.__tarjan_low_link[target])

        if self.__tarjan_low_link[v] == self.__tarjan_index[v]:
            scc = []
            while True:
                scc.append(self.__stack.pop())
                if v == scc[-1]:
                    break
            self.__scc_order.append(scc)

    def scc_order(self):
        """Return the strongly-connected components in order.

        The data structure is a list, in dependency order, of strongly
        connected components (which can be single nodes).  Appearance
        of a node in a set earlier in the list indicates that it has
        no dependencies on any node that appears in a subsequent set.
        This order is preferred over a depth-first-search order for
        code generation, since it detects loops.
        """
        if not self.__scc_order:
            self._tarjan()
        return self.__scc_order
    __scc_order = None

    def depends_on(self, node):
        """Get the nodes that 'node' directly depends on."""
        return sorted(self.__edge_map[node], key=node_key)

    def required_by(self, node):
        """Get the nodes that directly depend on 'node'."""
        return sorted(self.__reverse_map[node], key=node_key)

def node_key(node):
    # This sort key ensures that sibling nodes with the same name will
    # use unit addresses as tiebreakers. That in turn ensures ordinals
    # for otherwise indistinguishable siblings are in increasing order
    # by unit address, which is convenient for displaying output.

    if node.parent:
        parent_path = node.parent.path
    else:
        parent_path = '/'

    if node.unit_addr is not None:
        name = node.name.rsplit('@', 1)[0]
        unit_addr = node.unit_addr
    else:
        name = node.name
        unit_addr = -1

    return (parent_path, name, unit_addr)
