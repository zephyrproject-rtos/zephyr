#!/usr/bin/env python3

# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2024 SILA Embedded Solutions GmbH
# SPDX-License-Identifier: BSD-3-Clause

# This script uses edtlib to generate a header file from a pickled
# edt file.
#
# Note: Do not access private (_-prefixed) identifiers from edtlib here (and
# also note that edtlib is not meant to expose the dtlib API directly).
# Instead, think of what API you need, and add it as a public documented API in
# edtlib. This will keep this script simple.

import argparse
from collections import defaultdict
import os
import pathlib
import pickle
import re
import sys
from typing import Iterable, NoReturn, Optional

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))

import edtlib_logger
from devicetree import edtlib


def main():
    global header_file
    global flash_area_num

    args = parse_args()

    edtlib_logger.setup_edtlib_logging()

    with open(args.edt_pickle, 'rb') as f:
        edt = pickle.load(f)

    flash_area_num = 0

    # Create the generated header.
    with open(args.header_out, "w", encoding="utf-8") as header_file:
        write_top_comment(edt)

        write_utils()

        sorted_nodes = sorted(edt.nodes, key=lambda node: node.dep_ordinal)

        # populate all z_path_id first so any children references will
        # work correctly.
        for node in sorted_nodes:
            node.z_path_id = node_z_path_id(node)

        # Check to see if we have duplicate "zephyr,memory-region" property values.
        regions = dict()
        for node in sorted_nodes:
            if 'zephyr,memory-region' in node.props:
                region = node.props['zephyr,memory-region'].val
                if region in regions:
                    sys.exit(f"ERROR: Duplicate 'zephyr,memory-region' ({region}) properties "
                             f"between {regions[region].path} and {node.path}")
                regions[region] = node

        for node in sorted_nodes:
            write_node_comment(node)

            out_comment("Node's full path:")
            out_dt_define(f"{node.z_path_id}_PATH", f'"{escape(node.path)}"')

            out_comment("Node's name with unit-address:")
            out_dt_define(f"{node.z_path_id}_FULL_NAME",
                          f'"{escape(node.name)}"')
            out_dt_define(f"{node.z_path_id}_FULL_NAME_UNQUOTED",
                          f'{escape(node.name)}')
            out_dt_define(f"{node.z_path_id}_FULL_NAME_TOKEN",
                          f'{edtlib.str_as_token(escape(node.name))}')
            out_dt_define(f"{node.z_path_id}_FULL_NAME_UPPER_TOKEN",
                          f'{edtlib.str_as_token(escape(node.name)).upper()}')

            if node.parent is not None:
                out_comment(f"Node parent ({node.parent.path}) identifier:")
                out_dt_define(f"{node.z_path_id}_PARENT",
                              f"DT_{node.parent.z_path_id}")

                out_comment(f"Node's index in its parent's list of children:")
                out_dt_define(f"{node.z_path_id}_CHILD_IDX",
                              node.parent.child_index(node))

            out_comment("Helpers for dealing with node labels:")
            out_dt_define(f"{node.z_path_id}_NODELABEL_NUM", len(node.labels))
            out_dt_define(f"{node.z_path_id}_FOREACH_NODELABEL(fn)",
                          " ".join(f"fn({nodelabel})" for nodelabel in node.labels))
            out_dt_define(f"{node.z_path_id}_FOREACH_NODELABEL_VARGS(fn, ...)",
                          " ".join(f"fn({nodelabel}, __VA_ARGS__)" for nodelabel in node.labels))

            write_children(node)
            write_dep_info(node)
            write_idents_and_existence(node)
            write_bus(node)
            write_special_props(node)
            write_vanilla_props(node)

        write_chosen(edt)
        write_global_macros(edt)


def node_z_path_id(node: edtlib.Node) -> str:
    # Return the node specific bit of the node's path identifier:
    #
    # - the root node's path "/" has path identifier "N"
    # - "/foo" has "N_S_foo"
    # - "/foo/bar" has "N_S_foo_S_bar"
    # - "/foo/bar@123" has "N_S_foo_S_bar_123"
    #
    # This is used throughout this file to generate macros related to
    # the node.

    components = ["N"]
    if node.parent is not None:
        components.extend(f"S_{str2ident(component)}" for component in
                          node.path.split("/")[1:])

    return "_".join(components)


def parse_args() -> argparse.Namespace:
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--header-out", required=True,
                        help="path to write header to")
    parser.add_argument("--edt-pickle",
                        help="path to read pickled edtlib.EDT object from")

    return parser.parse_args()


def write_top_comment(edt: edtlib.EDT) -> None:
    # Writes an overview comment with misc. info at the top of the header and
    # configuration file

    s = f"""\
Generated by gen_defines.py

DTS input file:
  {edt.dts_path}

Directories with bindings:
  {", ".join(map(relativize, edt.bindings_dirs))}

Node dependency ordering (ordinal and path):
"""

    for scc in edt.scc_order:
        if len(scc) > 1:
            err("cycle in devicetree involving "
                + ", ".join(node.path for node in scc))
        s += f"  {scc[0].dep_ordinal:<3} {scc[0].path}\n"

    s += """
Definitions derived from these nodes in dependency order are next,
followed by /chosen nodes.
"""

    out_comment(s, blank_before=False)


