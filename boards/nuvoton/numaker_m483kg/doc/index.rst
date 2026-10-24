.. zephyr:board:: numaker_m483kg

Overview
********

The NuMaker-M483KG is a development board from Nuvoton based on the NuMicro®
M483KGCAE2A MCU with an Arm® Cortex®-M4F core. It consists of the M483 platform
and an on-board Nu-Link2-Me debugger and programmer, which can be separated
from the board.

Features:
=========
- 32-bit Arm Cortex®-M4F M483KGCAE2A MCU
- Core clock up to 192 MHz
- 256 KB embedded flash and 128 KB SRAM
- 12 MHz crystal
- USB 2.0 Full-Speed connector
- SD card slot
- M483KGCAE2A full pins extension connectors
- Arduino UNO compatible extension connectors
- Ammeter connector for measuring the MCU power consumption
- Three push-buttons: one is for reset and the other two are user-defined
- Four LEDs: one is for power indication and the other three are user-defined
- On-board Nu-Link2-Me debugger/programmer with SWD connector and virtual COM port

More information about the board can be found at the `NuMaker-M483KG User Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

More details about the supported peripherals are available in `M480 TRM`_.

Building and Flashing
*********************

.. zephyr:board-supported-runners::

Flashing
========

Flashing uses the OpenOCD fork from Nuvoton, `OpenOCD-Nuvoton`_, which
provides the ``numicroM4.cfg`` target and the M480 flash driver. Pass its
location to ``west flash`` with ``--openocd`` and ``--openocd-search``.

Here is an example for the :zephyr:code-sample:`hello_world` application.

The on-board Nu-Link2-Me can emulate UART0 as a virtual COM port over USB.
To enable this, set the VCOM switch ICESW2 1-2 (TXD RXD) to ON.
Connect the NuMaker-M483KG to your host computer using the ICE USB port, then
run a serial host program to connect with your board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: numaker_m483kg
   :goals: flash
   :flash-args: --openocd <OpenOCD-Nuvoton>/src/openocd --openocd-search <OpenOCD-Nuvoton>/tcl

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: numaker_m483kg
   :goals: debug

References
**********

.. target-notes::

.. _NuMaker-M483KG User Manual:
   https://www.nuvoton.com/resource-files/UM_NuMaker-M483KG_EN_Rev1.00.pdf
.. _OpenOCD-Nuvoton:
   https://github.com/OpenNuvoton/OpenOCD-Nuvoton

.. _M480 TRM:
   https://www.nuvoton.com/export/resource-files/TRM_M480_Series_EN_Rev2.02.pdf
