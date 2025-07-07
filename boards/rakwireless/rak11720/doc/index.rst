.. zephyr:board:: rak11720

The RAK11720 is a WisBlock Core module for RAK WisBlock.
It is based on the powerful ultra-low power Apollo3 Blue SoC (AMA3B1KK-KBR-B0)
from Ambiq together with a Semtech SX1262 LoRa® transceiver.

The AMA3B1KK-KBR-B0 has an integrated Bluetooth Low Energy transceiver
that enhances the communication capabilities. The RAK11720 stamp module
comes in the same size and footprint as our RAK3172 module which gives
you the opportunity to enhance your existing designs
with BLE without designing a new PCB.

Hardware
********

The easiset way to use a RAK11720, is the WisBlock Modular system.
A WisBlock Base board (RAK19007) which provides the power
supply and programming/debug interface is the base to plug a
RAK11722 (WisBlock Core module with the RAK11720) in.

- Apollo3 Blue SoC with up to 96 MHz operating frequency
- ARM® Cortex® M4F core
- 16 kB 2-way Associative/Direct-Mapped Cache per core
- Up to 1 MB of flash memory for code/data
- Up to 384 KB of low leakage / low power RAM for code/data
- Integrated Bluetooth 5 Low-energy controller
- Semtech SX1262 low power high range LoRa transceiver
- iPEX connectors for the LORA antenna and BLE antenna.
- 2 user LEDs on RAK19007 WisBlock Base board
- Powered by either Micro USB, 3.7V rechargeable battery or a 5V Solar Panel Port

For more information about the RAK11720 stamp module:

- `WisDuo RAK11720 Website`_
- `WisBlock RAK11722 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
=========================

.. zephyr:board-supported-runners::

The RAK11720 board shall be connected to a Segger Embedded Debugger Unit
`J-Link OB <https://www.segger.com/jlink-ob.html>`_. This provides a debug
interface to the Apollo3 Blue chip. You can use JLink to communicate with
the Apollo3 Blue.

Flashing an application
-----------------------

Connect your device to your host computer using the JLINK USB port.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11720
   :goals: flash

.. note::
   ``west flash`` requires `SEGGER J-Link software`_ and `pylink`_ Python module
   to be installed on you host computer.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! rak11720/apollo3_blue

.. _WisDuo RAK11720 Website:
   https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11720-Module/Overview/#product-description

.. _WisBlock RAK11722 Website:
   https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11722/Overview/#product-description

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink

.. _pylink:
   https://github.com/Square/pylink