def write_utils() -> None:
    # Writes utility macros

    out_comment("Used to remove brackets from around a single argument")
    out_define("DT_DEBRACKET_INTERNAL(...)", "__VA_ARGS__")


def write_node_comment(node: edtlib.Node) -> None:
    # Writes a comment describing 'node' to the header and configuration file

    s = f"""\
Devicetree node: {node.path}

Node identifier: DT_{node.z_path_id}
"""

    if node.matching_compat:
        if node.binding_path:
            s += f"""
Binding (compatible = {node.matching_compat}):
  {relativize(node.binding_path)}
"""
        else:
            s += f"""
Binding (compatible = {node.matching_compat}):
  No yaml (bindings inferred from properties)
"""

    if node.description:
        # We used to put descriptions in the generated file, but
        # devicetree bindings now have pages in the HTML
        # documentation. Let users who are accustomed to digging
        # around in the generated file where to find the descriptions
        # now.
        #
        # Keeping them here would mean that the descriptions
        # themselves couldn't contain C multi-line comments, which is
        # inconvenient when we want to do things like quote snippets
        # of .dtsi files within the descriptions, or otherwise
        # include the string "*/".
        s += ("\n(Descriptions have moved to the Devicetree Bindings Index\n"
              "in the documentation.)\n")

    out_comment(s)


def relativize(path) -> Optional[str]:
    # If 'path' is within $ZEPHYR_BASE, returns it relative to $ZEPHYR_BASE,
    # with a "$ZEPHYR_BASE/..." hint at the start of the string. Otherwise,
    # returns 'path' unchanged.

    zbase = os.getenv("ZEPHYR_BASE")
    if zbase is None:
        return path

    try:
        return str("$ZEPHYR_BASE" / pathlib.Path(path).relative_to(zbase))
    except ValueError:
        # Not within ZEPHYR_BASE
        return path


def write_idents_and_existence(node: edtlib.Node) -> None:
    # Writes macros related to the node's aliases, labels, etc.,
    # as well as existence flags.

    # Aliases
    idents = [f"N_ALIAS_{str2ident(alias)}" for alias in node.aliases]
    # Instances
    for compat in node.compats:
        instance_no = node.edt.compat2nodes[compat].index(node)
        idents.append(f"N_INST_{instance_no}_{str2ident(compat)}")
    # Node labels
    idents.extend(f"N_NODELABEL_{str2ident(label)}" for label in node.labels)

    out_comment("Existence and alternate IDs:")
    out_dt_define(node.z_path_id + "_EXISTS", 1)

    # Only determine maxlen if we have any idents
    if idents:
        maxlen = max(len("DT_" + ident) for ident in idents)
    for ident in idents:
        out_dt_define(ident, "DT_" + node.z_path_id, width=maxlen)


def write_bus(node: edtlib.Node) -> None:
    # Macros about the node's bus controller, if there is one

    bus = node.bus_node
    if not bus:
        return

    out_comment(f"Bus info (controller: '{bus.path}', type: '{node.on_buses}')")

    for one_bus in node.on_buses:
        out_dt_define(f"{node.z_path_id}_BUS_{str2ident(one_bus)}", 1)

    out_dt_define(f"{node.z_path_id}_BUS", f"DT_{bus.z_path_id}")


def write_special_props(node: edtlib.Node) -> None:
    # Writes required macros for special case properties, when the
    # data cannot otherwise be obtained from write_vanilla_props()
    # results

    # Macros that are special to the devicetree specification
    out_comment("Macros for properties that are special in the specification:")
    write_regs(node)
    write_ranges(node)
    write_interrupts(node)
    write_compatibles(node)
    write_status(node)

    # Macros that are special to bindings inherited from Linux, which
    # we can't capture with the current bindings language.
    write_pinctrls(node)
    write_fixed_partitions(node)
    write_gpio_hogs(node)


def write_ranges(node: edtlib.Node) -> None:
    # ranges property: edtlib knows the right #address-cells and
    # #size-cells of parent and child, and can therefore pack the
    # child & parent addresses and sizes correctly

    idx_vals = []
    path_id = node.z_path_id

    if node.ranges is not None:
        idx_vals.append((f"{path_id}_RANGES_NUM", len(node.ranges)))

    for i,range in enumerate(node.ranges):
        idx_vals.append((f"{path_id}_RANGES_IDX_{i}_EXISTS", 1))

        if "pcie" in node.buses:
            idx_vals.append((f"{path_id}_RANGES_IDX_{i}_VAL_CHILD_BUS_FLAGS_EXISTS", 1))
            idx_macro = f"{path_id}_RANGES_IDX_{i}_VAL_CHILD_BUS_FLAGS"
            idx_value = range.child_bus_addr >> ((range.child_bus_cells - 1) * 32)
            idx_vals.append((idx_macro,
                             f"{idx_value} /* {hex(idx_value)} */"))
        if range.child_bus_addr is not None:
            idx_macro = f"{path_id}_RANGES_IDX_{i}_VAL_CHILD_BUS_ADDRESS"
            if "pcie" in node.buses:
                idx_value = range.child_bus_addr & ((1 << (range.child_bus_cells - 1) * 32) - 1)
            else:
                idx_value = range.child_bus_addr
            idx_vals.append((idx_macro,
                             f"{idx_value} /* {hex(idx_value)} */"))
        if range.parent_bus_addr is not None:
            idx_macro = f"{path_id}_RANGES_IDX_{i}_VAL_PARENT_BUS_ADDRESS"
            idx_vals.append((idx_macro,
                             f"{range.parent_bus_addr} /* {hex(range.parent_bus_addr)} */"))
        if range.length is not None:
            idx_macro = f"{path_id}_RANGES_IDX_{i}_VAL_LENGTH"
            idx_vals.append((idx_macro,
                             f"{range.length} /* {hex(range.length)} */"))

    for macro, val in idx_vals:
        out_dt_define(macro, val)

    out_dt_define(f"{path_id}_FOREACH_RANGE(fn)",
            " ".join(f"fn(DT_{path_id}, {i})" for i,range in enumerate(node.ranges)))


