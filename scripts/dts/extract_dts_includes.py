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
from copy import deepcopy

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

class Bindings(yaml.Loader):

    ##
    # List of all yaml files available for yaml loaders
    # of this class. Must be preset before the first
    # load operation.
    _files = []

    ##
    # Files that are already included.
    # Must be reset on the load of every new binding
    _included = []

    @classmethod
    def bindings(cls, dts_compats, yaml_dirs):
        # Find all .yaml files in yaml_dirs
        cls._files = []
        for yaml_dir in yaml_dirs:
            for root, dirnames, filenames in os.walk(yaml_dir):
                for filename in fnmatch.filter(filenames, '*.yaml'):
                    cls._files.append(os.path.join(root, filename))

        compat_to_binding = {}
        # Maps buses to dictionaries that map compats to YAML nodes
        bus_to_binding = defaultdict(dict)
        compats = []

        loaded_yamls = set()

        for file in cls._files:
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
                cls._included = []
                binding = merge_included_bindings(file, yaml.load(yf, cls))

                if 'parent' in binding:
                    bus_to_binding[binding['parent']['bus']][compat] = binding
                else:
                    compat_to_binding[compat] = binding

        return compat_to_binding, bus_to_binding, compats

    def __init__(self, stream):
        filepath = os.path.realpath(stream.name)
        if filepath in self._included:
            print("Error: circular inclusion for file name '{}'".
                  format(stream.name))
            raise yaml.constructor.ConstructorError
        self._included.append(filepath)
        super(Bindings, self).__init__(stream)
        Bindings.add_constructor('!include', Bindings._include)

    def _include(self, node):
        # Implements !include. Returns a list with the top-level YAML
        # structures for the included files (a single-element list if there's
        # just one file).

        if isinstance(node, yaml.ScalarNode):
            # !include foo.yaml
            return [self._extract_file(self.construct_scalar(node))]

        if isinstance(node, yaml.SequenceNode):
            # !include [foo.yaml, bar.yaml]
            return [self._extract_file(fname)
                    for fname in self.construct_sequence(node)]

        print("Error: unrecognised node type in !include statement")
        raise yaml.constructor.ConstructorError

    def _extract_file(self, filename):
        filepaths = [filepath for filepath in self._files if filepath.endswith(filename)]
        if len(filepaths) == 0:
            print("Error: unknown file name '{}' in !include statement".
                  format(filename))
            raise yaml.constructor.ConstructorError
        elif len(filepaths) > 1:
            # multiple candidates for filename
            files = []
            for filepath in filepaths:
                if os.path.basename(filename) == os.path.basename(filepath):
                    files.append(filepath)
            if len(files) > 1:
                print("Error: multiple candidates for file name '{}' in !include statement".
                      format(filename), filepaths)
                raise yaml.constructor.ConstructorError
            filepaths = files
        with open(filepaths[0], 'r', encoding='utf-8') as f:
            return yaml.load(f, Bindings)

def extract_bus_name(node_address, def_label):
    label = def_label + '_BUS_NAME'
    prop_alias = {}

    add_compat_alias(node_address, 'BUS_NAME', label, prop_alias)

    # Generate defines for node aliases
    if node_address in aliases:
        add_prop_aliases(
            node_address,
            lambda alias: str_to_label(alias) + '_BUS_NAME',
            label,
            prop_alias)

    insert_defs(node_address,
                {label: '"' + find_parent_prop(node_address, 'label') + '"'},
                prop_alias)


def extract_string_prop(node_address, key, label):
    if node_address not in defs:
        # Make all defs have the special 'aliases' key, to remove existence
        # checks elsewhere
        defs[node_address] = {'aliases': {}}

    defs[node_address][label] = '"' + reduced[node_address]['props'][key] + '"'


