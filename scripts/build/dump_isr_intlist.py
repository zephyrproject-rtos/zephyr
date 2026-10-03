#!/usr/bin/env python3
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""Print a decoded view of the .intList section of a Zephyr pre-link ELF.

Companion of gen_isr_tables.py for multi-level interrupt configurations:
shows every statically connected interrupt (IRQ_CONNECT/Z_ISR_DECLARE) with
its multilevel decomposition (level, local IRQ, level-1 line), the connect
flags and the resolved handler symbol.

It then replays the placement decisions gen_isr_tables.py makes for the
interrupt-matrix layout, so they can be checked without reading the generated
table: the V / S / P region map, which CPU lines need the 2nd-level dispatcher,
and each 3rd-level aggregator's derived status mask and densely packed window.
"""

import argparse
import struct
import sys

from elftools.elf.elffile import ELFFile


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--kernel", required=True, help="Pre-link kernel ELF (zephyr_preX.elf)")
    parser.add_argument(
        "--intlist-section",
        action="append",
        required=True,
        help="Section name holding the intlist; may be repeated",
    )
    parser.add_argument("--output", help="Also write the dump to this file")
    return parser.parse_args()


def load_elf(kernel, section_names):
    with open(kernel, "rb") as fp:
        elf = ELFFile(fp)

        section = None
        for name in section_names:
            section = elf.get_section_by_name(name)
            if section is not None:
                break
        if section is None:
            sys.exit(f"no intlist section ({', '.join(section_names)}) in {kernel}")
        data = section.data()

        syms = {}
        cfg = {}
        for sec in elf.iter_sections():
            if sec.name != ".symtab":
                continue
            for sym in sec.iter_symbols():
                if sym.name.startswith("CONFIG_"):
                    cfg[sym.name] = sym.entry.st_value
                elif sym.entry.st_value and sym.name:
                    syms.setdefault(sym.entry.st_value, sym.name)

        return data, syms, cfg, elf.little_endian


def describe_layout(cfg, num_vectors, line_srcs, l3_groups):
    """Replay gen_isr_tables.py's placement for the interrupt-matrix layout.

    Mirrors gen_isr_config.note_multilevel_topology(): a CPU line vectors through
    the 2nd-level dispatcher when it carries two or more sources or hosts a
    3rd-level aggregator, and each aggregator's window is packed densely, one
    slot per connected status bit, in ascending source order.

    Gated on the same CONFIG_INTERRUPT_MATRIX_LAYOUT the generator reads, so a
    platform on the fixed-width windows gets the raw intlist and no layout map
    that would not match its table.
    """
    if not cfg.get("CONFIG_INTERRUPT_MATRIX_LAYOUT"):
        return []

    l2_base = cfg.get("CONFIG_2ND_LVL_ISR_TBL_OFFSET", 0)
    l3_base = cfg.get("CONFIG_3RD_LVL_ISR_TBL_OFFSET", 0)

    out = ["", f"V: level-1 CPU lines      0 .. {l2_base - 1}"]
    if l3_base:
        out.append(f"S: level-2 sources        {l2_base} .. {l3_base - 1} (slot = {l2_base} + src)")
        out.append(f"P: level-3 signals        {l3_base} .. {num_vectors - 1}")
    else:
        out.append(f"S: level-2 sources        {l2_base} .. {num_vectors - 1}")

    dispatched = sorted(
        line
        for line in line_srcs
        if len(line_srcs[line]) >= 2 or any(key[0] == line for key in l3_groups)
    )
    if dispatched:
        out.append("")
        out.append("level-1 lines vectoring through z_soc_2nd_lvl_isr:")
        for line in dispatched:
            reason = "hosts a 3rd-level aggregator"
            if len(line_srcs[line]) >= 2:
                reason = f"{len(line_srcs[line])} sources"
            out.append(f"  line {line:>3}: {reason}")

    if l3_groups:
        out.append("")
        out.append("3rd-level aggregators (z_soc_3rd_lvl_isr in their S slot):")
        # The catch-all leaf owns no status bit, so it stays out of the mask and
        # takes the slot after the masked ones. Derived exactly as
        # gen_isr_config.is_l3_catch_all() derives it.
        catch_all_bit = (1 << cfg.get("CONFIG_3RD_LEVEL_INTERRUPT_BITS", 8)) - 2
        win_base = l3_base
        for index, key in enumerate(sorted(l3_groups, key=lambda k: (k[1], k[0]))):
            all_bits = sorted(l3_groups[key])
            bits = [bit for bit in all_bits if bit != catch_all_bit]
            mask = sum(1 << bit for bit in bits)
            suffix = " + catch-all" if catch_all_bit in all_bits else ""
            out.append(
                f"  window {index}: source {key[1]} on line {key[0]}, mask {mask:#010x},"
                f" slots {win_base}..{win_base + len(all_bits) - 1}"
                f" for bits {bits}{suffix}"
            )
            win_base += len(all_bits)
        if win_base > num_vectors:
            out.append(f"  ERROR: windows end at {win_base}, past the table ({num_vectors})")

    return out


def main():
    args = parse_args()
    data, syms, cfg, little_endian = load_elf(args.kernel, args.intlist_section)

    prefix = "<" if little_endian else ">"
    l1_bits = cfg.get("CONFIG_1ST_LEVEL_INTERRUPT_BITS", 8)
    l2_bits = cfg.get("CONFIG_2ND_LEVEL_INTERRUPT_BITS", 8)
    l1_mask = (1 << l1_bits) - 1
    l2_mask = ((1 << l2_bits) - 1) << l1_bits

    # struct _isr_list is {int32 irq; int32 flags; void *func; const void *param;}
    entry_fmt = prefix + ("iiQQ" if cfg.get("CONFIG_64BIT") else "iiII")

    num_vectors, offset = struct.unpack_from(prefix + "II", data, 0)
    header_sz = struct.calcsize(prefix + "II")

    lines = [
        f"intlist: num_vectors={num_vectors} offset={offset}",
        f"{'irq':>10} {'lvl':>3} {'local':>5} {'line':>4} {'flags':>7}  handler (argument)",
    ]

    # line -> set of level-2 sources; (line, src) -> set of level-3 bits
    line_srcs = {}
    l3_groups = {}
    for irq, flags, func, param in struct.iter_unpack(entry_fmt, data[header_sz:]):
        l3 = irq >> (l1_bits + l2_bits)
        l2 = (irq & l2_mask) >> l1_bits
        line = irq & l1_mask
        if l3:
            level, local = 3, l3 - 1
        elif l2:
            level, local = 2, l2 - 1
        else:
            level, local = 1, line
        if l2:
            line_srcs.setdefault(line, set()).add(l2 - 1)
            if l3:
                l3_groups.setdefault((line, l2 - 1), set()).add(l3 - 1)
        handler = syms.get(func, hex(func))
        argument = syms.get(param, hex(param))
        lines.append(
            f"{irq:#10x} {level:>3} {local:>5} {line:>4} {flags:#7x}  {handler} ({argument})"
        )

    lines.extend(describe_layout(cfg, num_vectors, line_srcs, l3_groups))

    text = "\n".join(lines)
    print(text)
    if args.output:
        with open(args.output, "w") as fp:
            fp.write(text + "\n")


if __name__ == "__main__":
    main()
