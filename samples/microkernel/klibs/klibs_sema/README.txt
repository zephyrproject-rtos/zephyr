Title: klibs_sema

Description:

This test verifies that the microkernel semaphore APIs operate as expected when
using the klibs from the ukernel/test/test_fifo project.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

In ukernel/test/test/test_fifo:
    make pristine
    make klibs

In ukernel/klibs/klibs_sema:
    make KLIB_DIR=<path to ukernel/test/test_fifo>/outdir/klib
    make KLIB_DIR=<path to ukernel/test/test_fifo>/outdir/klib microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

Starting semaphore tests
===================================================================
Signal and test a semaphore without blocking
Signal and test a semaphore with blocking
Testing many tasks blocked on the same semaphore
Testing semaphore groups without blocking
Testing semaphore groups with blocking
Testing semaphore release by fiber
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
