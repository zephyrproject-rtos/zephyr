Title: Workqueue APIs

Description:

This test verifies the kernel workqueue APIs. It is organized into several
Ztest suites:

1. workqueue_api
   - test_k_work_queue_start_stop, test_k_work_queue_stop_sys_thread and
     test_k_work_queue_run_stop exercise starting and stopping a work
     queue and its worker thread.

2. workqueue_delayed
   - test_delayed submits delayed work items from cooperative and
     preemptible threads and checks they run in the expected order.
   - test_delayed_pending and test_delayed_cancel check pending and
     cancellation of delayed work.

3. workqueue_triggered
   - test_triggered and its variants (already_triggered, resubmit,
     no_wait, wait, from_msgq, cancel, ...) exercise poll-triggered work.
   - test_resubmit and test_delayed_work_define cover resubmission and
     static definition of work items.

4. workqueue_work_timeout (built with CONFIG_WORKQUEUE_WORK_TIMEOUT=y)
   - test_work verifies the per-work-item timeout behavior.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/workq/work_queue

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/workq/work_queue
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE workqueue_api
===================================================================
START - test_k_work_queue_run_stop
 PASS - test_k_work_queue_run_stop
===================================================================
START - test_k_work_queue_start_stop
 PASS - test_k_work_queue_start_stop
===================================================================
START - test_k_work_queue_stop_sys_thread
 PASS - test_k_work_queue_stop_sys_thread
===================================================================
TESTSUITE workqueue_api succeeded

Running TESTSUITE workqueue_delayed
===================================================================
START - test_delayed
 PASS - test_delayed
===================================================================
START - test_delayed_cancel
 PASS - test_delayed_cancel
===================================================================
START - test_delayed_pending
 PASS - test_delayed_pending
===================================================================
TESTSUITE workqueue_delayed succeeded

Running TESTSUITE workqueue_triggered
===================================================================
START - test_triggered
 PASS - test_triggered
===================================================================
... (remaining triggered cases) ...
===================================================================
TESTSUITE workqueue_triggered succeeded
