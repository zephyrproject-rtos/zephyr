Title: klibs_lifo

Description:

This test verifies that the nanokernel LIFO APIs operate as expected when using
the klibs from the nkernel/test/test_fifo project.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

In nkernel/test/test_fifo:
    make pristine
    make klibs

In nkernel/klibs/klibs_lifo:
    make KLIB_DIR=<path to nkernel/test/test_fifo>/outdir/klib
    make KLIB_DIR=<path to nkernel/test/test_fifo>/outdir/klib nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test Nanokernel LIFO
Nano objects initialized
Fiber waiting on an empty LIFO
Task waiting on an empty LIFO
Fiber to get LIFO items without waiting
Task to get LIFO items without waiting
ISR to get LIFO items without waiting
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