def write_regs(node: edtlib.Node) -> None:
    # reg property: edtlib knows the right #address-cells and
    # #size-cells, and can therefore pack the register base addresses
    # and sizes correctly

    idx_vals = []
    name_vals = []
    path_id = node.z_path_id

    if node.regs is not None:
        idx_vals.append((f"{path_id}_REG_NUM", len(node.regs)))

    for i, reg in enumerate(node.regs):
        idx_vals.append((f"{path_id}_REG_IDX_{i}_EXISTS", 1))
        if reg.addr is not None:
            idx_macro = f"{path_id}_REG_IDX_{i}_VAL_ADDRESS"
            idx_vals.append((idx_macro,
                             f"{reg.addr} /* {hex(reg.addr)} */"))
            if reg.name:
                name_vals.append((f"{path_id}_REG_NAME_{reg.name}_EXISTS", 1))
                name_macro = f"{path_id}_REG_NAME_{reg.name}_VAL_ADDRESS"
                name_vals.append((name_macro, f"DT_{idx_macro}"))

        if reg.size is not None:
            idx_macro = f"{path_id}_REG_IDX_{i}_VAL_SIZE"
            idx_vals.append((idx_macro,
                             f"{reg.size} /* {hex(reg.size)} */"))
            if reg.name:
                name_macro = f"{path_id}_REG_NAME_{reg.name}_VAL_SIZE"
                name_vals.append((name_macro, f"DT_{idx_macro}"))

    for macro, val in idx_vals:
        out_dt_define(macro, val)
    for macro, val in name_vals:
        out_dt_define(macro, val)


def write_interrupts(node: edtlib.Node) -> None:
    # interrupts property: we have some hard-coded logic for interrupt
    # mapping here.
    #
    # TODO: can we push map_arm_gic_irq_type() out of Python and into C with
    # macro magic in devicetree.h?

    def map_arm_gic_irq_type(irq, irq_num):
        # Maps ARM GIC IRQ (type)+(index) combo to linear IRQ number
        if "type" not in irq.data:
            err(f"Expected binding for {irq.controller!r} to have 'type' in "
                "interrupt-cells")
        irq_type = irq.data["type"]

        if irq_type == 0:  # GIC_SPI
            return irq_num + 32
        if irq_type == 1:  # GIC_PPI
            return irq_num + 16
        err(f"Invalid interrupt type specified for {irq!r}")

    idx_vals = []
    name_vals = []
    path_id = node.z_path_id

    if node.interrupts is not None:
        idx_vals.append((f"{path_id}_IRQ_NUM", len(node.interrupts)))

    for i, irq in enumerate(node.interrupts):
        for cell_name, cell_value in irq.data.items():
            name = str2ident(cell_name)

            if cell_name == "irq":
                if "arm,gic" in irq.controller.compats:
                    cell_value = map_arm_gic_irq_type(irq, cell_value)

            idx_vals.append((f"{path_id}_IRQ_IDX_{i}_EXISTS", 1))
            idx_macro = f"{path_id}_IRQ_IDX_{i}_VAL_{name}"
            idx_vals.append((idx_macro, cell_value))
            idx_vals.append((idx_macro + "_EXISTS", 1))
            if irq.name:
                name_macro = \
                    f"{path_id}_IRQ_NAME_{str2ident(irq.name)}_VAL_{name}"
                name_vals.append((name_macro, f"DT_{idx_macro}"))
                name_vals.append((name_macro + "_EXISTS", 1))

        idx_controller_macro = f"{path_id}_IRQ_IDX_{i}_CONTROLLER"
        idx_controller_path = f"DT_{irq.controller.z_path_id}"
        idx_vals.append((idx_controller_macro, idx_controller_path))
        if irq.name:
            name_controller_macro = f"{path_id}_IRQ_NAME_{str2ident(irq.name)}_CONTROLLER"
            name_vals.append((name_controller_macro, f"DT_{idx_controller_macro}"))

    # Interrupt controller info
    irqs = []
    while node.interrupts is not None and len(node.interrupts) > 0:
        irq = node.interrupts[0]
        irqs.append(irq)
        if node == irq.controller:
            break
        node = irq.controller
    idx_vals.append((f"{path_id}_IRQ_LEVEL", len(irqs)))

    for macro, val in idx_vals:
        out_dt_define(macro, val)
    for macro, val in name_vals:
        out_dt_define(macro, val)


