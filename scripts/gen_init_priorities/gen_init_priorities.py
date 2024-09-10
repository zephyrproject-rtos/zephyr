#!/usr/bin/env python3
#
# Copyright (c) 2024, Tomasz Bursztyka
#
# SPDX-License-Identifier: Apache-2.0

import os
import re
import sys
import pickle
import argparse
import fileinput
import pykwalify.core
from pathlib import Path

import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader     # type: ignore

# We will need to access edtlib
sys.path.insert(0, os.path.join(os.path.dirname(__file__),
                                '..', 'dts', 'python-devicetree', 'src'))

from devicetree.grutils import Graph
from devicetree import edtlib

LEVELS = """
EARLY:
    fake: true
    rank: 0

PRE_KERNEL_1:
    fake: true
    rank: 1
    dependencies:
        - EARLY

PRE_KERNEL_2:
    fake: true
    rank: 2
    dependencies:
        - PRE_KERNEL_1

POST_KERNEL:
    fake: true
    rank: 3
    dependencies:
        - PRE_KERNEL_2

APPLICATION:
    fake: true
    rank: 4
    dependencies:
        - POST_KERNEL

SMP:
    fake: true
    rank: 5
    dependencies:
        - APPLICATION
"""

# Q&D debug enabler
global debug
debug = False

# Zephyr Init Node
class ZInitNode:
    def __init__(self, name, node = None):
        self.name = name
        self.fake = False
        self.rank = -1 # used only on fake nodes (aka: actual level nodes)
        self.level = None
        self.explicit_level = False
        self.hard_dependencies = []

        # compat properties for Graph, as it is supposed to work with edt nodes
        self.parent = None
        self.unit_addr = None
        self.dep_ordinal = -1

        self.node = node
        if node is not None:
            # Let's fill in the compat properties relevantly then
            self.parent = node.parent
            self.unit_addr = node.unit_addr

    def __str__(self):
        if self.fake:
            return self.name

        s = f"ZInitNode({self.name} {self.dep_ordinal})"
        if self.node is not None:
            s += f" ({self.node.name})"

        return s

    def set_level(self, level):
        self.level = level
        self.explicit_level = True

    def add_hard_dependency(self, node):
        self.hard_dependencies.append(node)

    def set_implicit_level(self, depth, bottom):
        if (self.rank == 0 or
            (self.level is not None and self.explicit_level)):
            return

        impl_level = None
        for node in self.hard_dependencies:
            if node.level is None:
                if depth == bottom:
                    raise Exception(f"Check arrived at maximum depth")
                node.set_implicit_level(depth+1, bottom)

            if impl_level is None:
                if node.fake:
                    impl_level = node
                else:
                    impl_level = node.level
            elif (node.level is not None and
                  node.level.rank > impl_level.rank):
                impl_level = node.level

        if impl_level is not None and not self.fake and debug:
            print(f"-> Setting implicit level {impl_level} on {self}")

        self.level = impl_level

    def _get_define(self):
        if self.node is not None:
            basename = f"DT_{self.node.z_path_id}"
        else:
            basename = self.name.replace('@', '_').replace('-', '_')

        return f"#define ZINIT_{basename}_"

    def _get_level_macros(self):
        if self.level is None:
            return None

        implicit = 'IMPLICIT'
        if self.explicit_level:
            implicit = 'EXPLICIT'

        level_str = f"{self._get_define()}INIT_{implicit}_LEVEL_EXISTS 1\n"
        level_str += f"{self._get_define()}INIT_{implicit}_LEVEL {self.level}"

        return level_str

    def _get_priority_macro(self):
        return f"{self._get_define()}INIT_PRIORITY {self.dep_ordinal}"

    def write_macros(self, header):
        if self.fake:
            return

        if self.node is not None:
            name = self.node.name
            path = self.node.path
        else:
            name = self.name
            path = None

        header.write(f"/** Zephyr Init Node: {name} ")
        if path is not None:
            header.write(f"\n * DTS object path: {path}\n ")
        header.write("*/\n")

        header.write(f"{self._get_define()}EXISTS 1\n")

        level = self._get_level_macros()
        if level is not None:
            header.write(f"{level}\n")

        header.write(f"{self._get_priority_macro()}\n\n")

# ToDo: Move this out of gen_defines.py and make it into edt directly
# node's z_path_id are useful in both gen_defines.py,
# so let's generate it once.
def str2ident(s: str) -> str:
    # Converts 's' to a form suitable for (part of) an identifier

    return re.sub('[-,.@/+]', '_', s.lower())

