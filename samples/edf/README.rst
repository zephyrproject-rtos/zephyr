.. zephyr:code-sample:: edf
   :name: Earliest Deadline First (EDF)

   Runs three threads under the EDF scheduler.

Overview
********

A sample that creates a set of periodic threads and run them
under the Earliest Deadline First scheduling algorithm. The
threads do nothing special - they just increment a counter,
busy wait for the configured WCET (worst-case execution time),
and sleep waiting for the next period to come.

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

The example below shows the output for the target qemu_riscv32
considering thread 1 with 30-millisecond relative deadline, while
thread 2 has 20 milliseconds and thread 3 has 10 milliseconds. They
are initially triggered at roughly the same time in order 1-3-2 (as
you can see on the start timestamps), but thread 3 takes the lead
and finishes in first thanks to its earliest deadline. Thread 2
follows and thread 1 finishes in last, even though it was triggered
before the others. 

.. code-block:: console

  [ 3 ] 1         start 10135661   end 10146064    dead 10235018
  [ 2 ] 1         start 10171833   end 10271886    dead 10334639
  [ 1 ] 1         start 10112835   end 10300478    dead 10408821