def write_compatibles(node: edtlib.Node) -> None:
    # Writes a macro for each of the node's compatibles. We don't care
    # about whether edtlib / Zephyr's binding language recognizes
    # them. The compatibles the node provides are what is important.

    for i, compat in enumerate(node.compats):
        out_dt_define(
            f"{node.z_path_id}_COMPAT_MATCHES_{str2ident(compat)}", 1)

        if node.edt.compat2vendor[compat]:
            out_dt_define(f"{node.z_path_id}_COMPAT_VENDOR_IDX_{i}_EXISTS", 1)
            out_dt_define(f"{node.z_path_id}_COMPAT_VENDOR_IDX_{i}",
                          quote_str(node.edt.compat2vendor[compat]))

        if node.edt.compat2model[compat]:
            out_dt_define(f"{node.z_path_id}_COMPAT_MODEL_IDX_{i}_EXISTS", 1)
            out_dt_define(f"{node.z_path_id}_COMPAT_MODEL_IDX_{i}",
                          quote_str(node.edt.compat2model[compat]))


def write_children(node: edtlib.Node) -> None:
    # Writes helper macros for dealing with node's children.

    out_comment("Helper macros for child nodes of this node.")

    out_dt_define(f"{node.z_path_id}_CHILD_NUM", len(node.children))

    ok_nodes_num = 0
    for child in node.children.values():
        if child.status == "okay":
            ok_nodes_num = ok_nodes_num + 1

    out_dt_define(f"{node.z_path_id}_CHILD_NUM_STATUS_OKAY", ok_nodes_num)

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD(fn)",
            " ".join(f"fn(DT_{child.z_path_id})" for child in
                node.children.values()))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_SEP(fn, sep)",
            " DT_DEBRACKET_INTERNAL sep ".join(f"fn(DT_{child.z_path_id})"
            for child in node.children.values()))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_VARGS(fn, ...)",
            " ".join(f"fn(DT_{child.z_path_id}, __VA_ARGS__)"
            for child in node.children.values()))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_SEP_VARGS(fn, sep, ...)",
            " DT_DEBRACKET_INTERNAL sep ".join(f"fn(DT_{child.z_path_id}, __VA_ARGS__)"
            for child in node.children.values()))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_STATUS_OKAY(fn)",
            " ".join(f"fn(DT_{child.z_path_id})"
            for child in node.children.values() if child.status == "okay"))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_STATUS_OKAY_SEP(fn, sep)",
            " DT_DEBRACKET_INTERNAL sep ".join(f"fn(DT_{child.z_path_id})"
            for child in node.children.values() if child.status == "okay"))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_STATUS_OKAY_VARGS(fn, ...)",
            " ".join(f"fn(DT_{child.z_path_id}, __VA_ARGS__)"
            for child in node.children.values() if child.status == "okay"))

    out_dt_define(f"{node.z_path_id}_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(fn, sep, ...)",
            " DT_DEBRACKET_INTERNAL sep ".join(f"fn(DT_{child.z_path_id}, __VA_ARGS__)"
            for child in node.children.values() if child.status == "okay"))


def write_status(node: edtlib.Node) -> None:
    out_dt_define(f"{node.z_path_id}_STATUS_{str2ident(node.status)}", 1)


def write_pinctrls(node: edtlib.Node) -> None:
    # Write special macros for pinctrl-<index> and pinctrl-names properties.

    out_comment("Pin control (pinctrl-<i>, pinctrl-names) properties:")

    out_dt_define(f"{node.z_path_id}_PINCTRL_NUM", len(node.pinctrls))

    if not node.pinctrls:
        return

    for pc_idx, pinctrl in enumerate(node.pinctrls):
        out_dt_define(f"{node.z_path_id}_PINCTRL_IDX_{pc_idx}_EXISTS", 1)

        if not pinctrl.name:
            continue

        name = pinctrl.name_as_token

        # Below we rely on the fact that edtlib ensures the
        # pinctrl-<pc_idx> properties are contiguous, start from 0,
        # and contain only phandles.
        out_dt_define(f"{node.z_path_id}_PINCTRL_IDX_{pc_idx}_TOKEN", name)
        out_dt_define(f"{node.z_path_id}_PINCTRL_IDX_{pc_idx}_UPPER_TOKEN", name.upper())
        out_dt_define(f"{node.z_path_id}_PINCTRL_NAME_{name}_EXISTS", 1)
        out_dt_define(f"{node.z_path_id}_PINCTRL_NAME_{name}_IDX", pc_idx)
        for idx, ph in enumerate(pinctrl.conf_nodes):
            out_dt_define(f"{node.z_path_id}_PINCTRL_NAME_{name}_IDX_{idx}_PH",
                          f"DT_{ph.z_path_id}")


def write_fixed_partitions(node: edtlib.Node) -> None:
    # Macros for child nodes of each fixed-partitions node.

    if not (node.parent and "fixed-partitions" in node.parent.compats):
        return

    global flash_area_num
    out_comment("fixed-partitions identifier:")
    out_dt_define(f"{node.z_path_id}_PARTITION_ID", flash_area_num)
    flash_area_num += 1


def write_gpio_hogs(node: edtlib.Node) -> None:
    # Write special macros for gpio-hog node properties.

    macro = f"{node.z_path_id}_GPIO_HOGS"
    macro2val = {}
    for i, entry in enumerate(node.gpio_hogs):
        macro2val.update(controller_and_data_macros(entry, i, macro))

    if macro2val:
        out_comment("GPIO hog properties:")
        out_dt_define(f"{macro}_EXISTS", 1)
        out_dt_define(f"{macro}_NUM", len(node.gpio_hogs))
        for macro, val in macro2val.items():
            out_dt_define(macro, val)


