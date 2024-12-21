#!/usr/bin/env python3
#
# Copyright (c) 2023 KNS Group LLC (YADRO)
# Copyright (c) 2020 Yonatan Goldschmidt <yon.goldschmidt@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

"""
Stack compressor for FlameGraph

This translate stack samples captured by perf subsystem into format
used by flamegraph.pl. Translation uses .elf file to get function names
from addresses

Usage:
    ./script/perf/stackcollapse.py <file with perf printbuf output> <ELF file>
"""

import re
import sys
import struct
import binascii
from functools import lru_cache
from elftools.elf.elffile import ELFFile


@lru_cache(maxsize=None)
def addr_to_sym(addr, elf):
    symtab = elf.get_section_by_name(".symtab")
    for sym in symtab.iter_symbols():
        if sym.entry.st_info.type == "STT_FUNC" and sym.entry.st_value <= addr < sym.entry.st_value + sym.entry.st_size:
            return sym.name
    if addr == 0:
        return "nullptr"
    return "[unknown]"


def collapse(buf, elf):
    while buf:
        count, = struct.unpack_from(">Q", buf)
        assert count > 0
        addrs = struct.unpack_from(f">{count}Q", buf, 8)

        func_trace = reversed(list(map(lambda a: addr_to_sym(a, elf), addrs)))
        prev_func = next(func_trace)
        line = prev_func
        # merge dublicate functions
        for func in func_trace:
            if prev_func != func:
                prev_func = func
                line += ";" + func

        print(line, 1)
        buf = buf[8 + 8 * count:]


if __name__ == "__main__":
    elf = ELFFile(open(sys.argv[2], "rb"))
    with open(sys.argv[1], "r") as f:
        inp = f.read()

    lines = inp.splitlines()
    assert int(re.match(r"Perf buf length (\d+)", lines[0]).group(1)) == len(lines) - 1
    buf = binascii.unhexlify("".join(lines[1:]))
    collapse(buf, elf)
