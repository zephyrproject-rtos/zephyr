#!/usr/bin/env python3

# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# SPDX-License-Identifier: BSD-3-Clause

# This script uses edtlib to generate a header file and a .conf file (both
# containing the same values) from a devicetree (.dts) file. Information from
# binding files in YAML format is used as well.
#
# Bindings are files that describe devicetree nodes. Devicetree nodes are
# usually mapped to bindings via their 'compatible = "..."' property.
#
# See the docstring/comments at the top of edtlib.py for more information.
#
# Note: Do not access private (_-prefixed) identifiers from edtlib here (and
# also note that edtlib is not meant to expose the dtlib API directly).
# Instead, think of what API you need, and add it as a public documented API in
# edtlib. This will keep this script simple.

import argparse
import os
import pathlib
import sys

import edtlib


def main():
    global conf_file
    global header_file
    global flash_area_num

    args = parse_args()

    try:
        edt = edtlib.EDT(args.dts, args.bindings_dirs,
                         # Suppress this warning if it's suppressed in dtc
                         warn_reg_unit_address_mismatch=
                             "-Wno-simple_bus_reg" not in args.dtc_flags)
    except edtlib.EDTError as e:
        sys.exit(f"devicetree error: {e}")

    # Save merged DTS source, as a debugging aid
    with open(args.dts_out, "w", encoding="utf-8") as f:
        print(edt.dts_source, file=f)

    conf_file = open(args.conf_out, "w", encoding="utf-8")
    header_file = open(args.header_out, "w", encoding="utf-8")
    flash_area_num = 0

    write_top_comment(edt)

    for node in sorted(edt.nodes, key=lambda node: node.dep_ordinal):
        write_node_comment(node)

        # Flash partition nodes are handled as a special case. It
        # would be nicer if we had bindings that would let us
        # avoid that, but this will do for now.
        if node.name.startswith("partition@"):
            write_flash_partition(node, flash_area_num)
            flash_area_num += 1

        if node.enabled and node.matching_compat:
            augment_node(node)
            write_regs(node)
            write_irqs(node)
            write_props(node)
            write_clocks(node)
            write_spi_dev(node)
            write_bus(node)
            write_existence_flags(node)

    out_comment("Compatibles appearing on enabled nodes")
    for compat in sorted(edt.compat2enabled):
        #define DT_COMPAT_<COMPAT> 1
        out(f"COMPAT_{str2ident(compat)}", 1)

    # Definitions derived from /chosen nodes
    write_addr_size(edt, "zephyr,ccm", "CCM")
    write_addr_size(edt, "zephyr,dtcm", "DTCM")
    write_addr_size(edt, "zephyr,ipc_shm", "IPC_SHM")
    write_flash(edt)

    conf_file.close()
    header_file.close()

    print(f"Devicetree header saved to '{args.header_out}'")


def parse_args():
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser()
    parser.add_argument("--dts", required=True, help="DTS file")
    parser.add_argument("--dtc-flags",
                        help="'dtc' devicetree compiler flags, some of which "
                             "might be respected here")
    parser.add_argument("--bindings-dirs", nargs='+', required=True,
                        help="directory with bindings in YAML format, "
                        "we allow multiple")
    parser.add_argument("--header-out", required=True,
                        help="path to write header to")
    parser.add_argument("--conf-out", required=True,
                        help="path to write configuration file to")
    parser.add_argument("--dts-out", required=True,
                        help="path to write merged DTS source code to (e.g. "
                             "as a debugging aid)")

    return parser.parse_args()


