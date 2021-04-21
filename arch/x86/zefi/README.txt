Operation
=========

Run the "zefi.py" script, passing a path to a built zephyr.elf file
(NOT a "zephyr.strip" intended for grub/multiboot loading -- we need
the symbol table) for the target as its sole argument.  It will emit a
"zephyr.efi" file in the current directory.  Copy this to your target
device's EFI boot partition via whatever means you like and run it
from the EFI shell.

Theory
======

This works by creating a "zephyr.efi" EFI binary containing a zephyr
image extracted from a built zephyr.elf file.  EFI applications are
relocatable, and cannot be placed at specific locations in memory.
Instead, the stub code will copy the embedded zephyr sections to the
appropriate locations at startup, clear any zero-filled (BSS, etc...)
areas, then jump into the 64 bit entry point.

Arguably this is needlessly inefficient, having to load the whole
binary into memory and copy it vs. a bootloader like grub that will
load it from the EFI boot filesystem into its correct location with no
copies.  But then, the biggest Zephyr application binaries we have on
any platform top out at 200kb or so, and grub is at minimum about 5x
that size, and potentially MUCH more if you start enabling the default
modules.

Source Code & Linkage
=====================

The code and link environment here is non-obvious.  The simple rules
are:

1. All code must live in a single C file
2. All symbols must be declared static

And if you forget this and use a global by mistake, the build will
work fine and then fail inexplicably at runtime with a garbage
address.  The sole exception is the "efi_entry()" function, whose
address is not generated at runtime by the C code here (it's address
goes into the PE file header instead).

The reason is that we're building an EFI Application Binary with a
Linux toolchain.  EFI binaries are relocatable PE-COFF files --
basically Windows DLLs.  But our compiler only generates code for ELF
targets.

These environments differ in the way they implemenqt position
independent code.  Non-static global variables and function addresses
in ELF get found via GOT and PLT tables that are populated at load
time by a system binary (ld-linux.so).  But there is no ld-linux.so in
EFI firmware, and the EFI loader only knows how to fill in the PE
relocation fields, which are a different data structure.

So we can only rely on the C file's internal linkage, which is done
via the x86_64 RIP-relative addressing mode and doesn't rely on any
support from the environment.  But that only works for symbols that
the compiler (not the linker) knows a-priori will never be in
externally linked shared libraries.  Which is to say: only static
symbols work.

You might ask: why build a position-independent executable in the
first place?  Why not just link to a specific address?  The PE-COFF
format certainly allows that, and the UEFI specification doesn't
clearly rule it out.  But my experience with real EFI loaders is that
they ignore the preferred load address and will put the image at
arbitrary locations in memory.  I couldn't make it work, basically.

Bugs
====

No support for 32 bit EFI environments.  This would be a very tall
order given the linker constraints described above (i386 doesn't have
a IP-relative addressing mode, so we'd have to do some kind of
relocation of our own).  Will probably never happen unless we move to
a proper PE-COFF toolchain.

There is no protection against colliding mappings.  We just assume
that the EFI environment will load our image at an address that
doesn't overlap with the target Zephyr memory.  In practice on the
systems I've tried, EFI uses memory near the top of 32-bit-accessible
physical memory, while Zephyr prefers to load into the bottom of RAM
at 0x10000.  Even given collisions, this is probably tolerable, as we
could control the Zephyr mapping addresses per-platform if needed to
sneak around EFI areas.  But the Zephyr loader should at least detect
the overlap and warn about it.

It runs completely standalone, writing output to the current
directory, and uses the (x86_64 linux) host toolchain right now, when
it should be integrated with the Zephyr toolchain and build system.
