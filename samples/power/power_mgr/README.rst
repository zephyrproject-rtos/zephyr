Power management demo
#####################

Overview
********

A sample implementation of a power manager app that uses the Zephyr
power management infrastructure.

This app will cycle through the various power schemes at every call
to _sys_soc_suspend() hook function.
It will cycle through the following states:

1. CPU Low Power State

2. Deep Sleep - demonstrates invoking suspend/resume handlers of
   devices to save device states and switching to deep sleep state.

3. No-op - no operation and letting kernel do its idle

Requirements
************

This application uses Intel Quark SE Microcontroller C1000 board for
the demo. It demonstrates power operations on the x86 and ARC cores in
the board.

.. note::

  PM support on Intel Quark SE Microcontroller C1000 board requires
  `boot loader version 1.3
  <https://github.com/quark-mcu/qm-bootloader/releases>`_ or greater.


Building and Running
********************

.. code-block:: console

    make BOARD=<board>

Sample Output
=============

x86 core output
---------------

.. code-block:: console

  Power Management Demo on x86

  Application main thread

  Low power state entry!

  Low power state exit!
  Total Elapsed From Suspend To Resume = 131073 RTC Cycles
  Wake up event handler

  Application main thread

  Deep sleep entry!
  Wake from Deep Sleep!

  Deep sleep exit!
  Total Elapsed From Suspend To Resume = 291542 RTC Cycles
  Wake up event handler

  Application main thread

  No PM operations done

  Application main thread

  Low power state entry!

  Low power state exit!

  ...

ARC core output
---------------

.. code-block:: console

  Power Management Demo on arc

  Application main thread

  Low power state entry!

  Low power state exit!
  Total Elapsed From Suspend To Resume = 131073 RTC Cycles
  Wake up event handler

  Application main thread

  No PM operations done

  Application main thread

  Low power state entry!

  Low power state exit!

  ...
