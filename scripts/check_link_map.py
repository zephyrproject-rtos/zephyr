#!/usr/bin/env python3
import fileinput
import re
import sys

# Linker address generation validity checker.  By default, GNU ld is
# broken when faced with sections where the load address (i.e. the
# spot in the XIP program binary where initialized data lives) differs
# from the virtual address (i.e. the location in RAM where that data
# will live at runtime.  We need to be sure we're using the
# ALIGN_WITH_INPUT feature correctly everywhere, which is hard --
# especially so given that many of these bugs are semi-invisible at
# runtime (most initialized data is still a bunch of zeros and often
# "works" even if it's wrong).
#
# This quick test just checks the offsets between sequential segments
# with separate VMA/LMA addresses and verifies that the size deltas
# are identical.
#
# Note that this is assuming that the address generation is always
# in-order and that there is only one "ROM" LMA block.  It's possible
# to write a valid linker script that will fail this script, but we
# don't have such a use case and one isn't forseen.

section_re = re.compile('(?x)'                    # (allow whitespace)
                        '^([a-zA-Z0-9_\.]+) \s+'  # name
                        ' (0x[0-9a-f]+)     \s+'  # addr
                        ' (0x[0-9a-f]+)\s*')      # size

load_addr_re = re.compile('load address (0x[0-9a-f]+)')

in_prologue = True
lma = 0
last_sec = None

for line in fileinput.input():
    # Skip the header junk
    if line.find("Linker script and memory map") >= 0:
        in_prologue = False
        continue

    match = section_re.match(line.rstrip())
    if match:
        sec = match.group(1)
        vma = int(match.group(2), 16)
        size = int(match.group(3), 16)

        if (sec == "bss"):
            # Make sure we don't compare the last section of kernel data
            # with the first section of application data, the kernel's bss
            # and noinit are in between.
            last_sec = None
            continue

        lmatch = load_addr_re.search(line.rstrip())
        if lmatch:
            lma = int(lmatch.group(1), 16)
        else:
            last_sec = None
            continue

        if last_sec != None:
            dv = vma - last_vma
            dl = lma - last_lma
            if dv != dl:
                sys.stderr.write("ERROR: section %s is %d bytes "
                                 "in the virtual/runtime address space, "
                                 "but only %d in the loaded/XIP section!\n"
                                 % (last_sec, dv, dl))
                sys.exit(1)

        last_sec = sec
        last_vma = vma
        last_lma = lma
