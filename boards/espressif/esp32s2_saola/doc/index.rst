.. zephyr:board:: esp32s2_saola

Overview
********

ESP32-S2-Saola is a small-sized ESP32-S2 based development board produced by Espressif.
Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing.
Developers can either connect peripherals with jumper wires or mount ESP32-S2-Saola on a breadboard.
For more information, check `ESP32-S3-DevKitC`_.

Hardware
********

ESP32-S2 is a highly integrated, low-power, single-core Wi-Fi Microcontroller SoC, designed to be secure and
cost-effective, with a high performance and a rich set of IO capabilities.

The features include the following:

- RSA-3072-based secure boot
- AES-XTS-256-based flash encryption
- Protected private key and device secrets from software access
- Cryptographic accelerators for enhanced performance
- Protection against physical fault injection attacks
- Various peripherals:

  - 43x programmable GPIOs
  - 14x configurable capacitive touch GPIOs
  - USB OTG
  - LCD interface
  - camera interface
  - SPI
  - I2S
  - UART
  - ADC
  - DAC
  - LED PWM with up to 8 channels

For more information, check the datasheet at `ESP32-S2 Datasheet`_ or the technical reference
manual at `ESP32-S2 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building and Flashing
*********************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
*********

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

The following table shows the pin mapping between ESP32-S2 board and JTAG interface.

+---------------+-----------+
| ESP32 pin     | JTAG pin  |
+===============+===========+
| MTDO / GPIO40 | TDO       |
+---------------+-----------+
| MTDI / GPIO41 | TDI       |
+---------------+-----------+
| MTCK / GPIO39 | TCK       |
+---------------+-----------+
| MTMS / GPIO42 | TMS       |
+---------------+-----------+

References
**********

.. target-notes::

.. _`ESP32-S3-DevKitC`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-saola-1-v1.2.html
.. _`ESP32-S2 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s2_datasheet_en.pdf
.. _`ESP32-S2 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-s2_technical_reference_manual_en.pdf
.. _`JTAG debugging for ESP32-S2`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-guides/jtag-debugging/index.html
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
