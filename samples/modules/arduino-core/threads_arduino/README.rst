.. _arduino_nano_33_ble_multi_thread_blinky:

Basic Thread Example
####################

Overview
********

This example demonstrates spawning multiple threads using
:c:func:`K_THREAD_DEFINE`. It spawns three threads. Each thread is then defined
at compile time using :c:func:`K_THREAD_DEFINE`.

These three each control an LED. These LEDs, ``LED_BUILTIN``, ``D10`` and ``D11``, have
loop control and timing logic controlled by separate functions.

- ``blink0()`` controls ``LED_BUILTIN`` and has a 100ms sleep cycle
- ``blink1()`` controls ``D11`` and has a 1000ms sleep cycle
- ``loop()`` controls ``D10`` and has a 300ms sleep cycle

Requirements
************

The board must have two LEDs connected via GPIO pins and one builtin LED. These are called "User
LEDs" on many of Zephyr's :ref:`boards`. The LEDs must be mapped using the ``<board_name_pinmap.h>``
``LED_BUILTIN``, ``D10`` and ``D11`` to the :ref:`devicetree <dt-guide>` aliases, in the
variants folder.

You will see one of these errors if you try to build this sample for an
unsupported board:

.. code-block:: none

   Unsupported board: LED_BUILTIN devicetree alias is not defined
   Unsupported board: D11 devicetree alias is not defined

Building
********

For example, to build this sample for :ref:`arduino_nano_33_ble`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/arduino-threads
   :board: arduino_nano_33_ble
   :goals: build flash
   :compact:

Change ``arduino_nano_33_ble`` appropriately for other supported boards.