def extract_property(node_compat, node_address, prop, prop_val, names):

    node = reduced[node_address]
    yaml_node_compat = get_binding(node_address)
    def_label = get_node_label(node_address)

    if 'parent' in yaml_node_compat:
        if 'bus' in yaml_node_compat['parent']:
            # get parent label
            parent_address = get_parent_address(node_address)

            #check parent has matching child bus value
            try:
                parent_yaml = get_binding(parent_address)
                parent_bus = parent_yaml['child']['bus']
            except (KeyError, TypeError) as e:
                raise Exception(str(node_address) + " defines parent " +
                        str(parent_address) + " as bus master but " +
                        str(parent_address) + " not configured as bus master " +
                        "in yaml description")

            if parent_bus != yaml_node_compat['parent']['bus']:
                bus_value = yaml_node_compat['parent']['bus']
                raise Exception(str(node_address) + " defines parent " +
                        str(parent_address) + " as " + bus_value +
                        " bus master but " + str(parent_address) +
                        " configured as " + str(parent_bus) +
                        " bus master")

            # Generate alias definition if parent has any alias
            if parent_address in aliases:
                for i in aliases[parent_address]:
                    # Build an alias name that respects device tree specs
                    node_name = node_compat + '-' + node_address.split('@')[-1]
                    node_strip = node_name.replace('@','-').replace(',','-')
                    node_alias = i + '-' + node_strip
                    if node_alias not in aliases[node_address]:
                        # Need to generate alias name for this node:
                        aliases[node_address].append(node_alias)

            # Build the name from the parent node's label
            def_label = get_node_label(parent_address) + '_' + def_label

            # Generate *_BUS_NAME #define
            extract_bus_name(node_address, 'DT_' + def_label)

    def_label = 'DT_' + def_label

    if prop == 'reg':
        reg.extract(node_address, names, def_label, 1)
    elif prop == 'interrupts' or prop == 'interrupts-extended':
        interrupts.extract(node_address, prop, names, def_label)
    elif prop == 'compatible':
        compatible.extract(node_address, prop, def_label)
    elif 'pinctrl-' in prop:
        pinctrl.extract(node_address, prop, def_label)
    elif 'clocks' in prop:
        clocks.extract(node_address, prop, def_label)
    elif 'pwms' in prop or 'gpios' in prop:
        prop_values = reduced[node_address]['props'][prop]
        generic = prop[:-1]  # Drop the 's' from the prop

        extract_controller(node_address, prop, prop_values, 0,
                           def_label, generic)
        extract_cells(node_address, prop, prop_values,
                      names, 0, def_label, generic)
    else:
        default.extract(node_address, prop, prop_val['type'], def_label)


def extract_node_include_info(reduced, root_node_address, sub_node_address,
                              y_sub):

    filter_list = ['interrupt-names',
                    'reg-names',
                    'phandle',
                    'linux,phandle']
    node = reduced[sub_node_address]
    node_compat = get_compat(root_node_address)

    if node_compat not in get_binding_compats():
        return {}, {}

    if y_sub is None:
        y_node = get_binding(root_node_address)
    else:
        y_node = y_sub

    # check to see if we need to process the properties
    for k, v in y_node['properties'].items():
            if 'properties' in v:
                for c in reduced:
                    if root_node_address + '/' in c:
                        extract_node_include_info(
                            reduced, root_node_address, c, v)
            if 'generation' in v:

                match = False

                # Handle any per node extraction first.  For example we
                # extract a few different defines for a flash partition so its
                # easier to handle the partition node in one step
                if 'partition@' in sub_node_address:
                    flash.extract_partition(sub_node_address)
                    continue

                # Handle each property individually, this ends up handling common
                # patterns for things like reg, interrupts, etc that we don't need
                # any special case handling at a node level
                for c in node['props']:
                    # if prop is in filter list - ignore it
                    if c in filter_list:
                        continue

                    if re.match(k + '$', c):

                        if 'pinctrl-' in c:
                            names = deepcopy(node['props'].get(
                                                        'pinctrl-names', []))
                        else:
                            if not c.endswith("-names"):
                                names = deepcopy(node['props'].get(
                                                        c[:-1] + '-names', []))
                                if not names:
                                    names = deepcopy(node['props'].get(
                                                            c + '-names', []))
                        if not isinstance(names, list):
                            names = [names]

                        extract_property(
                            node_compat, sub_node_address, c, v, names)
                        match = True

                # Handle the case that we have a boolean property, but its not
                # in the dts
                if not match:
                    if v['type'] == "boolean":
                        extract_property(
                            node_compat, sub_node_address, k, v, None)

