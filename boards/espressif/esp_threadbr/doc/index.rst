.. zephyr:board:: esp_threadbr

Overview
********

The ESP Thread Border Router / ZigBee Gateway is a developer platform for
building Thread border router and ZigBee gateway applications.
It bridges 802.15.4 mesh networks with IP networks via Wi-Fi or Ethernet.

ESP32-S3 is used as the host SoC, and it provides the Wi-Fi backbone link.
Combining the board with an optional :ref:`esp_threadbr_ethernet` shield
adds an Ethernet link.

The board has an on-board IEEE 802.15.4 capable ESP32-H2 SoC used as a radio
co-processor (RCP). It provides the 802.15.4 PHY and MAC for Thread/ZigBee.
The host ESP32-S3 talks to the RCP over a UART or SPI interface.

Board design allows radio coexistence using Packet Traffic Arbitration (PTA).
It uses dedicated GPIO signals between both SoCs to coordinate radio access
when multiple wireless technologies share the same RF band.

For more information on hardware setup and software architecture, see the
`ESP Thread BR Zigbee GW Guide`_ and the `GitHub repository`_.

Hardware
********

The Wi-Fi based ESP Thread Border Router consists of two SoCs:

* The host Wi-Fi SoC, ESP32-S3.
* The radio co-processor (RCP), which is an ESP32-H2 series SoC.
  The RCP enables the Border Router to access the 802.15.4 physical and MAC layer.

.. note::
   The RCP runs its own firmware, usually based on ESP-IDF.
   This Zephyr board target is for the host, not the RCP.

.. include:: ../../common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP Thread BR Zigbee GW Guide`: https://docs.espressif.com/projects/esp-thread-br/en/latest/hardware_platforms.html
.. _`GitHub repository`: https://github.com/espressif/esp-thread-br
