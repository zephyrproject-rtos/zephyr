.. zephyr:code-sample:: multi-thread-blinky
   :name: Basic thread manipulation
   :relevant-api: gpio_interface thread_apis

   Spawn multiple threads that blink LEDs and print information to the console.

Overview
********

This example demonstrates spawning multiple threads using
:c:func:`K_THREAD_DEFINE`. It spawns three threads. Each thread is then defined
at compile time using K_THREAD_DEFINE.

The first two each control an LED. These LEDs, ``led0`` and ``led1``, have
loop control and timing logic controlled by separate functions.

- ``blink0()`` controls ``led0`` and has a 100ms sleep cycle
- ``blink1()`` controls ``led1`` and has a 1000ms sleep cycle

When either of these threads toggles its LED, it also pushes information into a
:ref:`FIFO <fifos_v2>` identifying the thread/LED and how many times it has
been toggled.

The third thread uses :c:func:`printk` to print the information added to the
FIFO to the device console.

Requirements
************

The board must have two LEDs connected via GPIO pins. These are called "User
LEDs" on many of Zephyr's :ref:`boards`. The LEDs must be configured using the
``led0`` and ``led1`` :ref:`devicetree <dt-guide>` aliases, usually in the
:ref:`BOARD.dts file <devicetree-in-out-files>`.

You will see one of these errors if you try to build this sample for an
unsupported board:

.. code-block:: none

   Unsupported board: led0 devicetree alias is not defined
   Unsupported board: led1 devicetree alias is not defined

Building
********

For example, to build this sample for :ref:`96b_carbon_board`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/threads
   :board: 96b_carbon
   :goals: build flash
   :compact:

Change ``96b_carbon`` appropriately for other supported boards.
