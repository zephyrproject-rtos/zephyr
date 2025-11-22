#!/usr/bin/env python3

# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2024 SILA Embedded Solutions GmbH
# SPDX-License-Identifier: Apache-2.0

# This script uses edtlib to generate a pickled edt from a devicetree
# (.dts) file. Information from binding files in YAML format is used
# as well.
#
# Bindings are files that describe devicetree nodes. Devicetree nodes are
# usually mapped to bindings via their 'compatible = "..."' property.
#
# See Zephyr's Devicetree user guide for details.
#
# Note: Do not access private (_-prefixed) identifiers from edtlib here (and
# also note that edtlib is not meant to expose the dtlib API directly).
# Instead, think of what API you need, and add it as a public documented API in
# edtlib. This will keep this script simple.

import argparse
import json
import os
import pickle
import sys
from typing import NoReturn

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))

import edtlib_logger
from devicetree import edtlib


def main():
    args = parse_args()

    edtlib_logger.setup_edtlib_logging()

    vendor_prefixes = {}
    for prefixes_file in args.vendor_prefixes:
        vendor_prefixes.update(edtlib.load_vendor_prefixes_txt(prefixes_file))

    try:
        edt = edtlib.EDT(args.dts, args.bindings_dirs,
                         workspace_dir=args.workspace_dir,
                         # Suppress this warning if it's suppressed in dtc
                         warn_reg_unit_address_mismatch=
                             "-Wno-simple_bus_reg" not in args.dtc_flags,
                         default_prop_types=True,
                         infer_binding_for_paths=["/zephyr,user", "/cpus"],
                         werror=args.edtlib_Werror,
                         vendor_prefixes=vendor_prefixes)
    except edtlib.EDTError as e:
        sys.exit(f"devicetree error: {e}")

    # Save merged DTS source, as a debugging aid
    with open(args.dts_out, "w", encoding="utf-8") as f:
        print(edt.dts_source, file=f)

    write_pickled_edt(edt, args.edt_pickle_out)
    write_json_metainfo_edt(edt, args.edt_json_out)

def parse_args() -> argparse.Namespace:
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--dts", required=True, help="DTS file")
    parser.add_argument("--dtc-flags",
                        help="'dtc' devicetree compiler flags, some of which "
                             "might be respected here")
    parser.add_argument("--bindings-dirs", nargs='+', required=True,
                        help="directory with bindings in YAML format, "
                        "we allow multiple")
    parser.add_argument("--workspace-dir", default=os.getcwd(),
                        help="directory to be used as reference for generated "
                        "relative paths (e.g. WEST_TOPDIR)")
    parser.add_argument("--dts-out", required=True,
                        help="path to write merged DTS source code to (e.g. "
                             "as a debugging aid)")
    parser.add_argument("--edt-pickle-out",
                        help="path to write pickled edtlib.EDT object to", required=True)
    parser.add_argument("--edt-json-out",
                        help="path to write json edtlib.EDT object to", required=True)
    parser.add_argument("--vendor-prefixes", action='append', default=[],
                        help="vendor-prefixes.txt path; used for validation; "
                             "may be given multiple times")
    parser.add_argument("--edtlib-Werror", action="store_true",
                        help="if set, edtlib-specific warnings become errors. "
                             "(this does not apply to warnings shared "
                             "with dtc.)")

    return parser.parse_args()


def write_pickled_edt(edt: edtlib.EDT, out_file: str) -> None:
    # Writes the edt object in pickle format to out_file.

    with open(out_file, 'wb') as f:
        # Pickle protocol version 4 is the default as of Python 3.8
        # and was introduced in 3.4, so it is both available and
        # recommended on all versions of Python that Zephyr supports
        # (at time of writing, Python 3.6 was Zephyr's minimum
        # version, and 3.10 the most recent CPython release).
        #
        # Using a common protocol version here will hopefully avoid
        # reproducibility issues in different Python installations.
        pickle.dump(edt, f, protocol=4)


