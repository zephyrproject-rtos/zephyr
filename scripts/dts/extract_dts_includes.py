#!/usr/bin/env python3
#
# Copyright (c) 2017, Linaro Limited
# Copyright (c) 2018, Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# NOTE: This file is part of the old device tree scripts, which will be removed
# later. They are kept to generate some legacy #defines via the
# --deprecated-only flag.
#
# The new scripts are gen_defines.py, edtlib.py, and dtlib.py.

# vim: ai:ts=4:sw=4

import os, fnmatch
import re
import argparse
from collections import defaultdict

import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # It's much faster.
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader

from devicetree import parse_file
from extract.globals import *
import extract.globals

from extract.clocks import clocks
from extract.compatible import compatible
from extract.interrupts import interrupts
from extract.reg import reg
from extract.flash import flash
from extract.default import default


def extract_bus_name(node_path, def_label):
    label = def_label + '_BUS_NAME'
    prop_alias = {}

    add_compat_alias(node_path, 'BUS_NAME', label, prop_alias)

    # Generate defines for node aliases
    if node_path in aliases:
        add_prop_aliases(
            node_path,
            lambda alias: str_to_label(alias) + '_BUS_NAME',
            label,
            prop_alias)

    insert_defs(node_path,
                {label: '"' + find_parent_prop(node_path, 'label') + '"'},
                prop_alias)


def extract_string_prop(node_path, key, label):
    if node_path not in defs:
        # Make all defs have the special 'aliases' key, to remove existence
        # checks elsewhere
        defs[node_path] = {'aliases': {}}

    defs[node_path][label] = '"' + reduced[node_path]['props'][key] + '"'


def generate_prop_defines(node_path, prop):
    # Generates #defines (and .conf file values) from the prop
    # named 'prop' on the device tree node at 'node_path'

    binding = get_binding(node_path)
    if 'parent-bus' in binding or \
       'parent' in binding and 'bus' in binding['parent']:
        # If the binding specifies a parent for the node, then include the
        # parent in the #define's generated for the properties
        parent_path = get_parent_path(node_path)
        def_label = 'DT_' + node_label(parent_path) + '_' \
                          + node_label(node_path)
    else:
        def_label = 'DT_' + node_label(node_path)

    names = prop_names(reduced[node_path], prop)

    if prop == 'reg':
        reg.extract(node_path, names, def_label, 1)
    elif prop in {'interrupts', 'interrupts-extended'}:
        interrupts.extract(node_path, prop, names, def_label)
    elif prop == 'compatible':
        compatible.extract(node_path, prop, def_label)
    elif 'clocks' in prop:
        clocks.extract(node_path, prop, def_label)
    elif 'pwms' in prop or '-gpios' in prop or prop == "gpios":
        prop_values = reduced[node_path]['props'][prop]
        generic = prop[:-1]  # Drop the 's' from the prop

        # Deprecated the non-'S' form
        extract_controller(node_path, prop, prop_values, 0,
                           def_label, generic, deprecate=True)
        extract_controller(node_path, prop, prop_values, 0,
                           def_label, prop)
        # Deprecated the non-'S' form
        extract_cells(node_path, prop, prop_values,
                      names, 0, def_label, generic, deprecate=True)
        extract_cells(node_path, prop, prop_values,
                      names, 0, def_label, prop)
    else:
        default.extract(node_path, prop,
                        binding['properties'][prop]['type'],
                        def_label)


def generate_node_defines(node_path):
    # Generates #defines (and .conf file values) from the device
    # tree node at 'node_path'

    if get_compat(node_path) not in get_binding_compats():
        return

    # We extract a few different #defines for a flash partition, so it's easier
    # to handle it in one step
    if 'partition@' in node_path:
        flash.extract_partition(node_path)
        return

    if get_binding(node_path) is None:
        return

    generate_bus_defines(node_path)

    props = get_binding(node_path).get('properties')
    if not props:
        return

    # Generate per-property ('foo = <1 2 3>', etc.) #defines
    for yaml_prop, yaml_val in props.items():
        if yaml_prop.startswith("#") or yaml_prop.endswith("-map"):
            continue

        match = False

        # Handle each property individually, this ends up handling common
        # patterns for things like reg, interrupts, etc that we don't need
        # any special case handling at a node level
        for prop in reduced[node_path]['props']:
            if re.fullmatch(yaml_prop, prop):
                match = True
                generate_prop_defines(node_path, prop)

        # Handle the case that we have a boolean property, but its not
        # in the dts
        if not match and yaml_val['type'] == 'boolean':
            generate_prop_defines(node_path, yaml_prop)


