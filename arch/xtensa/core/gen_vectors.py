#!/usr/bin/env python3
# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0
import sys
import re

# Xtensa Vector Table linker generator
#
# Takes a pre-processed (gcc -dM) core-isa.h file as its first
# argument, and emits a GNU linker section declartion which will
# correctly load the exception vectors and literals as long as their
# code is declared using standard conventions (see below).
#
# The section name will be ".z_xtensa_vectors", and a symbol
# "z_xtensa_vecbase" is emitted containing a valid value for the
# VECBASE SR at runtime.
#
# Obviously, this requires that XCHAL_HAVE_VECBASE=1.  A similar trick
# could be played to load vectors at fixed addresses on hardware that
# lacks VECBASE, but the core-isa.h interface is inexplicably
# different.
#
# Because the "standard conventions" (which descend from somewhere in
# Cadence) are not documented anywhere and just end up cut and pasted
# between devices, here's an attempt at a specification:
#
# + The six register window exception vectors are defined with offsets
#   internal to their assembly code.  They are linked in a single
#   section named ".WindowVectors.text".
#
# + The "kernel", "user" and "double exception" vectors are emitted in
#   sections named ".KernelExceptionVector.text",
#   "UserExceptionVector.text" and "DoubleExceptionVector.text"
#   respectively.
#
# + XEA2 interrupt vectors are in sections named
#   ".Level<n>InterruptVector.text", except (!) for ones which are
#   given special names.  The "debug" and "NMI" interrupts (if they
#   exist) are technically implemented as standard interrupt vectors
#   (of a platform-dependent level), but the code for them is emitted
#   in ".DebugExceptionVector.text" and ".NMIExceptionVector.text",
#   and not a section corresponding to their interrupt level.
#
# + Any unused bytes at the end of a vector are made available as
#   storage for immediate values used by the following vector (Xtensa
#   can only back-reference immediates for MOVI/L32R instructions) as
#   a "<name>Vector.literal" section.  Note that there is no guarantee
#   of how much space is available, it depends on the previous
#   vector's code size.  Zephyr code has historically not used this
#   space, as support in existing linker scripts is inconsistent.  But
#   it's exposed here.

coreisa = sys.argv[1]

debug_level = 0

# Translation for the core-isa.h vs. linker section naming conventions
sect_names = { "DOUBLEEXC" : "DoubleException",
               "KERNEL" : "KernelException",
               "NMI" : "NMIException",
               "USER" : "UserException" }

offsets = {}

with open(coreisa) as infile:
    for line in infile.readlines():
        m = re.match(r"^#define\s+XCHAL_([^ ]+)_VECOFS\s*(.*)", line.rstrip())
        if m:
            (sym, val) = (m.group(1), m.group(2))
            if sym == "WINDOW_OF4":
                # This must be the start of the section
                assert eval(val) == 0
            elif sym.startswith("WINDOW"):
                # Ignore the other window exceptions, they're internally sorted
                pass
            elif sym == "RESET":
                # Ignore, not actually part of the vector table
                pass
            elif sym == "DEBUG":
                # This one is a recursive macro that doesn't expand,
                # so handle manually
                m = re.match(r"XCHAL_INTLEVEL(\d+)_VECOFS", val)
                if not m:
                    print(f"no intlevel match for debug val {val}")
                assert m
                debug_level = eval(m.group(1))
            else:
                if val == "XCHAL_NMI_VECOFS":
                    # This gets recursively defined in the other
                    # direction, so ignore the INTLEVEL
                    pass
                else:
                    addr = eval(val)
                    m = re.match(r"^INTLEVEL(\d+)", sym)
                    if m:
                        offsets[f"Level{m.group(1)}Interrupt"] = addr
                    else:
                        offsets[sect_names[sym]] = addr

if debug_level > 0:
    old = f"Level{debug_level}Interrupt"
    offsets[f"DebugException"] = offsets[old]
    del offsets[old]

sects = list(offsets)
sects.sort(key=lambda s: offsets[s])

print("/* Automatically Generated Code - Do Not Edit */")
print("/* See arch/xtensa/core/gen_vector.py */")
print("")

# The 1k alignment is experimental, the docs on the Relocatable Vector
# Option doesn't specify an alignment at all, but writes to the
# bottom bits don't take...
print( "  .z_xtensa_vectors : ALIGN(1024) {")
print( "    z_xtensa_vecbase = .;")
print(f"    KEEP(*(.WindowVectors.text));")
for s in sects:
    print(f"    KEEP(*(.{s}Vector.literal));")
    print( "    . = 0x%3.3x;" % (offsets[s]))
    print(f"    KEEP(*(.{s}Vector.text));")
print("  }")
