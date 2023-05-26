#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2020 Nordic Semiconductor NA
#
# SPDX-License-Identifier: Apache-2.0
"""Translate generic handles into ones optimized for the application.

Immutable device data includes information about dependencies,
e.g. that a particular sensor is controlled through a specific I2C bus
and that it signals event on a pin on a specific GPIO controller.
This information is encoded in the first-pass binary using identifiers
derived from the devicetree.  This script extracts those identifiers
and replaces them with ones optimized for use with the devices
actually present.

For example the sensor might have a first-pass handle defined by its
devicetree ordinal 52, with the I2C driver having ordinal 24 and the
GPIO controller ordinal 14.  The runtime ordinal is the index of the
corresponding device in the static devicetree array, which might be 6,
5, and 3, respectively.

The output is a C source file that provides alternative definitions
for the array contents referenced from the immutable device objects.
In the final link these definitions supersede the ones in the
driver-specific object file.
"""

import sys
import argparse
import os
import pickle

from elf_parser import ZephyrElf

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..',
                                'dts', 'python-devicetree', 'src'))

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-k", "--kernel", required=True,
                        help="Input zephyr ELF binary")
    parser.add_argument("--dynamic-deps", action="store_true",
                        help="Indicates if device dependencies are dynamic")
    parser.add_argument("-d", "--num-dynamic-devices", required=False, default=0,
                        type=int, help="Input number of dynamic devices allowed")
    parser.add_argument("-o", "--output-source", required=True,
                        help="Output source file")
    parser.add_argument("-g", "--output-graphviz",
                        help="Output file for graphviz dependency graph")
    parser.add_argument("-z", "--zephyr-base",
                        help="Path to current Zephyr base. If this argument \
                        is not provided the environment will be checked for \
                        the ZEPHYR_BASE environment variable.")
    parser.add_argument("-s", "--start-symbol", required=True,
                        help="Symbol name of the section which contains the \
                        devices. The symbol name must point to the first \
                        device in that section.")

    args = parser.parse_args()

    ZEPHYR_BASE = args.zephyr_base or os.getenv("ZEPHYR_BASE")

    if ZEPHYR_BASE is None:
        sys.exit("-z / --zephyr-base not provided. Please provide "
                 "--zephyr-base or set ZEPHYR_BASE in environment")

    sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/dts"))

def c_handle_comment(dev, handles):
    def dev_path_str(dev):
        return dev.edt_node and dev.edt_node.path or dev.sym.name
    lines = [
        '',
        '/* {:d} : {:s}:'.format(dev.handle, (dev_path_str(dev))),
    ]
    if len(handles["depends"]) > 0:
        lines.append(' * Direct Dependencies:')
        for dep in handles["depends"]:
            lines.append(' *    - {:s}'.format(dev_path_str(dep)))
    if len(handles["injected"]) > 0:
        lines.append(' * Injected Dependencies:')
        for dep in handles["injected"]:
            lines.append(' *    - {:s}'.format(dev_path_str(dep)))
    if len(handles["supports"]) > 0:
        lines.append(' * Supported:')
        for sup in handles["supports"]:
            lines.append(' *    - {:s}'.format(dev_path_str(sup)))
    lines.append(' */')
    return lines

def c_handle_array(dev, handles, dynamic_deps, extra_support_handles=0):
    handles = [
        *[str(d.handle) for d in handles["depends"]],
        'Z_DEVICE_DEPS_SEP',
        *[str(d.handle) for d in handles["injected"]],
        'Z_DEVICE_DEPS_SEP',
        *[str(d.handle) for d in handles["supports"]],
        *(extra_support_handles * ['DEVICE_HANDLE_NULL']),
        'Z_DEVICE_DEPS_ENDS',
    ]
    ctype = (
        '{:s}Z_DECL_ALIGN(device_handle_t) '
        '__attribute__((__section__(".__device_deps_pass2")))'
    ).format('const ' if not dynamic_deps else '')
    return [
        # The `extern` line pretends this was first declared in some .h
        # file to silence "should it be static?" warnings in some
        # compilers and static analyzers.
        'extern {:s} {:s}[{:d}];'.format(ctype, dev.ordinals.sym.name, len(handles)),
        ctype,
        '{:s}[] = {{ {:s} }};'.format(dev.ordinals.sym.name, ', '.join(handles)),
    ]

def create_device_dt_list_each(devl, lines, log, log_level, fp):
    if devl.init is None:
        return
    lines.append('/* {:s} */'.format(parsed_elf.dev_path_str(devl)))
    lines.append('&{:s},'.format(devl.init))
    log.append('{:<16s}'.format(parsed_elf.dev_path_str(devl).split("/")[-1]))
    log.append('{:<16s}'.format(log_level))

    if len(devl.devs_depends_on) != 0:
        lines.append('/*parent nodes:')
        for parent_dev in devl.devs_depends_on:
            lines.append('{:s} '.format(parsed_elf.dev_path_str(parent_dev)))
            log.append('{:<16s} '.format(parsed_elf.dev_path_str(parent_dev).split("/")[-1]))
        lines.append('*/ \n')
    else :
        lines.append('/*No parent nodes*/ \n')
        log.append('{:<16s} '.format(':None'))
    log.append('\n')

