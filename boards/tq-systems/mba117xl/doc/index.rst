.. zephyr:board:: mba117xl

.. SPDX-License-Identifier: CC-BY-4.0
 
.. Copyright (c) 2019-2023 TQ-Systems GmbH <license@tq-group.com>,
.. D-82229 Seefeld, Germany.


Overview
********

The TQMa117xL based on the i.MX RT1170 a Cortex M7 with up to 1GHz supports 
with many features and with up to 2 Gigabit Ethernet interfaces in the 
implementation of applications where an energy efficient but still powerful 
system should be used.

For more information about the TQMa117xL SoC and TQMBa117xL board, see
these references:

- `TQ-Systems Website`_
- `TQMa117xL Website`_

Hardware Requirements
=====================

.. list-table::
   :header-rows: 1
   :widths: 25 45 10

   * - Hardware
     - Description
     - Pin
   * - Micro-USB cable
     - Provides power, programming, and console connectivity.
     - X1
   * - J-Link debug probe
     - Required for debugging and flashing.
     - X39
   * - Personal computer
     - Used to build, flash, and interact with applications.
     - –
   * - 24 V power supply
     - External supply for the board.
     - X13

.. note::

   These requirements apply to the *MBa117xL* carrier fitted with the
   *TQMBa1176L* system-on-module.

External Memory
===============

.. list-table::
   :header-rows: 1
   :widths: 25 15 40

   * - Device
     - Controller
     - Status
   * - **W9825G6KH** SDRAM
     - SEMC
     - Enabled by a Device Configuration Data (DCD) block that configures the
       SEMC at boot.
   * - **W25Q512NWEIQ** QSPI flash
     - FLEXSPI
     - Enabled by a Flash Configuration Block (FCB) that sets up FLEXSPI at
       boot.  Supported for XIP only.

Supported Features
==================

Only the Cortex-M7 core is enabled at present.

.. list-table::
   :header-rows: 1
   :widths: 12 12 34

   * - Interface
     - Controller
     - Driver / Component
   * - NVIC
     - on-chip
     - Nested-vector interrupt controller
   * - SYSTICK
     - on-chip
     - SysTick timer
   * - GPIO
     - on-chip
     - ``gpio``
   * - COUNTER
     - on-chip
     - ``gpt``
   * - TIMER
     - on-chip
     - ``gpt``
   * - CAN
     - on-chip
     - ``flexcan`` 
   * - SPI
     - on-chip
     - ``spi`` 
   * - I²C
     - on-chip
     - ``i2c``
   * - PWM
     - on-chip
     - ``for display usage``
   * - UART
     - on-chip
     - Serial (polling / interrupt / async)
   * - DMA
     - on-chip
     - ``dma``
   * - WATCHDOG
     - on-chip
     - ``watchdog`` 
   * - ENET1G
     - on-chip
     - 10/100/1000 Mbit Ethernet 
   * - SAI
     - on-chip
     - ``i2s`` audio
   * - USB
     - on-chip
     - USB device 
   * - HWINFO
     - on-chip
     - Unique device serial number 
   * - DISPLAY
     - on-chip
     - eLCDIF / MIPI-DSI, tested with
       ``rk055hdmipi4ma0``, ``mba117xl_tm070`` shields 
   * - FLEXSPI
     - on-chip
     - Flash programming 
   * - SDHC
     - on-chip
     - SD host controller 
   * - PIT
     - on-chip
     - ``pit`` 

The default configuration is at
``boards/tq-systems/mba117xl/mba117xl_cm7_defconfig``.

System Clock
============

The SoC uses SysTick as the Zephyr system clock and runs at **996 MHz**.

Serial Port
===========

The i.MX RT1170 has twelve LPUARTs; Zephyr maps the console to ``LPUART1``.

Programming and Debugging
*************************

Build and flash applications as usual; see :ref:`build_an_application` and
:ref:`application_run`.

Configuring a Debug Probe
=========================

Using J-Link
------------

J-Link is the default *west* runner.  Install
:ref:`jlink-debug-host-tools` and ensure they are in your *PATH*.

Either update the on-board debug circuit with Segger J-Link firmware or attach
an external :ref:`jlink-external-debug-probe`.

Ensure the FLEXSPI1 pin-mux is correct before debugging or flashing.

Typical device string for J-Link GDB:

.. code-block:: json

   "-device",
   "MIMXRT1176xxxA_M7?BankAddr=0x30000000&Loader=nCS@AD18_CLK@AD19_D0@AD20_D1@"
   "AD21_D2@AD22_D3@AD23&BankAddr=0x60000000&Loader="
   "nCS@SDB100_CLK@SDB101_D0@SDB102_D1@SDB103_D2@SDB104_D3@SDB105"

.. attention::

   Program the correct :ref:`setting-fuses` **before the first boot**.

Console Setup
=============

* 115200 baud
* 8 data bits
* No parity
* 1 stop bit
* No flow control

Select the COM port for the desired core.

Setting Fuses
=============

Set the fuses before the first boot.

- **S3:** OFF OFF OFF OFF
- **S4:** OFF OFF OFF OFF
- **S5:** OFF OFF OFF OFF
- **S6:** **ON** OFF ON ON

1. Power off the board and set the DIP switches as above.
2. Connect **UART1** to the host PC.
3. Open *Secure Provisioning Tool* and choose **MIMXRT1176**.
4. Select **UART**, the correct COM port, and 115200 baud.
5. Press *Test Connection*.
6. Open **Build image → OTP Configuration** and read the fuses.
7. Select **fuse 0x9A0** and set bit 10 (value **0x400**).
8. Verify, switch to *Advanced mode*, press *Burn*, and confirm.

.. warning::

   A wrong value in *fuse 0x9A0* will permanently brick the board.

Flashing
========

Example for :zephyr:code-sample:`hello_world`:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mba117xl/cm7
   :goals: flash

Set **SW6 = 0 1 1 1**, power on, open a serial terminal and flash. 
You should see the following output:

   ***** Booting Zephyr OS v3.4.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! mimxrt1170_evk

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mba117xl/cm7
   :goals: debug

Step through with the debugger and observe the same console output.

ENET1G Driver
=============

Current default of ethernet driver is to use 100M Ethernet instance ENET1G.
To use the 1G Ethernet instance ENET1G, include the overlay to west build with
the option ``-DEXTRA_DTC_OVERLAY_FILE=nxp,enet1g.overlay`` instead.

.. _TQ-Systems Website: https://www.tq-group.com
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt1170-crossover-mcu-family-first-ghz-mcu-with-arm-cortex-m7-and-cortex-m4-cores:i.MX-RT1170

.. _TQMa117xL Website: https://support.tq-group.com/en/arm/tqma117xl
