#
# Copyright (c) 2017 Linaro
# Copyright (c) 2017 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from collections import defaultdict
from copy import deepcopy

# globals
phandles = {}
aliases = defaultdict(list)
chosen = {}
reduced = {}
defs = {}
structs = {}
bindings = {}
bus_bindings = {}
bindings_compat = []
old_alias_names = False

regs_config = {
    'zephyr,sram'  : 'CONFIG_SRAM',
    'zephyr,ccm'   : 'CONFIG_CCM'
}

name_config = {
    'zephyr,console'     : 'CONFIG_UART_CONSOLE_ON_DEV_NAME',
    'zephyr,shell-uart'  : 'CONFIG_UART_SHELL_ON_DEV_NAME',
    'zephyr,bt-uart'     : 'CONFIG_BT_UART_ON_DEV_NAME',
    'zephyr,uart-pipe'   : 'CONFIG_UART_PIPE_ON_DEV_NAME',
    'zephyr,bt-mon-uart' : 'CONFIG_BT_MONITOR_ON_DEV_NAME',
    'zephyr,uart-mcumgr' : 'CONFIG_UART_MCUMGR_ON_DEV_NAME'
}


def convert_string_to_label(s):
    # Transmute ,-@/ to _
    s = s.replace("-", "_")
    s = s.replace(",", "_")
    s = s.replace("@", "_")
    s = s.replace("/", "_")
    # Uppercase the string
    s = s.upper()
    return s


def get_all_compatibles(d, name, comp_dict):
    if 'props' in d:
        compat = d['props'].get('compatible')
        enabled = d['props'].get('status')

    if enabled == "disabled":
        return comp_dict

    if compat is not None:
        comp_dict[name] = compat

    if name != '/':
        name += '/'

    if isinstance(d, dict):
        if d['children']:
            for k, v in d['children'].items():
                get_all_compatibles(v, name + k, comp_dict)

    return comp_dict


def get_aliases(root):
    if 'children' in root:
        if 'aliases' in root['children']:
            for k, v in root['children']['aliases']['props'].items():
                aliases[v].append(k)

    # Treat alternate names as aliases
    for k in reduced.keys():
        if reduced[k].get('alt_name', None) is not None:
            aliases[k].append(reduced[k]['alt_name'])

def get_node_compats(node_address):
    compat = None

    try:
        if 'props' in reduced[node_address].keys():
            compat = reduced[node_address]['props'].get('compatible')

        if not isinstance(compat, list):
            compat = [compat, ]

    except:
        pass

    return compat

def get_compat(node_address):
    compat = None

    try:
        if 'props' in reduced[node_address].keys():
            compat = reduced[node_address]['props'].get('compatible')

        if compat == None:
            compat = find_parent_prop(node_address, 'compatible')

        if isinstance(compat, list):
            compat = compat[0]

    except:
        pass

    return compat


def get_chosen(root):
    if 'children' in root:
        if 'chosen' in root['children']:
            for k, v in root['children']['chosen']['props'].items():
                chosen[k] = v


def get_phandles(root, name, handles):
    if 'props' in root:
        handle = root['props'].get('phandle')
        enabled = root['props'].get('status')

    if enabled == "disabled":
        return

    if handle is not None:
        phandles[handle] = name

    if name != '/':
        name += '/'

    if isinstance(root, dict):
        if root['children']:
            for k, v in root['children'].items():
                get_phandles(v, name + k, handles)


def insert_defs(node_address, new_defs, new_aliases):

    for key in new_defs.keys():
        if key.startswith('DT_COMPAT_'):
            node_address = 'compatibles'

    remove = [k for k in new_aliases if k in new_defs.keys()]
    for k in remove: del new_aliases[k]

    if node_address in defs:
        remove = [k for k in new_aliases if k in defs[node_address].keys()]
        for k in remove: del new_aliases[k]
        if 'aliases' in defs[node_address]:
            defs[node_address]['aliases'].update(new_aliases)
        else:
            defs[node_address]['aliases'] = new_aliases

        defs[node_address].update(new_defs)
    else:
        new_defs['aliases'] = new_aliases
        defs[node_address] = new_defs


def find_node_by_path(nodes, path):
    d = nodes
    for k in path[1:].split('/'):
        d = d['children'][k]

    return d


