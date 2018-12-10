.. _particle_argon_esp32:

particle argon esp32
####################

Overview
********

`particle argon`_ is a WiFi mesh gateway HW platform containing
an nRF52840 for mesh support and an ESP32 for WiFi.
The board was developed by Particle Industries and has a SWD connector
on it for programming.


The ESP32 part of the `particle argon` provides the following
functionality:

- Dual core Xtensa microprocessor (LX6), running at 160 or 240MHz
- 520KB of SRAM
- 802.11b/g/n/e/i
- Bluetooth v4.2 BR/EDR and BLE
- Various peripherals:

  - 12-bit ADC with up to 18 channels
  - 2x 8-bit DACs
  - 10x touch sensors
  - Temperature sensor
  - 4x SPI
  - 2x I2S
  - 2x I2C
  - 3x UART
  - SD/SDIO/MMC host
  - Slave (SDIO/SPI)
  - Ethernet MAC
  - CAN bus 2.0
  - IR (RX/TX)
  - Motor PWM
  - LED PWM with up to 16 channels
  - Hall effect sensor

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)
- 5uA deep sleep current

System requirements
*******************

Prerequisites
=============

Two components are required in order to build this port: the `toolchain`_
and the `SDK`_.  Both are provided by the SoC manufacturer.

The SDK contains headers and a hardware abstraction layer library
(provided only as object files) that are required for the port to
function.

The toolchain is available for Linux, Windows, and Mac hosts and
instructions to obtain and set them up are available in the ESP-IDF
repository, using the toolchain and SDK links above.

Set up build environment
========================

With both the toolchain and SDK installed, the Zephyr build system must be
instructed to use this particular variant by setting the
``ZEPHYR_TOOLCHAIN_VARIANT`` shell variable.  Two other environment variables
should also be set, pointing to, respectively, the path where ESP-IDF can be
located, and where the toolchain has been installed:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
   export ESP_IDF_PATH="/path/to/esp-idf"
   export ESPRESSIF_TOOLCHAIN_PATH="/path/to/xtensa-esp32-elf/"

Since ESP-IDF is an external project in constant development, it's possible
that files that Zephyr depends on will be moved, removed, or renamed.  Those
files are mostly header files containing hardware definitions, which are
unlikely to change and require fixes from the vendor.  In addition to
setting the environment variables above, also check out an earlier version
of ESP-IDF:

.. code-block:: console

   cd ${ESP_IDF_PATH}
   git checkout b2ff235bd00

Flashing
========

The usual ``flash`` target will work with the ``esp32`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: particle_argon_esp32

Refer to :ref:`build_an_application` and :ref:`application_run` for
more details.

Flashing
********

The only way of programming the image to the device, is performing an
update through the nrf52 residing on the board.
