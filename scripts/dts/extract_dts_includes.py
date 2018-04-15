#!/usr/bin/env python3
#
# Copyright (c) 2017, Linaro Limited
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

from devicetree import parse_file
from extract.globals import *

class Loader(yaml.Loader):
    def __init__(self, stream):
        self._root = os.path.realpath(stream.name)
        super(Loader, self).__init__(stream)
        Loader.add_constructor('!include', Loader.include)
        Loader.add_constructor('!import',  Loader.include)

    def include(self, node):
        if isinstance(node, yaml.ScalarNode):
            return self.extractFile(self.construct_scalar(node))

        elif isinstance(node, yaml.SequenceNode):
            result = []
            for filename in self.construct_sequence(node):
                result.append(self.extractFile(filename))
            return result

        elif isinstance(node, yaml.MappingNode):
            result = {}
            for k, v in self.construct_mapping(node).iteritems():
                result[k] = self.extractFile(v)
            return result

        else:
            print("Error:: unrecognised node type in !include statement")
            raise yaml.constructor.ConstructorError

    def extractFile(self, filename):
        filepath = os.path.join(os.path.dirname(self._root), filename)
        if not os.path.isfile(filepath):
            # we need to look in bindings/* directories
            # take path and back up 1 directory and parse in '/bindings/*'
            filepath = os.path.dirname(os.path.dirname(self._root))
            for root, dirnames, file in os.walk(filepath):
                if fnmatch.filter(file, filename):
                    filepath = os.path.join(root, filename)
        with open(filepath, 'r') as f:
            return yaml.load(f, Loader)

def find_parent_irq_node(node_address):
    address = ''

    for comp in node_address.split('/')[1:]:
        address += '/' + comp
        if 'interrupt-parent' in reduced[address]['props']:
            interrupt_parent = reduced[address]['props'].get(
                'interrupt-parent')

    return phandles[interrupt_parent]

def extract_interrupts(node_address, yaml, prop, names, defs, def_label):
    node = reduced[node_address]

    try:
        props = list(node['props'].get(prop))
    except:
        props = [node['props'].get(prop)]

    irq_parent = find_parent_irq_node(node_address)

    l_base = def_label.split('/')
    index = 0

    while props:
        prop_def = {}
        prop_alias = {}
        l_idx = [str(index)]

        try:
            name = [convert_string_to_label(names.pop(0))]
        except:
            name = []

        cell_yaml = yaml[get_compat(irq_parent)]
        l_cell_prefix = ['IRQ']

        for i in range(reduced[irq_parent]['props']['#interrupt-cells']):
            l_cell_name = [cell_yaml['#cells'][i].upper()]
            if l_cell_name == l_cell_prefix:
                l_cell_name = []

            l_fqn = '_'.join(l_base + l_cell_prefix + l_idx + l_cell_name)
            prop_def[l_fqn] = props.pop(0)
            if len(name):
                alias_list = l_base + l_cell_prefix + name + l_cell_name
                prop_alias['_'.join(alias_list)] = l_fqn

            if node_address in aliases:
                for i in aliases[node_address]:
                    alias_label = convert_string_to_label(i)
                    alias_list = [alias_label] + l_cell_prefix + name + l_cell_name
                    prop_alias['_'.join(alias_list)] = l_fqn

        index += 1
        insert_defs(node_address, defs, prop_def, prop_alias)


def extract_reg_prop(node_address, names, defs, def_label, div, post_label):

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

        insert_defs(node_address, defs, prop_def, prop_alias)

        # increment index for definition creation
        index += 1


def extract_controller(node_address, yaml, prop, prop_values, index, defs, def_label, generic):

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

        insert_defs(node_address, defs, prop_def, prop_alias)

    # prop off phandle + num_cells to get to next list item
    prop_values = prop_values[num_cells+1:]

    # recurse if we have anything left
    if len(prop_values):
        extract_controller(node_address, prop, prop_values, index +1, prefix, defs,
                           def_label)


