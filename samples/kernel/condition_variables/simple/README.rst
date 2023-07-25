.. _samples_kernel_simple_condition_variables:

Condition Variables
###################

Overview
********

This sample demonstrates the usage of condition variables in a
multithreaded application. Condition variables are used with a mutex
to signal changing states (conditions) from worker thread to the main
thread. Main thread uses a condition variable to wait for a condition to
become true. Main thread and the worker thread alternate between their
execution based on when the worker thread signals the main thread that is
pending on the condition variable. The sample can be used with any
:ref:`supported board <boards>` and prints the sample output shown to
the console.

Building and Running
********************

This application can be built and executed on Native Posix as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/condition_variables/simple
   :host-os: unix
   :board: native_posix
   :goals: run
   :compact:

To build for another board, change "native_posix" above to that board's name.

Sample Output
=============

.. code-block:: console

    [thread 0] working (0/5)
    [thread 1] working (0/5)
    [thread 2] working (0/5)
    [thread 3] working (0/5)
    [thread 4] working (0/5)
    [thread 5] working (0/5)
    [thread 6] working (0/5)
    [thread 7] working (0/5)
    [thread 8] working (0/5)
    [thread 9] working (0/5)
    [thread 10] working (0/5)
    [thread 11] working (0/5)
    [thread 12] working (0/5)
    [thread 13] working (0/5)
    [thread 14] working (0/5)
    [thread 15] working (0/5)
    [thread 16] working (0/5)
    [thread 17] working (0/5)
    [thread 18] working (0/5)
    [thread 19] working (0/5)
    [thread 0] working (1/5)
    [thread 1] working (1/5)
    [thread 2] working (1/5)
    [thread 3] working (1/5)
    [thread 4] working (1/5)
    [thread 5] working (1/5)
    [thread 6] working (1/5)
    [thread 7] working (1/5)
    [thread 8] working (1/5)
    [thread 9] working (1/5)
    [thread 10] working (1/5)
    [thread 11] working (1/5)
    [thread 12] working (1/5)
    [thread 13] working (1/5)
    [thread 14] working (1/5)
    [thread 15] working (1/5)
    [thread 16] working (1/5)
    [thread 17] working (1/5)
    [thread 18] working (1/5)
    [thread 19] working (1/5)
    [thread zephyr_app_main] done is 0 which is < 20 so waiting on cond
    [thread 0] working (2/5)
    [thread 1] working (2/5)
    [thread 2] working (2/5)
    [thread 3] working (2/5)
    [thread 4] working (2/5)
    [thread 5] working (2/5)
    [thread 6] working (2/5)
    [thread 7] working (2/5)
    [thread 8] working (2/5)
    [thread 9] working (2/5)
    [thread 10] working (2/5)
    [thread 11] working (2/5)
    [thread 12] working (2/5)
    [thread 13] working (2/5)
    [thread 14] working (2/5)
    [thread 15] working (2/5)
    [thread 16] working (2/5)
    [thread 17] working (2/5)
    [thread 18] working (2/5)
    [thread 19] working (2/5)
    [thread 0] working (3/5)
    [thread 1] working (3/5)
    [thread 2] working (3/5)
    [thread 3] working (3/5)
    [thread 4] working (3/5)
    [thread 5] working (3/5)
    [thread 6] working (3/5)
    [thread 7] working (3/5)
    [thread 8] working (3/5)
    [thread 9] working (3/5)
    [thread 10] working (3/5)
    [thread 11] working (3/5)
    [thread 12] working (3/5)
    [thread 13] working (3/5)
    [thread 14] working (3/5)
    [thread 15] working (3/5)
    [thread 16] working (3/5)
    [thread 17] working (3/5)
    [thread 18] working (3/5)
    [thread 19] working (3/5)
    [thread 0] working (4/5)
    [thread 1] working (4/5)
    [thread 2] working (4/5)
    [thread 3] working (4/5)
    [thread 4] working (4/5)
    [thread 5] working (4/5)
    [thread 6] working (4/5)
    [thread 7] working (4/5)
    [thread 8] working (4/5)
    [thread 9] working (4/5)
    [thread 10] working (4/5)
    [thread 11] working (4/5)
    [thread 12] working (4/5)
    [thread 13] working (4/5)
    [thread 14] working (4/5)
    [thread 15] working (4/5)
    [thread 16] working (4/5)
    [thread 17] working (4/5)
    [thread 18] working (4/5)
    [thread 19] working (4/5)
    [thread 0] done is now 1. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 1 which is < 20 so waiting on cond
    [thread 1] done is now 2. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 2 which is < 20 so waiting on cond
    [thread 2] done is now 3. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 3 which is < 20 so waiting on cond
    [thread 3] done is now 4. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 4 which is < 20 so waiting on cond
    [thread 4] done is now 5. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 5 which is < 20 so waiting on cond
    [thread 5] done is now 6. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 6 which is < 20 so waiting on cond
    [thread 6] done is now 7. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 7 which is < 20 so waiting on cond
    [thread 7] done is now 8. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 8 which is < 20 so waiting on cond
    [thread 8] done is now 9. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 9 which is < 20 so waiting on cond
    [thread 9] done is now 10. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 10 which is < 20 so waiting on cond
    [thread 10] done is now 11. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 11 which is < 20 so waiting on cond
    [thread 11] done is now 12. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 12 which is < 20 so waiting on cond
    [thread 12] done is now 13. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 13 which is < 20 so waiting on cond
    [thread 13] done is now 14. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 14 which is < 20 so waiting on cond
    [thread 14] done is now 15. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 15 which is < 20 so waiting on cond
    [thread 15] done is now 16. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 16 which is < 20 so waiting on cond
    [thread 16] done is now 17. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 17 which is < 20 so waiting on cond
    [thread 17] done is now 18. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 18 which is < 20 so waiting on cond
    [thread 18] done is now 19. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done is 19 which is < 20 so waiting on cond
    [thread 19] done is now 20. Signalling cond.
    [thread zephyr_app_main] wake - cond was signalled.
    [thread zephyr_app_main] done == 20 so everyone is done
