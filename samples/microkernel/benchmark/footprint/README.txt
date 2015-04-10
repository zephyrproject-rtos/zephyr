Title: Microkernel footprint project

Description:

This project is designed to characterize the memory requirements of a
standard microkernel image running on a Pentium4 target.


The project can be built using several configurations:

minimal (a "do nothing" image that has support for few kernel capabilities)
-------
- Idle task is only task in system.
- K_swapper is only fiber in system.
- ISRs for the system timer and spurious interrupt handling are present.
- IDT and stack memory sizes are very limited.

regular (a "typical" image that has support for some kernel capabilities)
-------
- As for "minimal" configuration, except as noted below.
- Supports larger IDT and utilizes larger stacks.
- A statically linked dummy ISR is present.
- Has "foreground" task that prints a message to the console via printk().
- Enables task scheduler support for time slicing.
- Links in support for EVENT, MUTEX, FIFO, and MAP objects.

maximal (a "complex" image that has support for many kernel capabilities)
-------
- As for "regular" configuration, except as noted below.
- Supports full IDT and utilizes even larger stacks.
- Foreground task dynamically links in the dummy ISR, rather than having
  it statically linked.
- Foreground task prints a message to the console via printf(),
  rather than printk().
- Links in support for SEMAPHORE, PIPE, MAILBOX, and POOL objects.
- Links in support for task device interrupt handling and additional task APIs.
- Adds advanced power management support (including tickless idle).
- Adds floating point support (for x87 FPU, including SSE).


NOTE:
- These configurations utilize standard security only.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project does not generate any output in the default case
(TEST=min). In the regular case (TEST=reg) and the maximal case (TEST=max),
it outputs to the console. It can be built and executed on QEMU as follows:

    make pristine
    make TEST=min microkernel.qemu          (minimal configuration)

    make pristine
    make TEST=reg microkernel.qemu          (regular configuration)

    make pristine
    make TEST=max microkernel.qemu          (maximal configuration)

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

The resulting image is bootable for all configurations, but produces different
output in each case.

minimal
-------
This configuration does NOT produce any output. To observe its operation,
invoke it using gdb and observe that:
- the kernel's timer ISR & K_swapper increment "K_LowTime" on a regular basis
- nano_cpu_idle() is invoked by the idle task each time K_LowTime is incremented

regular
-------
This configuration prints the following message to the console:

  Running regular microkernel configuration

maximal
-------
This configuration prints the following message to the console:

  Running maximal microkernel configuration

--------------------------------------------------------------------------------

Additional notes:

Various host utilities (such as the Unix "size" utility) can be used to
determine the footprint of the resulting outdir/microkernel.elf image.