def node_z_path_id(node: edtlib.Node) -> str:
    # Return the node specific bit of the node's path identifier:
    #
    # - the root node's path "/" has path identifier "N"
    # - "/foo" has "N_S_foo"
    # - "/foo/bar" has "N_S_foo_S_bar"
    # - "/foo/bar@123" has "N_S_foo_S_bar_123"
    #
    # This is used throughout this file to generate macros related to
    # the node.

    components = ["N"]
    if node.parent is not None:
        components.extend(f"S_{str2ident(component)}" for component in
                          node.path.split("/")[1:])

    return "_".join(components)

class EDTNodeMatcher:
    def __init__(self, edt_file):
        self.names = {}
        self.paths = {}
        self.yaml_nodes = {}
        self.macro_viability_rule = re.compile('[0-9a-zA-Z_]*')

        self.__load(edt_file)

    def __is_excluded(self, name):
        return name in [ '/', 'aliases', 'chosen' ]

    def __load(self, edt_file):
        with open(edt_file, 'rb') as f:
            self.edt = pickle.load(f)

        for node_list in self.edt.scc_order:
            for node in node_list:
                if self.__is_excluded(node.name):
                    continue

                self.add(node)
                self.yaml_nodes[node.path] = {}

                deps = []
                for d_node in self.edt._graph.depends_on(node):
                    if self.__is_excluded(d_node.name):
                        continue
                    deps.append(d_node.path)

                if len(deps) > 0:
                    self.yaml_nodes[node.path]['dependencies'] = deps

        # This should be done at EDT object instanciation and not in gen_defines.py
        # as we need this info here as well. See above.
        sorted_nodes = sorted(self.edt.nodes, key=lambda node: node.dep_ordinal)
        for node in sorted_nodes:
            node.z_path_id = node_z_path_id(node)

    def as_yaml_nodes(self):
        return self.yaml_nodes

    def get_real_name(self, name):
        chosen = self.edt.chosen_node(name)
        if chosen is not None:
            return chosen.path, True

        alias = self.edt.aliases_node(name)
        if alias is not None:
            return alias.path, True

        if name in self.edt.label2node:
            return self.edt.label2node[name].path, False

        if name in self.edt.compat2nodes:
            return None, True

        node = self.get(name)
        if node is None:
            return name, False

        return node.path, False

    def add(self, node):
        if node.path in self.paths:
            return

        if node.name not in self.names:
            self.names[node.name] = []

        self.names[node.name].append(node)
        self.paths[node.path] = node

    def get(self, desc):
        if desc in self.names:
            if len(self.names[desc]) > 1:
                raise Exception(f"Name {desc} leads to multiple nodes")
            return self.names[desc][0]

        if desc in self.paths:
            return self.paths[desc]

        # ToDo: desc could be a truncated path, if needed

        return None

    def check_viability(self, name=None, names=None):
        if name is not None:
            names = []
            names.append(name)

        results = []
        for n in names:
            rn, alias = self.get_real_name(n)
            if (n == rn and self.get(n) is None and
                self.macro_viability_rule.fullmatch(n) is None):
                if debug:
                    print(f"-> Skipping {n}: not EDT-based nor macro viable")
                continue

            if rn is None and alias is True:
                compats =  self.edt.compat2nodes[n]
                for node in compats:
                    results.append(node.path)
            else:
                results.append(rn)

        if len(results) == 0:
            return None

        return results


