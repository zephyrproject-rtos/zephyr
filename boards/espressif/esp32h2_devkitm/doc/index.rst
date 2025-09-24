.. zephyr:board:: esp32h2_devkitm

Overview
********

ESP32-H2-DevKitM-1 is an entry-level development board based on the ESP32-H2-MINI-1 module,
which integrates Bluetooth® Low Energy (LE) and IEEE 802.15.4 connectivity. It features
the ESP32-H2 SoC — a 32-bit RISC-V core designed for low-power, secure wireless communication,
supporting Bluetooth 5 (LE), Bluetooth Mesh, Thread, Matter, and Zigbee protocols.
This module is ideal for a wide range of low-power IoT applications.

For details on getting started, check `ESP32-H2-DevKitM-1`_.

Hardware
********

ESP32-H2 combines IEEE 802.15.4 connectivity with Bluetooth 5 (LE). The SoC is powered by
a single-core, 32-bit RISC-V microcontroller that can be clocked up to 96 MHz. The ESP32-H2 has
been designed to ensure low power consumption and security for connected devices. ESP32-H2 has
320 KB of SRAM with 16 KB of Cache, 128 KB of ROM, 4 KB LP of memory, and a built-in 2 MB or 4 MB
SiP flash. It has 19 programmable GPIOs with support for ADC, SPI, UART, I2C, I2S, RMT, GDMA
and LED PWM.

Most of ESP32-H2-DevKitM-1's I/O pins are broken out to the pin headers on both sides for easy
interfacing. Developers can either connect peripherals with jumper wires or mount the board
on a breadboard.

ESP32-H2 main features:

- RISC-V 32-bit single-core microprocessor
- 320 KB of internal RAM
- 4 KB LP Memory
- Bluetooth LE: Bluetooth 5.3 certified
- IEEE 802.15.4 (Zigbee and Thread)
- 19 programmable GPIOs
- Numerous peripherals (details below)

Digital interfaces:

- 19x GPIOs
- 2x UART
- 2x I2C
- 1x General-purpose SPI
- 1x I2S
- 1x Pulse counter
- 1x USB Serial/JTAG controller
- 1x TWAI® controller, compatible with ISO 11898-1 (CAN Specification 2.0)
- 1x LED PWM controller, up to 6 channels
- 1x Motor Control PWM (MCPWM)
- 1x Remote Control peripheral (RMT), with up to 2 TX and 2 RX channels
- 1x Parallel IO interface (PARLIO)
- General DMA controller (GDMA), with 3 transmit channels and 3 receive channels
- Event Task Matrix (ETM)

Analog interfaces:

- 1x 12-bit SAR ADCs, up to 5 channels
- 1x Temperature sensor (die)

Timers:

- 1x 52-bit system timer
- 2x 54-bit general-purpose timers
- 3x Watchdog timers

Low Power:

- Four power modes designed for typical scenarios: Active, Modem-sleep, Light-sleep, Deep-sleep

Security:

- Secure boot
- Flash encryption
- 4-Kbit OTP, up to 1792 bits for users
- Cryptographic hardware acceleration: (AES-128/256, ECC, HMAC, RSA, SHA, Digital signature, Hash)
- Random number generator (RNG)

For detailed information, check the datasheet at `ESP32-H2 Datasheet`_ or the Technical Reference
Manual at `ESP32-H2 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

Espressif HAL requires Bluetooth binary blobs in order work. Run the command
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

References
**********

.. target-notes::

.. _`ESP32-H2-DevKitM-1`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32h2/esp32-h2-devkitm-1/user_guide.html
.. _`ESP32-H2 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-h2_datasheet_en.pdf
.. _`ESP32-H2 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-h2_technical_reference_manual_en.pdf