def augment_node(node):
    # Augment an EDT node with these zephyr-specific attributes, which
    # are used to generate macros from it:
    #
    # - z_primary_ident: a primary node identifier based on the node's
    #   compatible, plus information from its unit address (or its
    #   parent's unit address) or its name, and/or its bus.
    #
    # - z_other_idents: a list of other identifiers for the node,
    #   besides z_primary_ident.
    #
    # - z_idents: a list of all identifiers, containing the primary and other
    #   identifiers in that order.
    #
    # The z_other_idents list contains the following other attributes,
    # concatenated in this order:
    #
    # - z_inst_idents: node identifiers based on the index of the node
    #   within the EDT list of nodes for each compatible, e.g.:
    #   ["INST_3_<NODE's_COMPAT>",
    #    "INST_2_<NODE's_OTHER_COMPAT>"]
    #
    # - z_alias_idents: node identifiers based on any /aliases pointing to
    #   the node in the devicetree source, e.g.:
    #   ["DT_ALIAS_<NODE's_ALIAS_NAME>"]

    # Add the <COMPAT>_<UNIT_ADDRESS> style legacy identifier.
    node.z_primary_ident = node_ident(node)

    # Add z_instances, which are used to create these macros:
    #
    # #define DT_INST_<N>_<COMPAT>_<DEFINE> <VAL>
    inst_idents = []
    for compat in node.compats:
        instance_no = node.edt.compat2enabled[compat].index(node)
        inst_idents.append(f"INST_{instance_no}_{str2ident(compat)}")
    node.z_inst_idents = inst_idents

    # Add z_aliases, which are used to create these macros:
    #
    # #define DT_ALIAS_<ALIAS>_<DEFINE> <VAL>
    # #define DT_<COMPAT>_<ALIAS>_<DEFINE> <VAL>
    #
    # TODO: See if we can remove or deprecate the second form.
    compat_s = str2ident(node.matching_compat)
    alias_idents = []
    for alias in node.aliases:
        alias_ident = str2ident(alias)
        alias_idents.append(f"ALIAS_{alias_ident}")
        # NOTE: in some cases (e.g. PWM_LEDS_BLUE_PWM_LET for
        # hexiwear_k64) this is a collision with node.z_primary_ident,
        # making the all_idents checking below necessary.
        alias_idents.append(f"{compat_s}_{alias_ident}")
    node.z_alias_idents = alias_idents

    # z_other_idents are all the other identifiers for the node. We
    # use the term "other" instead of "alias" here because that
    # overlaps with the node's true aliases in the DTS, which is just
    # part of what makes up z_other_idents.
    all_idents = set()
    all_idents.add(node.z_primary_ident)
    other_idents = []
    for ident in node.z_inst_idents + node.z_alias_idents:
        if ident not in all_idents:
            other_idents.append(ident)
            all_idents.add(ident)
    node.z_other_idents = other_idents

    node.z_idents = [node.z_primary_ident] + node.z_other_idents


def for_each_ident(node, function):
    # Call "function" on node.z_primary_ident, then on each of the
    # identifiers in node.z_other_idents, in that order.

    function(node, node.z_primary_ident)
    for ident in node.z_other_idents:
        function(node, ident)


def write_top_comment(edt):
    # Writes an overview comment with misc. info at the top of the header and
    # configuration file

    s = f"""\
Generated by gen_defines.py

DTS input file:
  {edt.dts_path}

Directories with bindings:
  {", ".join(map(relativize, edt.bindings_dirs))}

Nodes in dependency order (ordinal and path):
"""

    for scc in edt.scc_order():
        if len(scc) > 1:
            err("cycle in devicetree involving "
                + ", ".join(node.path for node in scc))
        s += f"  {scc[0].dep_ordinal:<3} {scc[0].path}\n"

    s += """
Definitions derived from these nodes in dependency order are next,
followed by tree-wide information (active compatibles, chosen nodes,
etc.).
"""

    out_comment(s, blank_before=False)


def write_node_comment(node):
    # Writes a comment describing 'node' to the header and configuration file

    s = f"""\
Devicetree node:
  {node.path}
"""
    if node.matching_compat:
        s += f"""
Binding (compatible = {node.matching_compat}):
  {relativize(node.binding_path)}
"""
    else:
        s += "\nNo matching binding.\n"

    s += f"\nDependency Ordinal: {node.dep_ordinal}\n"

    if node.depends_on:
        s += "\nRequires:\n"
        for dep in node.depends_on:
            s += f"  {dep.dep_ordinal:<3} {dep.path}\n"

    if node.required_by:
        s += "\nSupports:\n"
        for req in node.required_by:
            s += f"  {req.dep_ordinal:<3} {req.path}\n"

    if node.description:
        # Indent description by two spaces
        s += "\nDescription:\n" + \
            "\n".join("  " + line for line in
                      node.description.splitlines()) + \
            "\n"

    if not node.enabled:
        s += "\nNode is disabled.\n"

    out_comment(s)


def relativize(path):
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