def generate_bus_defines(node_path):
    # Generates any node-level #defines related to
    #
    # parent:
    #   bus: ...

    binding = get_binding(node_path)
    if not ('parent-bus' in binding or
            'parent' in binding and 'bus' in binding['parent']):
        return

    parent_path = get_parent_path(node_path)

    # Check that parent has matching child bus value
    try:
        parent_binding = get_binding(parent_path)
        if 'child-bus' in parent_binding:
            parent_bus = parent_binding['child-bus']
        else:
            parent_bus = parent_binding['child']['bus']
    except (KeyError, TypeError):
        raise Exception("{0} defines parent {1} as bus master, but {1} is not "
                        "configured as bus master in binding"
                        .format(node_path, parent_path))

    if 'parent-bus' in binding:
        bus = binding['parent-bus']
    else:
        bus = binding['parent']['bus']

    if parent_bus != bus:
        raise Exception("{0} defines parent {1} as {2} bus master, but {1} is "
                        "configured as {3} bus master"
                        .format(node_path, parent_path, bus, parent_bus))

    # Generate *_BUS_NAME #define
    extract_bus_name(
        node_path,
        'DT_' + node_label(parent_path) + '_' + node_label(node_path))


def prop_names(node, prop_name):
    # Returns a list with the *-names for the property (reg-names,
    # interrupt-names, etc.) The list is copied so that it can be modified
    # in-place later without stomping on the device tree data.

    # The first case turns 'interrupts' into 'interrupt-names'
    names = node['props'].get(prop_name[:-1] + '-names', []) or \
            node['props'].get(prop_name + '-names', [])

    if isinstance(names, list):
        # Allow the list of names to be modified in-place without
        # stomping on the property
        return names.copy()

    return [names]


def merge_properties(parent, fname, to_dict, from_dict):
    # Recursively merges the 'from_dict' dictionary into 'to_dict', to
    # implement !include. 'parent' is the current parent key being looked at.
    # 'fname' is the top-level .yaml file.

    for k in from_dict:
        if (k in to_dict and isinstance(to_dict[k], dict)
                         and isinstance(from_dict[k], dict)):
            merge_properties(k, fname, to_dict[k], from_dict[k])
        else:
            to_dict[k] = from_dict[k]


def merge_included_bindings(fname, node):
    # Recursively merges properties from files !include'd from the 'inherits'
    # section of the binding. 'fname' is the path to the top-level binding
    # file, and 'node' the current top-level YAML node being processed.

    res = node

    if "include" in node:
        included = node.pop("include")
        if isinstance(included, str):
            included = [included]

        for included_fname in included:
            binding = load_binding_file(included_fname)
            inherited = merge_included_bindings(fname, binding)
            merge_properties(None, fname, inherited, res)
            res = inherited

    if 'inherits' in node:
        for inherited in node.pop('inherits'):
            inherited = merge_included_bindings(fname, inherited)
            merge_properties(None, fname, inherited, res)
            res = inherited

    return res