def extract_cells(node_address, yaml, prop, prop_values, names, index, defs,
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

        insert_defs(node_address, defs, prop_def, prop_alias)

    # recurse if we have anything left
    if len(prop_values):
        extract_cells(node_address, yaml, prop, prop_values, names,
                      index + 1, defs, def_label, generic)


def extract_pinctrl(node_address, yaml, prop, names, index, defs,
                    def_label):

    pinconf = reduced[node_address]['props'][prop]

    prop_list = []
    if not isinstance(pinconf, list):
        prop_list.append(pinconf)
    else:
        prop_list = list(pinconf)

    def_prefix = def_label.split('_')

    prop_def = {}
    for p in prop_list:
        pin_node_address = phandles[p]
        pin_subnode = '/'.join(pin_node_address.split('/')[-1:])
        cell_yaml = yaml[get_compat(pin_node_address)]
        cell_prefix = 'PINMUX'
        post_fix = []

        if cell_prefix is not None:
            post_fix.append(cell_prefix)

        for subnode in reduced.keys():
            if pin_subnode in subnode and pin_node_address != subnode:
                # found a subnode underneath the pinmux handle
                pin_label = def_prefix + post_fix + subnode.split('/')[-2:]

                for i, cells in enumerate(reduced[subnode]['props']):
                    key_label = list(pin_label) + \
                        [cell_yaml['#cells'][0]] + [str(i)]
                    func_label = key_label[:-2] + \
                        [cell_yaml['#cells'][1]] + [str(i)]
                    key_label = convert_string_to_label('_'.join(key_label))
                    func_label = convert_string_to_label('_'.join(func_label))

                    prop_def[key_label] = cells
                    prop_def[func_label] = \
                        reduced[subnode]['props'][cells]

    insert_defs(node_address, defs, prop_def, {})


def extract_single(node_address, yaml, prop, key, defs, def_label):

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

    insert_defs(node_address, defs, prop_def, prop_alias)

def extract_string_prop(node_address, yaml, key, label, defs):

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
                     defs, label_override):

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
                           'bus-name', defs, def_label)

    if label_override is not None:
        def_label += '_' + label_override

    if prop == 'reg':
        extract_reg_prop(node_address, names, defs, def_label,
                         1, prop_val.get('label', None))
    elif prop == 'interrupts' or prop == 'interupts-extended':
        extract_interrupts(node_address, yaml, prop, names, defs, def_label)
    elif 'pinctrl-' in prop:
        p_index = int(prop.split('-')[1])
        extract_pinctrl(node_address, yaml, prop,
                        names[p_index], p_index, defs, def_label)
    elif 'clocks' in prop:
        try:
            prop_values = list(reduced[node_address]['props'].get(prop))
        except:
            prop_values = reduced[node_address]['props'].get(prop)

        extract_controller(node_address, yaml, prop, prop_values, 0, defs,
                           def_label, 'clock')
        extract_cells(node_address, yaml, prop, prop_values,
                      names, 0, defs, def_label, 'clock')
    elif 'gpios' in prop:
        try:
            prop_values = list(reduced[node_address]['props'].get(prop))
        except:
            prop_values = reduced[node_address]['props'].get(prop)

        extract_controller(node_address, yaml, prop, prop_values, 0, defs,
                           def_label, 'gpio')
        extract_cells(node_address, yaml, prop, prop_values,
                      names, 0, defs, def_label, 'gpio')
    else:
        extract_single(node_address, yaml,
                       reduced[node_address]['props'][prop], prop,
                       defs, def_label)


