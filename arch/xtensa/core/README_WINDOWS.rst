# How Xtensa register windows work

There is a paucity of introductory material on this subject, and
Zephyr plays some tricks here that require understanding the base
layer.

## Hardware

When register windows are configured in the CPU, there are either 32
or 64 "real" registers in hardware, with 16 visible at one time.
Registers are grouped and rotated in units of 4, so there are 8 or 16
such "quads" (my term, not Tensilica's) in hardware of which 4 are
visible as A0-A15.

The first quad (A0-A3) is pointed to by a special register called
WINDOWBASE.  The register file is cyclic, so for example if NREGS==64
and WINDOWBASE is 15, quads 15, 0, 1, and 2 will be visible as
(respectively) A0-A3, A4-A7, A8-A11, and A12-A15.

There is a ROTW instruction that can be used to manually rotate the
window by a immediate number of quads that are added to WINDOWBASE.
Positive rotations "move" high registers into low registers
(i.e. after "ROTW 1" the register that used to be called A4 is now
A0).

There are CALL4/CALL8/CALL12 instructions to effect rotated calls
which rotate registers upward (i.e. "hiding" low registers from the
callee) by 1, 2 or 3 quads.  These do not rotate the window
themselves.  Instead they place the rotation amount in two places
(yes, two; see below): the 2-bit CALLINC field of the PS register, and
the top two bits of the return address placed in A0.

There is an ENTRY instruction that does the rotation.  It adds CALLINC
to WINDOWBASE, at the same time copying the old (now hidden) stack
pointer in A1 into the "new" A1 in the rotated frame, subtracting an
immediate offset from it to make space for the new frame.

There is a RETW instruction that undoes the rotation.  It reads the
top two bits from the return address in A0 and subtracts that value
from WINDOWBASE before returning.  This is why the CALLINC bits went
in two places.  They have to be stored on the stack across potentially
many calls, so they need to be GPR data that lives in registers and
can be spilled.  But ENTRY isn't specified to assume a particular
return value format and is used immediately, so it makes more sense
for it to use processor state instead.

Note that we still don't know how to detect when the register file has
wrapped around and needs to be spilled or filled.  To do this there is
a WINDOWSTART register used to detect which register quads are in use.
The name "start" is somewhat confusing, this is not a pointer.
WINDOWSTART stores a bitmask with one bit per hardware quad (so it's 8
or 16 bits wide).  The bit in windowstart corresponding to WINDOWBASE
will be set by the ENTRY instruction, and remain set after rotations
until cleared by a function return (by RETW, see below).  Other bits
stay zero.  So there is one set bit in WINDOWSTART corresponding to
each call frame that is live in hardware registers, and it will be
followed by 0, 1 or 2 zero bits that tell you how "big" (how many
quads of registers) that frame is.

So the CPU executing RETW checks to make sure that the register quad
being brought into A0-A3 (i.e. the new WINDOWBASE) has a set bit
indicating it's valid. If it does not, the registers must have been
spilled and the CPU traps to an exception handler to fill them.

Likewise, the processor can tell if a high register is "owned" by
another call by seeing if there is a one in WINDOWSTART between that
register's quad and WINDOWBASE.  If there is, the CPU traps to a spill
handler to spill one frame.  Note that a frame might be only four
registers, but it's possible to hit registers 12 out from WINDOWBASE,
so it's actually possible to trap again when the instruction restarts
to spill a second quad, and even a third time at maximum.

Finally: note that hardware checks the two bits of WINDOWSTART after
the frame bit to detect how many quads are represented by the one
frame.  So there are six separate exception handlers to spill/fill
1/2/3 quads of registers.

## Software & ABI

The advantage of the scheme above is that it allows the registers to
be spilled naturally into the stack by using the stack pointers
embedded in the register file.  But the hardware design assumes and to
some extent enforces a fairly complicated stack layout to make that
work:

The spill area for a single frame's A0-A3 registers is not in its own
stack frame.  It lies in the 16 bytes below its CALLEE's stack
pointer.  This is so that the callee (and exception handlers invoked
on its behalf) can see its caller's potentially-spilled stack pointer
register (A1) on the stack and be able to walk back up on return.
Other architectures do this too by e.g. pushing the incoming stack
pointer onto the stack as a standard "frame pointer" defined in the
platform ABI.  Xtensa wraps this together with the natural spill area
for register windows.

By convention spill regions always store the lowest numbered register
in the lowest address.

The spill area for a frame's A4-A11 registers may or may not exist
depending on whether the call was made with CALL8/CALL12.  It is legal
to write a function using only A0-A3 and CALL4 calls and ignore higher
registers.  But if those 0-2 register quads are in use, they appear at
the top of the stack frame, immediately below the parent call's A0-A3
spill area.

There is no spill area for A12-A15.  Those registers are always
caller-save.  When using CALLn, you always need to overlap 4 registers
to provide arguments and take a return value.