def define_str(name, value, value_tabs, is_deprecated=False):
    line = "#define " + name
    if is_deprecated:
        line += " __DEPRECATED_MACRO "
    return line + (value_tabs - len(line)//8)*'\t' + str(value) + '\n'


def write_conf(f):
    for node in sorted(defs):
        f.write('# ' + node.split('/')[-1] + '\n')

        for prop in sorted(defs[node]):
            if prop != 'aliases' and prop.startswith("DT_"):
                f.write('%s=%s\n' % (prop, defs[node][prop]))

        for alias in sorted(defs[node]['aliases']):
            alias_target = defs[node]['aliases'][alias]
            if alias_target not in defs[node]:
                alias_target = defs[node]['aliases'][alias_target]
            if alias.startswith("DT_"):
                f.write('%s=%s\n' % (alias, defs[node].get(alias_target)))

        f.write('\n')


def write_header(f, deprecated_only):
    f.write('''\
/**********************************************
*                 Generated include file
*                      DO NOT MODIFY
*/
#ifndef GENERATED_DTS_BOARD_UNFIXED_H
#define GENERATED_DTS_BOARD_UNFIXED_H
''')

    def max_dict_key(dct):
        return max(len(key) for key in dct)

    for node in sorted(defs):
        f.write('/* ' + node.split('/')[-1] + ' */\n')

        maxlen = max_dict_key(defs[node])
        if defs[node]['aliases']:
            maxlen = max(maxlen, max_dict_key(defs[node]['aliases']))
        maxlen += len('#define ')

        value_tabs = (maxlen + 8)//8  # Tabstop index for value
        if 8*value_tabs - maxlen <= 2:
            # Add some minimum room between the macro name and the value
            value_tabs += 1

        for prop in sorted(defs[node]):
            if prop != 'aliases':
                deprecated_warn = False
                if prop in deprecated_main:
                    deprecated_warn = True
                if not prop.startswith('DT_'):
                    deprecated_warn = True
                if deprecated_only and not deprecated_warn:
                    continue
                f.write(define_str(prop, defs[node][prop], value_tabs, deprecated_warn))

        for alias in sorted(defs[node]['aliases']):
            alias_target = defs[node]['aliases'][alias]
            deprecated_warn = False
            # Mark any non-DT_ prefixed define as deprecated except
            # for now we special case LED, SW, and *PWM_LED*
            if not alias.startswith('DT_'):
                deprecated_warn = True
            if alias in deprecated:
                deprecated_warn = True
            if deprecated_only and not deprecated_warn:
                continue
            f.write(define_str(alias, alias_target, value_tabs, deprecated_warn))

        f.write('\n')

    f.write('#endif\n')


def load_bindings(root, binding_dirs):
    find_binding_files(binding_dirs)
    dts_compats = all_compats(root)

    compat_to_binding = {}
    # Maps buses to dictionaries that map compats to YAML nodes
    bus_to_binding = defaultdict(dict)
    compats = []

    # Add '!include foo.yaml' handling
    Loader.add_constructor('!include', yaml_include)

    # Code below is adapated from edtlib.py

    # Searches for any 'compatible' string mentioned in the devicetree
    # files, with a regex
    dt_compats_search = re.compile(
        "|".join(re.escape(compat) for compat in dts_compats)
    ).search

    for file in binding_files:
        with open(file, encoding="utf-8") as f:
            contents = f.read()

        if not dt_compats_search(contents):
            continue

        binding = yaml.load(contents, Loader=Loader)

        binding_compats = _binding_compats(binding)
        if not binding_compats:
            continue

        with open(file, 'r', encoding='utf-8') as yf:
            binding = merge_included_bindings(file,
                                              yaml.load(yf, Loader=Loader))

        for compat in binding_compats:
            if compat not in compats:
                compats.append(compat)

            if 'parent-bus' in binding:
                bus_to_binding[binding['parent-bus']][compat] = binding

            if 'parent' in binding:
                bus_to_binding[binding['parent']['bus']][compat] = binding

            compat_to_binding[compat] = binding

    if not compat_to_binding:
        raise Exception("No bindings found in '{}'".format(binding_dirs))

    extract.globals.bindings = compat_to_binding
    extract.globals.bus_bindings = bus_to_binding
    extract.globals.binding_compats = compats


def _binding_compats(binding):
    # Adapated from edtlib.py

    def new_style_compats():
        if binding is None or "compatible" not in binding:
            return []

        val = binding["compatible"]

        if isinstance(val, str):
            return [val]
        return val

    def old_style_compat():
        try:
            return binding["properties"]["compatible"]["constraint"]
        except Exception:
            return None

    new_compats = new_style_compats()
    old_compat = old_style_compat()
    if old_compat:
        return [old_compat]
    return new_compats


def find_binding_files(binding_dirs):
    # Initializes the global 'binding_files' variable with a list of paths to
    # binding (.yaml) files

    global binding_files

    binding_files = []

    for binding_dir in binding_dirs:
        for root, _, filenames in os.walk(binding_dir):
            for filename in fnmatch.filter(filenames, '*.yaml'):
                binding_files.append(os.path.join(root, filename))


def yaml_include(loader, node):
    # Implements !include. Returns a list with the top-level YAML structures
    # for the included files (a single-element list if there's just one file).

    if isinstance(node, yaml.ScalarNode):
        # !include foo.yaml
        return [load_binding_file(loader.construct_scalar(node))]

    if isinstance(node, yaml.SequenceNode):
        # !include [foo.yaml, bar.yaml]
        return [load_binding_file(fname)
                for fname in loader.construct_sequence(node)]

    yaml_inc_error("Error: unrecognised node type in !include statement")


def load_binding_file(fname):
    # yaml_include() helper for loading an !include'd file. !include takes just
    # the basename of the file, so we need to make sure that there aren't
    # multiple candidates.

    filepaths = [filepath for filepath in binding_files
                 if os.path.basename(filepath) == os.path.basename(fname)]

    if not filepaths:
        yaml_inc_error("Error: unknown file name '{}' in !include statement"
                       .format(fname))

    if len(filepaths) > 1:
        yaml_inc_error("Error: multiple candidates for file name '{}' in "
                       "!include statement: {}".format(fname, filepaths))

    with open(filepaths[0], 'r', encoding='utf-8') as f:
        return yaml.load(f, Loader=Loader)


def yaml_inc_error(msg):
    # Helper for reporting errors in the !include implementation

    raise yaml.constructor.ConstructorError(None, None, msg)


def generate_defines():
    # Generates #defines (and .conf file values) from DTS

    # sorted() otherwise Python < 3.6 randomizes the order of the flash
    # partition table
    for node_path in sorted(reduced.keys()):
        generate_node_defines(node_path)

    if not defs:
        raise Exception("No information parsed from dts file.")

    for k, v in regs_config.items():
        if k in chosen:
            reg.extract(chosen[k], None, v, 1024)

    for k, v in name_config.items():
        if k in chosen:
            extract_string_prop(chosen[k], "label", v)

    flash.extract_flash()
    flash.extract_code_partition()


def parse_arguments():
    rdh = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=rdh)

    parser.add_argument("-d", "--dts", required=True, help="DTS file")
    parser.add_argument("-y", "--yaml", nargs='+', required=True,
                        help="YAML file directories, we allow multiple")
    parser.add_argument("-i", "--include",
                        help="Generate include file for the build system")
    parser.add_argument("-k", "--keyvalue",
                        help="Generate config file for the build system")
    parser.add_argument("--old-alias-names", action='store_true',
                        help="Generate aliases also in the old way, without "
                             "compatibility information in their labels")
    parser.add_argument("--deprecated-only", action='store_true',
                        help="Generate only the deprecated defines")
    return parser.parse_args()


