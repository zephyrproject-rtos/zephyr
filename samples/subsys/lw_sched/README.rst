.. zephyr:code-sample:: lw_sched
   :name: Lightweight (LW) scheduler
   :relevant-api: subsys_lw_sched

   Use the lightweight scheduler to schedule a lightweight task.

Lightweight (LW) Scheduler
##########################

Overview
********

This sample demonstrates the use of the :ref:`lw_sched_api`, and goes through
the following steps:

#. Initializes the LW scheduler that executes every 50 msec
#. Starts two sample tasks that execute every interval
#. Display the tasks' count info every second for 10 seconds
#. Abort the LW scheduler
#. Display the tasks' final count info

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU
as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lw_sched
   :host-os: unix
   :board: qemu_x86_64
   :goals: run
   :compact:

To build for another board, change "qemu_x86_64" above to that board's name.

Sample output
=============
.. code-block:: console

    Initializing lightweight scheduler ...
    Initializing lightweight task1 (0x106000)...
    Initializing lightweight task2 (0x114448)...
    Sleeping for 10 seconds...
    Task counts (@ 1 sec) : {9 0}
    Task counts (@ 2 sec) : {30 0}
    Task counts (@ 3 sec) : {50 10}
    Lightweight task 0x106000 is done
    Task counts (@ 4 sec) : {70 30}
    Task counts (@ 5 sec) : {70 50}
    Task counts (@ 6 sec) : {70 70}
    Task counts (@ 7 sec) : {70 91}
    Task counts (@ 8 sec) : {70 111}
    Lightweight task 0x114448 is done
    Task counts (@ 9 sec) : {70 120}
    Task counts (@ 10 sec) : {70 120}
    Shutting down lightweight scheduler ...
    Auto-freeing lightweight task 0x114448
    Final task 1 count: 70
    Final task 2 count: 120