class InitGraph(Graph):
    def __init__(self):
        super(InitGraph, self).__init__()

        self.zinit_nodes = {}

    @property
    def nodes_count(self) -> int:
        return len(self.zinit_nodes)

    def populate(self, edt_nodes, yaml_nodes):
        for yn_name in list(yaml_nodes):
            z_n, alias = edt_nodes.get_real_name(yn_name)
            if z_n not in self.zinit_nodes:
                zinit_node = ZInitNode(z_n, node = edt_nodes.get(z_n))
                self.zinit_nodes[z_n] = zinit_node
            else:
                zinit_node = self.zinit_nodes[z_n]

            ynode = yaml_nodes[yn_name]
            if ynode is None and not alias:
                self.add_node(zinit_node)
                continue

            if 'level' in ynode:
                if ynode['level'] not in self.zinit_nodes:
                    raise Exception("No such level as "+ynode['level'])
                zinit_node.set_level(zinit_nodes[ynode['level']])

            if 'fake' in ynode:
                zinit_node.fake = ynode['fake']
                zinit_node.rank = ynode['rank'] # rank has to be provided

            if 'dependencies' in ynode:
                for d in ynode['dependencies']:
                    d_n, _ = edt_nodes.get_real_name(d)
                    if d_n not in self.zinit_nodes:
                        dnode = ZInitNode(d_n, node = edt_nodes.get(d_n))
                        self.zinit_nodes[d_n] = dnode
                    else:
                        dnode = self.zinit_nodes[d_n]

                    zinit_node.add_hard_dependency(dnode)
                    self.add_edge(zinit_node, dnode)
            else:
                self.add_node(zinit_node)

        # Updating edges depending on levels
        for zn in self.zinit_nodes:
            zinit_node = self.zinit_nodes[zn]
            if not zinit_node.explicit_level:
                zinit_node.set_implicit_level(1, self.nodes_count)
            if zinit_node.level is not None:
                self.add_edge(zinit_node, zinit_node.level)

    def verify_detected_loops(self):
        found = []
        for ns in self.zinit_nodes:
            node = self.zinit_nodes[ns]
            if not node.fake and node.dep_ordinal == -1:
                found.append(node)

        if len(found) > 0:
            s = ""
            for nf in found:
                s.join(f"{nf} ")

            raise Exception(f"Dependency loop detected around: {s}")

    def verify_level_sanity(self):
        def _level_check(node, level, depth, bottom):
            for hd_node in node.hard_dependencies:
                if (hd_node.level is not None and level.rank < hd_node.level.rank):
                    return hd_node

                if depth == bottom:
                    raise Exception("Check arrived at maximum depth")

                return _level_check(hd_node, level, depth+1, bottom)

            return None

        found = []
        for ns in self.zinit_nodes:
            node = self.zinit_nodes[ns]
            if node.level is None:
                continue

            hd_node = _level_check(node, node.level, 0, self.nodes_count)
            if hd_node is not None:
                found.append([node, hd_node])

        if len(found) > 0:
            s = ""
            for nf in found:
                s += f"{nf[0]} is on an earlier level than "
                s += f"{nf[1]} but requires the later\n"

            raise Exception(f"Level sanity broken: {s}")

    def generate_header_file(self, header):
        with open(header, "w") as hf:
            hf.write("/*\n * Generated by gen_init_priorities.py\n */\n\n")

            for l in self.scc_order():
                for n in l:
                    n.write_macros(hf)

    def output(self):
        if not debug:
            return
        print("\nFinal initialization graph:")
        for l in self.scc_order():
            for n in l:
                print(f"  {n}")
        print("")


def load_dotconfig(dotconfig_file):
    dotconfig = []
    with fileinput.input(files=(dotconfig_file), encoding="utf-8") as dcf:
        for line in dcf:
            if len(line) < 8 or line[0] == '#':
                continue

            opt, val = line.split('=', 1)
            val = val.strip('\r\n ')
            if val != 'y':
                continue

            dotconfig.append(opt)

    return dotconfig

def _yaml_init_obj_iterator(nodes):
    for sd in nodes:
        obj = nodes[sd]
        for sd_name in obj:
            if 'init' in obj[sd_name]:
                yield sd_name, obj[sd_name]['init']
            else:
                yield sd_name, {}

def update_from_yaml(yaml_nodes, updates, dotcfg, edt_nodes):
    if updates is None or ('service' not in updates and
                           'device' not in updates):
        return yaml_nodes

    for un, ui in _yaml_init_obj_iterator(updates):
        yn_l = edt_nodes.check_viability(name=un)
        if yn_l is None:
            continue

        for yn in yn_l:
            if yn not in yaml_nodes.keys():
                yaml_nodes[yn] = {}

            if 'level' in ui:
                yaml_nodes[yn]['level'] = ui['level']

            if 'dependencies' in ui:
                n_l = ui['dependencies']
                deps = edt_nodes.check_viability(names=n_l)
                if deps is not None:
                    if 'dependencies' in yaml_nodes[yn]:
                        yaml_nodes[yn]['dependencies'] += deps
                    else:
                        yaml_nodes[yn]['dependencies'] = deps

            if 'if' in ui and 'then' in ui:
                cond = ui['if']
                if cond not in dotcfg:
                    continue

                stmt = ui['then']
                if 'level' in stmt:
                    yaml_nodes[yn]['level'] = stmt['level']
                if 'dependencies' in stmt:
                    n_l = stmt['dependencies']
                    deps = edt_nodes.check_viability(names=n_l)
                    if deps is not None:
                        yaml_nodes[yn]['dependencies'] = deps

    return yaml_nodes

