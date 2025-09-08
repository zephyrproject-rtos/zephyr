.. zephyr:board:: ttgo_t8s3

Overview
********

Lilygo TTGO T8-S3 is an IoT mini development board based on the
Espressif ESP32-S3 WiFi/Bluetooth dual-mode chip.

It features the following integrated components:

- ESP32-S3 chip (240MHz dual core, Bluetooth LE, Wi-Fi)
- on board antenna and IPEX connector
- USB-C connector for power and communication
- MX 1.25mm 2-pin battery connector
- JST SH 1.0mm 4-pin UART connector
- SD card slot

Hardware
********

This board is based on the ESP32-S3 with 16MB of flash, WiFi and BLE support. It
has an USB-C port for programming and debugging, integrated battery charging
and an on-board antenna. The fitted U.FL external antenna connector can be
enabled by moving a 0-ohm resistor.

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your Lilygo TTGO T8-S3, please make sure that the board is in good
condition with no obvious signs of damage.

System requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order to work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

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

Sample Applications
*******************

The following code samples will run out of the box on the TTGO T8-S3 board:

* :zephyr:code-sample:`wifi-shell`
* :zephyr:code-sample:`fs`

References
**********

.. target-notes::

.. _`Lilygo TTGO T8-S3 schematic`: https://github.com/Xinyuan-LilyGO/T8-S3/blob/main/schematic/T8_S3_V1.0.pdf
.. _`Lilygo github repo`: https://github.com/Xinyuan-LilyGo
.. _`ESP32-S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
.. _`ESP32-S3 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
