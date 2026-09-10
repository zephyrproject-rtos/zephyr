.. zephyr:board:: eora_hub_900tb

Overview
********

EoRa-HUB-900TB is a LR1121 development board designed and produced by Ebyte.
The board is equipped with an Espressif ESP32S3 SoC with 4MB of flash and 2MB of PSRAM,
a 0.96" PMOLED display, a LTC4054ES5 Lithium-Ion battery charger, a CH340 USB to UART adapter, and
an abundance of free pins broken out to an header.
EoRa-HUB-900TB features a Ebyte E80-900M2213S module with a Semtech LR1121 modem that is configured
for High Frequency LoRa bands (868MHz, 915MHz, etc) and the 2.4GHz LoRa band.

Hardware
********

EoRa-HUB-900TB provides the following hardware components:

- ESP32S3 H4R2 SoC

- Two LEDs:

   - Blue User LED, Charger / ADC Enable LED

- Two Buttons:

   - Reset Button (RST), Bootsel Button (PRG)

- CH340 USB to UART adapter.

- LTC4054ES5 Lithium-Ion Battery charger.

- SSD1306 0.96" PMOLED Display with white phosphor.

- E80-900M2213S Module, featuring semtech LR1121, configured for 850-930 MHz and 2.4GHz LoRa.

More information about EoRa-HUB-900TB can be found here:

- `EoRa-HUB-900TB Page`_
- `EoRa-HUB-900TB Schematic`_
- `E80-900M2213S Page`_

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

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

Remarks
*******

The USB to UART Adapter mounted on the board does not allow communication as one would expect,
an external adapter is recommended to allow communication with the onboard SoC beside flashing.

References
**********

.. target-notes::

.. _EoRa-HUB-900TB Page:
   https://www.cdebyte.com/products/EoRa-HUB-900TB/

.. _EoRa-HUB-900TB Schematic:
   https://www.cdebyte.com/pdf-down.aspx?id=3738

.. _E80-900M2213S Page:
   https://www.cdebyte.com/products/E80-900M2213S/
