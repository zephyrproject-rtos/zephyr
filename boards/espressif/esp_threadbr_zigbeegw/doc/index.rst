.. _esp_threadbr_zigbeegw:

Overview
********

The ESP Thread Border Router / ZigBee Gateway developer platform.
It supports both Wi-Fi and Ethernet interfaces as backbone link.
For more information, check `ESP Thread BR Zigbee GW Guide`_.


Hardware
********

The Wi-Fi based ESP Thread Border Router consists of two SoCs:

* The host Wi-Fi SoC, which can be ESP32, ESP32-S and ESP32-C series SoC.
* The radio co-processor (RCP), which is an ESP32-H series SoC.
  The RCP enables the Border Router to access the 802.15.4 physical and MAC layer.

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

.. _`ESP Thread BR Zigbee GW Guide`: https://docs.espressif.com/projects/esp-thread-br/en/latest/hardware_platforms.html
