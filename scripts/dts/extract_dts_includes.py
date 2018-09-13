#!/usr/bin/env python3
#
# Copyright (c) 2017, Linaro Limited
# Copyright (c) 2018, Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# vim: ai:ts=4:sw=4

import sys
from os import listdir
import os, fnmatch
import re
import yaml
import argparse
import collections
from copy import deepcopy

from devicetree import parse_file
from extract.globals import *

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
    def bindings(cls, compatibles, yaml_dirs):
        # find unique set of compatibles across all active nodes
        s = set()
        for k, v in compatibles.items():
            if isinstance(v, list):
                for item in v:
                    s.add(item)
            else:
                s.add(v)

        # scan YAML files and find the ones we are interested in
        cls._files = []
        for yaml_dir in yaml_dirs:
            for root, dirnames, filenames in os.walk(yaml_dir):
                for filename in fnmatch.filter(filenames, '*.yaml'):
                    cls._files.append(os.path.join(root, filename))

        yaml_list = {}
        file_load_list = set()
        for file in cls._files:
            for line in open(file, 'r'):
                if re.search('^\s+constraint:*', line):
                    c = line.split(':')[1].strip()
                    c = c.strip('"')
                    if c in s:
                        if file not in file_load_list:
                            file_load_list.add(file)
                            with open(file, 'r') as yf:
                                cls._included = []
                                yaml_list[c] = yaml.load(yf, cls)
        return yaml_list

    def __init__(self, stream):
        filepath = os.path.realpath(stream.name)
        if filepath in self._included:
            print("Error:: circular inclusion for file name '{}'".
                  format(stream.name))
            raise yaml.constructor.ConstructorError
        self._included.append(filepath)
        super(Bindings, self).__init__(stream)
        Bindings.add_constructor('!include', Bindings._include)
        Bindings.add_constructor('!import',  Bindings._include)

    def _include(self, node):
        if isinstance(node, yaml.ScalarNode):
            return self._extract_file(self.construct_scalar(node))

        elif isinstance(node, yaml.SequenceNode):
            result = []
            for filename in self.construct_sequence(node):
                result.append(self._extract_file(filename))
            return result

        elif isinstance(node, yaml.MappingNode):
            result = {}
            for k, v in self.construct_mapping(node).iteritems():
                result[k] = self._extract_file(v)
            return result

        else:
            print("Error:: unrecognised node type in !include statement")
            raise yaml.constructor.ConstructorError

    def _extract_file(self, filename):
        filepaths = [filepath for filepath in self._files if filepath.endswith(filename)]
        if len(filepaths) == 0:
            print("Error:: unknown file name '{}' in !include statement".
                  format(filename))
            raise yaml.constructor.ConstructorError
        elif len(filepaths) > 1:
            # multiple candidates for filename
            files = []
            for filepath in filepaths:
                if os.path.basename(filename) == os.path.basename(filepath):
                    files.append(filepath)
            if len(files) > 1:
                print("Error:: multiple candidates for file name '{}' in !include statement".
                      format(filename), filepaths)
                raise yaml.constructor.ConstructorError
            filepaths = files
        with open(filepaths[0], 'r') as f:
            return yaml.load(f, Bindings)


