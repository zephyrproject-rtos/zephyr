.. zephyr:code-sample:: edf
   :name: Earliest Deadline First (EDF)

   Runs three threads under the EDF scheduler.

Overview
********

A sample that creates a set of periodic threads and run them
under the Earliest Deadline First scheduling algorithm. The
threads do nothing special - they just busy wait for the
configured WCET (worst-case execution time) and sleep waiting
for the next period to come.

Each thread has a timer and a message queue associated with it.
What makes them run periodically is the timer, which regularly
sends a message to the respective thread message queue. Keep in
mind that the *message* is not important; instead, the *act* of
sending the message is. When the thread receives a message, it
unblocks and executes for a cycle, remaining dormant afterwards
until a new message comes by.


Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/edf
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build and run on a physical target (i.e. XIAO ESP32-C3) instead,
run the following:

.. zephyr-app-commands::
   :zephyr-app: samples/edf
   :board: xiao_esp32c3
   :goals: build flash
   :compact:

Sample Output
=============

A thread execution cycle is composed of, for a thread of id = 1:

.. code-block:: console

  [1-x-1-1-1-1]

Where **x** is the thread absolute deadline and gets updated at every
new cycle based on the relative value. The interval between the ids
is given by the task's wcet/5 (thus, the quicker the task, the smaller
the period). An expected outcome would be the following:

.. code-block:: console

   [1-3009-1-1-1-1-1]-[2-6010-2-2-2-2-2]-[3-10010-3-3-3-3-[1-9009-1-1-1-1-1]-3]

In this setting, tasks 1, 2 and 3 are activated at the same instant, so
they execute sequentially in the order of their absolute deadlines.
However, the second instance of task 1 is activated with an absolute
deadline of T=9009, which is smaller than the T=10010 of the running
task 3. Thus, task 1 *preempts* task 3.

The sample allows the user to tweak the values of each thread and watch
the outcome following the convention above.
