Title: Thread Sleep and Wakeup APIs

Description:

This test verifies that the kernel thread sleep and wakeup APIs operate as
expected. It exercises three behaviors:

1. test_sleep
   - k_sleep() blocks for the requested duration (within one tick of slop).
   - k_wakeup() cancels a pending sleep immediately, whether the wakeup is
     issued from a helper thread, an ISR (via irq_offload()), or the main
     thread.

2. test_sleep_forever
   - k_sleep(K_FOREVER) never expires on its own and only returns after an
     explicit k_wakeup(), reporting K_TICKS_FOREVER.

3. test_usleep
   - k_usleep() never sleeps for less than the minimum tick granularity. The
     test loops minimal sleeps to span one second of ticks and checks the
     aggregate elapsed time against computed lower/upper bounds, retrying to
     tolerate timing jitter under emulation.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/sleep

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/sleep
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE sleep
===================================================================
START - test_sleep
 PASS - test_sleep
===================================================================
START - test_sleep_forever
 PASS - test_sleep_forever
===================================================================
START - test_usleep
 PASS - test_usleep
===================================================================
TESTSUITE sleep succeeded