def write_vanilla_props(node: edtlib.Node) -> None:
    # Writes macros for any and all properties defined in the
    # "properties" section of the binding for the node.
    #
    # This does generate macros for special properties as well, like
    # regs, etc. Just let that be rather than bothering to add
    # never-ending amounts of special case code here to skip special
    # properties. This function's macros can't conflict with
    # write_special_props() macros, because they're in different
    # namespaces. Special cases aren't special enough to break the rules.

    macro2val = {}
    for prop_name, prop in node.props.items():
        prop_id = str2ident(prop_name)
        macro = f"{node.z_path_id}_P_{prop_id}"
        val = prop2value(prop)
        if val is not None:
            # DT_N_<node-id>_P_<prop-id>
            macro2val[macro] = val

        if prop.spec.type == 'string':
            macro2val.update(string_macros(macro, prop.val))
            # DT_N_<node-id>_P_<prop-id>_IDX_0:
            # DT_N_<node-id>_P_<prop-id>_IDX_0_EXISTS:
            # Allows treating the string like a degenerate case of a
            # string-array of length 1.
            macro2val[macro + "_IDX_0"] = quote_str(prop.val)
            macro2val[macro + "_IDX_0_EXISTS"] = 1

        if prop.enum_indices is not None:
            macro2val.update(enum_macros(prop, macro))

        if "phandle" in prop.type:
            macro2val.update(phandle_macros(prop, macro))
        elif "array" in prop.type:
            macro2val.update(array_macros(prop, macro))

        plen = prop_len(prop)
        if plen is not None:
            # DT_N_<node-id>_P_<prop-id>_FOREACH_PROP_ELEM
            macro2val[f"{macro}_FOREACH_PROP_ELEM(fn)"] = \
                ' \\\n\t'.join(
                    f'fn(DT_{node.z_path_id}, {prop_id}, {i})'
                    for i in range(plen))

            # DT_N_<node-id>_P_<prop-id>_FOREACH_PROP_ELEM_SEP
            macro2val[f"{macro}_FOREACH_PROP_ELEM_SEP(fn, sep)"] = \
                ' DT_DEBRACKET_INTERNAL sep \\\n\t'.join(
                    f'fn(DT_{node.z_path_id}, {prop_id}, {i})'
                    for i in range(plen))

            # DT_N_<node-id>_P_<prop-id>_FOREACH_PROP_ELEM_VARGS
            macro2val[f"{macro}_FOREACH_PROP_ELEM_VARGS(fn, ...)"] = \
                ' \\\n\t'.join(
                    f'fn(DT_{node.z_path_id}, {prop_id}, {i}, __VA_ARGS__)'
                    for i in range(plen))

            # DT_N_<node-id>_P_<prop-id>_FOREACH_PROP_ELEM_SEP_VARGS
            macro2val[f"{macro}_FOREACH_PROP_ELEM_SEP_VARGS(fn, sep, ...)"] = \
                ' DT_DEBRACKET_INTERNAL sep \\\n\t'.join(
                    f'fn(DT_{node.z_path_id}, {prop_id}, {i}, __VA_ARGS__)'
                    for i in range(plen))

            # DT_N_<node-id>_P_<prop-id>_LEN
            macro2val[macro + "_LEN"] = plen

        # DT_N_<node-id>_P_<prop-id>_EXISTS
        macro2val[f"{macro}_EXISTS"] = 1

    if macro2val:
        out_comment("Generic property macros:")
        for macro, val in macro2val.items():
            out_dt_define(macro, val)
    else:
        out_comment("(No generic property macros)")


def string_macros(macro: str, val: str):
    # Returns a dict of macros for a string 'val'.
    # The 'macro' argument is the N_<node-id>_P_<prop-id>... part.

    as_token = edtlib.str_as_token(val)
    return {
        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_STRING_UNQUOTED
        f"{macro}_STRING_UNQUOTED": escape_unquoted(val),
        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_STRING_TOKEN
        f"{macro}_STRING_TOKEN": as_token,
        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_STRING_UPPER_TOKEN
        f"{macro}_STRING_UPPER_TOKEN": as_token.upper()}


def enum_macros(prop: edtlib.Property, macro: str):
    # Returns a dict of macros for property 'prop' with a defined enum in their dt-binding.
    # The 'macro' argument is the N_<node-id>_P_<prop-id> part.

    spec = prop.spec
    # DT_N_<node-id>_P_<prop-id>_IDX_<i>_ENUM_IDX
    ret = {f"{macro}_IDX_{i}_ENUM_IDX": index for i, index in enumerate(prop.enum_indices)}
    val = prop.val_as_tokens if spec.enum_tokenizable else (prop.val if isinstance(prop.val, list) else [prop.val])

    for i, subval in enumerate(val):
        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_EXISTS
        ret[macro + f"_IDX_{i}_EXISTS"] = 1
        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_ENUM_VAL_<val>_EXISTS 1
        ret[macro + f"_IDX_{i}_ENUM_VAL_{subval}_EXISTS"] = 1
        if not spec.enum_tokenizable:
            continue

        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_ENUM_TOKEN
        ret[macro + f"_IDX_{i}_ENUM_TOKEN"] = subval
        if spec.enum_upper_tokenizable:
            # DT_N_<node-id>_P_<prop-id>_IDX_<i>_ENUM_UPPER_TOKEN
            ret[macro + f"_IDX_{i}_ENUM_UPPER_TOKEN"] = subval.upper()

    return ret