def dump_dev_init_order(pre_k, post_k, app):
    with open(os.path.abspath('dts_nodes_init.log'), "w") as log_file:
        log_heading = ['\n DTS based device initializtion in accesnding order \n\n']
        log_heading.append('{:<16}'.format('Node Name:'))
        log_heading.append('{:<16}'.format('Init Level:'))
        log_heading.append('{:<16}'.format('Dep Node:'))
        log_heading.append('\n---------------------------------------------------\n')
        print(''.join(log_heading))
        log_file.write(''.join(log_heading))

        print(''.join(pre_k))
        log_file.write(''.join(pre_k))
        print(''.join(post_k))
        log_file.write(''.join(post_k))
        print(''.join(app))
        log_file.write(''.join(app))

def create_dt_init_func(fp):
    file_dr = os.path.join(sys.path[0])
    file = os.path.join(file_dr, '../build/dev_dt_init.txt')

    # Dump device init code into generated dev_handles.c
    with open(os.path.abspath(file), "r") as fpc:
        fp.write(fpc.read())

def create_device_dt_list(parsed_elf):
    def get_parent_init_level(parent_dev, level):
        for dev in parent_dev:
            if dev.init_level > level:
                level = dev.init_level
        return level

    # Create static array of device init_entry for each initializtion level
    dev_list = parsed_elf.dt_devices
    with open(args.output_source, "a") as fp:
        pre_k_lines = ['\nconst struct init_entry *__pre_k_device_dt_list[] = {']
        post_k_lines = ['const struct init_entry *__post_k_device_dt_list[] = {']
        app_lines = ['const struct init_entry *__app_device_dt_list[] = {']
        pre_k_log = []
        post_k_log = []
        app_k_log = []

        header_line = ['\n']
        for devl in dev_list:
            if devl.init is None:
                continue
            header_line.append('extern const Z_DECL_ALIGN(struct init_entry) {:s};'.format(devl.init))

        for devl in dev_list:
            if devl.init is None:
                continue

            # Create static array also based on parent initializtion level
            level = get_parent_init_level(devl.devs_depends_on, devl.init_level)
            if level == (1,) :
                create_device_dt_list_each(devl, pre_k_lines, pre_k_log, 'PRE-KERNEL', fp)
            elif level == (2,):
                create_device_dt_list_each(devl, post_k_lines, post_k_log, 'POST-KERNEL', fp)
            elif level == (3,):
                create_device_dt_list_each(devl, app_lines, app_k_log, 'APP-LEVEL', fp)

        pre_k_lines.append('};\n')
        post_k_lines.append('};\n')
        app_lines.append('};\n')

        fp.write('\n'.join(header_line))
        fp.write('\n'.join(pre_k_lines))
        fp.write('\n'.join(post_k_lines))
        fp.write('\n'.join(app_lines))

        create_dt_init_func(fp)
        dump_dev_init_order(pre_k_log, post_k_log, app_k_log)

def main():
    parse_args()
    global parsed_elf

    edtser = os.path.join(os.path.split(args.kernel)[0], "edt.pickle")
    with open(edtser, 'rb') as f:
        edt = pickle.load(f)

    parsed_elf = ZephyrElf(args.kernel, edt, args.start_symbol)
    if parsed_elf.relocatable:
        # While relocatable elf files will load cleanly, the pointers pulled from
        # the symbol table are invalid (as expected, because the structures have not
        # yet been allocated addresses). Fixing this will require iterating over
        # the relocation sections to find the symbols those pointers will end up
        # referring to.
        sys.exit('Relocatable elf files are not yet supported')

    if args.output_graphviz:
        # Try and output the dependency tree
        try:
            dot = parsed_elf.device_dependency_graph('Device dependency graph', args.kernel)
            with open(args.output_graphviz, 'w') as f:
                f.write(dot.source)
        except ImportError:
            pass

    with open(args.output_source, "w") as fp:
        fp.write('#include <zephyr/device.h>\n')
        fp.write('#include <zephyr/toolchain.h>\n')
        for dev in parsed_elf.devices:
            # The device handle are collected up in a set, which has no
            # specified order.  Sort each sub-category of device handle types
            # separately, so that the generated C array is reproducible across
            # builds.
            sorted_handles = {
                "depends": sorted(dev.devs_depends_on, key=lambda d: d.handle),
                "injected": sorted(dev.devs_depends_on_injected, key=lambda d: d.handle),
                "supports": sorted(dev.devs_supports, key=lambda d: d.handle),
            }
            extra_sups = args.num_dynamic_devices if dev.pm and dev.pm.is_power_domain else 0
            lines = c_handle_comment(dev, sorted_handles)
            lines.extend(
                c_handle_array(dev, sorted_handles, args.dynamic_deps, extra_sups)
            )
            lines.extend([''])
            fp.write('\n'.join(lines))

    create_device_dt_list(parsed_elf)

if __name__ == "__main__":
    main()
