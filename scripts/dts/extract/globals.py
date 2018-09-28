#
# Copyright (c) 2017 Linaro
# Copyright (c) 2017 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from collections import defaultdict

# globals
phandles = {}
aliases = defaultdict(list)
chosen = {}
reduced = {}
defs = {}
structs = {}

regs_config = {
    'zephyr,flash' : 'CONFIG_FLASH',
    'zephyr,sram'  : 'CONFIG_SRAM',
    'zephyr,ccm'   : 'CONFIG_CCM'
}

name_config = {
    'zephyr,console'     : 'CONFIG_UART_CONSOLE_ON_DEV_NAME',
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
        if key.startswith('CONFIG_DT_COMPAT_'):
            node_address = 'Compatibles'

    if node_address in defs:
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
    if 'props' in nodes:
        status = nodes['props'].get('status')

        if status == "disabled":
            return

    if isinstance(nodes, dict):
        reduced[path] = dict(nodes)
        reduced[path].pop('children', None)
        if path != '/':
            path += '/'
        if nodes['children']:
            for k, v in nodes['children'].items():
                get_reduced(v, path + k)


def get_node_label(node_compat, node_address):
    def_label = convert_string_to_label(node_compat)
    if '@' in node_address:
        def_label += '_' + \
                convert_string_to_label(node_address.split('@')[-1])
    else:
        def_label += convert_string_to_label(node_address)

    return def_label

def find_parent_prop(node_address, prop):
    parent_address = ''

    for comp in node_address.split('/')[1:-1]:
        parent_address += '/' + comp

    if prop in reduced[parent_address]['props']:
        parent_prop = reduced[parent_address]['props'].get(prop)
    else:
        raise Exception("Parent of node " + node_address +
                        " has no " + prop + " property")

    return parent_prop