def extract_reg_prop(node_address, names, def_label, div, post_label):

    reg = reduced[node_address]['props']['reg']
    if type(reg) is not list: reg = [ reg ]
    props = list(reg)

    address_cells = reduced['/']['props'].get('#address-cells')
    size_cells = reduced['/']['props'].get('#size-cells')
    address = ''
    for comp in node_address.split('/')[1:-1]:
        address += '/' + comp
        address_cells = reduced[address]['props'].get(
            '#address-cells', address_cells)
        size_cells = reduced[address]['props'].get('#size-cells', size_cells)

    if post_label is None:
        post_label = "BASE_ADDRESS"

    index = 0
    l_base = def_label.split('/')
    l_addr = [convert_string_to_label(post_label)]
    l_size = ["SIZE"]

    while props:
        prop_def = {}
        prop_alias = {}
        addr = 0
        size = 0
        # Check is defined should be indexed (_0, _1)
        if index == 0 and len(props) < 3:
            # 1 element (len 2) or no element (len 0) in props
            l_idx = []
        else:
            l_idx = [str(index)]

        try:
            name = [names.pop(0).upper()]
        except:
            name = []

        for x in range(address_cells):
            addr += props.pop(0) << (32 * x)
        for x in range(size_cells):
            size += props.pop(0) << (32 * x)

        l_addr_fqn = '_'.join(l_base + l_addr + l_idx)
        l_size_fqn = '_'.join(l_base + l_size + l_idx)
        if address_cells:
            prop_def[l_addr_fqn] = hex(addr)
        if size_cells:
            prop_def[l_size_fqn] = int(size / div)
        if len(name):
            if address_cells:
                prop_alias['_'.join(l_base + name + l_addr)] = l_addr_fqn
            if size_cells:
                prop_alias['_'.join(l_base + name + l_size)] = l_size_fqn

        # generate defs for node aliases
        if node_address in aliases:
            for i in aliases[node_address]:
                alias_label = convert_string_to_label(i)
                alias_addr = [alias_label] + l_addr
                alias_size = [alias_label] + l_size
                prop_alias['_'.join(alias_addr)] = '_'.join(l_base + l_addr)
                prop_alias['_'.join(alias_size)] = '_'.join(l_base + l_size)

        insert_defs(node_address, prop_def, prop_alias)

        # increment index for definition creation
        index += 1


def extract_controller(node_address, yaml, prop, prop_values, index, def_label, generic):

    prop_def = {}
    prop_alias = {}

    # get controller node (referenced via phandle)
    cell_parent = phandles[prop_values[0]]

    for k in reduced[cell_parent]['props'].keys():
        if k[0] == '#' and '-cells' in k:
            num_cells = reduced[cell_parent]['props'].get(k)

    # get controller node (referenced via phandle)
    cell_parent = phandles[prop_values[0]]

    try:
       l_cell = reduced[cell_parent]['props'].get('label')
    except KeyError:
        l_cell = None

    if l_cell is not None:

        l_base = def_label.split('/')

        # Check is defined should be indexed (_0, _1)
        if index == 0 and len(prop_values) < (num_cells + 2):
            # 0 or 1 element in prop_values
            # ( ie len < num_cells + phandle + 1 )
            l_idx = []
        else:
            l_idx = [str(index)]

        # Check node generation requirements
        try:
            generation = yaml[get_compat(node_address)]['properties'][prop][
                'generation']
        except:
            generation = ''

        if 'use-prop-name' in generation:
            l_cellname = convert_string_to_label(prop + '_' + 'controller')
        else:
            l_cellname = convert_string_to_label(generic + '_' + 'controller')

        label = l_base + [l_cellname] + l_idx

        prop_def['_'.join(label)] = "\"" + l_cell + "\""

        #generate defs also if node is referenced as an alias in dts
        if node_address in aliases:
            for i in aliases[node_address]:
                alias_label = \
                    convert_string_to_label(i)
                alias = [alias_label] + label[1:]
                prop_alias['_'.join(alias)] = '_'.join(label)

        insert_defs(node_address, prop_def, prop_alias)

    # prop off phandle + num_cells to get to next list item
    prop_values = prop_values[num_cells+1:]

    # recurse if we have anything left
    if len(prop_values):
        extract_controller(node_address, yaml, prop, prop_values, index + 1,
                           def_label, generic)


