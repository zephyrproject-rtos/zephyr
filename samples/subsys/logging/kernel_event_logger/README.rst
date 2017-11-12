.. _kerneleventlogger_sample:

Kernel Event Logger Sample
################################

Overview
********

A simple application that demonstrates use of kernel event
logger feature. Two threads (A and B) of the same priority
are created and main thread is configured to have low priority,
which reads the events from kernel event logger's buffer and
prints to the console. When thread 'A' gets scheduled it will
sleep for 1 second. Similarly when thread 'B' gets scheduled
it will sleep for 0.5 seconds. When both A and B are sleeping,
the main thread gets scheduled and retrieves the events captured
from kernel event logger's buffer and prints to the console.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/logging/kernel_event_logger
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

       tid of context switched thread = 400080 at time = 96538
       tid of context switched thread = 4000c0 at time = 98047
       thread = 400080, is moved to = REDAY_Q , at time = 51019657
       thread = 400040, is moved to = REDAY_Q , at time = 51024998

