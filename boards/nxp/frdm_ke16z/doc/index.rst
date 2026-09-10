.. zephyr:board:: frdm_ke16z

Overview
********

The FRDM-KE16Z is a development board for NXP Kinetis KE1xZ 32-bit
MCU-based platforms. The FRDM-KE16Z contains a robust TSI module
with up to 25 channels which makes this board highly flexible
for touch keys. Offers options for serial
communication, flash programming, and run-control debugging.

Hardware
********

- MKE16Z64VLF4 MCU (up to 48 MHz, 64 KB flash memory, 8 KB RAM)
- OpenSDA Debug Circuit with a virtual serial port
- Touch electrodes in the self-capacitive mode
- Compatible with FRDM-TOUCH, FRDM-MC-LVBLDC, and Arduino® boards
- User Components such as Reset; RGB LED and two user buttons
- 6-axis FXOS8700CQ digital accelerometer and magnetometer

For more information about the KE1xZ SoC and the FRDM-KE16Z board, see
these NXP reference documents:

- `KE1XZ SOC Website`_
- `FRDM-KE16Z Datasheet`_
- `FRDM-KE16Z Reference Manual`_
- `FRDM-KE16Z Website`_
- `FRDM-KE16Z User Guide`_
- `FRDM-KE16Z Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The KE16 SoC is configured to run at 48 MHz using the FIRC.

Serial Port
===========

The KE16 SoC has three UARTs. UART0 (PTB1, PTB0) is configured for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use Linkserver.

Early versions of this board have an outdated version of the OpenSDA bootloader
and require an update. Please see the `DAPLink Bootloader Update`_ page for
instructions to update from the CMSIS-DAP bootloader to the DAPLink bootloader.

Option 1: Linkserver
-------------------------------------------------------

Install the :ref:`linkserver-debug-host-tools` and make sure they are in your
search path.  LinkServer works with the default CMSIS-DAP firmware included in
the on-board debugger.

Linkserver is the default for this board, ``west flash`` and ``west debug`` will
call the linkserver runner.

Option 2: :ref:`opensda-jlink-onboard-debug-probe`
--------------------------------------------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.


Use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the OpenSDA
microcontroller as a usb-to-serial adapter for the serial console.

Connect a USB cable from your PC to J6.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_ke16z
   :goals: flash

Open a serial terminal, reset the board (press the SW1 button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-3478-gb923667860b1 ***
   Hello World! frdm_ke16z/mke16z4

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_ke16z
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.6.0-xxx-gxxxxxxxxxxxx *****
   Hello World! frdm_ke16z

.. include:: ../../common/board-footer.rst.inc

.. _KE1XZ SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/ke-series-arm-cortex-m4-m0-plus/ke1xz-arm-cortex-m0-plus-5v-main-stream-mcu-with-nxp-touch-and-can-control:KE1xZ

.. _FRDM-KE16Z Datasheet:
   https://www.nxp.com/docs/en/data-sheet/KE1xZP48M48SF0.pdf

.. _FRDM-KE16Z Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/KE1xZP48M48SF0RM.pdf

.. _FRDM-KE16Z Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/freedom-development-platform-for-kinetis-ke1xmcus:FRDM-KE16Z

.. _FRDM-KE16Z User Guide:
   https://www.nxp.com/document/guide/get-started-with-the-frdm-ke16z:NGS-FRDM-KE16Z

.. _FRDM-KE16Z Schematics:
   https://www.nxp.com/downloads/en/schematics/FRDM-KE16ZSCH.zip

.. _DAPLink Bootloader Update:
   https://os.mbed.com/blog/entry/DAPLink-bootloader-update/
