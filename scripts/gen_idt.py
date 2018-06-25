#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys
import struct
import os
import elftools
from distutils.version import LooseVersion
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if LooseVersion(elftools.__version__) < LooseVersion('0.24'):
    sys.stderr.write("pyelftools is out of date, need version 0.24 or later\n")
    sys.exit(1)

# This will never change, first selector in the GDT after the null selector
KERNEL_CODE_SEG = 0x08

# These exception vectors push an error code onto the stack.
ERR_CODE_VECTORS = [8, 10, 11, 12, 13, 14, 17]


def debug(text):
    if not args.verbose:
        return
    sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def error(text):
    sys.stderr.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")
    sys.exit(1)


# See Section 6.11 of the Intel Architecture Software Developer's Manual
gate_desc_format = "<HHBBH"


def create_irq_gate(handler, dpl):
    present = 1
    gate_type = 0xE  # 32-bit interrupt gate
    type_attr = gate_type | (dpl << 5) | (present << 7)

    offset_hi = handler >> 16
    offset_lo = handler & 0xFFFF

    data = struct.pack(gate_desc_format, offset_lo, KERNEL_CODE_SEG, 0,
                       type_attr, offset_hi)
    return data


def create_task_gate(tss, dpl):
    present = 1
    gate_type = 0x5  # 32-bit task gate
    type_attr = gate_type | (dpl << 5) | (present << 7)

    data = struct.pack(gate_desc_format, 0, tss, 0, type_attr, 0)
    return data


def create_idt_binary(idt_config, filename):
    with open(filename, "wb") as fp:
        for handler, tss, dpl in idt_config:
            if handler and tss:
                error("entry specifies both handler function and tss")

            if not handler and not tss:
                error("entry does not specify either handler or tss")

            if handler:
                data = create_irq_gate(handler, dpl)
            else:
                data = create_task_gate(tss, dpl)

            fp.write(data)


map_fmt = "<B"


def create_irq_vec_map_binary(irq_vec_map, filename):
    with open(filename, "wb") as fp:
        for i in irq_vec_map:
            fp.write(struct.pack(map_fmt, i))


def priority_range(prio):
    # Priority levels are represented as groups of 16 vectors within the IDT
    base = 32 + (prio * 16)
    return range(base, base + 16)


def update_irq_vec_map(irq_vec_map, irq, vector, max_irq):
    # No IRQ associated; exception or software interrupt
    if irq == -1:
        return

    if irq >= max_irq:
        error("irq %d specified, but CONFIG_MAX_IRQ_LINES is %d" %
              (irq, max_irq))

    # This table will never have values less than 32 since those are for
    # exceptions; 0 means unconfigured
    if irq_vec_map[irq] != 0:
        error("multiple vector assignments for interrupt line %d", irq)

    debug("assign IRQ %d to vector %d" % (irq, vector))
    irq_vec_map[irq] = vector


def setup_idt(spur_code, spur_nocode, intlist, max_vec, max_irq):
    irq_vec_map = [0 for i in range(max_irq)]
    vectors = [None for i in range(max_vec)]

    # Pass 1: sanity check and set up hard-coded interrupt vectors
    for handler, irq, prio, vec, dpl, tss in intlist:
        if vec == -1:
            if prio == -1:
                error("entry does not specify vector or priority level")
            continue

        if vec >= max_vec:
            error("Vector %d specified, but size of IDT is only %d vectors" %
                  (vec, max_vec))

        if vectors[vec] is not None:
            error("Multiple assignments for vector %d" % vec)

        vectors[vec] = (handler, tss, dpl)
        update_irq_vec_map(irq_vec_map, irq, vec, max_irq)

    # Pass 2: set up priority-based interrupt vectors
    for handler, irq, prio, vec, dpl, tss in intlist:
        if vec != -1:
            continue

        for vi in priority_range(prio):
            if vi >= max_vec:
                break
            if vectors[vi] is None:
                vec = vi
                break

        if vec == -1:
            error("can't find a free vector in priority level %d" % prio)

        vectors[vec] = (handler, tss, dpl)
        update_irq_vec_map(irq_vec_map, irq, vec, max_irq)

    # Pass 3: fill in unused vectors with spurious handler at dpl=0
    for i in range(max_vec):
        if vectors[i] is not None:
            continue

        if i in ERR_CODE_VECTORS:
            handler = spur_code
        else:
            handler = spur_nocode

        vectors[i] = (handler, 0, 0)

    return vectors, irq_vec_map


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")

# struct genidt_header_s {
#	uint32_t spurious_addr;
#	uint32_t spurious_no_error_addr;
#	int32_t num_entries;
# };


intlist_header_fmt = "<II"

# struct genidt_entry_s {
#	uint32_t isr;
#	int32_t irq;
#	int32_t priority;
#	int32_t vector_id;
#	int32_t dpl;
#	int32_t tss;
# };

intlist_entry_fmt = "<Iiiiii"


def get_intlist(elf):
    intdata = elf.get_section_by_name("intList").data()

    header_sz = struct.calcsize(intlist_header_fmt)
    header = struct.unpack_from(intlist_header_fmt, intdata, 0)
    intdata = intdata[header_sz:]

    spurious_code = header[0]
    spurious_nocode = header[1]

    debug("spurious handler (code)    : %s" % hex(header[0]))
    debug("spurious handler (no code) : %s" % hex(header[1]))

    intlist = [i for i in
               struct.iter_unpack(intlist_entry_fmt, intdata)]

    debug("Configured interrupt routing")
    debug("handler    irq pri vec dpl")
    debug("--------------------------")

    for irq in intlist:
        debug("{0:<10} {1:<3} {2:<3} {3:<3} {4:<2}".format(
            hex(irq[0]),
            "-" if irq[1] == -1 else irq[1],
            "-" if irq[2] == -1 else irq[2],
            "-" if irq[3] == -1 else irq[3],
            irq[4]))

    return (spurious_code, spurious_nocode, intlist)


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-m", "--vector-map", required=True,
                        help="Output file mapping IRQ lines to IDT vectors")
    parser.add_argument("-o", "--output-idt", required=True,
                        help="Output file containing IDT binary")
    parser.add_argument("-k", "--kernel", required=True,
                        help="Zephyr kernel image")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1


def main():
    parse_args()

    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)

        syms = get_symbols(kernel)
        spur_code, spur_nocode, intlist = get_intlist(kernel)

    max_irq = syms["CONFIG_MAX_IRQ_LINES"]
    max_vec = syms["CONFIG_IDT_NUM_VECTORS"]

    vectors, irq_vec_map = setup_idt(spur_code, spur_nocode, intlist, max_vec,
                                     max_irq)

    create_idt_binary(vectors, args.output_idt)
    create_irq_vec_map_binary(irq_vec_map, args.vector_map)


if __name__ == "__main__":
    main()
