Title: klibs_sema

Description:

This test verifies that the nanokernel semaphore APIs operate as expected when
using the klibs from the nkernel/test/test_fifo project.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

In nkernel/test/test_fifo:
    make klibs

In nkernel/klibs/klibs_sema:
    make KLIB_DIR=<path to nkernel/test/test_fifo>/outdir/klib
    make KLIB_DIR=<path to nkernel/test/test_fifo>/outdir/klib nanokernel.qemu

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test Nanokernel Semaphores
Nano objects initialized
Giving and taking a semaphore in a task (non-blocking)
Giving and taking a semaphore in an ISR (non-blocking)
Giving and taking a semaphore in a fiber (non-blocking)
Semaphore from the task woke the fiber
Semaphore from the fiber woke the task
Semaphore from the ISR woke the task.
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
