.. zephyr:board:: pic32cx_sg41_cult

Overview
********

The PIC32CX SG41 Curiosity Ultra evaluation kit is a hardware platform
to evaluate the Microchip PIC32CX SG41 microcontrollers, and the
evaluation kit part number is EV06X38A. The evaluation kit offers a
set of features that enables the PIC32CX SG41 users to get started with
the PIC32CX SG41 peripherals, and to obtain an understanding of how to
integrate the device in their own design.

Hardware
********

- PIC32CX1025SG41128 microcontroller
- One mechanical reset button
- Two mechanical programmable buttons
- Two user LEDs
- 32.768 kHz crystal
- 12 MHz oscillator
- USB interface, Host or Device
- 64 Mbit Quad SPI Flash
- AT24MAC402 Serial EEPROM with EUI-48™ MAC address
- Ethernet 10/100 Mbps
   - RMII Interface with modular PHY Header
- SD/SDIO card connector
- Two CAN interfaces with on-board transceivers
- I²C-based temperature sensor
- DAC Output Header
- mikroBUS header connector
- Arduino Uno header connectors
- CoreSight 10 connector
   - 10-pin Cortex® Debug header (SWD)
- CoreSight 20 connector
   - 20-pin Cortex Debug + ETM Connector (SWD and 4-bit Trace)
- Virtual COM port (CDC)
- USB powered
- Power Header and Barrel Jack connector for external power sources

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `PIC32CX SG41 Curiosity Ultra User Guide`_ has detailed information about board connections.

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

   - Connect the `J32 Debug Probe`_ to the board's **CORTEX DEBUG** (J400) header.
   - Connect the other end of the J32 Debug Probe to your **host machine (PC)** via USB.
   - Connect the DEBUG USB port on the board to your host machine to **power up the board**.

3. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32cx_sg41_cult -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32cx_sg41_cult`` board.

4. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

   This uses the default ``jlink`` runner to flash the application to the board.

5. Observe the Result

   After flashing, **LED1** on the board should start **blinking**, indicating that the
   application is running successfully.

References
**********

PIC32CX SG41 Product Page:
   https://www.microchip.com/en-us/product/PIC32CX1025SG41128

PIC32CX SG41 Curiosity Ultra evaluation kit Page:
   https://www.microchip.com/en-us/development-tool/EV06X38A

.. _PIC32CX SG41 Curiosity Ultra User Guide:
   https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/PIC32CX-SG41-SG61-Curiosity-Ultra-User-Guide-DS70005520.pdf

.. _J-Link software:
    https://www.segger.com/downloads/jlink

.. _J32 Debug Probe:
    https://www.microchip.com/en-us/development-tool/dv164232