def array_macros(prop: edtlib.Property, macro: str):
    # Returns a dict of macros for array property 'prop'.
    # The 'macro' argument is the N_<node-id>_P_<prop-id> part.

    ret = {}
    for i, subval in enumerate(prop.val):
        # DT_N_<node-id>_P_<prop-id>_IDX_<i>_EXISTS
        ret[macro + f"_IDX_{i}_EXISTS"] = 1

        # DT_N_<node-id>_P_<prop-id>_IDX_<i>
        if isinstance(subval, str):
            ret[macro + f"_IDX_{i}"] = quote_str(subval)
            # DT_N_<node-id>_P_<prop-id>_IDX_<i>_STRING_...
            ret.update(string_macros(macro + f"_IDX_{i}", subval))
        else:
            ret[macro + f"_IDX_{i}"] = subval

    return ret


def write_dep_info(node: edtlib.Node) -> None:
    # Write dependency-related information about the node.

    def fmt_dep_list(dep_list):
        if dep_list:
            # Sort the list by dependency ordinal for predictability.
            sorted_list = sorted(dep_list, key=lambda node: node.dep_ordinal)
            return "\\\n\t" + \
                " \\\n\t".join(f"{n.dep_ordinal}, /* {n.path} */"
                               for n in sorted_list)
        else:
            return "/* nothing */"

    out_comment("Node's dependency ordinal:")
    out_dt_define(f"{node.z_path_id}_ORD", node.dep_ordinal)
    out_dt_define(f"{node.z_path_id}_ORD_STR_SORTABLE", f"{node.dep_ordinal:0>5}")

    out_comment("Ordinals for what this node depends on directly:")
    out_dt_define(f"{node.z_path_id}_REQUIRES_ORDS",
                  fmt_dep_list(node.depends_on))

    out_comment("Ordinals for what depends directly on this node:")
    out_dt_define(f"{node.z_path_id}_SUPPORTS_ORDS",
                  fmt_dep_list(node.required_by))


def prop2value(prop: edtlib.Property) -> edtlib.PropertyValType:
    # Gets the macro value for property 'prop', if there is
    # a single well-defined C rvalue that it can be represented as.
    # Returns None if there isn't one.

    if prop.type == "string":
        return quote_str(prop.val)

    if prop.type == "int":
        return prop.val

    if prop.type == "boolean":
        return 1 if prop.val else 0

    if prop.type in ["array", "uint8-array"]:
        return list2init(f"{val} /* {hex(val)} */" for val in prop.val)

    if prop.type == "string-array":
        return list2init(quote_str(val) for val in prop.val)

    # phandle, phandles, phandle-array, path, compound: nothing
    return None


def prop_len(prop: edtlib.Property) -> Optional[int]:
    # Returns the property's length if and only if we should generate
    # a _LEN macro for the property. Otherwise, returns None.
    #
    # The set of types handled here coincides with the allowable types
    # that can be used with DT_PROP_LEN(). If you change this set,
    # make sure to update the doxygen string for that macro, and make
    # sure that DT_FOREACH_PROP_ELEM() works for the new types too.
    #
    # This deliberately excludes ranges, dma-ranges, reg and interrupts.
    # While they have array type, their lengths as arrays are
    # basically nonsense semantically due to #address-cells and
    # #size-cells for "reg", #interrupt-cells for "interrupts"
    # and #address-cells, #size-cells and the #address-cells from the
    # parent node for "ranges" and "dma-ranges".
    #
    # We have special purpose macros for the number of register blocks
    # / interrupt specifiers. Excluding them from this list means
    # DT_PROP_LEN(node_id, ...) fails fast at the devicetree.h layer
    # with a build error. This forces users to switch to the right
    # macros.

    if prop.type in ["phandle", "string"]:
        # phandle is treated as a phandles of length 1.
        # string is treated as a string-array of length 1.
        return 1

    if (prop.type in ["array", "uint8-array", "string-array",
                      "phandles", "phandle-array"] and
                prop.name not in ["ranges", "dma-ranges", "reg", "interrupts"]):
        return len(prop.val)

    return None


def phandle_macros(prop: edtlib.Property, macro: str) -> dict:
    # Returns a dict of macros for phandle or phandles property 'prop'.
    #
    # The 'macro' argument is the N_<node-id>_P_<prop-id> bit.
    #
    # These are currently special because we can't serialize their
    # values without using label properties, which we're trying to get
    # away from needing in Zephyr. (Label properties are great for
    # humans, but have drawbacks for code size and boot time.)
    #
    # The names look a bit weird to make it easier for devicetree.h
    # to use the same macros for phandle, phandles, and phandle-array.

    ret = {}

    if prop.type == "phandle":
        # A phandle is treated as a phandles with fixed length 1.
        ret[f"{macro}"] = f"DT_{prop.val.z_path_id}"
        ret[f"{macro}_IDX_0"] = f"DT_{prop.val.z_path_id}"
        ret[f"{macro}_IDX_0_PH"] = f"DT_{prop.val.z_path_id}"
        ret[f"{macro}_IDX_0_EXISTS"] = 1
    elif prop.type == "phandles":
        for i, node in enumerate(prop.val):
            ret[f"{macro}_IDX_{i}"] = f"DT_{node.z_path_id}"
            ret[f"{macro}_IDX_{i}_PH"] = f"DT_{node.z_path_id}"
            ret[f"{macro}_IDX_{i}_EXISTS"] = 1
    elif prop.type == "phandle-array":
        for i, entry in enumerate(prop.val):
            if entry is None:
                # Unspecified element. The phandle-array at this index
                # does not point at a ControllerAndData value, but
                # subsequent indices in the array may.
                ret[f"{macro}_IDX_{i}_EXISTS"] = 0
                continue

            ret.update(controller_and_data_macros(entry, i, macro))

    return ret


