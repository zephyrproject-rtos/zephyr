Title: Preemptible Threads Pending on Kernel Objects

Description:

This test verifies that preemptible threads can pend on kernel objects and
are unblocked in the expected order. The pending suite runs three cases:

1. test_pending_fifo
   - Preemptible threads block on a k_fifo, time out in the correct order
     and receive the queued data correctly.

2. test_pending_lifo
   - Preemptible threads block on a k_lifo, time out in the correct order
     and receive the queued data correctly.

3. test_pending_timer
   - A preemptible thread waits on a k_timer and is released when the
     timer expires.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/pending

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/pending
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE pending
===================================================================
START - test_pending_fifo
 PASS - test_pending_fifo
===================================================================
START - test_pending_lifo
 PASS - test_pending_lifo
===================================================================
START - test_pending_timer
 PASS - test_pending_timer
===================================================================
TESTSUITE pending succeeded
