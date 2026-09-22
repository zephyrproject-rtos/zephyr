Title: Workqueue APIs

Description:

This test verifies the kernel workqueue APIs. It is organized into several
Ztest suites:

1. workqueue_api
   - test_workq_start_stop, test_workq_stop_essential and test_workq_run_stop
     exercise starting and stopping a work queue and its worker thread.
   - test_workq_priority and test_workq_delayable_remaining cover the queue
     thread priority and the remaining delay of a delayable item.

2. workqueue_delayed
   - test_workq_submit_sequence submits work items from cooperative and
     preemptible threads and checks they run in submission order.
   - test_workq_delayed_submit_on_expiry schedules delayed work items with
     staggered delays and checks they are all submitted and processed.
   - test_workq_delayed_pending and test_workq_delayed_cancel check pending
     and cancellation of delayed work.
   - test_workq_delayable_define covers static definition of a delayable
     work item.

3. workqueue_triggered
   - test_workq_triggered_signal and its variants (already_signalled,
     resubmit, no_wait, wait, from_msgq, cancel, ...) exercise poll-triggered
     work.
   - test_workq_resubmit_from_handler covers resubmitting a work item from
     within its own handler.

4. workqueue_work_timeout (built with CONFIG_WORKQUEUE_WORK_TIMEOUT=y)
   - test_workq_work_timeout verifies the per-work-item timeout behavior.

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
START - test_workq_run_stop
 PASS - test_workq_run_stop
===================================================================
START - test_workq_start_stop
 PASS - test_workq_start_stop
===================================================================
START - test_workq_stop_essential
 PASS - test_workq_stop_essential
===================================================================
TESTSUITE workqueue_api succeeded

Running TESTSUITE workqueue_delayed
===================================================================
START - test_workq_submit_sequence
 PASS - test_workq_submit_sequence
===================================================================
START - test_workq_delayed_cancel
 PASS - test_workq_delayed_cancel
===================================================================
START - test_workq_delayed_pending
 PASS - test_workq_delayed_pending
===================================================================
TESTSUITE workqueue_delayed succeeded

Running TESTSUITE workqueue_triggered
===================================================================
START - test_workq_triggered_signal
 PASS - test_workq_triggered_signal
===================================================================
... (remaining triggered cases) ...
===================================================================
TESTSUITE workqueue_triggered succeeded
