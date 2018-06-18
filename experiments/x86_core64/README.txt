It's an x86_64 OS underkernel!

* It boots from multiboot/qemu into 32 bit mode just like everything else
* It builds an identity page table for the first 4G of memory
* It switches to long mode.
* It installs vectors for all 256 interrupts (with ~10 cycle dispatch!)
* It context switches on ISR exit
* It has a compliant Zephyr _arch_switch() implementation[1]
* It boots extra CPUs (as many as you like!) and initializes them all
  the way up the layer stack from 16 bit real mode.
* CPU-specific pointer access through the FS/GS segments?  I got 'em.
* Nested interrupts with CR8 priority masking?  That too.
* Get your interprocessor interrupts right here.
* A full 64 bit port too scary?  No problem, switch on x32 in the
  makefile and use your existing code with legacy pointer sizes.
* It even includes, at absolutely no charge, a tiny micro-printf
  logging layer which can spit to the VGA console, serial port, or
  both!
* All that, and more, in under 1700 lines of code[2]

OK, there's still needed features missing:

* A TSS so you can enter userspace.
* Stack switching for ISRs, which on x86_64[3] needs to wait for a TSS
* It's only ever been run in qemu.

And, of course, it's not Zephyr.  It's just a demo, all the glue needs
to be written.  **But all the hardware pieces are there and working.**
Finishing this up is just code now.  And bugs.

[1] Somewhat weakly tested.

[2] Including outrageously verbose logging and a bunch of whitebox
hooks that haven't been replaced with API-level unit tests.

[3] It has to do with the red zone (128 bytes of untouchable area
below the stack pointer) in the ABI.  IA hardware pushes junk on the
stack on interrupt entry.  But that would clobber the userspace red
zone, so you can't use software stack switching and have to get the
hardware to do the stack switch for you, which requires a TSS.  And
once you have the TSS transitions you basically have userspace
working.  So they sort of go hand in hand.

========================================================================

Using it:

Should build from source on current Linux distros.

Requires an x86_64 binutils/gcc (with support for -m32 and -m16 modes,
and -mx32 if you want to use that).

Requires a recent enough qemu-system-x86_64 to support the -icount
argument.