def controller_and_data_macros(entry: edtlib.ControllerAndData, i: int, macro: str):
    # Helper procedure used by phandle_macros().
    #
    # Its purpose is to write the "controller" (i.e. label property of
    # the phandle's node) and associated data macros for a
    # ControllerAndData.

    ret = {}
    data = entry.data

    # DT_N_<node-id>_P_<prop-id>_IDX_<i>_EXISTS
    ret[f"{macro}_IDX_{i}_EXISTS"] = 1
    # DT_N_<node-id>_P_<prop-id>_IDX_<i>_PH
    ret[f"{macro}_IDX_{i}_PH"] = f"DT_{entry.controller.z_path_id}"
    # DT_N_<node-id>_P_<prop-id>_IDX_<i>_VAL_<VAL>
    for cell, val in data.items():
        ret[f"{macro}_IDX_{i}_VAL_{str2ident(cell)}"] = val
        ret[f"{macro}_IDX_{i}_VAL_{str2ident(cell)}_EXISTS"] = 1

    if not entry.name:
        return ret

    name = str2ident(entry.name)
    # DT_N_<node-id>_P_<prop-id>_IDX_<i>_EXISTS
    ret[f"{macro}_IDX_{i}_EXISTS"] = 1
    # DT_N_<node-id>_P_<prop-id>_IDX_<i>_NAME
    ret[f"{macro}_IDX_{i}_NAME"] = quote_str(entry.name)
    # DT_N_<node-id>_P_<prop-id>_NAME_<NAME>_PH
    ret[f"{macro}_NAME_{name}_PH"] = f"DT_{entry.controller.z_path_id}"
    # DT_N_<node-id>_P_<prop-id>_NAME_<NAME>_EXISTS
    ret[f"{macro}_NAME_{name}_EXISTS"] = 1
    # DT_N_<node-id>_P_<prop-id>_NAME_<NAME>_VAL_<VAL>
    for cell, val in data.items():
        cell_ident = str2ident(cell)
        ret[f"{macro}_NAME_{name}_VAL_{cell_ident}"] = \
            f"DT_{macro}_IDX_{i}_VAL_{cell_ident}"
        ret[f"{macro}_NAME_{name}_VAL_{cell_ident}_EXISTS"] = 1

    return ret


def write_chosen(edt: edtlib.EDT):
    # Tree-wide information such as chosen nodes is printed here.

    out_comment("Chosen nodes\n")
    chosen = {}
    for name, node in edt.chosen_nodes.items():
        chosen[f"DT_CHOSEN_{str2ident(name)}"] = f"DT_{node.z_path_id}"
        chosen[f"DT_CHOSEN_{str2ident(name)}_EXISTS"] = 1
    max_len = max(map(len, chosen), default=0)
    for macro, value in chosen.items():
        out_define(macro, value, width=max_len)


def write_global_macros(edt: edtlib.EDT):
    # Global or tree-wide information, such as number of instances
    # with status "okay" for each compatible, is printed here.


    out_comment("Macros for iterating over all nodes and enabled nodes")
    out_dt_define("FOREACH_HELPER(fn)",
                  " ".join(f"fn(DT_{node.z_path_id})" for node in edt.nodes))
    out_dt_define("FOREACH_OKAY_HELPER(fn)",
                  " ".join(f"fn(DT_{node.z_path_id})" for node in edt.nodes
                           if node.status == "okay"))
    out_dt_define("FOREACH_VARGS_HELPER(fn, ...)",
                  " ".join(f"fn(DT_{node.z_path_id}, __VA_ARGS__)" for node in edt.nodes))
    out_dt_define("FOREACH_OKAY_VARGS_HELPER(fn, ...)",
                  " ".join(f"fn(DT_{node.z_path_id}, __VA_ARGS__)" for node in edt.nodes
                           if node.status == "okay"))

    n_okay_macros = {}
    for_each_macros = {}
    compat2buses = defaultdict(list)  # just for "okay" nodes
    for compat, okay_nodes in edt.compat2okay.items():
        for node in okay_nodes:
            buses = node.on_buses
            for bus in buses:
                if bus is not None and bus not in compat2buses[compat]:
                    compat2buses[compat].append(bus)

        ident = str2ident(compat)
        n_okay_macros[f"DT_N_INST_{ident}_NUM_OKAY"] = len(okay_nodes)

        # Helpers for non-INST for-each macros that take node
        # identifiers as arguments.
        for_each_macros[f"DT_FOREACH_OKAY_{ident}(fn)"] = \
            " ".join(f"fn(DT_{node.z_path_id})"
                     for node in okay_nodes)
        for_each_macros[f"DT_FOREACH_OKAY_VARGS_{ident}(fn, ...)"] = \
            " ".join(f"fn(DT_{node.z_path_id}, __VA_ARGS__)"
                     for node in okay_nodes)

        # Helpers for INST versions of for-each macros, which take
        # instance numbers. We emit separate helpers for these because
        # avoiding an intermediate node_id --> instance number
        # conversion in the preprocessor helps to keep the macro
        # expansions simpler. That hopefully eases debugging.
        for_each_macros[f"DT_FOREACH_OKAY_INST_{ident}(fn)"] = \
            " ".join(f"fn({edt.compat2nodes[compat].index(node)})"
                     for node in okay_nodes)
        for_each_macros[f"DT_FOREACH_OKAY_INST_VARGS_{ident}(fn, ...)"] = \
            " ".join(f"fn({edt.compat2nodes[compat].index(node)}, __VA_ARGS__)"
                     for node in okay_nodes)

    for compat, nodes in edt.compat2nodes.items():
        for node in nodes:
            if compat == "fixed-partitions":
                for child in node.children.values():
                    if "label" in child.props:
                        label = child.props["label"].val
                        macro = f"COMPAT_{str2ident(compat)}_LABEL_{str2ident(label)}"
                        val = f"DT_{child.z_path_id}"

                        out_dt_define(macro, val)
                        out_dt_define(macro + "_EXISTS", 1)

    out_comment('Macros for compatibles with status "okay" nodes\n')
    for compat, okay_nodes in edt.compat2okay.items():
        if okay_nodes:
            out_define(f"DT_COMPAT_HAS_OKAY_{str2ident(compat)}", 1)

    out_comment('Macros for status "okay" instances of each compatible\n')
    for macro, value in n_okay_macros.items():
        out_define(macro, value)
    for macro, value in for_each_macros.items():
        out_define(macro, value)

    out_comment('Bus information for status "okay" nodes of each compatible\n')
    for compat, buses in compat2buses.items():
        for bus in buses:
            out_define(
                f"DT_COMPAT_{str2ident(compat)}_BUS_{str2ident(bus)}", 1)