def get_reduced(nodes, path):
    # compress nodes list to nodes w/ paths, add interrupt parent
    if 'last_used_id' not in get_reduced.__dict__:
        get_reduced.last_used_id = {}

    if 'props' in nodes:
        status = nodes['props'].get('status')

        if status == "disabled":
            return

    if isinstance(nodes, dict):
        reduced[path] = dict(nodes)

        # assign an instance ID for each compat
        compat = nodes['props'].get('compatible')
        if (compat is not None):
            if type(compat) is not list: compat = [ compat, ]
            reduced[path]['instance_id'] = {}
            for k in compat:
                if k not in get_reduced.last_used_id.keys():
                    get_reduced.last_used_id[k] = 0
                else:
                    get_reduced.last_used_id[k] += 1
                reduced[path]['instance_id'][k] = get_reduced.last_used_id[k]

        reduced[path].pop('children', None)
        if path != '/':
            path += '/'
        if nodes['children']:
            for k, v in sorted(nodes['children'].items()):
                get_reduced(v, path + k)


def get_node_label(node_address):
    node_compat = get_compat(node_address)
    def_label = convert_string_to_label(node_compat)
    if '@' in node_address:
        # See if we have number we can convert
        try:
            unit_addr = int(node_address.split('@')[-1], 16)
            (nr_addr_cells, nr_size_cells) = get_addr_size_cells(node_address)
            unit_addr += translate_addr(unit_addr, node_address,
                         nr_addr_cells, nr_size_cells)
            unit_addr = "%x" % unit_addr
        except:
            unit_addr = node_address.split('@')[-1]
        def_label += '_' + convert_string_to_label(unit_addr)
    else:
        def_label += '_' + \
                convert_string_to_label(node_address.split('/')[-1])
    return def_label

def get_parent_address(node_address):
    parent_address = ''

    for comp in node_address.split('/')[1:-1]:
        parent_address += '/' + comp

    return parent_address


def find_parent_prop(node_address, prop):
    parent_address = get_parent_address(node_address)

    if prop in reduced[parent_address]['props']:
        parent_prop = reduced[parent_address]['props'].get(prop)
    else:
        raise Exception("Parent of node " + node_address +
                        " has no " + prop + " property")

    return parent_prop

# Get the #{address,size}-cells for a given node
def get_addr_size_cells(node_address):
    parent_addr = get_parent_address(node_address)
    if parent_addr == '':
        parent_addr = '/'

    # The DT spec says that if #address-cells is missing default to 2
    # if #size-cells is missing default to 1
    nr_addr = reduced[parent_addr]['props'].get('#address-cells', 2)
    nr_size = reduced[parent_addr]['props'].get('#size-cells', 1)

    return (nr_addr, nr_size)

def translate_addr(addr, node_address, nr_addr_cells, nr_size_cells):

    try:
        ranges = deepcopy(find_parent_prop(node_address, 'ranges'))
        if type(ranges) is not list: ranges = [ ]
    except:
        return 0

    parent_address = get_parent_address(node_address)

    (nr_p_addr_cells, nr_p_size_cells) = get_addr_size_cells(parent_address)

    range_offset = 0
    while ranges:
        child_bus_addr = 0
        parent_bus_addr = 0
        range_len = 0
        for x in range(nr_addr_cells):
            val = ranges.pop(0) << (32 * (nr_addr_cells - x - 1))
            child_bus_addr += val
        for x in range(nr_p_addr_cells):
            val = ranges.pop(0) << (32 * (nr_p_addr_cells - x - 1))
            parent_bus_addr += val
        for x in range(nr_size_cells):
            range_len += ranges.pop(0) << (32 * (nr_size_cells - x - 1))
        # if we are outside of the range we don't need to translate
        if child_bus_addr <= addr <= (child_bus_addr + range_len):
            range_offset = parent_bus_addr - child_bus_addr
            break

    parent_range_offset = translate_addr(addr + range_offset,
            parent_address, nr_p_addr_cells, nr_p_size_cells)
    range_offset += parent_range_offset

    return range_offset

def enable_old_alias_names(enable):
    global old_alias_names
    old_alias_names = enable

