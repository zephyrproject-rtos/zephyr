# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
from collections import defaultdict, namedtuple
from itertools import chain
import sys

import edtlib

NRF_COMPATS_WITH_PINS = [
    'nordic,nrf-i2s',
    'nordic,nrf-pdm',
    'nordic,nrf-pwm',
    'nordic,nrf-qspi',
    'nordic,nrf-spi',
    'nordic,nrf-spim',
    'nordic,nrf-spis',
    'nordic,nrf-twi',
    'nordic,nrf-twim',
    'nordic,nrf-twis',
    'nordic,nrf-uart',
    'nordic,nrf-uarte',
]

COMPATS_WITH_GPIO_PHAS_IN_PROPS = {
    'nordic,nrf-spi': ['cs-gpios'],
    'nordic,nrf-spim': ['cs-gpios'],
    'nordic,nrf-spis': ['cs-gpios'],
}

COMPATS_WITH_GPIO_PHAS_IN_SUBNODES = [
    'gpio-keys',
    'gpio-leds',
]

PinUser = namedtuple('PinUser', 'node prop')

def main():
    args = parse_args()
    try:
        edt = get_edt(args)
    except edtlib.EDTError as e:
        sys.exit(f"Can't load {args.dts}: {e}")

    print_nrf_pin_mux(edt)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dts', required=True, help='path to zephyr.dts')
    parser.add_argument('--bindings-dirs', nargs='+', required=True,
                        help='DTS bindings root')
    parser.add_argument('-O', dest='outfile',
                        help='output file, default is standard output')

    return parser.parse_args()

def get_edt(args):
    # Load EDT object.

    return edtlib.EDT(args.dts, args.bindings_dirs,
                      warn_reg_unit_address_mismatch=False)

def print_nrf_pin_mux(edt):
    # Print a summary of pin allocations from a devicetree. This might
    # miss some users, but any user it does print should be accurate.

    def pprint_node(pin_user):
        node = pin_user.node
        if len(node.labels) >= 1:
            return f'{node.labels[0]} ({node.path})'
        else:
            return node.path

    pin_map = get_pin_map(edt)

    all_users = list(chain(*pin_map.values()))
    if not all_users:
        print('No pins used.')
        return

    # user -> pretty-print strings for that user's node and path
    user_pprint = {user: (pprint_node(user), user.prop.name)
                   for user in all_users}

    port_pin_width = len('P0.00')
    node_width = max(len(up[0]) for up in user_pprint.values())
    prop_width = max(len(up[1]) for up in user_pprint.values())
    if len('Property') > prop_width:
        prop_width = len('Property')

    def span_string(character):
        return (character * port_pin_width + ' ' +
                character * node_width + ' ' +
                character * prop_width)

    table_head_foot = span_string('=')
    pin_used_twice = span_string('-')

    print(table_head_foot)
    print(' '.join(['Pin'.ljust(port_pin_width),
                    'User'.ljust(node_width),
                    'Property'.ljust(prop_width)]))
    print(table_head_foot)
    for port_pin, users in sorted(pin_map.items(), key=sort_key):
        if len(users) > 1:
            print(pin_used_twice)

        port, pin = port_pin
        print(f'P{port}.{pin:02} ', end='')
        for i, user in enumerate(users):
            if i > 0:
                print(' ' * (port_pin_width + 1), end='')
            node, prop = user_pprint[user]
            print(f'{node.ljust(node_width)} {prop.ljust(prop_width)}')

        if len(users) > 1:
            print(pin_used_twice)
    print(table_head_foot)

def sort_key(port_pin_user):
    return port_pin_user[0]

def get_pin_map(edt):
    # Find nodes in the devicetree that are using SoC pins.
    # Returns a map: (port, pin) -> [potential users]

    pin_map = defaultdict(list)

    for node in edt.nodes:
        if node.matching_compat in NRF_COMPATS_WITH_PINS:
            add_nrf_pins(edt, node, pin_map)
        if node.matching_compat in COMPATS_WITH_GPIO_PHAS_IN_PROPS:
            add_gpio_phas_in_props(edt, node, pin_map)
        if node.matching_compat in COMPATS_WITH_GPIO_PHAS_IN_SUBNODES:
            add_gpio_phas_in_children(edt, node, pin_map)

    return pin_map

def add_nrf_pins(edt, node, pin_map):
    # Handle a nordic IP block node in the devicetree, adding any pins
    # it claims to pin_map. This is based on common patterns in
    # nordic,nrf-xyz bindings.

    if not node.enabled:
        return

    for name, prop in node.props.items():
        if '-pin' not in name or prop.type != 'int':
            continue
        port_pin = pin_to_port_bit(prop.val)
        pin_map[port_pin].append(PinUser(node, prop))

def add_gpio_phas_in_props(edt, node, pin_map):
    # Handle a node which contains a GPIO phandle-array in the
    # devicetree, adding any pins it claims to pin_map. This uses some
    # heuristics based on node labels.

    for prop_name in COMPATS_WITH_GPIO_PHAS_IN_PROPS[node.matching_compat]:
        if prop_name not in node.props:
            continue
        prop = node.props[prop_name]
        if 'gpio' not in prop_name or prop.type != 'phandle-array':
            continue
        for entry in prop.val:
            controller = entry.controller
            if controller.matching_compat != 'nordic,nrf-gpio':
                continue
            port_pin = pha_entry_to_port_bit(entry)
            pin_map[port_pin].append(PinUser(node, prop))

def add_gpio_phas_in_children(edt, node, pin_map):
    # Handle a node which contains a GPIO phandle-array in the
    # devicetree, adding any pins it claims to pin_map. This uses some
    # heuristics based on node labels.

    for child in node.children.values():
        for prop_name, prop in child.props.items():
            if 'gpio' not in prop_name or prop.type != 'phandle-array':
                continue
            for entry in prop.val:
                controller = entry.controller
                if controller.matching_compat != 'nordic,nrf-gpio':
                    continue
                port_pin = pha_entry_to_port_bit(entry)
                pin_map[port_pin].append(PinUser(child, prop))

def pin_to_port_bit(pin):
    # Converts a plain 'pin' number to a (port, pin) tuple.

    return (pin >> 5, pin & 0x1F)

def pha_entry_to_port_bit(entry):
    # Converts a phandle-array entry to a (port, pin) tuple.

    ctlr = entry.controller

    if 'gpio0' in ctlr.labels:
        port = 0
    elif 'gpio1' in ctlr.labels:
        port = 1
    else:
        port = None

    for cell, val in entry.data.items():
        if cell == 'pin':
            pin = val
            break
    else:
        pin = None

    return (port if port is not None else '<ERROR>',
            pin if pin is not None else '<ERROR>')

if __name__ == '__main__':
    main()
