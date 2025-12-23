.. zephyr:board:: kincony_kc868_a32

Overview
********

Kincony KC868-A32 is a home automation relay module based on the
Espressif ESP32 ESP-WROOM-32 module with all its inherent capabilities
(Wi-Fi, Bluetooth, etc.)

Hardware
********

The features include the following:

- 32 digital optoisolated inputs “dry contact”
- 4 analog inputs 0-5 V
- 32 relays 220 V, 10 A (COM, NO, NC)
- RS485 interface
- I2C connector
- Connector GSM/HMI
- Ethernet LAN8270A
- USB Type-B connector for programming and filling firmware
- RESET and DOWNLOAD buttons
- Powered by 12V DC

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

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

Enabling Ethernet
*****************

Enable Ethernet in KConfig:

.. code-block:: cfg

    CONFIG_NETWORKING=y
    CONFIG_NET_L2_ETHERNET=y
    CONFIG_MDIO=y

References
**********

.. target-notes::

.. _KINCONY KC868-A32 User Guide: https://www.kincony.com/arduino-esp32-32-channel-relay-module-kc868-a32.html