def str2ident(s: str) -> str:
    # Converts 's' to a form suitable for (part of) an identifier

    return re.sub('[-,.@/+]', '_', s.lower())


def list2init(l: Iterable[str]) -> str:
    # Converts 'l', a Python list (or iterable), to a C array initializer

    return "{" + ", ".join(l) + "}"


def out_dt_define(
    macro: str,
    val: str,
    width: Optional[int] = None,
    deprecation_msg: Optional[str] = None,
) -> str:
    # Writes "#define DT_<macro> <val>" to the header file
    #
    # The macro will be left-justified to 'width' characters if that
    # is specified, and the value will follow immediately after in
    # that case. Otherwise, this function decides how to add
    # whitespace between 'macro' and 'val'.
    #
    # If a 'deprecation_msg' string is passed, the generated identifiers will
    # generate a warning if used, via __WARN(<deprecation_msg>)).
    #
    # Returns the full generated macro for 'macro', with leading "DT_".
    ret = "DT_" + macro
    out_define(ret, val, width=width, deprecation_msg=deprecation_msg)
    return ret


def out_define(
    macro: str,
    val: str,
    width: Optional[int] = None,
    deprecation_msg: Optional[str] = None,
) -> None:
    # Helper for out_dt_define(). Outputs "#define <macro> <val>",
    # adds a deprecation message if given, and allocates whitespace
    # unless told not to.

    warn = fr' __WARN("{deprecation_msg}")' if deprecation_msg else ""

    if width:
        s = f"#define {macro.ljust(width)}{warn} {val}"
    else:
        s = f"#define {macro}{warn} {val}"

    print(s, file=header_file)


def out_comment(s: str, blank_before=True) -> None:
    # Writes 's' as a comment to the header and configuration file. 's' is
    # allowed to have multiple lines. blank_before=True adds a blank line
    # before the comment.

    if blank_before:
        print(file=header_file)

    if "\n" in s:
        # Format multi-line comments like
        #
        #   /*
        #    * first line
        #    * second line
        #    *
        #    * empty line before this line
        #    */
        res = ["/*"]
        for line in s.splitlines():
            # Avoid an extra space after '*' for empty lines. They turn red in
            # Vim if space error checking is on, which is annoying.
            res.append(" *" if not line.strip() else " * " + line)
        res.append(" */")
        print("\n".join(res), file=header_file)
    else:
        # Format single-line comments like
        #
        #   /* foo bar */
        print("/* " + s + " */", file=header_file)


ESCAPE_TABLE = str.maketrans(
    {
        "\n": "\\n",
        "\r": "\\r",
        "\\": "\\\\",
        '"': '\\"',
    }
)


def escape(s: str) -> str:
    # Backslash-escapes any double quotes, backslashes, and new lines in 's'

    return s.translate(ESCAPE_TABLE)


def quote_str(s: str) -> str:
    # Puts quotes around 's' and escapes any double quotes and
    # backslashes within it

    return f'"{escape(s)}"'


def escape_unquoted(s: str) -> str:
    # C macros cannot contain line breaks, so replace them with spaces.
    # Whitespace is used to separate preprocessor tokens, but it does not matter
    # which whitespace characters are used, so a line break and a space are
    # equivalent with regards to unquoted strings being used as C code.

    return s.replace("\r", " ").replace("\n", " ")


def err(s: str) -> NoReturn:
    raise Exception(s)


if __name__ == "__main__":
    main()
