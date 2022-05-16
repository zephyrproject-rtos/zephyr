#!/usr/bin/env python3
#
# Copyright (c) 2020-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# ADSP devices have their RAM regions mapped twice.  The first mapping
# is set in the CPU to bypass the L1 cache, and so access through
# pointers in that region is coherent between CPUs (but slow).  The
# second region accesses the same memory through the L1 cache and
# requires careful flushing when used with shared data.
#
# This distinction is exposed in the linker script, where some symbols
# (e.g. stack regions) are linked into cached memory, but others
# (general kernel memory) are not.  But the rimage signing tool
# doesn't understand that and fails if regions aren't contiguous.
#
# Walk the sections in the ELF file, changing the VMA/LMA of each
# uncached section to the equivalent address in the cached area of
# memory.

import os
import sys
from elftools.elf.elffile import ELFFile

objcopy_bin = sys.argv[1]
elffile = sys.argv[2]
cached_reg = int(sys.argv[3])
uncached_reg = int(sys.argv[4])

uc_min = uncached_reg << 29
uc_max = uc_min | 0x1fffffff
cache_off = "0x%x" % ((cached_reg - uncached_reg) << 29)

fixup =[]
with open(elffile, "rb") as fd:
    elf = ELFFile(fd)
    for s in elf.iter_sections():
        addr = s.header.sh_addr
        if uc_min <= addr <= uc_max:
            print(f"fix_elf_addrs.py: Moving section {s.name} to cached SRAM region")
            fixup.append(s.name)

for s in fixup:
    # Note redirect: the sof-derived linker scripts currently emit
    # some zero-length sections at address zero.  This is benign, and
    # the linker is happy, but objcopy will emit an unsilenceable
    # error (no --quiet option, no -Werror=no-whatever, nothing).
    # Just swallow the error stream for now pending rework to the
    # linker framework.
    cmd = f"{objcopy_bin} --change-section-address {s}+{cache_off} {elffile} 2>{os.devnull}"
    os.system(cmd)