def main():
    args = parse_arguments()
    enable_old_alias_names(args.old_alias_names)

    # Parse DTS and fetch the root node
    with open(args.dts, 'r', encoding='utf-8') as f:
        root = parse_file(f)['/']

    # Create some global data structures from the parsed DTS
    create_reduced(root, '/')
    create_phandles(root, '/')
    create_aliases(root)
    create_chosen(root)

    # Re-sort instance_id by reg addr
    #
    # Note: this is a short term fix and should be removed when
    # generate defines for instance with a prefix like 'DT_INST'
    #
    # Build a dict of dicts, first level is index by compat
    # second level is index by reg addr
    compat_reg_dict = defaultdict(dict)
    for node in reduced.values():
        instance = node.get('instance_id')
        if instance and node['addr'] is not None:
            for compat in instance:
                reg = node['addr']
                compat_reg_dict[compat][reg] = node

    # Walk the reg addr in sorted order to re-index 'instance_id'
    for compat in compat_reg_dict:
        # only update if we have more than one instance
        if len(compat_reg_dict[compat]) > 1:
            for idx, reg_addr in enumerate(sorted(compat_reg_dict[compat])):
                compat_reg_dict[compat][reg_addr]['instance_id'][compat] = idx

    # Load any bindings (.yaml files) that match 'compatible' values from the
    # DTS
    load_bindings(root, args.yaml)

    # Generate keys and values for the configuration file and the header file
    generate_defines()

    # Write the configuration file and the header file

    if args.keyvalue is not None:
        with open(args.keyvalue, 'w', encoding='utf-8') as f:
            write_conf(f)

    if args.include is not None:
        with open(args.include, 'w', encoding='utf-8') as f:
            write_header(f, args.deprecated_only)


if __name__ == '__main__':
    main()
