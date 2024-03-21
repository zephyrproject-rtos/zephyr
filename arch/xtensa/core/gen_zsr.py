#!/usr/bin/env python3
# Copyright (c) 2022 Intel corporation
# SPDX-License-Identifier: Apache-2.0
import argparse
import re

# Scratch register allocator.  Zephyr uses multiple Xtensa SRs as
# scratch space for various special purposes.  Unfortunately the
# configurable architecture means that not all registers will be the
# same on every device.  This script parses a pre-cooked ("gcc -E
# -dM") core-isa.h file for the current architecture and assigns
# registers to usages.

def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("--coherence", action="store_true",
                        help="Enable scratch registers for CONFIG_KERNEL_COHERENCE")
    parser.add_argument("--mmu", action="store_true",
                        help="Enable scratch registers for MMU usage")
    parser.add_argument("coreisa",
                        help="Path to preprocessed core-isa.h")
    parser.add_argument("outfile",
                        help="Output file")

    return parser.parse_args()

args = parse_args()

NEEDED = ["A0SAVE", "CPU"]
if args.mmu:
    NEEDED += ["MMU_0", "MMU_1", "DBLEXC"]
if args.coherence:
    NEEDED += ["FLUSH"]

coreisa = args.coreisa
outfile = args.outfile

syms = {}

def get(s):
    return syms[s] if s in syms else 0

with open(coreisa) as infile:
    for line in infile.readlines():
        m = re.match(r"^#define\s+([^ ]+)\s*(.*)", line.rstrip())
        if m:
            syms[m.group(1)] = m.group(2)

# Use MISC registers first if available, that's what they're for
regs = [ f"MISC{n}" for n in range(0, int(get("XCHAL_NUM_MISC_REGS"))) ]

# Next come EXCSAVE. Also record our highest non-debug interrupt level.
maxint = 0
for il in range(1, 1 + int(get("XCHAL_NUM_INTLEVELS"))):
    regs.append(f"EXCSAVE{il}")
    if il != int(get("XCHAL_DEBUGLEVEL")):
        maxint = max(il, maxint)

# Find the highest priority software interrupt.  We'll use that for
# arch_irq_offload().
irqoff_level = -1
irqoff_int = -1
for sym, val in syms.items():
    if val == "XTHAL_INTTYPE_SOFTWARE":
        m = re.match(r"XCHAL_INT(\d+)_TYPE", sym)
        if m:
            intnum = int(m.group(1))
            levelsym = f"XCHAL_INT{intnum}_LEVEL"
            if levelsym in syms:
                intlevel = int(syms[levelsym])
                if intlevel > irqoff_level:
                    irqoff_int = intnum
                    irqoff_level = intlevel

# Now emit our output header with the assignments we chose
with open(outfile, "w") as f:
    f.write("/* Generated File, see gen_zsr.py */\n")
    f.write("#ifndef ZEPHYR_ZSR_H\n")
    f.write("#define ZEPHYR_ZSR_H\n")
    for i, need in enumerate(NEEDED):
        f.write(f"# define ZSR_{need} {regs[i]}\n")
        f.write(f"# define ZSR_{need}_STR \"{regs[i]}\"\n")

    # Emit any remaining registers as generics
    for i in range(len(NEEDED), len(regs)):
        f.write(f"# define ZSR_EXTRA{i - len(NEEDED)} {regs[i]}\n")
        f.write(f"# define ZSR_EXTRA{i - len(NEEDED)}_STR \"{regs[i]}\"\n")

    # Also, our highest level EPC/EPS registers
    f.write(f"# define ZSR_RFI_LEVEL {maxint}\n")
    f.write(f"# define ZSR_EPC EPC{maxint}\n")
    f.write(f"# define ZSR_EPS EPS{maxint}\n")

    # And the irq offset interrupt
    if irqoff_int >= 0:
        f.write(f"# define ZSR_IRQ_OFFLOAD_INT {irqoff_int}\n")

    f.write("#endif\n")
