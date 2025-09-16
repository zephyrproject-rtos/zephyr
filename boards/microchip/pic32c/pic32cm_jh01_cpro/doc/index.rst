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

Flash Using J-Link
==================

To flash the board using the J-Link debugger, follow the steps below:

1. Install J-Link Software

   - Download and install the `J-Link software`_ tools from Segger.
   - Make sure the installed J-Link executables (e.g., ``JLink``, ``JLinkGDBServer``)
     are available in your system's PATH.

2. Connect the Board

   - Connect the `J32 Debug Probe`_ to the board's **CORTEX DEBUG** header.
   - Connect the other end of the J32 Debug Probe to your **host machine (PC)** via USB.
   - Connect the DEBUG USB port on the board to your host machine to **power up the board**.

3. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32cm_jh01_cpro -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32cm_jh01_cpro`` board.

4. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

   This uses the default ``jlink`` runner to flash the application to the board.

5. Observe the Result

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
