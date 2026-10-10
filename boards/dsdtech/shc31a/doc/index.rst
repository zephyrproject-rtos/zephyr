.. zephyr:board:: dsdtech_shc31a

Overview
********

The DSD TECH SH-C31A is a non-isolated USB to CAN FD adapter derived from the CANable 2.0 open
hardware design. Its schematic and board files are published by DSD TECH under the
CERN-OHL-S-2.0 license (see `SH-C31A hardware files`_).

- STM32G431C8T6 or STM32G431CBT6 MCU in LQFP48 package
- SIT1051T CAN FD transceiver, up to 5 Mbit/s data phase bit rate
- USB 2.0 full speed, USB Type-A plug
- 3.81 mm 3-pin screw terminal (CAN_H, CAN_L, GND)
- 120 Ohm termination switch
- BOOT switch for entering the STM32 system bootloader
- Red power LED and two green LEDs (TXD and RXD) controlled by the MCU
- Five pin SWD header (H1)

More information about the board, including its user manual, component datasheets and vendor
firmware, can be found in the `SH-C31A repository`_. The schematic is also available as a PDF
(`SH-C31A schematic`_).

Hardware
********

The board uses the 64 KiB flash layout of the STM32G431C8, which also runs on boards fitted with
the 128 KiB STM32G431CB.

The transceiver has no MCU-controlled mode pins: its silent mode input is tied to ground, and
its I/O supply is switched on by an RC-delayed load switch after power-up.

More information about the STM32G431 can be found here:

- `STM32G431C8 on www.st.com`_
- `STM32G4 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

.. list-table::
   :header-rows: 1

   * - Pin
     - Function
   * - PB8
     - FDCAN1_RX, also BOOT0
   * - PB9
     - FDCAN1_TX
   * - PA15
     - TXD LED (green, active low)
   * - PA0
     - RXD LED (green, active low)
   * - PA11
     - USB_DM
   * - PA12
     - USB_DP
   * - PA13
     - SWDIO (H1 pin 1)
   * - PA14
     - SWCLK (H1 pin 2)
   * - PG10
     - NRST (H1 pin 3)

H1 pin 4 is +3.3 V and pin 5 is GND.

System Clock
------------

The board has no crystal. The system clock is 160 MHz from the PLL, driven by HSI16. FDCAN1 is
clocked from PLLQ at 80 MHz, and USB from HSI48.

BOOT0 and CAN RX
----------------

PB8 is both FDCAN1 RX and BOOT0. The transceiver drives it high whenever its I/O supply is on
and the bus is recessive or unconnected. The RC delay on that supply holds PB8 low during a
normal power-up, but not on a reset or after a short power dip, so with the factory option bytes
such a reset can start the STM32 system bootloader instead of the application.

The ``nSWBOOT0`` and ``nBOOT0`` option bytes make the MCU ignore BOOT0 and always start from main
flash. The DSD TECH firmware v1.5 sets them, and they persist after other firmware is flashed;
the BOOT switch then has no effect. See the `SH-C31A firmware notes`_ for how to restore it.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

The default runner is ``dfu-util``, which programs the board through the STM32 system
bootloader over USB:

#. Unplug the adapter, set the BOOT switch, and plug it back in. The adapter enumerates as
   ``0483:df11``.
#. Build and flash an application, for example :zephyr:code-sample:`blinky`:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: dsdtech_shc31a
      :goals: build flash
      :compact:

#. Set the BOOT switch back and replug the adapter to start the application.

Alternatively, connect an ST-LINK or another SWD probe to header H1 and flash with
``west flash -r stm32cubeprogrammer`` or ``west flash -r openocd``.

Debugging
=========

Applications can be debugged through header H1 in the usual way:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: dsdtech_shc31a
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _SH-C31A repository:
   https://github.com/dsdtech-official/can-adapters/tree/main/SH-C31A

.. _SH-C31A hardware files:
   https://github.com/dsdtech-official/can-adapters/tree/main/SH-C31A/hardware

.. _SH-C31A schematic:
   https://github.com/dsdtech-official/can-adapters/blob/main/SH-C31A/hardware/SH-C31A-V1.1.pdf

.. _SH-C31A firmware notes:
   https://github.com/dsdtech-official/can-adapters/blob/main/SH-C31A/firmware/README.md

.. _STM32G431C8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431c8.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
