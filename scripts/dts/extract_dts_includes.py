#!/usr/bin/env python3
#
# Copyright (c) 2017, Linaro Limited
# Copyright (c) 2018, Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# vim: ai:ts=4:sw=4

import sys
import os, fnmatch
import re
import yaml
import argparse
from collections import defaultdict
from collections.abc import Mapping

from devicetree import parse_file
from extract.globals import *
import extract.globals

from extract.clocks import clocks
from extract.compatible import compatible
from extract.interrupts import interrupts
from extract.reg import reg
from extract.flash import flash
from extract.pinctrl import pinctrl
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
    if 'parent' in binding and 'bus' in binding['parent']:
        # If the binding specifies a parent for the node, then include the
        # parent in the #define's generated for the properties
        parent_path = get_parent_path(node_path)
        def_label = 'DT_' + get_node_label(parent_path) + '_' \
                          + get_node_label(node_path)
    else:
        def_label = 'DT_' + get_node_label(node_path)

    names = prop_names(reduced[node_path], prop)

    if prop == 'reg':
        reg.extract(node_path, names, def_label, 1)
    elif prop == 'interrupts' or prop == 'interrupts-extended':
        interrupts.extract(node_path, prop, names, def_label)
    elif prop == 'compatible':
        compatible.extract(node_path, prop, def_label)
    elif 'pinctrl-' in prop:
        pinctrl.extract(node_path, prop, def_label)
    elif 'clocks' in prop:
        clocks.extract(node_path, prop, def_label)
    elif 'pwms' in prop or 'gpios' in prop:
        prop_values = reduced[node_path]['props'][prop]
        generic = prop[:-1]  # Drop the 's' from the prop

        extract_controller(node_path, prop, prop_values, 0,
                           def_label, generic)
        extract_cells(node_path, prop, prop_values,
                      names, 0, def_label, generic)
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

    generate_bus_defines(node_path)

    # Generate per-property ('foo = <1 2 3>', etc.) #defines
    for yaml_prop, yaml_val in get_binding(node_path)['properties'].items():
        if 'generation' not in yaml_val:
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
    if not ('parent' in binding and 'bus' in binding['parent']):
        return

    parent_path = get_parent_path(node_path)

    # Check that parent has matching child bus value
    try:
        parent_binding = get_binding(parent_path)
        parent_bus = parent_binding['child']['bus']
    except (KeyError, TypeError) as e:
        raise Exception("{0} defines parent {1} as bus master, but {1} is not "
                        "configured as bus master in binding"
                        .format(node_path, parent_path))

    if parent_bus != binding['parent']['bus']:
        raise Exception("{0} defines parent {1} as {2} bus master, but {1} is "
                        "configured as {3} bus master"
                        .format(node_path, parent_path,
                                binding['parent']['bus'], parent_bus))

    # Generate alias definition if parent has any alias
    if parent_path in aliases:
        for i in aliases[parent_path]:
            # Build an alias name that respects device tree specs
            node_name = get_compat(node_path) + '-' + node_path.split('@')[-1]
            node_strip = node_name.replace('@','-').replace(',','-')
            node_alias = i + '-' + node_strip
            if node_alias not in aliases[node_path]:
                # Need to generate alias name for this node:
                aliases[node_path].append(node_alias)

    # Generate *_BUS_NAME #define
    extract_bus_name(
        node_path,
        'DT_' + get_node_label(parent_path) + '_' + get_node_label(node_path))


def prop_names(node, prop_name):
    # Returns a list with the *-names for the property (reg-names,
    # interrupt-names, etc.) The list is copied so that it can be modified
    # in-place later without stomping on the device tree data.

    if prop_name.startswith('pinctrl-'):
        names = node['props'].get('pinctrl-names', [])
    else:
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

    for k, v in from_dict.items():
        if (k in to_dict and isinstance(to_dict[k], dict)
                         and isinstance(from_dict[k], dict)):
            merge_properties(k, fname, to_dict[k], from_dict[k])
        else:
            to_dict[k] = from_dict[k]

            # Warn when overriding a property and changing its value...
            if (k in to_dict and to_dict[k] != from_dict[k] and
                # ...unless it's the 'title', 'description', or 'version'
                # property. These are overriden deliberately.
                not k in {'title', 'version', 'description'} and
                # Also allow the category to be changed from 'optional' to
                # 'required' without a warning
                not (k == "category" and to_dict[k] == "optional" and
                     from_dict[k] == "required")):

                print("extract_dts_includes.py: {}('{}') merge of property "
                      "'{}': '{}' overwrites '{}'"
                      .format(fname, parent, k, from_dict[k], to_dict[k]))


