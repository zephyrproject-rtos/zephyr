.. zephyr:board:: walter

Overview
********

Walter is a compact IoT development board that combines an Espressif ESP32-S3 SoC
with a Sequans Monarch 2 GM02SP LTE-M/NB-IoT/GNSS modem.
More information about Walter can be found on the `QuickSpot Website`_ and on the
`QuickSpot GitHub page`_.

Hardware
********

ESP32-S3-WROOM-1-N16R2 microcontroller:

- Xtensa dual-core 32-bit LX7 CPU
- 16 MiB quad SPI flash memory
- 2 MiB quad SPI PSRAM
- 150 Mbps 802.11 b/g/n Wi-Fi 4 with on-board PCB antenna
- 2 Mbps Bluetooth 5 Low Energy with on-board PCB antenna

Sequans Monarch 2 GM02SP modem:

- Dual-mode LTE-M / NB-IoT (NB1, NB2)
- 3GPP LTE release 14 (Upgradable up to release 17)
- Ultra-low, deep-sleep mode in eDRX and PSM
- Adaptive +23 dBm, +20 dBm and +14 dBm output power
- Integrated LNA and SAW filter for GNSS reception
- Assisted and non-assisted GNSS with GPS and Galileo constellations
- Integrated SIM card
- Nano-SIM card slot
- u.FL RF connectors for GNSS and 5G antennas

Inputs & outputs:

- 24 GPIO pins for application use
- UART, SPI, I²C, CAN, I²S, and SD available on any of the GPIO pins
- ADC, DAC, and PWM integrated in ESP32-S3
- 3.3 V software-controllable output
- USB Type-C connector for flashing and debugging
- 22 test points for production programming and testing
- On-board reset button

Power supply

- 5.0 V via USB Type-C
- 3.0 - 5.5 V via Vin pin
- Not allowed to use both power inputs simultaneously
- Designed for extremely low quiescent current

Form factor

- Easy to integrate via 2.54 mm headers
- 55 mm x 24.8 mm board dimensions
- Pin and footprint compatible with EOL Pycom GPy
- Breadboard friendly

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

.. _`QuickSpot Website`: https://www.quickspot.io/
.. _`QuickSpot GitHub page`: https://github.com/QuickSpot
