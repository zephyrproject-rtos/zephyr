.. zephyr:board:: sam_e54_xpro

Overview
********

The SAM E54 Xplained Pro evaluation kit is ideal for evaluation and
prototyping with the SAM E54 Cortex®-M4F processor-based
microcontrollers. The kit includes Microchip’s Embedded Debugger (EDBG),
which provides a full debug interface without the need for additional
hardware.

Hardware
********

- ATSAME54P20A ARM Cortex-M4F processor at 120 MHz
- 32.768 kHz crystal oscillator
- 12 MHz crystal oscillator
- 1024 KiB flash memory and 256 KiB of RAM
- One yellow user LED
- One mechanical user push button
- One reset button
- On-board USB based EDBG unit with serial console
- One QTouch® PTC button
- 32 MiB QSPI Flash
- ATECC508 CryptoAuthentication™  device
- AT24MAC402 serial EEPROM with EUI-48™ MAC address
- Ethernet

   - RJ45 connector with built-in magnetics
   - KSZ8091RNA PHY
   - 10Base-T/100Base-TX IEE 802.3 compliant Ethernet transceiver

- USB interface, host, and device
- SD/SDIO card connector

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `SAM E54 Xplained Pro User Guide`_ has detailed information about board connections.

Programming and Debugging
*************************

The SAM E54 Xplained Pro features an on-board Microchip Embedded Debugger (EDBG),
which provides both a standard debug interface and a virtual serial port used as the Zephyr console.
SERCOM2 of the ATSAME54P20A MCU is configured as a USART for console output.

#. Connect the board to your host machine using the debug USB port.

#. Open a terminal and start a serial console on the corresponding port.
   On Linux, this is typically ``/dev/ttyACM0``. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Build and flash the Zephyr ``hello_world`` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: sam_e54_xpro
      :goals: flash
      :compact:

#. Observe output on the terminal. If everything is set up correctly, you should see:

   .. code-block:: console

      Hello World! same_54_xpro

References
**********

SAM E54 Product Page:
    https://www.microchip.com/en-us/product/ATSAME54P20A

SAM E54 Xplained Pro evaluation kit Page:
    https://www.microchip.com/en-us/development-tool/ATSAME54-XPRO

.. _SAM E54 Xplained Pro User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/UserGuides/70005321A.pdf