def write_regs(node):
    # Writes address/size output for the registers in the node's 'reg'
    # property. This is where the BASE_ADDRESS and SIZE macros come from.

    if not node.regs:
        return

    # This maps a reg_i (see below) to the "primary" BASE_ADDRESS and SIZE
    # macros used to identify it, which look like:
    #
    #   DT_<PRIMARY_NODE_IDENTIFIER>_BASE_ADDRESS_<reg_i>
    #   DT_<PRIMARY_NODE_IDENTIFIER>_SIZE_<reg_i>
    #
    # Or, for backwards compatibility if there's only one reg:
    #
    #   DT_<IDENT>_BASE_ADDRESS
    #   DT_<IDENT>_SIZE
    #
    # It's up to augment_node() to decide which identifier for the
    # node is its "primary" identifier, and which identifiers are
    # "other" identifiers. The "other" identifier BASE_ADDRESS and
    # SIZE macros are defined in terms of the "primary" ones.
    reg_i2primary_addr_size = {}

    def write_regs_for_ident(node, ident):
        # Write BASE_ADDRESS and SIZE macros for a given identifier
        # 'ident'. If we have already generated primary address and
        # size macros and saved in them in primary_addrs and
        # primary_sizes, we just reuse those. Otherwise (i.e. the
        # first time this is called), they are generated from the
        # actual reg.addr and reg.size attributes, and the names of
        # the primary macros are saved.

        for reg_i, reg in enumerate(node.regs):
            # DT_<IDENT>_BASE_ADDRESS_<reg_i>
            # DT_<IDENT>_SIZE_<reg_i>
            prim_addr, prim_size = reg_i2primary_addr_size.get(reg_i,
                                                               (None, None))
            suffix = f"_{reg_i}" if len(node.regs) > 1 else ""

            if prim_addr is not None:
                write_reg(ident, reg, prim_addr, prim_size,
                          "", suffix)
            else:
                prim_addr, prim_size = write_reg(ident, reg, None, None,
                                                 "", suffix)
                reg_i2primary_addr_size[reg_i] = (prim_addr, prim_size)

            # DT_<IDENT>_<reg.name>_BASE_ADDRESS
            # DT_<IDENT>_<reg.name>_SIZE
            if reg.name:
                write_reg(ident, reg, prim_addr, prim_size,
                          f"{str2ident(reg.name)}_", "")

    def write_reg(ident, reg, prim_addr, prim_size, prefix, suffix):
        addr = hex(reg.addr) if prim_addr is None else prim_addr
        size = reg.size if prim_size is None else prim_size

        addr_ret = out(f"{ident}_{prefix}BASE_ADDRESS{suffix}", addr)
        if size is not None and size != 0:
            size_ret = out(f"{ident}_{prefix}SIZE{suffix}", size)
        else:
            size_ret = None

        return (addr_ret, size_ret)

    out_comment("BASE_ADDRESS and SIZE macros from the 'reg' property",
                blank_before=False)
    for_each_ident(node, write_regs_for_ident)

def write_props(node):
    # Writes any properties defined in the "properties" section of the binding
    # for the node

    for prop in node.props.values():
        if not should_write(prop):
            continue

        if prop.description is not None:
            out_comment(prop.description, blank_before=False)

        ident = str2ident(prop.name)

        if prop.type == "boolean":
            out_node(node, ident, 1 if prop.val else 0)
        elif prop.type == "string":
            out_node_s(node, ident, prop.val)
        elif prop.type == "int":
            out_node(node, ident, prop.val)
        elif prop.type == "array":
            for i, val in enumerate(prop.val):
                out_node(node, f"{ident}_{i}", val)
            out_node_init(node, ident, prop.val)
        elif prop.type == "string-array":
            for i, val in enumerate(prop.val):
                out_node_s(node, f"{ident}_{i}", val)
        elif prop.type == "uint8-array":
            out_node_init(node, ident,
                          [f"0x{b:02x}" for b in prop.val])
        else:  # prop.type == "phandle-array"
            write_phandle_val_list(prop)

        # Generate DT_..._ENUM if there's an 'enum:' key in the binding
        if prop.enum_index is not None:
            out_node(node, ident + "_ENUM", prop.enum_index)


def should_write(prop):
    # write_props() helper. Returns True if output should be generated for
    # 'prop'.

    # Skip #size-cell and other property starting with #. Also skip mapping
    # properties like 'gpio-map'.
    if prop.name[0] == "#" or prop.name.endswith("-map"):
        return False

    # See write_clocks()
    if prop.name == "clocks":
        return False

    # For these, Property.val becomes an edtlib.Node, a list of edtlib.Nodes,
    # or None. Nothing is generated for them at the moment.
    if prop.type in {"phandle", "phandles", "path", "compound"}:
        return False

    # Skip properties that we handle elsewhere
    if prop.name in {
        "reg", "compatible", "status", "interrupts",
        "interrupt-controller", "gpio-controller"
    }:
        return False

    return True