def extract_node_include_info(reduced, root_node_address, sub_node_address,
                              yaml, defs, structs, y_sub):
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
                            reduced, root_node_address, c, yaml, defs, structs,
                            v)
            if 'generation' in v:

                for c in node['props'].keys():
                    if c.endswith("-names"):
                        pass

                    if re.match(k + '$', c):

                        if 'pinctrl-' in c:
                            names = node['props'].get('pinctrl-names', [])
                        else:
                            names = node['props'].get(c[:-1] + '-names', [])
                            if not names:
                                names = node['props'].get(c + '-names', [])

                        if not isinstance(names, list):
                            names = [names]

                        extract_property(
                            node_compat, yaml, sub_node_address, c, v, names,
                            defs, label_override)


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
    :param node:
    :return: node
    """

    if 'node_type' not in node:
        node['node_type'] = []
    if 'inherits' in node:
        if isinstance(node['inherits'], list):
            inherits_list  = node['inherits']
        else:
            inherits_list  = [node['inherits'],]
        node.pop('inherits')
        for inherits in inherits_list:
            if 'id' in inherits:
                node['node_type'].append(inherits['id'])
                inherits.pop('id')
            # title, description, version of inherited node are overwritten
            # by intention. Remove to prevent dct_merge to complain about
            # duplicates.
            if 'title' in inherits and 'title' in node:
                inherits.pop('title')
            if 'version' in inherits and 'version' in node:
                inherits.pop('version')
            if 'description' in inherits and 'description' in node:
                inherits.pop('description')

            if 'inherits' in inherits:
                inherits = yaml_traverse_inherited(inherits)
                if 'node_type' in inherits:
                    node['node_type'].extend(inherits['node_type'])
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


def output_keyvalue_lines(fd, defs):
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

def generate_keyvalue_file(defs, kv_file):
    with open(kv_file, "w") as fd:
        output_keyvalue_lines(fd, defs)


def output_include_lines(fd, defs, fixups):
    compatible = reduced['/']['props']['compatible'][0]

    fd.write("/**************************************************\n")
    fd.write(" * Generated include file for " + compatible)
    fd.write("\n")
    fd.write(" *               DO NOT MODIFY\n")
    fd.write(" */\n")
    fd.write("\n")
    fd.write("#ifndef _DEVICE_TREE_BOARD_H" + "\n")
    fd.write("#define _DEVICE_TREE_BOARD_H" + "\n")
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


def generate_include_file(defs, inc_file, fixups):
    with open(inc_file, "w") as fd:
        output_include_lines(fd, defs, fixups)


def load_and_parse_dts(dts_file):
    with open(dts_file, "r") as fd:
        dts = parse_file(fd)

    return dts


def load_yaml_descriptions(dts, yaml_dir):
    compatibles = get_all_compatibles(dts['/'], '/', {})
    # find unique set of compatibles across all active nodes
    s = set()
    for k, v in compatibles.items():
        if isinstance(v, list):
            for item in v:
                s.add(item)
        else:
            s.add(v)

    # scan YAML files and find the ones we are interested in
    yaml_files = []
    for root, dirnames, filenames in os.walk(yaml_dir):
        for filename in fnmatch.filter(filenames, '*.yaml'):
            yaml_files.append(os.path.join(root, filename))

    yaml_list = {}
    file_load_list = set()
    for file in yaml_files:
        for line in open(file, 'r'):
            if re.search('^\s+constraint:*', line):
                c = line.split(':')[1].strip()
                c = c.strip('"')
                if c in s:
                    if file not in file_load_list:
                        file_load_list.add(file)
                        with open(file, 'r') as yf:
                            yaml_list[c] = yaml.load(yf, Loader)

    if yaml_list == {}:
        raise Exception("Missing YAML information.  Check YAML sources")

    # collapse the yaml inherited information
    yaml_list = yaml_collapse(yaml_list)

    return yaml_list


def lookup_defs(defs, node, key):
    if node not in defs:
        return None

    if key in defs[node]['aliases']:
        key = defs[node]['aliases'][key]

    return defs[node].get(key, None)


def generate_node_definitions(yaml_list):
    defs = {}
    structs = {}

    for k, v in reduced.items():
        node_compat = get_compat(k)
        if node_compat is not None and node_compat in yaml_list:
            extract_node_include_info(
                reduced, k, k, yaml_list, defs, structs, None)

    if defs == {}:
        raise Exception("No information parsed from dts file.")

    for k, v in regs_config.items():
        if k in chosen:
            extract_reg_prop(chosen[k], None, defs, v, 1024, None)

    for k, v in name_config.items():
        if k in chosen:
            extract_string_prop(chosen[k], None, "label", v, defs)

    # This should go away via future DTDirective class
    if 'zephyr,flash' in chosen:
        load_defs = {}
        node_addr = chosen['zephyr,flash']
        flash_keys = ["label", "write-block-size", "erase-block-size"]

        for key in flash_keys:
            if key in reduced[node_addr]['props']:
                prop = reduced[node_addr]['props'][key]
                extract_single(node_addr, None, prop, key, defs, "FLASH")

        # only compute the load offset if a code partition exists and
        # it is not the same as the flash base address
        if 'zephyr,code-partition' in chosen and \
           reduced[chosen['zephyr,flash']] is not \
           reduced[chosen['zephyr,code-partition']]:
            part_defs = {}
            extract_reg_prop(chosen['zephyr,code-partition'], None,
                             part_defs, "PARTITION", 1, 'offset')
            part_base = lookup_defs(part_defs,
                                    chosen['zephyr,code-partition'],
                                    'PARTITION_OFFSET')
            load_defs['CONFIG_FLASH_LOAD_OFFSET'] = part_base
            load_defs['CONFIG_FLASH_LOAD_SIZE'] = \
                lookup_defs(part_defs,
                            chosen['zephyr,code-partition'],
                            'PARTITION_SIZE')
        else:
            load_defs['CONFIG_FLASH_LOAD_OFFSET'] = 0
            load_defs['CONFIG_FLASH_LOAD_SIZE'] = 0
    else:
        # We will add addr/size of 0 for systems with no flash controller
        # This is what they already do in the Kconfig options anyway
        defs['dummy-flash'] = {
            'CONFIG_FLASH_BASE_ADDRESS': 0,
            'CONFIG_FLASH_SIZE': 0
        }

    if 'zephyr,flash' in chosen:
        insert_defs(chosen['zephyr,flash'], defs, load_defs, {})

    return defs


def parse_arguments():
    rdh = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=rdh)

    parser.add_argument("-d", "--dts", nargs=1, required=True, help="DTS file")
    parser.add_argument("-y", "--yaml", nargs=1, required=True,
                        help="YAML file")
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

    yaml_list = load_yaml_descriptions(dts, args.yaml[0])

    defs = generate_node_definitions(yaml_list)

     # generate config and include file
    generate_keyvalue_file(defs, args.keyvalue[0])

    generate_include_file(defs, args.include[0], args.fixup)


if __name__ == '__main__':
    main()
