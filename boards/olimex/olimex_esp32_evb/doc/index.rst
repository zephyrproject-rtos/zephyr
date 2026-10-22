.. zephyr:board:: olimex_esp32_evb

Overview
********

The Olimex ESP32-EVB is an OSHW certified, open-source IoT board based on the
Espressif ESP32-WROOM-32E/UE module. It has a wired 100Mbit/s Ethernet Interface,
Bluetooth LE, WiFi, infrared remote control, and CAN connectivity. Two relays
allows switching power appliances on and off.

The board can operate from a single LiPo backup battery as it has an internal
LiPo battery charger. There is no step-up converter, so relays, CAN, and USB
power does not work when running off battery.

Hardware
********

- ESP32-WROOM-32E/UE module with 4MB flash.
- On-board programmer, CH340T USB-to-UART
- WiFi, Bluetooth LE connectivity.
- 100Mbit/s Ethernet interface, Microchip LAN8710A PHY.
- MicroSD card slot.
- 2 x 10A/250VAC (15A/120VAC 15A/24VDC) relays with connectors and status LEDs.
- CAN interface, Microchip MCP2562-E high-speed CAN transceiver.
- IR receiver and transmitter, up to 5 meters distance.
- BL4054B LiPo battery charger with status LEDs for stand-alone operation during
  power outages.
- Power jack for external 5VDC power supply.
- Universal EXTension (UEXT) connector for connecting UEXT modules.
- User push button.
- 40 pin GPIO connector with all ESP32 pins.

For more information about the ESP32-EVB and the ESP32-WROOM-32E/UE module, see
these reference documents:

- `ESP32-EVB Website`_
- `ESP32-EVB Schematic`_
- `ESP32-EVB GitHub Repository`_
- `ESP32-WROOM32-E/UE Datasheet`_

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-EVB Website`: https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware
.. _`ESP32-EVB Schematic`: https://github.com/OLIMEX/ESP32-EVB/raw/master/HARDWARE/REV-I/ESP32-EVB_Rev_I.pdf
.. _`ESP32-EVB GitHub Repository`: https://github.com/OLIMEX/ESP32-EVB
.. _`ESP32-WROOM32-E/UE Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf
