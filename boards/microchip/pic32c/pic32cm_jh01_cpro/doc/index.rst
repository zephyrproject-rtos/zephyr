.. zephyr:board:: pic32cm_jh01_cpro

Overview
********

The PIC32CM JH01 Curiosity Pro evaluation kit is a hardware platform
to evaluate the Microchip PIC32CM JH01 microcontroller, and the
evaluation kit part number is EV81X90A. The evaluation kit offers a
set of features that enables the PIC32CM JH01 users to get started with
the PIC32CM JH01 peripherals, and to obtain an understanding of how to
integrate the device in their own design.

Hardware
********

- 100-pin TQFP PIC32CM5164 JH01 microcontroller
- 32.768 kHz crystal oscillator
- 32 MHz crystal oscillator
- 512 KiB flash memory and 64 KiB of RAM
- One yellow user LED
- One green board power LED
- One mechanical user push button
- One reset button
- One driven shield Touch button
- Trust Anchor (TA100) Secure Element
- Virtual COM port (CDC)
- Programming and debugging of on-board PIC32CM JH01 through Serial Wire Debug (SWD)
- Arduino uno connector

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `PIC32CM JH01 Curiosity Pro User Guide`_ has detailed information about board connections.

Programming & Debugging
***********************

.. zephyr:board-supported-runners::

Setting Up the Debug Interface
==============================

PyOCD Setup
===========

1. Install Device Pack

   - Add support for the PIC32CM family devices using the following command:

   .. code-block:: console

      pyocd pack install pic32cm

2. Verify Device Support

   - Confirm that the target is recognized:

   .. code-block:: console

      pyocd list --targets

   - You should see an entry similar to:

   .. code-block:: text

      pic32cm5164jh01100        Microchip                PIC32CM5164JH01100           PIC32CM-JH   pack


3. Connect the Board

   - Connect the DEBUG USB port on the board to your host machine.
   - This connection **power up the board** and provides access to the **on-board Embedded Debugger (EDBG)**,
     which enables programming and debugging of the target microcontroller through PyOCD.


J-Link Setup
============

1. Install J-Link Software

   - Download and install the `J-Link software`_ tools from Segger.
   - Make sure the installed J-Link executables (e.g., ``JLink``, ``JLinkGDBServer``)
     are available in your system's PATH.

2. Connect the Board

   - Connect the `J32 Debug Probe`_ to the board's **CORTEX DEBUG** header.
   - Connect the other end of the J32 Debug Probe to your **host machine (PC)** via USB.
   - Connect the DEBUG USB port on the board to your host machine to **power up the board**.


Building and Flashing the Application
=====================================

1. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32cm_jh01_cpro -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32cm_jh01_cpro`` board.

2. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

   By default, this command uses the PyOCD runner to program the device.

   If both the J-Link probe (connected via the **CORTEX DEBUG** header) and the PyOCD supported debug
   interface (connected through the **DEBUG USB** port) are available, you can explicitly select the desired
   runner as shown below:

   .. code-block:: console

      west flash --runner jlink

   or

   .. code-block:: console

      west flash --runner pyocd

   This ensures the application is flashed using the respective connected interface.

3. Observe the Result

   After flashing, **LED0** on the board should start **blinking**, indicating that the
   application is running successfully.

References
**********

PIC32CM JH01 Product Page:
    https://www.microchip.com/en-us/product/PIC32CM5164JH01100

PIC32CM JH01 Curiosity Pro evaluation kit Page:
    https://www.microchip.com/en-us/development-tool/ev81x90a

.. _PIC32CM JH01 Curiosity Pro User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/PIC32CM-JH01-Curiosity-Pro-Evaluation-Kit-User-Guide-DS70005482.pdf

.. _J-Link software:
    https://www.segger.com/downloads/jlink

.. _J32 Debug Probe:
    https://www.microchip.com/en-us/development-tool/dv164232
