.. zephyr:board:: pic32cm_jh01_cnano

Overview
********

The PIC32CM JH01 Curiosity Nano+ Touch Evaluation Kit (EV29G58A) is
a hardware platform that contains a PIC32CM5164JH01048 microcontroller (MCU).
The Curiosity Nano+ Touch Evaluation Kit provides easy access
to the MCU features and can develop custom applications.

Hardware
********

- PIC32CM5164JH01048 MCU
- Arm® Cortex®-M0+ based MCU
- One yellow user LED
- One mechanical user switch
- One user touch button
- CAN interface
- LIN interface
- USB for debugger

  - Can be used for powering the board
  - Must be used to program, or debug, the board
- On-board nano debugger (nEDBG)

  - One green power/status LED
  - Programming and debugging
  - Communications Device Class (CDC) virtual COM port
  - One logic analyzer DGI GPIO
  - The target device is programmed and debugged by the on-board Nano
    debugger; no external programmer, or debugging tool, is required
- Adjustable target voltage

  - MIC5353 LDO regulator controlled by the on-board debugger
  - 1.7V to 3.6V output voltage
  - 500-mA maximum output current (limited by ambient
    temperature and output voltage)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `PIC32CM JH01 Curiosity Nano User Guide`_ has detailed information about board connections.

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

        pic32cm5164jh01048        Microchip                PIC32CM5164JH01048           PIC32CM-JH   pack

3. Connect the Board

   - Connect the DEBUG USB(J18) port on the board to your host machine.
   - This connection **power up the board** and provides access to the **on-board Embedded Debugger (EDBG)**,
     which enables programming and debugging of the target microcontroller through PyOCD.

Building and Flashing the Application
=====================================

1. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32cm_jh01_cnano -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32cm_jh01_cnano`` board.

2. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

3. Observe the Result

   After flashing, **LED1** on the board should start **blinking**, indicating that the
   application is running successfully.

References
**********

PIC32CM JH01 Product Page:
    https://www.microchip.com/en-us/product/PIC32CM5164JH01048

PIC32CM JH01 Curiosity Nano evaluation kit Page:
    https://www.microchip.com/en-us/development-tool/ev29g58a

.. _PIC32CM JH01 Curiosity Nano User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/PIC32CM-JH01-Curiosity-Nano%2B-Touch-User-Guide-DS70005552.pdf