def reg_to_dict(reg: edtlib.Register) -> dict:
    return {
        'name': reg.name,
        'addr': f'{hex(reg.addr) if reg.addr is not None else hex(0)}',
        'size': f'{hex(reg.size) if reg.size is not None else hex(0)}',
    } if reg is not None else {}


def range_to_dict(rangx: edtlib.Range) -> dict:
    return {
        'child_bus_cells': rangx.child_bus_cells,
        'child_bus_addr': rangx.child_bus_addr,
        'parent_bus_cells': rangx.parent_bus_cells,
        'parent_bus_addr': rangx.parent_bus_addr,
        'length_cells': rangx.length_cells,
        'length': rangx.length,
    } if rangx is not None else {}


def prop_to_dict(prop_name: str, prop: edtlib.Property, cache=None) -> dict:
    def propspec_to_dict(spec: edtlib.PropertySpec) -> dict:
        return {
            'path': spec.path,
            'type': spec.type,
            'enum': spec.enum,
            'const': spec.const,
            'default': spec.default,
            'required': spec.required
        }

    def propval_as(val: edtlib.PropertyValType):
        if isinstance(val, int):
            return val
        if isinstance(val, str):
            return val
        if isinstance(val, edtlib.Node):
            return node_to_dict(val, cache)
        if isinstance(val, bytes):
            return list(val)
        if isinstance(val, list):
            if all(isinstance(v, int) for v in val):
                return val
            if all(isinstance(v, str) for v in val):
                return val
            if all(isinstance(v, edtlib.Node) or v is None for v in val):
                return [node_to_dict(n, cache) for n in val]
            if all(isinstance(v, edtlib.ControllerAndData) or v is None for v in val):
                return [ctrl_to_dict(c, cache) for c in val]

    return {
        prop_name: {
            'spec': propspec_to_dict(prop.spec),
            'val': propval_as(prop.val),
            'type': prop.type
        }} if prop is not None else {}

def pinctrl_to_dict(pinctrl: edtlib.PinCtrl, cache=None) -> dict:
    return {
        'name': pinctrl.name,
        'conf_nodes': [node_to_dict(p, cache) for p in pinctrl.conf_nodes]
    } if pinctrl is not None else {}


def ctrl_to_dict(ctrl: edtlib.ControllerAndData, cache=None) -> dict:
    return {
        'name': ctrl.name,
        'basename': ctrl.basename,
        'data': ctrl.data,
        'controller': node_to_dict(ctrl.controller, cache)
    } if ctrl is not None else {}


def node_to_dict(node: edtlib.Node, cache=None) -> dict:
    if cache is None:
        cache = set()

    # Avoid deep recursion
    if node is not None and id(node) in cache:
        return {'node_ref': node.path}
    cache.add(id(node))

    return {
        'name': node.name,
        'compats': node.compats,
        'path': node.path,
        'labels': node.labels,
        'aliases': node.aliases,
        'status': node.status,
        'dep_ordinal': node.dep_ordinal,
        'hash': node.hash,
        'ranges': [range_to_dict(r) for r in node.ranges],
        'regs': [reg_to_dict(r) for r in node.regs],
        'props': [prop_to_dict(p_name, p, cache) for p_name, p in node.props.items()],
        'interrupts': [ctrl_to_dict(i, cache) for i in node.interrupts],
        'pinctrls': [pinctrl_to_dict(p, cache) for p in node.pinctrls]
    } if node is not None else {}


def write_json_metainfo_edt(edt: edtlib.EDT, out_file: str) -> None:
    # Writes the edt object in pickle format to out_file.

    nodes = {}

    with open(out_file, 'w') as f:
        nodes['nodes'] = [node_to_dict(node) for node in edt.nodes if node is not None]
        # Sorry, I was supported before Python 2.6
        json.dump(nodes, f)


def err(s: str) -> NoReturn:
    raise Exception(s)


if __name__ == "__main__":
    main()