def extract_cells(node_address, yaml, prop, prop_values, names, index,
                  def_label, generic):

    cell_parent = phandles[prop_values.pop(0)]

    try:
        cell_yaml = yaml[get_compat(cell_parent)]
    except:
        raise Exception(
            "Could not find yaml description for " +
                reduced[cell_parent]['name'])

    try:
        name = names.pop(0).upper()
    except:
        name = []

    # Get number of cells per element of current property
    for k in reduced[cell_parent]['props'].keys():
        if k[0] == '#' and '-cells' in k:
            num_cells = reduced[cell_parent]['props'].get(k)
    try:
        generation = yaml[get_compat(node_address)]['properties'][prop][
            'generation']
    except:
        generation = ''

    if 'use-prop-name' in generation:
        l_cell = [convert_string_to_label(str(prop))]
    else:
        l_cell = [convert_string_to_label(str(generic))]

    l_base = def_label.split('/')
    # Check if #define should be indexed (_0, _1, ...)
    if index == 0 and len(prop_values) < (num_cells + 2):
        # Less than 2 elements in prop_values (ie len < num_cells + phandle + 1)
        # Indexing is not needed
        l_idx = []
    else:
        l_idx = [str(index)]

    prop_def = {}
    prop_alias = {}

    # Generate label for each field of the property element
    for i in range(num_cells):
        l_cellname = [str(cell_yaml['#cells'][i]).upper()]
        if l_cell == l_cellname:
            label = l_base + l_cell + l_idx
        else:
            label = l_base + l_cell + l_cellname + l_idx
        label_name = l_base + name + l_cellname
        prop_def['_'.join(label)] = prop_values.pop(0)
        if len(name):
            prop_alias['_'.join(label_name)] = '_'.join(label)

        # generate defs for node aliases
        if node_address in aliases:
            for i in aliases[node_address]:
                alias_label = convert_string_to_label(i)
                alias = [alias_label] + label[1:]
                prop_alias['_'.join(alias)] = '_'.join(label)

        insert_defs(node_address, prop_def, prop_alias)

    # recurse if we have anything left
    if len(prop_values):
        extract_cells(node_address, yaml, prop, prop_values, names,
                      index + 1, def_label, generic)


def extract_single(node_address, yaml, prop, key, def_label):

    prop_def = {}
    prop_alias = {}

    if isinstance(prop, list):
        for i, p in enumerate(prop):
            k = convert_string_to_label(key)
            label = def_label + '_' + k
            if isinstance(p, str):
                p = "\"" + p + "\""
            prop_def[label + '_' + str(i)] = p
    else:
        k = convert_string_to_label(key)
        label = def_label + '_' + k

        if prop == 'parent-label':
            prop = find_parent_prop(node_address, 'label')

        if isinstance(prop, str):
            prop = "\"" + prop + "\""
        prop_def[label] = prop

        # generate defs for node aliases
        if node_address in aliases:
            for i in aliases[node_address]:
                alias_label = convert_string_to_label(i)
                alias = alias_label + '_' + k
                prop_alias[alias] = label

    insert_defs(node_address, prop_def, prop_alias)

def extract_string_prop(node_address, yaml, key, label):

    prop_def = {}

    node = reduced[node_address]
    prop = node['props'][key]

    k = convert_string_to_label(key)
    prop_def[label] = "\"" + prop + "\""

    if node_address in defs:
        defs[node_address].update(prop_def)
    else:
        defs[node_address] = prop_def


