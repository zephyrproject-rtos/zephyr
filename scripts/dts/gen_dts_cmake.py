#!/usr/bin/env python3

# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

'''
This script uses edtlib and the devicetree data in the build directory
to generate a CMake file which contains devicetree data.

That data can then be used in the rest of the build system.

The generated CMake file looks like this:

  add_custom_target(${DEVICETREE_TARGET})
  set_target_properties(${DEVICETREE_TARGET} PROPERTIES
                        "DT_PROP|/soc|compatible" "vnd,soc;")
  ...

It takes an input variable - DEVICETREE_TARGET - and creates a special
CMake target with this name, which will contain various values in the
devicetree as CMake target properties.

Be careful:

  "Property" here can refer to a CMake target property or a
  DTS property. DTS property values are stored inside
  CMake target properties, along with other devicetree data.

The build system includes this generated file early on, so
devicetree values can be used at CMake processing time.

Access is not done directly, but with Zephyr CMake extension APIs,
like this:

  # sets 'compat' to "vnd,soc" in CMake
  dt_prop(compat PATH "/soc" PROPERTY compatible INDEX 0)

This is analogous to how DTS values are encoded as C macros,
which can be read in source code using C APIs like
DT_PROP(node_id, foo) from devicetree.h.
'''

import argparse
import os
import pickle
import sys
from collections import defaultdict

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))


def parse_args():
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--cmake-out", required=True,
                        help="path to write the CMake property file")
    parser.add_argument("--edt-pickle", required=True,
                        help="path to read the pickled edtlib.EDT object from")

    return parser.parse_args()


def main():
    args = parse_args()

    with open(args.edt_pickle, 'rb') as f:
        edt = pickle.load(f)

    # In what looks like an undocumented implementation detail, CMake
    # target properties are stored in a C++ standard library map whose
    # keys and values are each arbitrary strings, so we can use
    # whatever we want as target property names.
    #
    # We therefore use '|' as a field separator character below within
    # because it's not a valid character in DTS node paths or property
    # names. This lets us store the "real" paths and property names
    # without conversion to lowercase-and-underscores like we have to
    # do in C.
    #
    # If CMake adds restrictions on target property names later, we
    # can just tweak the generated file to use a more restrictive
    # property encoding, perhaps reusing the same style documented in
    # macros.bnf for C macros.

    cmake_props = []
    chosen_nodes = edt.chosen_nodes
    for node in chosen_nodes:
        path = chosen_nodes[node].path
        cmake_props.append(f'"DT_CHOSEN|{node}" "{path}"')

    # The separate loop over edt.nodes here is meant to keep
    # all of the alias-related properties in one place.
    for node in edt.nodes:
        path = node.path
        for alias in node.aliases:
            cmake_props.append(f'"DT_ALIAS|{alias}" "{path}"')

    compatible2paths = defaultdict(list)
    for node in edt.nodes:
        cmake_props.append(f'"DT_NODE|{node.path}" TRUE')

        for label in node.labels:
            cmake_props.append(f'"DT_NODELABEL|{label}" "{node.path}"')

        for item in node.props:
            # We currently do not support phandles for edt -> cmake conversion.
            if "phandle" not in node.props[item].type:
                if "array" in node.props[item].type:
                    # Convert array to CMake list
                    cmake_value = ''
                    for val in node.props[item].val:
                        cmake_value = f'{cmake_value}{val};'
                else:
                    cmake_value = node.props[item].val

                # Encode node's property 'item' as a CMake target property
                # with a name like 'DT_PROP|<path>|<property>'.
                cmake_prop = f'DT_PROP|{node.path}|{item}'
                cmake_props.append(f'"{cmake_prop}" "{cmake_value}"')

                if item == 'compatible':
                    # compatibles is always an array
                    for comp in node.props[item].val:
                        compatible2paths[comp].append(node.path)

        if node.regs is not None:
            cmake_props.append(f'"DT_REG|{node.path}|NUM" "{len(node.regs)}"')
            cmake_addr = ''
            cmake_size = ''

            for reg in node.regs:
                if reg.addr is None:
                    cmake_addr = f'{cmake_addr}NONE;'
                else:
                    cmake_addr = f'{cmake_addr}{hex(reg.addr)};'

                if reg.size is None:
                    cmake_size = f'{cmake_size}NONE;'
                else:
                    cmake_size = f'{cmake_size}{hex(reg.size)};'

            cmake_props.append(f'"DT_REG|{node.path}|ADDR" "{cmake_addr}"')
            cmake_props.append(f'"DT_REG|{node.path}|SIZE" "{cmake_size}"')

    for comp in compatible2paths.keys():
        cmake_path = ''
        for path in compatible2paths[comp]:
            cmake_path = f'{cmake_path}{path};'

        # Remove the last ';'
        cmake_path = cmake_path[:-1]

        cmake_comp = f'DT_COMP|{comp}'
        cmake_props.append(f'"{cmake_comp}" "{cmake_path}"')

    cmake_props = map(
        'set_target_properties(${{DEVICETREE_TARGET}} PROPERTIES {})'.format,
        cmake_props
    )
    with open(args.cmake_out, "w", encoding="utf-8") as cmake_file:
        print('add_custom_target(${DEVICETREE_TARGET})\n', *cmake_props,
              sep='\n', file=cmake_file)


if __name__ == "__main__":
    main()
