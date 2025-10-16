.. zephyr:board:: weact_esp32s3_mini

Overview
********

The WeAct Studio ESP32-S3-MINI is a compact development board featuring the ESP32-S3-MINI-1-N4R2
module with integrated 4MB flash and 2MB PSRAM. This board provides complete Wi-Fi and Bluetooth
Low Energy functionality with an onboard RGB WS2812 LED and BOOT button. For more information,
check `WeAct Studio ESP32-S3-MINI`_.

Hardware
********

ESP32-S3 is a low-power MCU-based system on a chip (SoC) with integrated 2.4 GHz Wi-Fi
and Bluetooth® Low Energy (Bluetooth LE). It consists of high-performance dual-core microprocessor
(Xtensa® 32-bit LX7), a low power coprocessor, a Wi-Fi baseband, a Bluetooth LE baseband,
RF module, and numerous peripherals.

WeAct Studio ESP32-S3-MINI includes the following features:

- Dual core 32-bit Xtensa Microprocessor (Tensilica LX7), running up to 240MHz
- 4MB integrated Flash memory
- 2MB integrated PSRAM
- 512KB of SRAM
- Wi-Fi 802.11b/g/n
- Bluetooth LE 5.0 with long-range support and up to 2Mbps data rate
- Onboard RGB WS2812 LED (GPIO48)
- BOOT button (GPIO0)
- USB Type-C connector

Digital interfaces:

- 36 programmable GPIOs
- 4x SPI
- 3x UART
- 2x I2C
- 2x I2S
- 1x RMT (TX/RX)
- LED PWM controller, up to 8 channels
- 1x full-speed USB OTG
- 1x USB Serial/JTAG controller
- 2x MCPWM
- General DMA controller (GDMA), with 5 transmit channels and 5 receive channels
- 1x TWAI® controller, compatible with ISO 11898-1 (CAN Specification 2.0)
- Addressable RGB LED, driven by GPIO48.

Analog interfaces:

- 2x 12-bit SAR ADCs, up to 20 channels
- 1x temperature sensor
- 14x touch sensing IOs

Timers:

- 4x 54-bit general-purpose timers
- 1x 52-bit system timer
- 3x watchdog timers

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

WeAct Studio ESP32-S3-MINI supports dual-core execution allowing two different applications
to run on ESP32-S3 SoC. Each core can execute customized tasks in stand-alone mode
and/or exchange data over OpenAMP framework. See :zephyr:code-sample-category:`ipc` folder as reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_ or the technical reference
manual at `ESP32-S3 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`WeAct Studio ESP32-S3-MINI`: https://github.com/WeActStudio/WeActStudio.ESP32S3-MINI/
.. _`ESP32-S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
.. _`ESP32-S3 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
