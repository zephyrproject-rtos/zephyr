#!/usr/bin/env python3
#
# Copyright (c) 2023 Bjarki Arge Andreasen
#
# SPDX-License-Identifier: Apache-2.0

'''
Script to generate a header file containing kernel struct devices and
their properties.

This script uses an early build of the kernel, which builds all device
drivers, to find built struct devices and their properties from sections
in the elf file and properties from the devicetree, with the help of the
elf_parser.py script.

The output is then used by the application and subsystems to get pointers
to and information about the existing struct devices at compile time.

struct devices are sorted by their class, and then sorted within their
class by either their unit address They are then given an index based on
their position within their class. This results in consistent indexes of struct
devices for a board across builds.

Note that applications and subsystems should not rely on the index of the
struct devices.
'''

import sys
import argparse
import os
import pickle
import textwrap

from elf_parser import ZephyrElf

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..',
                                'dts', 'python-devicetree', 'src'))

DEVICE_CLASS_PREFIX = 'DC'

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument('-k', '--kernel', required=True,
                        help='Input zephyr ELF binary')
    parser.add_argument('-o', '--output-header', required=True,
                        help='Output header file')
    parser.add_argument('-z', '--zephyr-base',
                        help='Path to current Zephyr base. If this argument \
                        is not provided the environment will be checked for \
                        the ZEPHYR_BASE environment variable.')
    parser.add_argument('-s', '--start-symbol', required=True,
                        help='Symbol name of the section which contains the \
                        devices. The symbol name must point to the first \
                        device in that section.')

    args = parser.parse_args()

    ZEPHYR_BASE = args.zephyr_base or os.getenv('ZEPHYR_BASE')

    if ZEPHYR_BASE is None:
        sys.exit('-z / --zephyr-base not provided. Please provide '
                 '--zephyr-base or set ZEPHYR_BASE in environment')

    sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts/dts'))

def devices_from_parsed_elf(parsed_elf):
    devices = []

    for device in parsed_elf.devices:
        if not device.api:
            continue

        api_section = parsed_elf.find_section_by_address(device.api)

        if not api_section or not api_section.name.endswith('_driver_api_area'):
            print(f'struct device {device.sym.name} could not be classified')
            continue

        parsed_device = {
            'symbol name': device.sym.name,
            'class': api_section.name[:-16],
            'node id': 'DT_' + device.edt_node.z_path_id if device.edt_node else None,
            'node unit address': device.edt_node.unit_addr if device.edt_node else None,
        }

        devices.append(parsed_device)

    return devices

def sort_devices_by_class(devices):
    devices_by_class = {}

    for device in devices:
        if device['class'] not in devices_by_class:
            devices_by_class[device['class']] = [device]
        else:
            devices_by_class[device['class']].append(device)

    devices_by_class = dict(sorted(devices_by_class.items()))

    def sort_key(d):
        return d['node unit address'] if d['node unit address'] is not None else 0xFFFFFFFF

    for key in devices_by_class:
        devices_by_class[key] = sorted(devices_by_class[key], key=sort_key)

    sorted_devices = [item for sublist in devices_by_class.values() for item in sublist]

    return sorted_devices, devices_by_class

def enumerate_devices_by_class(devices_by_class):
    global DEVICE_CLASS_PREFIX

    for device_class in devices_by_class.values():
        for i, device in enumerate(device_class):
            device['index'] = i
            device['device enum'] = '_'.join([DEVICE_CLASS_PREFIX, 'E', device['class'], str(device['index'])])

def write_devices_by_class(f, devices_by_class: dict):
    for device_class, devices in devices_by_class.items():
        f.write(textwrap.dedent(f'''
            /*
             * {device_class.capitalize()} devices
             */
        '''))

        for device in devices:
            f.write(textwrap.dedent(f'''
                /* {device["class"]}_{device["index"]} */
                #define {device["device enum"]}_SYM {device["symbol name"]}
                #define {device["device enum"]}_CLASS {device["class"]}
                #define {device["device enum"]}_INDEX {device["index"]}
                #define {device["device enum"]}_DT_NODE {device["node id"]}
            '''))

def write_edt_reverse_links(f, devices: dict):
    global DEVICE_CLASS_PREFIX

    f.write(textwrap.dedent(f'''
        /*
         * Links from devicetree nodes to device enumerations
         */
    '''))

    for device in devices:
        if 'node id' not in device:
            continue

        f.write(f'#define {DEVICE_CLASS_PREFIX}_E_DT_RL_{device["node id"]} {device["device enum"]}\n')

def write_number_of_devices_by_class(f, devices_by_class: dict):
    global DEVICE_CLASS_PREFIX

    f.write(textwrap.dedent(f'''
        /*
         * Length of device enumerations by class
         */
    '''))

    for device_class, devices in devices_by_class.items():
        f.write(f'#define {DEVICE_CLASS_PREFIX}_E_CNT_{device_class} {len(devices)}\n')

def write_foreach_device_by_class(f, devices_by_class: dict):
    global DEVICE_CLASS_PREFIX

    f.write(textwrap.dedent(f'''
        /*
         * For each device within specified device class
         */
    '''))

    for device_class, devices in devices_by_class.items():
        fns = " ".join([f"fn({d['device enum']})" for d in devices])
        f.write(f'#define {DEVICE_CLASS_PREFIX}_E_FOREACH_{device_class}(fn) {fns}\n')

def write_foreach_device(f, sorted_devices: list):
    global DEVICE_CLASS_PREFIX

    f.write(textwrap.dedent(f'''
        /*
         * For each device regardless of device class
         */
    '''))

    fns = " ".join([f"fn({d['device enum']})" for d in sorted_devices])
    f.write(f'#define {DEVICE_CLASS_PREFIX}_E_FOREACH(fn) {fns}\n')

def main():
    parse_args()

    edtser = os.path.join(os.path.split(args.kernel)[0], "edt.pickle")

    with open(edtser, 'rb') as f:
        edt = pickle.load(f)

    parsed_elf = ZephyrElf(args.kernel, edt, args.start_symbol)

    devices = devices_from_parsed_elf(parsed_elf)

    sorted_devices, devices_by_class = sort_devices_by_class(devices)

    enumerate_devices_by_class(devices_by_class)

    with open(args.output_header, 'w') as f:
        write_devices_by_class(f, devices_by_class)

        write_edt_reverse_links(f, sorted_devices)

        write_number_of_devices_by_class(f, devices_by_class)

        write_foreach_device_by_class(f, devices_by_class)

        write_foreach_device(f, sorted_devices)

if __name__ == '__main__':
    main()
