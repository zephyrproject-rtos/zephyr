.. zephyr:board:: imx943_evk

Overview
********

The IMX943LP5EVK-19 board is a design and evaluation platform based on the
NXP i.MX 943 processor. The i.MX 943 processor integrates up to four Arm
Cortex-A55 cores, along with two Arm Cortex-M33 cores and two Arm Cortex-M7
cores for functional safety. With PLCs, I/O controllers, V2X accelerators,
ML acceleration, energy management, and advanced security, the i.MX 943
processor provides optimized performance and power efficiency for industrial,
IoT, and automotive devices. The i.MX943 device on the board comes in a
compact 19 x 19 mm package.

Hardware
********

- i.MX 943 automotive applications processor

  - The processor integrates up to four Arm Cortex-A55 cores, and supports
    functional safety with built-in Arm Cortex-M33 and -M7 cores

- DRAM memory: 8-Gbit LPDDR5 DRAM
- XSPI interface: 64 MB octal SPI NOR flash memory
- eMMC: 32 GB eMMC NAND flash memory
- uSDHC interface: an SD card slot
- USB interface: Two USB Type-C ports
- Ethernet interface: seven Ethernet ports
- PCIe interface: one M.2 slot and one PCIe x4 slot.
- FlexCAN interface: four CAN controller with four CAN connector.
- LPUART interface
- LPSPI interface
- LPI2C interface
- SAI interface
- MQS interface
- MICFIL interface
- LVDS interface
- ADC interface
- SINC interface
- Debug interface
  - One USB-to-UART/MPSSE device, FT4232H
  - One USB 3.2 Type-C connector (J15) for FT4232H provides quad serial ports
  - JTAG header J16

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
------------

This board configuration uses a system clock frequency of 24 MHz for Cortex-A55.
Cortex-A55 Core runs up to 1.7 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1 for Cortex-A55.

Programming and Debugging (A55)
*******************************

.. zephyr:board-supported-runners::

There are multiple methods to program and debug Zephyr on the A55 core:

Option 1. Boot Zephyr by Using JLink Runner
===========================================

Dependency
----------

Need to disable all watchdog in U-Boot, otherwise, watchdog will reset the board
after Zephyr start up from the same A55 Core.

Setup
-----

The default runner for the board is JLink, connect the EVK board's JTAG connector to
the host computer using a J-Link debugger, power up the board and stop the board at
U-Boot command line, execute the following U-boot command to disable D-Cache:

.. code-block:: console

    dcache off

then use "west flash" or "west debug" command to load the zephyr.bin
image from the host computer and start the Zephyr application on A55 core0.

Flash and Run
-------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx943_evk/mimx94398/a55
   :goals: flash

Then the following log could be found on UART1 console:

.. code-block:: console

    *** Booting Zephyr OS build v4.1.0-3650-gdb71736adb68 ***
    Hello World! imx943_evk/mimx94398/a55

Debug
-----

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx943_evk/mimx94398/a55
   :goals: debug

Option 2. Boot Zephyr by Using U-Boot Command
=============================================

U-Boot "go" command can be used to start Zephyr on A55 Core0.

Dependency
----------

Need to disable all watchdog in U-Boot, otherwise, watchdog will reset the board
after Zephyr start up from the same A55 Core.

Step 1: Build Zephyr application
--------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx943_evk/mimx94398/a55
   :goals: build

Step 2: Download Zephyr Image into DDR Memory
---------------------------------------------

Firstly need to download Zephyr binary image into DDR memory, it can use tftp:

.. code-block:: console

    tftp 0xd0000000 zephyr.bin

Or copy the Zephyr image ``zephyr.bin`` SD card and plug the card into the board, for example
if copy to the FAT partition of the SD card, use the following U-Boot command to load the image
into DDR memory (assuming the SD card is dev 1, fat partition ID is 1, they could be changed
based on actual setup):

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin;

Step 3: Boot Zephyr
-------------------

Use the following command to boot Zephyr on the core0:

.. code-block:: console

    dcache off; icache flush; go 0xd0000000;

Then the following log could be found on UART1 console:

.. code-block:: console

    *** Booting Zephyr OS build v4.1.0-3650-gdb71736adb68 ***
    Hello World! imx943_evk/mimx94398/a55

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer
