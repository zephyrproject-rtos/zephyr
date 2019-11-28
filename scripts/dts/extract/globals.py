#
# Copyright (c) 2017 Linaro
# Copyright (c) 2017 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# NOTE: This file is part of the old device tree scripts, which will be removed
# later. They are kept to generate some legacy #defines via the
# --deprecated-only flag.
#
# The new scripts are gen_defines.py, edtlib.py, and dtlib.py.

import sys

from collections import defaultdict

# globals
phandles = {}
aliases = defaultdict(list)
chosen = {}
reduced = {}
defs = {}
bindings = {}
bus_bindings = {}
binding_compats = []
deprecated = []
deprecated_main = []
old_alias_names = False

regs_config = {
    'zephyr,sram'  : 'DT_SRAM',
    'zephyr,ccm'   : 'DT_CCM',
    'zephyr,dtcm'  : 'DT_DTCM'
}

name_config = {
    'zephyr,console'     : 'DT_UART_CONSOLE_ON_DEV_NAME',
    'zephyr,shell-uart'  : 'DT_UART_SHELL_ON_DEV_NAME',
    'zephyr,bt-uart'     : 'DT_BT_UART_ON_DEV_NAME',
    'zephyr,bt-c2h-uart' : 'DT_BT_C2H_UART_ON_DEV_NAME',
    'zephyr,uart-pipe'   : 'DT_UART_PIPE_ON_DEV_NAME',
    'zephyr,bt-mon-uart' : 'DT_BT_MONITOR_ON_DEV_NAME',
    'zephyr,uart-mcumgr' : 'DT_UART_MCUMGR_ON_DEV_NAME'
}


def str_to_label(s):
    # Change ,-@/ to _ and uppercase
    return s.replace('-', '_') \
            .replace(',', '_') \
            .replace('@', '_') \
            .replace('/', '_') \
            .replace('.', '_') \
            .replace('+', 'PLUS') \
            .upper()


def all_compats(node):
    # Returns a set() of all 'compatible' strings that appear at or below
    # 'node', skipping disabled nodes

    if node['props'].get('status') == 'disabled':
        return set()

    compats = set()

    if 'compatible' in node['props']:
        val = node['props']['compatible']
        if isinstance(val, list):
            compats.update(val)
        else:
            compats.add(val)

    for child_node in node['children'].values():
        compats.update(all_compats(child_node))

    return compats


def create_aliases(root):
    if 'aliases' in root['children']:
        for name, node_path in root['children']['aliases']['props'].items():
            aliases[node_path].append(name)

def get_compat(node_path):
    # Returns the value of the 'compatible' property for the node at
    # 'node_path'. Also checks the node's parent.
    #
    # Returns None if neither the node nor its parent has a 'compatible'
    # property.

    compat = reduced[node_path]['props'].get('compatible') or \
             reduced[get_parent_path(node_path)]['props'].get('compatible')

    if isinstance(compat, list):
        return compat[0]

    return compat


def create_chosen(root):
    if 'chosen' in root['children']:
        chosen.update(root['children']['chosen']['props'])


def create_phandles(root, name):
    if root['props'].get('status') == 'disabled':
        return

    if 'phandle' in root['props']:
        phandles[root['props']['phandle']] = name

    if name != '/':
        name += '/'

    for child_name, child_node in root['children'].items():
        create_phandles(child_node, name + child_name)


def insert_defs(node_path, new_defs, new_aliases):
    for key in new_defs:
        if key.startswith('DT_COMPAT_'):
            node_path = 'compatibles'

    remove = [k for k in new_aliases if k in new_defs]
    for k in remove: del new_aliases[k]

    if node_path in defs:
        remove = [k for k in new_aliases if k in defs[node_path]]
        for k in remove: del new_aliases[k]
        defs[node_path]['aliases'].update(new_aliases)
        defs[node_path].update(new_defs)
    else:
        new_defs['aliases'] = new_aliases
        defs[node_path] = new_defs


# Dictionary where all keys default to 0. Used by create_reduced().
last_used_id = defaultdict(int)


def create_reduced(node, path):
    # Compress nodes list to nodes w/ paths, add interrupt parent

    if node['props'].get('status') == 'disabled':
        return

    reduced[path] = node.copy()
    reduced[path].pop('children', None)

    # Assign an instance ID for each compat
    compat = node['props'].get('compatible')
    if compat:
        if not isinstance(compat, list):
            compat = [compat]

        reduced[path]['instance_id'] = {}
        for comp in compat:
            reduced[path]['instance_id'][comp] = last_used_id[comp]
            last_used_id[comp] += 1

    # Flatten 'prop = <1 2>, <3 4>' (which turns into nested lists) to
    # 'prop = <1 2 3 4>'
    for val in node['props'].values():
        if isinstance(val, list) and isinstance(val[0], list):
            # In-place modification
            val[:] = [item for sublist in val for item in sublist]

    if node['children']:
        if path != '/':
            path += '/'

        for child_name, child_node in sorted(node['children'].items()):
            create_reduced(child_node, path + child_name)


