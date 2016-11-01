Title: Nanokernel Footprint Measurement

Description:

This project is designed to characterize the memory requirements of a
nanokernel-only image running on an Atom target.


The project can be built using several configurations:

minimal (a "do nothing" image that has support for few kernel capabilities)
-------
- Background task is only task in system; no fibers are utilized.
- Background task simply runs an endless loop that increments a global variable.
- No system timer support.
- ISR for the spurious interrupt handling is present.
- IDT and stack memory sizes are very limited.

regular (a "typical" image that has support for some kernel capabilities)
-------
- As for "minimal" configuration, except as noted below.
- Supports larger IDT and utilizes larger stacks.
- A statically linked dummy ISR is present.
- Background task also starts a fiber.
- Fiber prints a message to the console via printk().
- Supports system timer, along with NANO_TIMER objects.
- Links in support for NANO_SEM objects.

maximal (a "complex" image that has support for many kernel capabilities)
-------
- As for "regular" configuration, except as noted below.
- Supports full IDT and utilizes even larger stacks.
- Background task dynamically links in the dummy ISR, rather than having
  it statically linked.
- Fiber prints a message to the console via printf(), rather than printk().
- Links in support for NANO_LIFO, NANO_STACK, and NANO_FIFO objects.
- Adds floating point support (for x87 FPU, including SSE).


--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project does not generate any output in the default case
(TEST=min). In the regular case (TEST=reg) and the maximal case (TEST=max),
it outputs to the console. It can be built and executed on QEMU as follows:

    make TEST=min qemu         (minimal configuration)

    make TEST=reg qemu         (regular configuration)

    make TEST=max qemu         (maximal configuration)

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

The resulting image is bootable for all configurations, but produces different
output in each case.

minimal
-------
This configuration does NOT produce any output. To observe its operation,
invoke it using gdb and observe that:
- main() increments "i" each time it loops
- the kernel's timer ISR increments "nanoTicks" on a regular basis

regular
-------
This configuration prints the following message to the console:

  Running regular nanokernel configuration

maximal
-------
This configuration prints the following message to the console:

  Running maximal nanokernel configuration

--------------------------------------------------------------------------------

Additional notes:

Various host utilities (such as the Unix "size" utility) can be used to
determine the footprint of the resulting zephyr.elf image.
