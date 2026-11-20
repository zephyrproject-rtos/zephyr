#!/usr/bin/env python3
#
# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Push an AArch64 local-exec TLS relocation past its checked range.

The linker happily emits R_AARCH64_TLSLE_ADD_TPREL_HI12 for any thread-pointer
offset, so an extension can only be made to overflow after the fact. This takes
a valid llext and adds TPREL_DELTA to the addend of the first HI12 relocation,
which puts TPREL(S+A) at or above 2**24 -- outside the range that AAELF64
defines for this relocation.

With --wrap-symbol the addend is left alone and the symbol value is rewritten
instead, to a value whose 64-bit unsigned sum wraps back below 2**24. The true
offset is then far outside the checked range while the arithmetic result looks
acceptable, which is what an unbounded symbol value would do.

With --sum-only both operands stay inside the range the relocation check bounds
them to and only their sum crosses 2**24, so the guard on the computed offset
itself is the one that has to reject the load.

With --negative the same holds but the offset lands on -1 instead, which is the
only way to reach the signed side of that guard: the addend is negative yet well
inside the range, and the symbol value is untouched.

The relocation is looked up by type in every relocation section that applies to
an executable section, because with -ffunction-sections it lands in the section
relocation for the function holding the thread-local access (for example
.rela.text.test_entry) rather than in .rela.text.
"""

import argparse
import struct
import sys

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile

# ELF relocation type, AAELF64.
TLSLE_ADD_TPREL_HI12 = 549

# 2**24: the first value a 12-bit immediate shifted left by 12 cannot hold.
TPREL_DELTA = 1 << 24

# AARCH64_TLS_TCB_SIZE in arch/arm64/core/elf.c: the thread pointer points at a
# two-pointer TCB and the TLS block follows it.
TCB_SIZE = 16

# Byte offset of r_addend inside an Elf32_Rela / Elf64_Rela entry.
ADDEND_OFFSET = {32: 8, 64: 16}

# Byte offset of st_value inside an Elf32_Sym / Elf64_Sym entry.
SYM_VALUE_OFFSET = {32: 4, 64: 8}


def find_hi12(elf):
    """Return (relocation section, index) of the first HI12 relocation."""

    for sect in elf.iter_sections():
        if sect["sh_type"] != "SHT_RELA":
            continue

        target = elf.get_section(sect["sh_info"])
        if target is None or not target["sh_flags"] & SH_FLAGS.SHF_EXECINSTR:
            continue

        for i, reloc in enumerate(sect.iter_relocations()):
            if reloc["r_info_type"] == TLSLE_ADD_TPREL_HI12:
                return sect, i

    return None, None


def find_symbol(elf, name):
    """Return (symbol table section, index) of the first symbol called name."""

    for sect in elf.iter_sections():
        if sect["sh_type"] != "SHT_SYMTAB":
            continue

        for i, sym in enumerate(sect.iter_symbols()):
            if sym.name == name:
                return sect, i

    return None, None


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("input", help="input llext file")
    parser.add_argument("output", help="output llext file")
    parser.add_argument(
        "--wrap-symbol",
        metavar="NAME",
        help="rewrite the value of NAME instead of the addend, so that the unsigned "
        "offset wraps back into the accepted range",
    )
    parser.add_argument(
        "--sum-only",
        action="store_true",
        help="keep both operands inside the checked range and let only their sum cross 2**24",
    )
    parser.add_argument(
        "--negative",
        action="store_true",
        help="keep both operands inside the checked range and put TPREL(S+A) at -1",
    )
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        data = bytearray(f.read())
        f.seek(0)
        elf = ELFFile(f)

        rela, index = find_hi12(elf)
        if rela is None:
            sys.exit(
                f"{args.input}: no R_AARCH64_TLSLE_ADD_TPREL_HI12 relocation "
                "in an executable section"
            )

        addend_off = rela["sh_offset"] + index * rela["sh_entsize"] + ADDEND_OFFSET[elf.elfclass]
        fmt = "<" if elf.little_endian else ">"
        addend = struct.unpack_from(fmt + "q", data, addend_off)[0]

        if args.wrap_symbol:
            symtab, sym_index = find_symbol(elf, args.wrap_symbol)
            if symtab is None:
                sys.exit(f"{args.input}: no symbol called {args.wrap_symbol}")

            value_off = (
                symtab["sh_offset"]
                + sym_index * symtab["sh_entsize"]
                + SYM_VALUE_OFFSET[elf.elfclass]
            )
            value = struct.unpack_from(fmt + "Q", data, value_off)[0]
        elif args.sum_only or args.negative:
            symtab = elf.get_section(rela["sh_link"])
            sym_index = rela.get_relocation(index)["r_info_sym"]
            tls_value = symtab.get_symbol(sym_index)["st_value"]
            # Put TPREL(S+A) exactly on the value the mode is after and leave the
            # addend itself inside the range, so only the offset is out of bounds.
            wanted = -1 if args.negative else TPREL_DELTA
            new_addend = wanted - TCB_SIZE - tls_value
            if not -TPREL_DELTA < new_addend < TPREL_DELTA:
                sys.exit(
                    f"{args.input}: cannot put TPREL({tls_value:#x}) at "
                    f"{wanted} with an in-range addend"
                )

    if args.wrap_symbol:
        # st_value + addend + TCB has to land on zero modulo 2**64, so the sum
        # relocates to the TCB itself while st_value alone is out of range.
        width = 1 << elf.elfclass
        value = (width - addend - TCB_SIZE) % width
        struct.pack_into(fmt + "Q", data, value_off, value)
        print(f"{args.output}: symbol {args.wrap_symbol} value -> {value:#x} (sum wraps to 0x0)")
    elif args.sum_only or args.negative:
        struct.pack_into(fmt + "q", data, addend_off, new_addend)
        print(
            f"{args.output}: {rela.name}[{index}] HI12 addend "
            f"{addend:#x} -> {new_addend:#x} (TPREL(S+A) == {wanted:#x})"
        )
    else:
        struct.pack_into(fmt + "q", data, addend_off, addend + TPREL_DELTA)
        print(
            f"{args.output}: {rela.name}[{index}] HI12 addend "
            f"{addend:#x} -> {addend + TPREL_DELTA:#x}"
        )

    with open(args.output, "wb") as f:
        f.write(data)


if __name__ == "__main__":
    main()