def merge_included_bindings(fname, node):
    # Recursively merges properties from files !include'd from the 'inherits'
    # section of the binding. 'fname' is the path to the top-level binding
    # file, and 'node' the current top-level YAML node being processed.

    check_binding_properties(node)

    if 'inherits' in node:
        for inherited in node.pop('inherits'):
            inherited = merge_included_bindings(fname, inherited)
            merge_properties(None, fname, inherited, node)
            node = inherited

    return node


def check_binding_properties(node):
    # Checks that the top-level YAML node 'node' has the expected properties.
    # Prints warnings and substitutes defaults otherwise.

    if 'title' not in node:
        print("extract_dts_includes.py: node without 'title' -", node)

    for prop in 'title', 'version', 'description':
        if prop not in node:
            node[prop] = "<unknown {}>".format(prop)
            print("extract_dts_includes.py: '{}' property missing "
                  "in '{}' binding. Using '{}'."
                  .format(prop, node['title'], node[prop]))

    if 'id' in node:
        print("extract_dts_includes.py: WARNING: id field set "
              "in '{}', should be removed.".format(node['title']))


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


def write_header(f):
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
                f.write(define_str(prop, defs[node][prop], value_tabs))

        for alias in sorted(defs[node]['aliases']):
            alias_target = defs[node]['aliases'][alias]
            deprecated_warn = False
            # Mark any non-DT_ prefixed define as deprecated except
            # for now we special case LED, SW, and *PWM_LED*
            if not alias.startswith(('DT_', 'LED', 'SW')) and not 'PWM_LED' in alias:
                deprecated_warn = True
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
    yaml.add_constructor('!include', yaml_include)

    loaded_yamls = set()

    for file in binding_files:
        # Extract compat from 'constraint:' line
        for line in open(file, 'r', encoding='utf-8'):
            match = re.match(r'\s+constraint:\s*"([^"]*)"', line)
            if match:
                break
        else:
            # No 'constraint:' line found. Move on to next yaml file.
            continue

        compat = match.group(1)
        if compat not in dts_compats or file in loaded_yamls:
            # The compat does not appear in the device tree, or the yaml
            # file has already been loaded
            continue

        # Found a binding (.yaml file) for a 'compatible' value that
        # appears in DTS. Load it.

        loaded_yamls.add(file)

        if compat not in compats:
            compats.append(compat)

        with open(file, 'r', encoding='utf-8') as yf:
            binding = merge_included_bindings(file, yaml.load(yf))

            if 'parent' in binding:
                bus_to_binding[binding['parent']['bus']][compat] = binding
            else:
                compat_to_binding[compat] = binding

    if not compat_to_binding:
        raise Exception("No bindings found in '{}'".format(binding_dirs))

    extract.globals.bindings = compat_to_binding
    extract.globals.bus_bindings = bus_to_binding
    extract.globals.binding_compats = compats


def find_binding_files(binding_dirs):
    # Initializes the global 'binding_files' variable with a list of paths to
    # binding (.yaml) files

    global binding_files

    binding_files = []

    for binding_dir in binding_dirs:
        for root, dirnames, filenames in os.walk(binding_dir):
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
        return yaml.load(f)


def yaml_inc_error(msg):
    # Helper for reporting errors in the !include implementation

    raise yaml.constructor.ConstructorError(None, None, msg)


def generate_defines():
    # Generates #defines (and .conf file values) from DTS

    for node_path in reduced.keys():
        generate_node_defines(node_path)

    if not defs:
        raise Exception("No information parsed from dts file.")

    for k, v in regs_config.items():
        if k in chosen:
            reg.extract(chosen[k], None, v, 1024)

    for k, v in name_config.items():
        if k in chosen:
            extract_string_prop(chosen[k], "label", v)

    node_path = chosen.get('zephyr,flash', 'dummy-flash')
    flash.extract(node_path, 'zephyr,flash', 'DT_FLASH')
    node_path = chosen.get('zephyr,code-partition', node_path)
    flash.extract(node_path, 'zephyr,code-partition', None)

    # Add DT_CHOSEN_<X> defines
    for c in sorted(chosen):
        insert_defs('chosen', {'DT_CHOSEN_' + str_to_label(c): '1'}, {})


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
            write_header(f)


if __name__ == '__main__':
    main()