def node_label(node_path):
    node_compat = get_compat(node_path)
    def_label = str_to_label(node_compat)
    if '@' in node_path:
        # See if we have number we can convert
        try:
            unit_addr = int(node_path.split('@')[-1], 16)
            (nr_addr_cells, nr_size_cells) = get_addr_size_cells(node_path)
            unit_addr += translate_addr(unit_addr, node_path,
                         nr_addr_cells, nr_size_cells)
            unit_addr = "%x" % unit_addr
        except Exception:
            unit_addr = node_path.split('@')[-1]
        def_label += '_' + str_to_label(unit_addr)
    else:
        def_label += '_' + str_to_label(node_path.split('/')[-1])
    return def_label


def get_parent_path(node_path):
    # Turns /foo/bar into /foo. Returns None for /.

    if node_path == '/':
        return None

    return '/'.join(node_path.split('/')[:-1]) or '/'


def find_parent_prop(node_path, prop):
    parent_path = get_parent_path(node_path)

    if prop not in reduced[parent_path]['props']:
        raise Exception("Parent of node " + node_path +
                        " has no " + prop + " property")

    return reduced[parent_path]['props'][prop]


# Get the #{address,size}-cells for a given node
def get_addr_size_cells(node_path):
    parent_addr = get_parent_path(node_path)

    # The DT spec says that if #address-cells is missing default to 2
    # if #size-cells is missing default to 1
    nr_addr = reduced[parent_addr]['props'].get('#address-cells', 2)
    nr_size = reduced[parent_addr]['props'].get('#size-cells', 1)

    return (nr_addr, nr_size)

def translate_addr(addr, node_path, nr_addr_cells, nr_size_cells):
    parent_path = get_parent_path(node_path)

    ranges = reduced[parent_path]['props'].get('ranges')
    if not ranges:
        return 0

    if isinstance(ranges, list):
        ranges = ranges.copy()  # Modified in-place below
    else:
        # Empty value ('ranges;'), meaning the parent and child address spaces
        # are the same
        ranges = []

    nr_p_addr_cells, nr_p_size_cells = get_addr_size_cells(parent_path)

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
            parent_path, nr_p_addr_cells, nr_p_size_cells)
    range_offset += parent_range_offset

    return range_offset

def enable_old_alias_names(enable):
    global old_alias_names
    old_alias_names = enable

def add_compat_alias(node_path, label_postfix, label, prop_aliases, deprecate=False):
    if 'instance_id' in reduced[node_path]:
        instance = reduced[node_path]['instance_id']
        for k in instance:
            i = instance[k]
            b = 'DT_' + str_to_label(k) + '_' + str(i) + '_' + label_postfix
            deprecated.append(b)
            prop_aliases[b] = label
            b = "DT_INST_{}_{}_{}".format(str(i), str_to_label(k), label_postfix)
            prop_aliases[b] = label
            if deprecate:
                deprecated.append(b)

def add_prop_aliases(node_path,
                     alias_label_function, prop_label, prop_aliases, deprecate=False):
    node_compat = get_compat(node_path)
    new_alias_prefix = 'DT_'

    for alias in aliases[node_path]:
        old_alias_label = alias_label_function(alias)
        new_alias_label = new_alias_prefix + 'ALIAS_' + old_alias_label
        new_alias_compat_label = new_alias_prefix + str_to_label(node_compat) + '_' + old_alias_label

        if new_alias_label != prop_label:
            prop_aliases[new_alias_label] = prop_label
            if deprecate:
                deprecated.append(new_alias_label)
        if new_alias_compat_label != prop_label:
            prop_aliases[new_alias_compat_label] = prop_label
            if deprecate:
                deprecated.append(new_alias_compat_label)
        if old_alias_names and old_alias_label != prop_label:
            prop_aliases[old_alias_label] = prop_label