def write_bus(node):
    # Generate bus-related #defines

    if not node.bus_node:
        return

    if node.bus_node.label is None:
        err(f"missing 'label' property on bus node {node.bus_node!r}")

    # #define DT_<DEV-IDENT>_BUS_NAME <BUS-LABEL>
    out_node_s(node, "BUS_NAME", str2ident(node.bus_node.label))

    for compat in node.compats:
        # #define DT_<COMPAT>_BUS_<BUS-TYPE> 1
        out(f"{str2ident(compat)}_BUS_{str2ident(node.on_bus)}", 1)


def write_existence_flags(node):
    # Generate #defines of the form
    #
    #   #define DT_INST_<instance no.>_<compatible string> 1
    #
    # for enabled nodes. These are flags for which devices exist.

    for compat in node.compats:
        instance_no = node.edt.compat2enabled[compat].index(node)
        out(f"INST_{instance_no}_{str2ident(compat)}", 1)


def node_ident(node):
    # Returns an identifier for 'node'. Used e.g. when building macro names.

    # TODO: Handle PWM on STM
    # TODO: Better document the rules of how we generate things

    ident = ""

    if node.bus_node:
        ident += "{}_{:X}_".format(
            str2ident(node.bus_node.matching_compat), node.bus_node.unit_addr)

    ident += f"{str2ident(node.matching_compat)}_"

    if node.unit_addr is not None:
        ident += f"{node.unit_addr:X}"
    elif node.parent.unit_addr is not None:
        ident += f"{node.parent.unit_addr:X}_{str2ident(node.name)}"
    else:
        # This is a bit of a hack
        ident += str2ident(node.name)

    return ident


def node_aliases(node):
    # Returns a list of aliases for 'node', used e.g. when building macro names

    return node_path_aliases(node) + node_instance_aliases(node)


def node_path_aliases(node):
    # Returns a list of aliases for 'node', based on the aliases registered for
    # it in the /aliases node. Used e.g. when building macro names.

    if node.matching_compat is None:
        return []

    compat_s = str2ident(node.matching_compat)

    aliases = []
    for alias in node.aliases:
        aliases.append(f"ALIAS_{str2ident(alias)}")
        # TODO: See if we can remove or deprecate this form
        aliases.append(f"{compat_s}_{str2ident(alias)}")

    return aliases


def node_instance_aliases(node):
    # Returns a list of aliases for 'node', based on the compatible string and
    # the instance number (each node with a particular compatible gets its own
    # instance number, starting from zero).
    #
    # This is a list since a node can have multiple 'compatible' strings, each
    # with their own instance number.

    res = []
    for compat in node.compats:
        instance_no = node.edt.compat2enabled[compat].index(node)
        res.append(f"INST_{instance_no}_{str2ident(compat)}")
    return res