def add_compat_alias(node_address, label_postfix, label, prop_aliases):
    if ('instance_id' in reduced[node_address]):
        instance = reduced[node_address]['instance_id']
        for k in instance:
            i = instance[k]
            b = 'DT_' + convert_string_to_label(k) + '_' + str(i) + '_' + label_postfix
            prop_aliases[b] = label

def add_prop_aliases(node_address,
                     alias_label_function, prop_label, prop_aliases):
    node_compat = get_compat(node_address)
    new_alias_prefix = 'DT_' + convert_string_to_label(node_compat)

    for alias in aliases[node_address]:
        old_alias_label = alias_label_function(alias)
        new_alias_label = new_alias_prefix + '_' + old_alias_label

        if (new_alias_label != prop_label):
            prop_aliases[new_alias_label] = prop_label
        if (old_alias_names and old_alias_label != prop_label):
            prop_aliases[old_alias_label] = prop_label

def get_binding(node_address):
    compat = get_compat(node_address)

    # For just look for the binding in the main dict
    # if we find it here, return it, otherwise it best
    # be in the bus specific dict
    if compat in bindings:
        return bindings[compat]

    parent_addr = get_parent_address(node_address)
    parent_compat = get_compat(parent_addr)

    parent_binding = bindings[parent_compat]

    bus = parent_binding['child']['bus']
    binding = bus_bindings[bus][compat]

    return binding

def get_binding_compats():
    return bindings_compat

def extract_controller(node_address, prop, prop_values, index, def_label, generic):

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
            generation = get_binding(node_address)['properties'
                    ][prop]['generation']
        except:
            generation = ''

        if 'use-prop-name' in generation:
            l_cellname = convert_string_to_label(prop + '_' + 'controller')
        else:
            l_cellname = convert_string_to_label(generic + '_' + 'controller')

        label = l_base + [l_cellname] + l_idx

        add_compat_alias(node_address, '_'.join(label[1:]), '_'.join(label), prop_alias)
        prop_def['_'.join(label)] = "\"" + l_cell + "\""

        #generate defs also if node is referenced as an alias in dts
        if node_address in aliases:
            add_prop_aliases(
                node_address,
                lambda alias:
                    '_'.join([convert_string_to_label(alias)] + label[1:]),
                '_'.join(label),
                prop_alias)

        insert_defs(node_address, prop_def, prop_alias)

    # prop off phandle + num_cells to get to next list item
    prop_values = prop_values[num_cells+1:]

    # recurse if we have anything left
    if len(prop_values):
        extract_controller(node_address, prop, prop_values, index + 1,
                           def_label, generic)


def extract_cells(node_address, prop, prop_values, names, index,
                  def_label, generic):

    cell_parent = phandles[prop_values.pop(0)]

    try:
        cell_yaml = get_binding(cell_parent)
    except:
        raise Exception(
            "Could not find yaml description for " +
                reduced[cell_parent]['name'])

    try:
        name = names.pop(0).upper()
    except:
        name = ''

    # Get number of cells per element of current property
    for k in reduced[cell_parent]['props'].keys():
        if k[0] == '#' and '-cells' in k:
            num_cells = reduced[cell_parent]['props'].get(k)
            if k in cell_yaml.keys():
                cell_yaml_names = k
            else:
                cell_yaml_names = '#cells'
    try:
        generation = get_binding(node_address)['properties'][prop
                ]['generation']
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
        l_cellname = [str(cell_yaml[cell_yaml_names][i]).upper()]
        if l_cell == l_cellname:
            label = l_base + l_cell + l_idx
        else:
            label = l_base + l_cell + l_cellname + l_idx
        label_name = l_base + [name] + l_cellname
        add_compat_alias(node_address, '_'.join(label[1:]), '_'.join(label), prop_alias)
        prop_def['_'.join(label)] = prop_values.pop(0)
        if len(name):
            prop_alias['_'.join(label_name)] = '_'.join(label)

        # generate defs for node aliases
        if node_address in aliases:
            add_prop_aliases(
                node_address,
                lambda alias:
                    '_'.join([convert_string_to_label(alias)] + label[1:]),
                '_'.join(label),
                prop_alias)

        insert_defs(node_address, prop_def, prop_alias)

    # recurse if we have anything left
    if len(prop_values):
        extract_cells(node_address, prop, prop_values, names,
                      index + 1, def_label, generic)



