.. zephyr:board:: esp8684_devkitm

Overview
********

The ESP8684-DevKitM is an entry-level development board based on ESP8684-MINI-1, a general-purpose
module with 1 MB/2 MB/4 MB SPI flash. This board integrates complete Wi-Fi and Bluetooth LE functions.
For more information, check `ESP8684-DevKitM User Guide`_

Hardware
********

ESP32-C2 (ESP8684 core) is a low-cost, Wi-Fi 4 & Bluetooth 5 (LE) chip. Its unique design
makes the chip smaller and yet more powerful than ESP8266. ESP32-C2 is built around a RISC-V
32-bit, single-core processor, with 272 KB of SRAM (16 KB dedicated to cache) and 576 KB of ROM.
ESP32-C2 has been designed to target simple, high-volume, and low-data-rate IoT applications,
such as smart plugs and smart light bulbs. ESP32-C2 offers easy and robust wireless connectivity,
which makes it the go-to solution for developing simple, user-friendly and reliable
smart-home devices. For more information, check `ESP8684 Datasheet`_.

Features include the following:

- 32-bit core RISC-V microcontroller with a maximum clock speed of 120 MHz
- 2 MB or 4 MB in chip (ESP8684) or in package (ESP32-C2) flash
- 272 KB of internal RAM
- 802.11b/g/n
- A Bluetooth LE subsystem that supports features of Bluetooth 5 and Bluetooth Mesh
- Various peripherals:

  - 14 programmable GPIOs
  - 3 SPI
  - 2 UART
  - 1 I2C Master
  - LED PWM controller, with up to 6 channels
  - General DMA controller (GDMA)
  - 1 12-bit SAR ADC, up to 5 channels
  - 1 temperature sensor
  - 1 54-bit general-purpose timer
  - 2 watchdog timers
  - 1 52-bit system timer

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)

For detailed information check `ESP8684 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

For a getting started user guide, please check `ESP8684-DevKitM User Guide`_.

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

References
**********

.. target-notes::

.. _`ESP8684-DevKitM User Guide`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp8684/esp8684-devkitm-1/user_guide.html
.. _`ESP8684 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp8684_datasheet_en.pdf
.. _`ESP8684 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp8684_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