def dict_merge(parent, fname, dct, merge_dct):
    # from https://gist.github.com/angstwad/bf22d1822c38a92ec0a9

    """ Recursive dict merge. Inspired by :meth:``dict.update()``, instead of
    updating only top-level keys, dict_merge recurses down into dicts nested
    to an arbitrary depth, updating keys. The ``merge_dct`` is merged into
    ``dct``.
    :param parent: parent tuple key
    :param fname: yaml file being processed
    :param dct: dict onto which the merge is executed
    :param merge_dct: dct merged into dct
    :return: None
    """
    for k, v in merge_dct.items():
        if (k in dct and isinstance(dct[k], dict)
                and isinstance(merge_dct[k], Mapping)):
            dict_merge(k, fname, dct[k], merge_dct[k])
        else:
            if k in dct and dct[k] != merge_dct[k]:
                merge_warn = True
                # Don't warn if we are changing the "category" from "optional
                # to "required"
                if (k == "category" and dct[k] == "optional" and
                    merge_dct[k] == "required"):
                    merge_warn = False
                if merge_warn:
                    print("extract_dts_includes.py: {}('{}') merge of property '{}': '{}' overwrites '{}'.".format(
                            fname, parent, k, merge_dct[k], dct[k]))
            dct[k] = merge_dct[k]


def merge_included_bindings(fname, node):
    # Recursively merges properties from files !include'd from the 'inherits'
    # section of the binding. 'fname' is the path to the top-level binding
    # file, and 'node' the current top-level YAML node being processed.

    check_binding_properties(node)

    if 'inherits' in node:
        for inherits in node.pop('inherits'):
            inherits = merge_included_bindings(fname, inherits)
            # title, description, version of inherited node
            # are overwritten by intention. Remove to prevent dct_merge to
            # complain about duplicates.
            inherits.pop('title', None)
            inherits.pop('version', None)
            inherits.pop('description', None)
            dict_merge(None, fname, inherits, node)
            node = inherits

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
            if prop != 'aliases':
                f.write('%s=%s\n' % (prop, defs[node][prop]))

        for alias in sorted(defs[node]['aliases']):
            alias_target = defs[node]['aliases'][alias]
            if alias_target not in defs[node]:
                alias_target = defs[node]['aliases'][alias_target]
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


def load_yaml_descriptions(root, yaml_dirs):
    compatibles = all_compats(root)

    (bindings, bus, bindings_compat) = Bindings.bindings(compatibles, yaml_dirs)
    if not bindings:
        raise Exception("Missing YAML information.  Check YAML sources")

    return (bindings, bus, bindings_compat)


def generate_node_definitions():

    for k, v in reduced.items():
        node_compat = get_compat(k)
        if node_compat is not None and node_compat in get_binding_compats():
            extract_node_include_info(reduced, k, k, None)

    if not defs:
        raise Exception("No information parsed from dts file.")

    for k, v in regs_config.items():
        if k in chosen:
            reg.extract(chosen[k], None, v, 1024)

    for k, v in name_config.items():
        if k in chosen:
            extract_string_prop(chosen[k], "label", v)

    node_address = chosen.get('zephyr,flash', 'dummy-flash')
    flash.extract(node_address, 'zephyr,flash', 'DT_FLASH')
    node_address = chosen.get('zephyr,code-partition', node_address)
    flash.extract(node_address, 'zephyr,code-partition', None)


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

    (extract.globals.bindings, extract.globals.bus_bindings,
     extract.globals.bindings_compat) = load_yaml_descriptions(root, args.yaml)

    generate_node_definitions()

    # Add DT_CHOSEN_<X> defines to generated files
    for c in sorted(chosen):
        insert_defs('chosen', {'DT_CHOSEN_' + str_to_label(c): '1'}, {})

    # Generate config and header files

    if args.keyvalue is not None:
        with open(args.keyvalue, 'w', encoding='utf-8') as f:
            write_conf(f)

    if args.include is not None:
        with open(args.include, 'w', encoding='utf-8') as f:
            write_header(f)


if __name__ == '__main__':
    main()