def extract_property(node_compat, yaml, node_address, prop, prop_val, names,
                     label_override):

    if 'base_label' in yaml[node_compat]:
        def_label = yaml[node_compat].get('base_label')
    else:
        def_label = get_node_label(node_compat, node_address)

    if 'parent' in yaml[node_compat]:
        if 'bus' in yaml[node_compat]['parent']:
            # get parent label
            parent_address = ''
            for comp in node_address.split('/')[1:-1]:
                parent_address += '/' + comp

            #check parent has matching child bus value
            try:
                parent_yaml = \
                    yaml[reduced[parent_address]['props']['compatible']]
                parent_bus = parent_yaml['child']['bus']
            except (KeyError, TypeError) as e:
                raise Exception(str(node_address) + " defines parent " +
                        str(parent_address) + " as bus master but " +
                        str(parent_address) + " not configured as bus master " +
                        "in yaml description")

            if parent_bus != yaml[node_compat]['parent']['bus']:
                bus_value = yaml[node_compat]['parent']['bus']
                raise Exception(str(node_address) + " defines parent " +
                        str(parent_address) + " as " + bus_value +
                        " bus master but " + str(parent_address) +
                        " configured as " + str(parent_bus) +
                        " bus master")

            # Generate alias definition if parent has any alias
            if parent_address in aliases:
                for i in aliases[parent_address]:
                    node_alias = i + '_' + def_label
                    aliases[node_address].append(node_alias)

            # Use parent label to generate label
            parent_label = get_node_label(
                find_parent_prop(node_address,'compatible') , parent_address)
            def_label = parent_label + '_' + def_label

            # Generate bus-name define
            extract_single(node_address, yaml, 'parent-label',
                           'bus-name', def_label)

    if label_override is not None:
        def_label += '_' + label_override

    if prop == 'reg':
        if 'partition@' in node_address:
            # reg in partition is covered by flash handling
            flash.extract(node_address, yaml, prop, names, def_label)
        else:
            reg.extract(node_address, yaml, prop, names, def_label)
    elif prop == 'interrupts' or prop == 'interrupts-extended':
        interrupts.extract(node_address, yaml, prop, names, def_label)
    elif prop == 'compatible':
        compatible.extract(node_address, yaml, prop, names, def_label)
    elif 'pinctrl-' in prop:
        pinctrl.extract(node_address, yaml, prop, names, def_label)
    elif 'clocks' in prop:
        clocks.extract(node_address, yaml, prop, names, def_label)
    elif 'gpios' in prop:
        try:
            prop_values = list(reduced[node_address]['props'].get(prop))
        except:
            prop_values = reduced[node_address]['props'].get(prop)

        extract_controller(node_address, yaml, prop, prop_values, 0,
                           def_label, 'gpio')
        extract_cells(node_address, yaml, prop, prop_values,
                      names, 0, def_label, 'gpio')
    else:
        default.extract(node_address, yaml, prop, names, def_label)


def extract_node_include_info(reduced, root_node_address, sub_node_address,
                              yaml, y_sub):
    node = reduced[sub_node_address]
    node_compat = get_compat(root_node_address)
    label_override = None

    if node_compat not in yaml.keys():
        return {}, {}

    if y_sub is None:
        y_node = yaml[node_compat]
    else:
        y_node = y_sub

    if yaml[node_compat].get('use-property-label', False):
        try:
            label = y_node['properties']['label']
            label_override = convert_string_to_label(node['props']['label'])
        except KeyError:
            pass

    # check to see if we need to process the properties
    for k, v in y_node['properties'].items():
            if 'properties' in v:
                for c in reduced:
                    if root_node_address + '/' in c:
                        extract_node_include_info(
                            reduced, root_node_address, c, yaml, v)
            if 'generation' in v:

                for c in node['props'].keys():
                    if c.endswith("-names"):
                        pass

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
                            else:
                                names = []
                        if not isinstance(names, list):
                            names = [names]

                        extract_property(
                            node_compat, yaml, sub_node_address, c, v, names,
                            label_override)


def dict_merge(dct, merge_dct):
    # from https://gist.github.com/angstwad/bf22d1822c38a92ec0a9

    """ Recursive dict merge. Inspired by :meth:``dict.update()``, instead of
    updating only top-level keys, dict_merge recurses down into dicts nested
    to an arbitrary depth, updating keys. The ``merge_dct`` is merged into
    ``dct``.
    :param dct: dict onto which the merge is executed
    :param merge_dct: dct merged into dct
    :return: None
    """
    for k, v in merge_dct.items():
        if (k in dct and isinstance(dct[k], dict)
                and isinstance(merge_dct[k], collections.Mapping)):
            dict_merge(dct[k], merge_dct[k])
        else:
            if k in dct and dct[k] != merge_dct[k]:
                print("extract_dts_includes.py: Merge of '{}': '{}'  overwrites '{}'.".format(
                        k, merge_dct[k], dct[k]))
            dct[k] = merge_dct[k]


