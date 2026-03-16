.. zephyr:code-sample:: at_client
   :name: AT Command Client
   :relevant-api: uart_interface modem_pipe

   Create a bidirectional UART bridge between console and modem for AT command communication.

Overview
********

This sample demonstrates how to create an AT command client that bridges
communication between the host console and a cellular modem. It uses the
Zephyr modem subsystem with ``uart_pipe`` for console communication and
``modem_pipe`` for modem communication.

The sample provides a transparent bidirectional bridge:

* Commands typed in the console are forwarded to the modem
* Responses from the modem are displayed on the console
* Supports AT command interaction and firmware update via XMODEM protocol

Requirements
************

* A board with two UART peripherals
* One UART connected to the host console (typically USB CDC-ACM or ST-LINK)
* Second UART connected to a cellular modem
* Device tree configuration for both UARTs

Wiring
******

The sample requires two UART connections to be defined in the device tree:

* ``zephyr,console`` - Console UART for host communication
* ``zephyr,modem-uart`` - Modem UART for cellular modem communication

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/modem/at_client
   :board: <your_board>
   :goals: build flash
   :compact:

After flashing, open a serial terminal to the console UART. Any AT commands
typed will be forwarded to the modem, and responses will be displayed.

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.x.x ***
    [00:00:00.100,000] <inf> at_client: Console UART pipe registered
    [00:00:00.150,000] <inf> at_client: Modem pipe initialized and opened
    [00:00:00.151,000] <inf> at_client: Console <-> Modem communication established
    [00:00:00.151,000] <inf> at_client: Ready to forward AT commands


    AT
    OK

    AT+CGMI
    Sierra Wireless
    OK

Usage Example
=============

Once running, you can send AT commands directly from your terminal:

.. code-block:: console

    AT+CGSN                  # Query modem IMEI
    AT+CSQ                   # Check signal quality
    AT+COPS?                 # Query operator
    AT+CGDCONT=1,"IP","apn"  # Configure APN

Firmware Update
===============

This sample supports modem firmware updates using the XMODEM protocol.
Use the companion Python script ``hl78xx_firmware_update.py`` to perform
firmware updates through the AT client bridge.

See ``scripts/hl78xx_firmware_update.py`` for details.