def get_binding(node_path):
    compat = reduced[node_path]['props'].get('compatible')
    if isinstance(compat, list):
        compat = compat[0]

    # Support two levels of recursive 'child-binding:'. The new scripts support
    # any number of levels, but it gets a bit tricky to implement here, because
    # nodes don't store their bindings.

    parent_path = get_parent_path(node_path)
    pparent_path = get_parent_path(parent_path)

    parent_compat = get_compat(parent_path)
    pparent_compat = get_compat(pparent_path) if pparent_path else None

    if parent_compat in bindings or pparent_compat in bindings:
        if compat is None:
            # The node doesn't get a binding from 'compatible'. See if it gets
            # one via 'sub-node' or 'child-binding'.

            parent_binding = bindings.get(parent_compat)
            if parent_binding:
                for sub_key in 'sub-node', 'child-binding':
                    if sub_key in parent_binding:
                        return parent_binding[sub_key]

            # Look for 'child-binding: child-binding: ...' in grandparent node

            pparent_binding = bindings.get(pparent_compat)
            if pparent_binding and 'child-binding' in pparent_binding:
                pp_child_binding = pparent_binding['child-binding']
                if 'child-binding' in pp_child_binding:
                    return pp_child_binding['child-binding']

        # look for a bus-specific binding

        parent_binding = bindings.get(parent_compat)
        if parent_binding:
            if 'child-bus' in parent_binding:
                bus = parent_binding['child-bus']
                return bus_bindings[bus][compat]

            if 'child' in parent_binding and 'bus' in parent_binding['child']:
                bus = parent_binding['child']['bus']
                return bus_bindings[bus][compat]

    # No bus-specific binding found, look in the main dict.
    if compat:
        return bindings[compat]
    return None

def get_binding_compats():
    return binding_compats

def build_cell_array(prop_array):
    index = 0
    ret_array = []

    if isinstance(prop_array, int):
        # Work around old code generating an integer for e.g.
        # 'pwms = <&foo>'
        prop_array = [prop_array]

    while index < len(prop_array):
        handle = prop_array[index]

        if handle in {0, -1}:
            ret_array.append([])
            index += 1
        else:
            # get controller node (referenced via phandle)
            cell_parent = phandles[handle]

            for prop in reduced[cell_parent]['props']:
                if prop[0] == '#' and '-cells' in prop:
                    num_cells = reduced[cell_parent]['props'][prop]
                    break

            ret_array.append(prop_array[index:index+num_cells+1])

            index += num_cells + 1

    return ret_array

def child_to_parent_unmap(cell_parent, gpio_index):
    # This function returns a (gpio-controller, pin number) tuple from
    # cell_parent (identified as a 'nexus node', ie: has a 'gpio-map'
    # property) and gpio_index.
    # Note: Nexus nodes and gpio-map property are described in the
    # upcoming (presumably v0.3) Device Tree specification, chapter
    # 'Nexus nodes and Specifier Mapping'.

    # First, retrieve gpio-map as a list
    gpio_map = reduced[cell_parent]['props']['gpio-map']

    # Before parsing, we need to know 'gpio-map' row size
    # gpio-map raws are encoded as follows:
    # [child specifier][gpio controller phandle][parent specifier]

    # child specifier field length is connector property #gpio-cells
    child_specifier_size = reduced[cell_parent]['props']['#gpio-cells']

    # parent specifier field length is parent property #gpio-cells
    # Assumption 1: We assume parent #gpio-cells is constant across
    # the map, so we take the value of the first occurrence and apply
    # to the whole map.
    parent = phandles[gpio_map[child_specifier_size]]
    parent_specifier_size = reduced[parent]['props']['#gpio-cells']

    array_cell_size = child_specifier_size + 1 + parent_specifier_size

    # Now that the length of each entry in 'gpio-map' is known,
    # look for a match with gpio_index
    for i in range(0, len(gpio_map), array_cell_size):
        entry = gpio_map[i:i+array_cell_size]

        if entry[0] == gpio_index:
            parent_controller_phandle = entry[child_specifier_size]
            # Assumption 2: We assume optional properties 'gpio-map-mask'
            # and 'gpio-map-pass-thru' are not specified.
            # So, for now, only the pin number (first value of the parent
            # specifier field) should be returned.
            parent_pin_number = entry[child_specifier_size+1]

            # Return gpio_controller and specifier pin
            return phandles[parent_controller_phandle], parent_pin_number

    # gpio_index did not match any entry in the gpio-map
    return None, None