def yaml_traverse_inherited(node):
    """ Recursive overload procedure inside ``node``
    ``inherits`` section is searched for and used as node base when found.
    Base values are then overloaded by node values
    Additionally, 'id' key of 'inherited' dict is converted to 'node_type'
    and some consistency checks are done.
    :param node:
    :return: node
    """

    # do some consistency checks. Especially id is needed for further
    # processing. title must be first to check.
    if 'title' not in node:
        # If 'title' is missing, make fault finding more easy.
        # Give a hint what node we are looking at.
        print("extract_dts_includes.py: node without 'title' -", node)
    for prop in ('title', 'id', 'version', 'description'):
        if prop not in node:
            node[prop] = "<unknown {}>".format(prop)
            print("extract_dts_includes.py: '{}' property missing".format(prop),
                  "in '{}' binding. Using '{}'.".format(node['title'], node[prop]))

    if 'node_type' not in node:
        node['node_type'] = [node['id'],]

    if 'inherits' in node:
        if isinstance(node['inherits'], list):
            inherits_list  = node['inherits']
        else:
            inherits_list  = [node['inherits'],]
        node.pop('inherits')
        for inherits in inherits_list:
            if 'inherits' in inherits:
                inherits = yaml_traverse_inherited(inherits)
            if 'node_type' in inherits:
                node['node_type'].extend(inherits['node_type'])
            else:
                if 'id' not in inherits:
                    inherits['id'] = "<unknown id>"
                    title = inherits.get('title', "<unknown title>")
                    print("extract_dts_includes.py: 'id' property missing in",
                          "'{}' binding. Using '{}'.".format(title,
                                                             inherits['id']))
                node['node_type'].append(inherits['id'])
            # id, node_type, title, description, version of inherited node
            # are overwritten by intention. Remove to prevent dct_merge to
            # complain about duplicates.
            inherits.pop('id')
            inherits.pop('node_type', None)
            inherits.pop('title', None)
            inherits.pop('version', None)
            inherits.pop('description', None)
            dict_merge(inherits, node)
            node = inherits
    return node


def yaml_collapse(yaml_list):

    collapsed = dict(yaml_list)

    for k, v in collapsed.items():
        v = yaml_traverse_inherited(v)
        collapsed[k]=v

    return collapsed


def get_key_value(k, v, tabstop):
    label = "#define " + k

    # calculate the name's tabs
    if len(label) % 8:
        tabs = (len(label) + 7) >> 3
    else:
        tabs = (len(label) >> 3) + 1

    line = label
    for i in range(0, tabstop - tabs + 1):
        line += '\t'
    line += str(v)
    line += '\n'

    return line


def output_keyvalue_lines(fd):
    node_keys = sorted(defs.keys())
    for node in node_keys:
        fd.write('# ' + node.split('/')[-1])
        fd.write("\n")

        prop_keys = sorted(defs[node].keys())
        for prop in prop_keys:
            if prop == 'aliases':
                for entry in sorted(defs[node][prop]):
                    a = defs[node][prop].get(entry)
                    fd.write("%s=%s\n" % (entry, defs[node].get(a)))
            else:
                fd.write("%s=%s\n" % (prop, defs[node].get(prop)))

        fd.write("\n")

def generate_keyvalue_file(kv_file):
    with open(kv_file, "w") as fd:
        output_keyvalue_lines(fd)


