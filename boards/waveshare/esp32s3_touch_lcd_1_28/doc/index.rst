.. zephyr:board:: esp32s3_touch_lcd_1_28

Overview
********

The ESP32-S3-Touch-LCD-1.28 is an ESP32S3 development board from Waveshare with a round LCD,
suitable to build watches or similar projects. This board integrates complete Wi-Fi and Bluetooth
Low Energy functions, an accelerometer and gyroscope, a battery charger and GPIO extension port.

Hardware
********

ESP32-S3 is a low-power MCU-based system on a chip (SoC) with integrated 2.4 GHz Wi-Fi
and Bluetooth® Low Energy (Bluetooth LE). It consists of high-performance dual-core microprocessor
(Xtensa® 32-bit LX7), a low power coprocessor, a Wi-Fi baseband, a Bluetooth LE baseband,
RF module, and numerous peripherals.

ESP32-S3-Touch-LCD-1.28 includes the following features:

- Dual core 32-bit Xtensa Microprocessor (Tensilica LX7), running up to 240MHz
- Additional vector instructions support for AI acceleration
- 2MB of SRAM
- 16MB of FLASH
- Wi-Fi 802.11b/g/n
- Bluetooth LE 5.0 with long-range support and up to 2Mbps data rate
- Round 1.28" LCD with touchscreen controller
- Accelerometer/gyroscope
- Battery charger

Digital interfaces:

- 6 programmable GPIOs
- 2 open-drain outputs

Low Power:

- Power Management Unit with five power modes
- Ultra-Low-Power (ULP) coprocessors: ULP-RISC-V and ULP-FSM

Security:

- Secure boot
- Flash encryption
- 4-Kbit OTP, up to 1792 bits for users
- Cryptographic hardware acceleration: (AES-128/256, Hash, RSA, RNG, HMAC, Digital signature)

Asymmetric Multiprocessing (AMP)
********************************

ESP32-S3 allows 2 different applications to be executed in ESP32-S3 SoC. Due to its dual-core
architecture, each core can be enabled to execute customized tasks in stand-alone mode
and/or exchanging data over OpenAMP framework. See :zephyr:code-sample-category:`ipc` folder as code reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_ or the technical reference
manual at `ESP32-S3 Technical Reference Manual`_.

Supported Features
==================

Current Zephyr's ESP32-S3-Touch-LCD-1.28 board supports the following features:

+------------+------------+-------------------------------------+
| Interface  | Controller | Driver/Component                    |
+============+============+=====================================+
| UART       | on-chip    | serial port                         |
+------------+------------+-------------------------------------+
| GPIO       | on-chip    | gpio                                |
+------------+------------+-------------------------------------+
| PINMUX     | on-chip    | pinmux                              |
+------------+------------+-------------------------------------+
| SPI Master | on-chip    | spi                                 |
+------------+------------+-------------------------------------+
| TWAI/CAN   | on-chip    | can                                 |
+------------+------------+-------------------------------------+
| ADC        | on-chip    | adc                                 |
+------------+------------+-------------------------------------+
| Timers     | on-chip    | counter                             |
+------------+------------+-------------------------------------+
| Watchdog   | on-chip    | watchdog                            |
+------------+------------+-------------------------------------+
| TRNG       | on-chip    | entropy                             |
+------------+------------+-------------------------------------+
| LEDC       | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| MCPWM      | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| PCNT       | on-chip    | qdec                                |
+------------+------------+-------------------------------------+
| GDMA       | on-chip    | dma                                 |
+------------+------------+-------------------------------------+

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
*******************

Simple boot
===========

The board could be loaded using the single binary image, without 2nd stage bootloader.
It is the default option when building the application without additional configuration.

.. note::

   Simple boot does not provide any security features nor OTA updates.

References
**********

.. target-notes::

.. _ESP32-S3-Touch-LCD-1.28 Waveshare Wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28
.. _ESP32-S3 Datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
.. _ESP32-S3 Technical Reference Manual: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