def print_yaml_nodes(yaml_nodes):
    if not debug:
        return
    print("\n# YAML nodes :")
    print("init:")
    for yn in list(yaml_nodes):
        print(f"    {yn}:")
        ynode = yaml_nodes[yn]
        if 'fake' in ynode:
            print(f"        fake: true")
            print(f"        rank: {ynode['rank']}")
        if 'level' in ynode:
            print(f"        level: {ynode['level']}")
        if 'dependencies' in ynode:
            print(f"        dependencies:")
            for elt in ynode['dependencies']:
                print(f"            - {elt}")
        print("")

def print_edt_nodes(edt_nodes):
    if not debug:
        return
    print("EDT original graph:")
    for l in edt_nodes.edt.scc_order:
        for n in l:
            print(f"  {n.name} -> {n.path}")
    print("")

def generate_init_priorities(args):
    global debug
    debug = args.debug

    # 1 - Loading the generated .config
    dotconfig = load_dotconfig(args.dotconfig_file)

    # 2 - Loading levels, to act as fake nodes
    yaml_nodes = yaml.safe_load(LEVELS)

    # 3a - Loading EDT DTS nodes (pair of name/path leading to node)
    edt_nodes = EDTNodeMatcher(args.edt_pickle)
    print_edt_nodes(edt_nodes)

    # 3b - Integrating EDT as yaml nodes
    yaml_nodes.update(edt_nodes.as_yaml_nodes())

    # 4 - Now loading any extra yaml nodes, and integrating them relevantly
    for i_p in args.init_files:
        print(f'Loading {i_p}')
        with open(i_p, 'r', encoding="utf-8") as i_f:
            yaml_nodes = update_from_yaml(yaml_nodes,
                                          yaml.load(i_f, SafeLoader),
                                          dotconfig,
                                          edt_nodes)
    print_yaml_nodes(yaml_nodes)

    # 5 - Generating dependency graph from the yaml_nodes
    graph = InitGraph()
    graph.populate(edt_nodes, yaml_nodes)
    graph.scc_order()
    graph.output()

    # 6 - Sanity check
    graph.verify_detected_loops()
    graph.verify_level_sanity()

    # 7 - Finally, as all went well, let's generate the header
    graph.generate_header_file(args.header_out)

def load_schemas():
    schemas = {}
    p = Path(os.path.abspath(os.path.dirname(__file__)))
    for sch_p in sorted(p.glob('*.yaml')):
        sch_n = Path(sch_p).stem
        if sch_n in schemas:
            raise Exception(f"Duplicate schema file name {sch_n}")

        with open(sch_p, 'r', encoding='utf-8') as sch_f:
            schemas[sch_n] = yaml.load(sch_f, SafeLoader)

    return schemas

def validate_input(init_files):
    schemas = load_schemas()
    if len(schemas) == 0:
        return

    for i_p in init_files:
         with open(i_p, 'r', encoding="utf-8") as i_f:
             try:
                 content = yaml.load(i_f, SafeLoader)
                 for sch in schemas:
                     pykwalify.core.Core(source_data=content,
                                         schema_data=schemas[sch]).validate()
             except (yaml.YAMLError, pykwalify.errors.SchemaError) as e:
                 sys.exit(f"ERROR: malformed service file {i_p}")

def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument('--debug', action='store_true',
                        help='Print debug messages')
    parser.add_argument('--validate', action='store_true',
                        help='Validate the input files prior to using them')
    parser.add_argument('--init-files', nargs='+', default=[],
                        help='Init description files to load (services and/or devices)')
    parser.add_argument("--dotconfig-file", required=True,
                        help="Path to the general .config file")
    parser.add_argument("--edt-pickle", required=True,
                        help="Path to the pickled edtlib.EDT object file")
    parser.add_argument("--header-out", required=True,
                        help="Path to write generated header to")

    args = parser.parse_args()

    if args.validate:
        validate_input(args.init_files)

    generate_init_priorities(args)

if __name__ == "__main__":
    main()
