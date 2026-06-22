.. zephyr:board:: pic32cz_ca80_cult

Overview
********

The PIC32CZ CA80 Curiosity Ultra development board is a hardware platform
to evaluate the Microchip PIC32CZ CA80 microcontroller, and the
development board part number is EV51S73A. The development board offers a
set of features that enables the PIC32CZ CA80 users to get started with
the PIC32CZ CA80 peripherals, and to obtain an understanding of how to
integrate the device in their own design.

Hardware
********

- 208-Pin TFBGA PIC32CZ8110 CA80 microcontroller
- 32.768 kHz crystal oscillator
- 8M flash memory and 1M of RAM
- Xplained pro extension compatible interface
- Two yellow user LEDs
- Two mechanical user push button
- One reset button
- Virtual COM port (VCOM)
- Programming and debugging of on-board PIC32CZ CA80 through Serial Wire Debug (SWD)
- Arduino uno R3 compatible interface
- MikroBus Socket
- On-board temperature sensor
- Graphics interface
- G-bit Ethernet
- 2 high-speed USB (Type-C and Micro A/B)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `PIC32CZ CA80 Curiosity Ultra User Guide`_ has detailed information about board connections.

Programming & Debugging
***********************

.. zephyr:board-supported-runners::

Flash Using J-Link
==================

To flash the board using the J-Link debugger, follow the steps below:

1. Install J-Link Software

   - Download and install the `J-Link software <https://www.segger.com/downloads/jlink>`_ tools from Segger.
   - Make sure the installed J-Link executables (e.g., ``JLink``, ``JLinkGDBServer``) are available in your system's PATH.

2. Connect the Board

   - Connect the `J32 Debug Probe <https://www.microchip.com/en-us/development-tool/dv164232>`_ to the board's **CORTEX DEBUG** header.
   - Connect the other end of the J32 Debug Probe to your **host machine (PC)** via USB.
   - Connect the DEBUG USB port on the board to your host machine to **power up the board**.

3. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool. Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32cz_ca80_cult -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32cz_ca80_cult`` board.

4. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

   This uses the default ``jlink`` runner to flash the application to the board.

5. Observe the Result

   After flashing, **LED0** on the board should start **blinking**, indicating that the application is running successfully.

References
**********

PIC32CZ CA80 Product Page:
    https://www.microchip.com/en-us/product/PIC32CZ8110CA80208

PIC32CZ CA80 Curiosity Ultra Development Board Page:
    https://www.microchip.com/en-us/development-tool/ev51s73a

.. _PIC32CZ CA80 Curiosity Ultra User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/PIC32CZ-CA80-CA90-Curiosity-Ultra-User-Guide-DS70005522.pdf
