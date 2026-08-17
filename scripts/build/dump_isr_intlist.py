#!/usr/bin/env python3
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""Print a decoded view of the .intList section of a Zephyr pre-link ELF.

Companion of gen_isr_tables.py for multi-level interrupt configurations:
shows every statically connected interrupt (IRQ_CONNECT/Z_ISR_DECLARE) with
its multilevel decomposition (level, local IRQ, level-1 line), the connect
flags, the _sw_isr_table slot it is placed in and the resolved handler symbol.

The slot is computed by gen_isr_tables.py's own placement code, so it is the
slot the generated table really uses. A map of the aggregator windows follows,
one line per aggregator with the slot range it owns and how many of the
connected interrupts land in it, so the window arithmetic can be checked
without reading the generated table.
"""

import argparse
import struct
import sys

from elftools.elf.elffile import ELFFile
from gen_isr_tables import gen_isr_config, gen_isr_log

ISR_FLAG_DIRECT = 1 << 0


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
                elif sym.entry.st_value and sym.name and sym.entry.st_shndx != "SHN_ABS":
                    # Absolute symbols (struct offsets and the like) are not
                    # addresses, so they would only mislabel small arguments.
                    # A function or object wins over a linker label, such as
                    # a section start, at the same address.
                    if sym.entry.st_info.type in ("STT_FUNC", "STT_OBJECT"):
                        syms[sym.entry.st_value] = sym.name
                    else:
                        syms.setdefault(sym.entry.st_value, sym.name)

        return data, syms, cfg, elf.little_endian


def max_irq_per_aggregator(cfg, level):
    """Window size of a level-@level aggregator, as gen_isr_tables.py reads it."""
    name = "2ND" if level == 2 else "3RD"
    return cfg.get(f"CONFIG_MAX_IRQ_PER_{name}_LEVEL_AGGREGATOR") or cfg.get(
        "CONFIG_MAX_IRQ_PER_AGGREGATOR", 0
    )


def describe_windows(cfg, num_vectors, offset, slots):
    """Map the fixed-width aggregator windows of _sw_isr_table.

    Each level-N aggregator owns MAX_IRQ_PER_<N>_LEVEL_AGGREGATOR consecutive
    slots from CONFIG_<N>_LVL_ISR_TBL_OFFSET, picked by its position in the
    CONFIG_<N>_LVL_INTR_NN_OFFSET list. @slots are the _sw_isr_table indices of
    the connected interrupts, counted per window. Like the generated table, all
    indices are relative to @offset (CONFIG_GEN_IRQ_START_VECTOR).
    """
    if not cfg.get("CONFIG_2ND_LEVEL_INTERRUPTS"):
        return []

    table_size = num_vectors - offset
    l2_base = cfg.get("CONFIG_2ND_LVL_ISR_TBL_OFFSET", 0) - offset
    out = ["", f"level 1:  slots    0 .. {l2_base - 1}"]

    levels = [(2, "CONFIG_NUM_2ND_LEVEL_AGGREGATORS", "line")]
    if cfg.get("CONFIG_3RD_LEVEL_INTERRUPTS"):
        levels.append((3, "CONFIG_NUM_3RD_LEVEL_AGGREGATORS", "level-2 IRQ"))

    region_end = {}
    for level, num_sym, parent in levels:
        name = "2ND" if level == 2 else "3RD"
        base = cfg.get(f"CONFIG_{name}_LVL_ISR_TBL_OFFSET", 0) - offset
        size = max_irq_per_aggregator(cfg, level)
        for agg in range(cfg.get(num_sym, 0)):
            start = base + agg * size
            end = start + size - 1
            used = sum(1 for slot in slots if start <= slot <= end)
            parent_irq = cfg.get(f"CONFIG_{name}_LVL_INTR_{agg:02}_OFFSET")
            out.append(
                f"level {level}:  slots {start:>4} .. {end:<4} aggregator {agg}"
                f" on {parent} {parent_irq}, {used} connected"
            )
            region_end[level] = end + 1

    l3_base = cfg.get("CONFIG_3RD_LVL_ISR_TBL_OFFSET", 0) - offset
    if 3 in region_end and region_end.get(2, 0) > l3_base:
        out.append(
            f"ERROR: level 2 windows end at {region_end[2]}, past the level 3 base {l3_base}"
        )
    last = max(region_end.values(), default=0)
    if last > table_size:
        out.append(f"ERROR: windows end at {last}, past the table ({table_size})")

    return out


def write_dump(lines, args):
    text = "\n".join(lines)
    print(text)
    if args.output:
        with open(args.output, "w") as fp:
            fp.write(text + "\n")


def main():
    args = parse_args()
    data, syms, cfg, little_endian = load_elf(args.kernel, args.intlist_section)

    if cfg.get("CONFIG_ISR_TABLES_LOCAL_DECLARATION"):
        # That intlist holds handler section names, not addresses
        write_dump(["intlist: CONFIG_ISR_TABLES_LOCAL_DECLARATION layout, not decoded"], args)
        return

    prefix = "<" if little_endian else ">"
    l1_bits = cfg.get("CONFIG_1ST_LEVEL_INTERRUPT_BITS", 8)
    l2_bits = cfg.get("CONFIG_2ND_LEVEL_INTERRUPT_BITS", 8)
    l1_mask = (1 << l1_bits) - 1
    l2_mask = ((1 << l2_bits) - 1) << l1_bits

    # The placement code of gen_isr_tables.py, fed the same configuration.
    gen_args = argparse.Namespace(
        sw_isr_table=True, intlist_section=args.intlist_section, big_endian=not little_endian
    )
    placement = gen_isr_config(gen_args, cfg, gen_isr_log())

    # struct _isr_list is {int32 irq; int32 flags; void *func; const void *param;}
    entry_fmt = prefix + ("iiQQ" if cfg.get("CONFIG_64BIT") else "iiII")

    num_vectors, offset = struct.unpack_from(prefix + "II", data, 0)
    header_sz = struct.calcsize(prefix + "II")

    lines = [
        f"intlist: num_vectors={num_vectors} offset={offset}",
        f"{'irq':>10} {'lvl':>3} {'local':>5} {'line':>4} {'flags':>7} {'slot':>6}"
        "  handler (argument)",
    ]

    slots = []
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
        index = placement.get_swt_table_index(offset, irq)
        if flags & ISR_FLAG_DIRECT:
            # Direct ISRs go to the vector table, not _sw_isr_table
            slot = f"vt:{index}"
        else:
            slot = str(index)
            slots.append(index)
        handler = syms.get(func, hex(func))
        argument = syms.get(param, hex(param))
        lines.append(
            f"{irq:#10x} {level:>3} {local:>5} {line:>4} {flags:#7x} {slot:>6}"
            f"  {handler} ({argument})"
        )

    lines.extend(describe_windows(cfg, num_vectors, offset, slots))
    write_dump(lines, args)


if __name__ == "__main__":
    main()