def write_addr_size(edt, prop_name, prefix):
    # Writes <prefix>_BASE_ADDRESS and <prefix>_SIZE for the node pointed at by
    # the /chosen property named 'prop_name', if it exists

    node = edt.chosen_node(prop_name)
    if not node:
        return

    if not node.regs:
        err("missing 'reg' property in node pointed at by "
            f"/chosen/{prop_name} ({node!r})")

    out_comment(f"/chosen/{prop_name} ({node.path})")
    out(f"{prefix}_BASE_ADDRESS", hex(node.regs[0].addr))
    out(f"{prefix}_SIZE", node.regs[0].size//1024)


def write_flash(edt):
    # Writes chosen and tree-wide flash-related output

    write_flash_node(edt)
    write_code_partition(edt)

    if flash_area_num != 0:
        out_comment("Number of flash partitions")
        out("FLASH_AREA_NUM", flash_area_num)


def write_flash_node(edt):
    # Writes output for the top-level flash node pointed at by
    # zephyr,flash in /chosen

    node = edt.chosen_node("zephyr,flash")

    out_comment(f"/chosen/zephyr,flash ({node.path if node else 'missing'})")

    if not node:
        # No flash node. Write dummy values.
        out("FLASH_BASE_ADDRESS", 0)
        out("FLASH_SIZE", 0)
        return

    if len(node.regs) != 1:
        err("expected zephyr,flash to have a single register, has "
            f"{len(node.regs)}")

    if node.on_bus == "spi" and len(node.bus_node.regs) == 2:
        reg = node.bus_node.regs[1]  # QSPI flash
    else:
        reg = node.regs[0]

    out("FLASH_BASE_ADDRESS", hex(reg.addr))
    if reg.size:
        out("FLASH_SIZE", reg.size//1024)

    if "erase-block-size" in node.props:
        out("FLASH_ERASE_BLOCK_SIZE", node.props["erase-block-size"].val)

    if "write-block-size" in node.props:
        out("FLASH_WRITE_BLOCK_SIZE", node.props["write-block-size"].val)


def write_code_partition(edt):
    # Writes output for the node pointed at by zephyr,code-partition in /chosen

    node = edt.chosen_node("zephyr,code-partition")

    out_comment("/chosen/zephyr,code-partition "
                f"({node.path if node else 'missing'})")

    if not node:
        # No code partition. Write dummy values.
        out("CODE_PARTITION_OFFSET", 0)
        out("CODE_PARTITION_SIZE", 0)
        return

    if not node.regs:
        err(f"missing 'regs' property on {node!r}")

    out("CODE_PARTITION_OFFSET", node.regs[0].addr)
    out("CODE_PARTITION_SIZE", node.regs[0].size)


def write_flash_partition(partition_node, index):
    if partition_node.label is None:
        err(f"missing 'label' property on {partition_node!r}")

    # Generate label-based identifiers
    write_flash_partition_prefix(
        "FLASH_AREA_" + str2ident(partition_node.label), partition_node, index)

    # Generate index-based identifiers
    write_flash_partition_prefix(f"FLASH_AREA_{index}", partition_node, index)


def write_flash_partition_prefix(prefix, partition_node, index):
    # write_flash_partition() helper. Generates identifiers starting with
    # 'prefix'.

    out(f"{prefix}_ID", index)

    out(f"{prefix}_READ_ONLY", 1 if partition_node.read_only else 0)

    for i, reg in enumerate(partition_node.regs):
        # Also add aliases that point to the first sector (TODO: get rid of the
        # aliases?)
        out(f"{prefix}_OFFSET_{i}", reg.addr,
            aliases=[f"{prefix}_OFFSET"] if i == 0 else [])
        out(f"{prefix}_SIZE_{i}", reg.size,
            aliases=[f"{prefix}_SIZE"] if i == 0 else [])

    controller = partition_node.flash_controller
    if controller.label is not None:
        out_s(f"{prefix}_DEV", controller.label)


def write_irqs(node):
    # Writes IRQ num and data for the interrupts in the node's 'interrupt'
    # property

    if not node.interrupts:
        return

    # maps (irq_i, cell_name) to the primary macro for that pair,
    # if there is one.
    irq_cell2primary_macro = {}

    def write_irqs_for_ident(node, ident):
        # Like write_regs_for_ident(), but for interrupts.

        for irq_i, irq in enumerate(node.interrupts):
            for cell_name, cell_value in irq.data.items():
                if cell_name == "irq":
                    if "arm,gic" in irq.controller.compats:
                        cell_value = map_arm_gic_irq_type(irq, cell_value)
                    cell_value = encode_zephyr_multi_level_irq(irq, cell_value)
                    name_suffix = ""
                else:
                    name_suffix = f"_{str2ident(cell_name)}"

                # DT_<IDENT>_IRQ_<irq_i>[_<CELL_NAME>]
                idx_macro = f"{ident}_IRQ_{irq_i}{name_suffix}"
                primary_macro = irq_cell2primary_macro.get((irq_i, cell_name))
                if primary_macro is not None:
                    out(idx_macro, primary_macro)
                else:
                    primary_macro = out(idx_macro, cell_value)
                    irq_cell2primary_macro[(irq_i, cell_name)] = primary_macro

                # DT_<IDENT>_IRQ_<IRQ_NAME>[_<CELL_NAME>]
                if irq.name:
                    out(f"{ident}_IRQ_{str2ident(irq.name)}{name_suffix}",
                        primary_macro)

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

    def encode_zephyr_multi_level_irq(irq, irq_num):
        # See doc/reference/kernel/other/interrupts.rst for details
        # on how this encoding works

        irq_ctrl = irq.controller
        # Look for interrupt controller parent until we have none
        while irq_ctrl.interrupts:
            irq_num = (irq_num + 1) << 8
            if "irq" not in irq_ctrl.interrupts[0].data:
                err(f"Expected binding for {irq_ctrl!r} to have 'irq' in "
                    "interrupt-cells")
            irq_num |= irq_ctrl.interrupts[0].data["irq"]
            irq_ctrl = irq_ctrl.interrupts[0].controller
        return irq_num

    out_comment("IRQ macros from the 'interrupts' property",
                blank_before=False)
    for_each_ident(node, write_irqs_for_ident)


def write_spi_dev(node):
    # Writes SPI device GPIO chip select data if there is any

    cs_gpio = node.spi_cs_gpio
    if cs_gpio is not None:
        write_phandle_val_list_entry(node, cs_gpio, None, "CS_GPIOS")


def write_phandle_val_list(prop):
    # Writes output for a phandle/value list, e.g.
    #
    #    pwms = <&pwm-ctrl-1 10 20
    #            &pwm-ctrl-2 30 40>;
    #
    # prop:
    #   phandle/value Property instance.
    #
    #   If only one entry appears in 'prop' (the example above has two), the
    #   generated identifier won't get a '_0' suffix, and the '_COUNT' and
    #   group initializer are skipped too.
    #
    # The base identifier is derived from the property name. For example, 'pwms = ...'
    # generates output like this:
    #
    #   #define <node prefix>_PWMS_CONTROLLER_0 "PWM_0"  (name taken from 'label = ...')
    #   #define <node prefix>_PWMS_CHANNEL_0 123         (name taken from *-cells in binding)
    #   #define <node prefix>_PWMS_0 {"PWM_0", 123}
    #   #define <node prefix>_PWMS_CONTROLLER_1 "PWM_1"
    #   #define <node prefix>_PWMS_CHANNEL_1 456
    #   #define <node prefix>_PWMS_1 {"PWM_1", 456}
    #   #define <node prefix>_PWMS_COUNT 2
    #   #define <node prefix>_PWMS {<node prefix>_PWMS_0, <node prefix>_PWMS_1}
    #   ...

    # pwms -> PWMS
    # foo-gpios -> FOO_GPIOS
    ident = str2ident(prop.name)

    initializer_vals = []
    for i, entry in enumerate(prop.val):
        initializer_vals.append(write_phandle_val_list_entry(
            prop.node, entry, i if len(prop.val) > 1 else None, ident))

    if len(prop.val) > 1:
        out_node(prop.node, ident + "_COUNT", len(initializer_vals))
        out_node_init(prop.node, ident, initializer_vals)


def write_phandle_val_list_entry(node, entry, i, ident):
    # write_phandle_val_list() helper. We could get rid of it if it wasn't for
    # write_spi_dev(). Adds 'i' as an index to identifiers unless it's None.
    #
    # 'entry' is an edtlib.ControllerAndData instance.
    #
    # Returns the identifier for the macro that provides the
    # initializer for the entire entry.

    initializer_vals = []
    if entry.controller.label is not None:
        ctrl_ident = ident + "_CONTROLLER"  # e.g. PWMS_CONTROLLER
        if entry.name:
            name_alias = f"{str2ident(entry.name)}_{ctrl_ident}"
        else:
            name_alias = None
        # Ugly backwards compatibility hack. Only add the index if there's
        # more than one entry.
        if i is not None:
            ctrl_ident += f"_{i}"
        initializer_vals.append(quote_str(entry.controller.label))
        out_node_s(node, ctrl_ident, entry.controller.label, name_alias)

    for cell, val in entry.data.items():
        cell_ident = f"{ident}_{str2ident(cell)}"  # e.g. PWMS_CHANNEL
        if entry.name:
            # From e.g. 'pwm-names = ...'
            name_alias = f"{str2ident(entry.name)}_{cell_ident}"
        else:
            name_alias = None
        # Backwards compatibility (see above)
        if i is not None:
            cell_ident += f"_{i}"
        out_node(node, cell_ident, val, name_alias)

    initializer_vals += entry.data.values()

    initializer_ident = ident
    if entry.name:
        name_alias = f"{initializer_ident}_{str2ident(entry.name)}"
    else:
        name_alias = None
    if i is not None:
        initializer_ident += f"_{i}"
    return out_node_init(node, initializer_ident, initializer_vals, name_alias)


def write_clocks(node):
    # Writes clock information.
    #
    # Like write_regs(), but for clocks.
    #
    # Most of this ought to be handled in write_props(), but the identifiers
    # that get generated for 'clocks' are inconsistent with the with other
    # 'phandle-array' properties.
    #
    # See https://github.com/zephyrproject-rtos/zephyr/pull/19327#issuecomment-534081845.

    if "clocks" not in node.props:
        return

    # Maps a clock_i to (primary_controller, primary_data,
    # primary_frequency) macros for that clock index
    clock_i2primary_cdf = {}

    def write_clocks_for_ident(node, ident):
        clocks = node.props["clocks"].val
        for clock_i, clock in enumerate(clocks):
            primary_cdf = clock_i2primary_cdf.get(clock_i)
            if primary_cdf is None:
                # Print the primary macros and save them for future use
                clock_i2primary_cdf[clock_i] = primary_cdf = \
                    write_clock_primary(ident, clock_i, clock)
            else:
                # Print other macros in terms of primary macros
                write_clock_other(ident, clock_i, clock, primary_cdf)

    def write_clock_primary(ident, clock_i, clock):
        # Write clock macros for the primary identifier 'ident'.
        # Return a (primary_controller, primary_data,
        # primary_frequency) tuple to use for other idents.

        controller = clock.controller
        # If the clock controller has a label property:
        # DT_<IDENT>_CLOCK_CONTROLLER <LABEL_PROPERTY>
        if controller.label:
            # FIXME this is replicating previous behavior that didn't
            # generate an indexed controller bug-for-bug. There's a
            # missing _{clock_i} at the end of the f-string.
            prim_controller = out_s(f"{ident}_CLOCK_CONTROLLER",
                                    controller.label)
        else:
            prim_controller = None

        # For each additional cell in the controller + data:
        # DT_<IDENT>_CLOCK_<CELL_NAME>_<clock_i> <CELL_VALUE>
        #
        # Cell names are from the controller binding's "clock-cells".
        prim_data = []
        for name, val in clock.data.items():
            prim_macro = out(f"{ident}_CLOCK_{str2ident(name)}_{clock_i}", val)
            prim_data.append(prim_macro)
            if clock_i == 0:
                out(f"{ident}_CLOCK_{str2ident(name)}", prim_macro)

        # If the clock has a "fixed-clock" compat:
        # DT_<IDENT>_CLOCKS_CLOCK_FREQUENCY_{clock_i} <CLOCK'S_FREQ>
        if "fixed-clock" in controller.compats:
            if "clock-frequency" not in controller.props:
                err(f"{controller!r} is a 'fixed-clock' but lacks a "
                    "'clock-frequency' property")
            # FIXME like the CLOCK_CONTROLLER, this is missing a clock_i.
            # We need to go bug-for-bug with the previous implementation
            # to make sure there are no differences before fixing.
            prim_freq = out(f"{ident}_CLOCKS_CLOCK_FREQUENCY",
                            controller.props["clock-frequency"].val)
        else:
            prim_freq = None

        return (prim_controller, prim_data, prim_freq)

    def write_clock_other(ident, clock_i, clock, primary_cdf):
        # Write clock macros for a secondary identifier 'ident'

        prim_controller, prim_data, prim_freq = primary_cdf
        if prim_controller is not None:
            # FIXME this is replicating previous behavior that didn't
            # generate an indexed controller bug-for-bug. There's a
            # missing _{clock_i} at the end of the f-string.
            out(f"{ident}_CLOCK_CONTROLLER", prim_controller)
        for name_i, name in enumerate(clock.data.keys()):
            out(f"{ident}_CLOCK_{str2ident(name)}_{clock_i}", prim_data[name_i])
            if clock_i == 0:
                out(f"{ident}_CLOCK_{str2ident(name)}", prim_data[name_i])
        if prim_freq is not None:
            # FIXME this is also a bug-for-bug match with the previous
            # implementation.
            out(f"{ident}_CLOCKS_CLOCK_FREQUENCY", prim_freq)

    out_comment("Clock gate macros from the 'clocks' property",
                blank_before=False)
    for_each_ident(node, write_clocks_for_ident)


def str2ident(s):
    # Converts 's' to a form suitable for (part of) an identifier

    return s.replace("-", "_") \
            .replace(",", "_") \
            .replace("@", "_") \
            .replace("/", "_") \
            .replace(".", "_") \
            .replace("+", "PLUS") \
            .upper()


def out_node(node, ident, val, name_alias=None, deprecation_msg=None):
    # Writes a
    #
    #   <node prefix>_<ident> = <val>
    #
    # assignment, along with a set of
    #
    #   <node alias>_<ident>
    #
    # aliases, for each path/instance alias for the node. If 'name_alias' (a
    # string) is passed, then these additional aliases are generated:
    #
    #   <node prefix>_<name alias>
    #   <node alias>_<name alias> (for each node alias)
    #
    # 'name_alias' is used for reg-names and the like.
    #
    # If a 'deprecation_msg' string is passed, the generated identifiers will
    # generate a warning if used, via __WARN(<deprecation_msg>)).
    #
    # Returns the identifier used for the macro that provides the value
    # for 'ident' within 'node', e.g. DT_MFG_MODEL_CTL_GPIOS_PIN.

    node_prefix = node_ident(node)

    aliases = [f"{alias}_{ident}" for alias in node_aliases(node)]
    if name_alias is not None:
        aliases.append(f"{node_prefix}_{name_alias}")
        aliases += [f"{alias}_{name_alias}" for alias in node_aliases(node)]

    return out(f"{node_prefix}_{ident}", val, aliases, deprecation_msg)


def out_node_s(node, ident, s, name_alias=None, deprecation_msg=None):
    # Like out_node(), but emits 's' as a string literal
    #
    # Returns the generated macro name for 'ident'.

    return out_node(node, ident, quote_str(s), name_alias, deprecation_msg)


def out_node_init(node, ident, elms, name_alias=None, deprecation_msg=None):
    # Like out_node(), but generates an {e1, e2, ...} initializer with the
    # elements in the iterable 'elms'.
    #
    # Returns the generated macro name for 'ident'.

    return out_node(node, ident, "{" + ", ".join(map(str, elms)) + "}",
                    name_alias, deprecation_msg)


def out_s(ident, val):
    # Like out(), but puts quotes around 'val' and escapes any double
    # quotes and backslashes within it
    #
    # Returns the generated macro name for 'ident'.

    return out(ident, quote_str(val))


def out(ident, val, aliases=(), deprecation_msg=None):
    # Writes '#define <ident> <val>' to the header and '<ident>=<val>' to the
    # the configuration file.
    #
    # Also writes any aliases listed in 'aliases' (an iterable). For the
    # header, these look like '#define <alias> <ident>'. For the configuration
    # file, the value is just repeated as '<alias>=<val>' for each alias.
    #
    # See out_node() for the meaning of 'deprecation_msg'.
    #
    # Returns the generated macro name for 'ident'.

    out_define(ident, val, deprecation_msg, header_file)
    primary_ident = f"DT_{ident}"

    # Exclude things that aren't single token values from .conf.  At
    # the moment the only such items are unquoted string
    # representations of initializer lists, which begin with a curly
    # brace.
    output_to_conf = not (isinstance(val, str) and val.startswith("{"))
    if output_to_conf:
        print(f"{primary_ident}={val}", file=conf_file)

    for alias in aliases:
        if alias != ident:
            out_define(alias, "DT_" + ident, deprecation_msg, header_file)
            if output_to_conf:
                # For the configuration file, the value is just repeated for all
                # the aliases
                print(f"DT_{alias}={val}", file=conf_file)

    return primary_ident


def out_define(ident, val, deprecation_msg, out_file):
    # out() helper for writing a #define. See out_node() for the meaning of
    # 'deprecation_msg'.

    s = f"#define DT_{ident:40}"
    if deprecation_msg:
        s += fr' __WARN("{deprecation_msg}")'
    s += f" {val}"
    print(s, file=out_file)


def out_comment(s, blank_before=True):
    # Writes 's' as a comment to the header and configuration file. 's' is
    # allowed to have multiple lines. blank_before=True adds a blank line
    # before the comment.

    if blank_before:
        print(file=header_file)
        print(file=conf_file)

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

    print("\n".join("# " + line if line.strip() else "#"
                    for line in s.splitlines()), file=conf_file)


def escape(s):
    # Backslash-escapes any double quotes and backslashes in 's'

    # \ must be escaped before " to avoid double escaping
    return s.replace("\\", "\\\\").replace('"', '\\"')


def quote_str(s):
    # Puts quotes around 's' and escapes any double quotes and
    # backslashes within it

    return f'"{escape(s)}"'


def err(s):
    raise Exception(s)


if __name__ == "__main__":
    main()
