Title: Kernel Footprint Measurement

Description:

This project is designed to characterize the memory requirements of a
standard kernel image running on an Atom target.


The project can be built using several configurations:

minimal (a "do nothing" image that has support for few kernel capabilities)
-------
- Idle task is only task in system.
- No system timer support.
- ISR for the spurious interrupt handling is present.
- IDT and stack memory sizes are very limited.
- Provides support for 2 task priorities.

regular (a "typical" image that has support for some kernel capabilities)
-------
- As for "minimal" configuration, except as noted below.
- Supports larger IDT and utilizes larger stacks.
- A statically linked dummy ISR is present.
- Has "foreground" task that prints a message to the console via printk().
- Provides support for 16 task priorities.
- Supports system timer, along with task scheduler support for time slicing.
- Links in support for EVENT, MUTEX, FIFO, and MAP objects.

maximal (a "complex" image that has support for many kernel capabilities)
-------
- As for "regular" configuration, except as noted below.
- Supports full IDT and utilizes even larger stacks.
- Provides support for 64 task priorities.
- Foreground task dynamically links in the dummy ISR, rather than having
  it statically linked.
- Foreground task prints a message to the console via printf(),
  rather than printk().
- Links in support for SEMAPHORE, PIPE, MAILBOX, and POOL objects.
- Links in support for task device interrupt handling and additional task APIs.
- Adds advanced power management support (including tickless idle).
- Adds floating point support (for x87 FPU, including SSE).


--------------------------------------------------------------------------------

Building and Running Project:

This project does not generate any output in the default case
(TEST=min). In the regular case (TEST=reg) and the maximal case (TEST=max),
it outputs to the console. It can be built and executed on QEMU as follows:

    make TEST=min qemu          (minimal configuration)

    make TEST=reg qemu          (regular configuration)

    make TEST=max qemu          (maximal configuration)

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
- the kernel's timer ISR & main thread increment "_sys_clock_tick_count" on a
  regular basis
- k_cpu_idle() is invoked by the idle task each time _sys_clock_tick_count
  is incremented

regular
-------
This configuration prints the following message to the console:

  Running regular kernel configuration

maximal
-------
This configuration prints the following message to the console:

  Running maximal kernel configuration

--------------------------------------------------------------------------------

Additional notes:

Various host utilities (such as the Unix "size" utility) can be used
to determine the footprint of the resulting
outdir/$BOARD/zephyr.elf image.
