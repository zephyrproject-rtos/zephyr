Title:  BootTime

Description:

BootTime measures the time:
   a) from system reset to kernel start (crt0.s's __start)
   b) from kernel start to begin of main()
   c) from kernel start to begin of first task

The project can be built using one of the following three configurations:

best
-------
 - Disables most features
 - Provides best case boot measurement

default
-------
 - Default config options
 - Provides typical boot measurement

worst
-------
 - Enables most features
 - Provides worst case boot measurement

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU in three possibile configurations as follows:

    make pristine
    make BOOTTIME_QUALIFIER=best nanokernel.qemu

    make pristine
    make BOOTTIME_QUALIFIER=default nanokernel.qemu

    make pristine
    make BOOTTIME_QUALIFIER=worst nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

tc_start() - Boot Time Measurement
NanoKernel Boot Result: Clock Frequency: 20 MHz
__start       : 377787 cycles, 18889 us
_start->main(): 5287 cycles, 264 us
_start->task  : 5653 cycles, 282 us
Boot Time Measurement finished
===================================================================
PASS - bootTimeTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
