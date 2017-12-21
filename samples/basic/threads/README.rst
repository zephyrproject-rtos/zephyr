.. _96b_carbon_multi_thread_blinky:

Basic Thread Example
####################

Overview
********

This example demonstrates spawning of multiple threads using K_THREAD_DEFINE.

The example works by spawning three threads. The first two threads control a
separate LED. Both of these LEDs (USR1 and USR2) have their individual loop
control and timing logic defined by separate functions.

After either thread toggles its LED, it also pushes information into a
FIFO identifying which thread toggled its LED and how many times it
was done.

The third thread, ``uart_out()``, uses printk (over the UART) to
display the information that comes through the FIFO.

- blink1() controls the USR1 LED that has a 100ms sleep cycle
- blink2() controls the USR2 LED that has a 1000ms sleep cycle

Each thread is then defined at compile time using K_THREAD_DEFINE.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/basic/threads
   :board: 96b_carbon
   :goals: build flash
   :compact:
