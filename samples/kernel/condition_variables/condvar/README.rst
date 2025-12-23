.. zephyr:code-sample:: kernel-condvar
   :name: Condition Variables
   :relevant-api: condvar_apis

   Manipulate condition variables in a multithreaded application.

Overview
********

This sample demonstrates the usage of :ref:`condition variables <condvar>` in a
multithreaded application. Condition variables are used with a mutex
to signal changing states (conditions) from one thread to another
thread. A thread uses a condition variable to wait for a condition to
become true. Different threads alternate between their entry point
function execution based on when they signal the other thread that is
pending on the condition variable. The sample can be used with any
:ref:`supported board <boards>` and prints the sample output shown to
the console.

Building and Running
********************

This application can be built and executed on :zephyr:board:`native_sim <native_sim>` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/condition_variables/condvar
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

To build for another board, change ``native_sim`` above to that board's name.

Sample Output
=============

.. code-block:: console

    Starting watch_count: thread 1
    watch_count: thread 1 Count= 0. Going into wait...
    inc_count: thread 2, count = 1, unlocking mutex
    inc_count: thread 3, count = 2, unlocking mutex
    inc_count: thread 2, count = 3, unlocking mutex
    inc_count: thread 3, count = 4, unlocking mutex
    inc_count: thread 2, count = 5, unlocking mutex
    inc_count: thread 3, count = 6, unlocking mutex
    inc_count: thread 2, count = 7, unlocking mutex
    inc_count: thread 3, count = 8, unlocking mutex
    inc_count: thread 2, count = 9, unlocking mutex
    inc_count: thread 3, count = 10, unlocking mutex
    inc_count: thread 2, count = 11, unlocking mutex
    inc_count: thread 3, count = 12  Threshold reached.Just sent signal.
    inc_count: thread 3, count = 12, unlocking mutex
    watch_count: thread 1 Condition signal received. Count= 12
    watch_count: thread 1 Updating the value of count...
    watch_count: thread 1 count now = 137.
    watch_count: thread 1 Unlocking mutex.
    inc_count: thread 2, count = 138, unlocking mutex
    inc_count: thread 3, count = 139, unlocking mutex
    inc_count: thread 2, count = 140, unlocking mutex
    inc_count: thread 3, count = 141, unlocking mutex
    inc_count: thread 2, count = 142, unlocking mutex
    inc_count: thread 3, count = 143, unlocking mutex
    inc_count: thread 2, count = 144, unlocking mutex
    inc_count: thread 3, count = 145, unlocking mutex
    Main(): Waited and joined with 3 threads. Final value of count = 145. Done.