def output_include_lines(fd, fixups):
    compatible = reduced['/']['props']['compatible'][0]

    fd.write("/**************************************************\n")
    fd.write(" * Generated include file for " + compatible)
    fd.write("\n")
    fd.write(" *               DO NOT MODIFY\n")
    fd.write(" */\n")
    fd.write("\n")
    fd.write("#ifndef DEVICE_TREE_BOARD_H" + "\n")
    fd.write("#define DEVICE_TREE_BOARD_H" + "\n")
    fd.write("\n")

    node_keys = sorted(defs.keys())
    for node in node_keys:
        fd.write('/* ' + node.split('/')[-1] + ' */')
        fd.write("\n")

        max_dict_key = lambda d: max(len(k) for k in d.keys())
        maxlength = 0
        if defs[node].get('aliases'):
            maxlength = max_dict_key(defs[node]['aliases'])
        maxlength = max(maxlength, max_dict_key(defs[node])) + len('#define ')

        if maxlength % 8:
            maxtabstop = (maxlength + 7) >> 3
        else:
            maxtabstop = (maxlength >> 3) + 1

        if (maxtabstop * 8 - maxlength) <= 2:
            maxtabstop += 1

        prop_keys = sorted(defs[node].keys())
        for prop in prop_keys:
            if prop == 'aliases':
                for entry in sorted(defs[node][prop]):
                    a = defs[node][prop].get(entry)
                    fd.write(get_key_value(entry, a, maxtabstop))
            else:
                fd.write(get_key_value(prop, defs[node].get(prop), maxtabstop))

        fd.write("\n")

    if fixups:
        for fixup in fixups:
            if os.path.exists(fixup):
                fd.write("\n")
                fd.write(
                    "/* Following definitions fixup the generated include */\n")
                try:
                    with open(fixup, "r") as fixup_fd:
                        for line in fixup_fd.readlines():
                            fd.write(line)
                        fd.write("\n")
                except:
                    raise Exception(
                        "Input file " + os.path.abspath(fixup) +
                        " does not exist.")

    fd.write("#endif\n")


def generate_include_file(inc_file, fixups):
    with open(inc_file, "w") as fd:
        output_include_lines(fd, fixups)


def load_and_parse_dts(dts_file):
    with open(dts_file, "r") as fd:
        dts = parse_file(fd)

    return dts


def load_yaml_descriptions(dts, yaml_dirs):
    compatibles = get_all_compatibles(dts['/'], '/', {})

    yaml_list = Bindings.bindings(compatibles, yaml_dirs)
    if yaml_list == {}:
        raise Exception("Missing YAML information.  Check YAML sources")

    # collapse the yaml inherited information
    yaml_list = yaml_collapse(yaml_list)

    return yaml_list


def generate_node_definitions(yaml_list):

    for k, v in reduced.items():
        node_compat = get_compat(k)
        if node_compat is not None and node_compat in yaml_list:
            extract_node_include_info(
                reduced, k, k, yaml_list, None)

    if defs == {}:
        raise Exception("No information parsed from dts file.")

    for k, v in regs_config.items():
        if k in chosen:
            extract_reg_prop(chosen[k], None, v, 1024, None)

    for k, v in name_config.items():
        if k in chosen:
            extract_string_prop(chosen[k], None, "label", v)

    node_address = chosen.get('zephyr,flash', 'dummy-flash')
    flash.extract(node_address, yaml_list, 'zephyr,flash', None, 'FLASH')
    node_address = chosen.get('zephyr,code-partition', node_address)
    flash.extract(node_address, yaml_list, 'zephyr,code-partition', None,
                  'FLASH')

    return defs


def parse_arguments():
    rdh = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=rdh)

    parser.add_argument("-d", "--dts", nargs=1, required=True, help="DTS file")
    parser.add_argument("-y", "--yaml", nargs='+', required=True,
                        help="YAML file directories, we allow multiple")
    parser.add_argument("-f", "--fixup", nargs='+',
                        help="Fixup file(s), we allow multiple")
    parser.add_argument("-i", "--include", nargs=1, required=True,
                        help="Generate include file for the build system")
    parser.add_argument("-k", "--keyvalue", nargs=1, required=True,
                        help="Generate config file for the build system")
    return parser.parse_args()


def main():
    args = parse_arguments()

    dts = load_and_parse_dts(args.dts[0])

    # build up useful lists
    get_reduced(dts['/'], '/')
    get_phandles(dts['/'], '/', {})
    get_aliases(dts['/'])
    get_chosen(dts['/'])

    yaml_list = load_yaml_descriptions(dts, args.yaml)

    defs = generate_node_definitions(yaml_list)

     # generate config and include file
    generate_keyvalue_file(args.keyvalue[0])

    generate_include_file(args.include[0], args.fixup)


if __name__ == '__main__':
    main()
