Title: klibs_events

Description:

This test verifies that the microkernel event APIs operate as expected when
using the klibs from the ukernel/test/test_fifo project.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

In ukernel/test/test/test_fifo:
    make klibs

In ukernel/klibs/klibs_events:
    make KLIB_DIR=<path to ukernel/test/test_fifo>/outdir/klib
    make KLIB_DIR=<path to ukernel/test/test_fifo>/outdir/klib microkernel.qemu

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

tc_start() - Test Microkernel Events

Microkernel objects initialized
Testing task_event_recv() and task_event_send() ...
Testing task_event_recv_wait() and task_event_send() ...
Testing task_event_recv_wait_timeout() and task_event_send() ...
Testing isr_event_send() ...
Testing fiber_event_send() ...
Testing task_event_set_handler() ...
===================================================================
PASS - RegressionTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