def extract_controller(node_path, prop, prop_values, index,
                       def_label, generic, handle_single=False,
                       deprecate=False):

    prop_def = {}
    prop_alias = {}

    prop_array = build_cell_array(prop_values)
    if handle_single:
        prop_array = [prop_array[index]]

    for i, elem in enumerate(prop_array):
        num_cells = len(elem)

        # if the entry is empty, skip
        if num_cells == 0:
            continue

        cell_parent = phandles[elem[0]]

        if 'gpio-map' in reduced[cell_parent]['props']:
            # Parent is a gpio 'nexus node' (ie has gpio-map).
            # Controller should be found in the map, using elem[1] as index.
            # Pin attribues (number, flag) will not be used in this function
            cell_parent, _ = child_to_parent_unmap(cell_parent, elem[1])
            if cell_parent is None:
                raise Exception("No parent matching child specifier")

        l_cell = reduced[cell_parent]['props'].get('label')

        if l_cell is None:
            continue

        l_base = [def_label]

        # Check is defined should be indexed (_0, _1)
        if handle_single or i == 0 and len(prop_array) == 1:
            # 0 or 1 element in prop_values
            l_idx = []
        else:
            l_idx = [str(i)]

        l_cellname = str_to_label(generic + '_' + 'controller')

        label = l_base + [l_cellname] + l_idx

        add_compat_alias(node_path, '_'.join(label[1:]), '_'.join(label), prop_alias, deprecate)
        prop_def['_'.join(label)] = "\"" + l_cell + "\""

        #generate defs also if node is referenced as an alias in dts
        if node_path in aliases:
            add_prop_aliases(
                node_path,
                lambda alias: '_'.join([str_to_label(alias)] + label[1:]),
                '_'.join(label),
                prop_alias, deprecate)

        insert_defs(node_path, prop_def, prop_alias)

        if deprecate:
            deprecated_main.extend(list(prop_def.keys()))


def extract_cells(node_path, prop, prop_values, names, index,
                  def_label, generic, handle_single=False,
                  deprecate=False):

    prop_array = build_cell_array(prop_values)
    if handle_single:
        prop_array = [prop_array[index]]

    for i, elem in enumerate(prop_array):
        num_cells = len(elem)

        # if the entry is empty, skip
        if num_cells == 0:
            continue

        cell_parent = phandles[elem[0]]

        if 'gpio-map' in reduced[cell_parent]['props']:
            # Parent is a gpio connector ie 'nexus node', ie has gpio-map).
            # Controller and pin number should be found in the connector map,
            # using elem[1] as index.
            # Parent pin flag is not used, so child flag(s) value (elem[2:])
            # is kept as is.
            cell_parent, elem[1] = child_to_parent_unmap(cell_parent, elem[1])
            if cell_parent is None:
                raise Exception("No parent matching child specifier")

        try:
            cell_yaml = get_binding(cell_parent)
        except:
            raise Exception(
                "Could not find yaml description for " +
                    reduced[cell_parent]['name'])

        try:
            name = names.pop(0).upper()
        except Exception:
            name = ''

        # Get number of cells per element of current property
        for props in reduced[cell_parent]['props']:
            if props[0] == '#' and '-cells' in props:
                if props[1:] in cell_yaml:
                    cell_yaml_names = props[1:]  # #foo-cells -> foo-cells
                else:
                    cell_yaml_names = '#cells'

        l_cell = [str_to_label(str(generic))]

        l_base = [def_label]
        # Check if #define should be indexed (_0, _1, ...)
        if handle_single or i == 0 and len(prop_array) == 1:
            # Less than 2 elements in prop_values
            # Indexing is not needed
            l_idx = []
        else:
            l_idx = [str(i)]

        prop_def = {}
        prop_alias = {}

        # Generate label for each field of the property element
        for j in range(num_cells-1):
            l_cellname = [str(cell_yaml[cell_yaml_names][j]).upper()]
            if l_cell == l_cellname:
                label = l_base + l_cell + l_idx
            else:
                label = l_base + l_cell + l_cellname + l_idx
            label_name = l_base + [name] + l_cellname
            add_compat_alias(node_path, '_'.join(label[1:]), '_'.join(label), prop_alias, deprecate)
            prop_def['_'.join(label)] = elem[j+1]
            if name:
                prop_alias['_'.join(label_name)] = '_'.join(label)

            # generate defs for node aliases
            if node_path in aliases:
                add_prop_aliases(
                    node_path,
                    lambda alias: '_'.join([str_to_label(alias)] + label[1:]),
                    '_'.join(label),
                    prop_alias, deprecate)

            insert_defs(node_path, prop_def, prop_alias)

            if deprecate:
                deprecated_main.extend(list(prop_def.keys()))


def err(msg):
    # General error reporting helper. Prints a message to stderr and exits with
    # status 1.

    sys.exit("error: " + msg)
