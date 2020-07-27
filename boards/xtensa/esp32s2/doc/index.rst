.. _esp32-s2:

ESP32-S2
########

Overview
********


ESP32-S2 is a series of low cost, low power system on a chip microcontrollers
with integrated Wi-Fi.  The ESP32 series employs a single Tensilica Xtensa LX7
microprocessor.  ESP32-S2 is created and developed by Espressif Systems, a
Shanghai-based Chinese company.

The features include the following:

- Xtensa microprocessor (LX7), running at 160 or 240MHz
- 320KB of SRAM
- 802.11b/g/n/e/i
- Various peripherals:

  - 12-bit ADC with up to 20 channels
  - 2x 8-bit DACs
  - 14x touch sensors
  - Temperature sensor
  - 4x SPI
  - 1x I2S
  - 2x I2C
  - 2x UART
  - 1x USB 1.1 OTG
  - IR (RX/TX)
  - Motor PWM
  - LED PWM with up to 16 channels

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)
- 20uA deep sleep current

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
``ZEPHYR_TOOLCHAIN_VARIANT`` shell variable.  One more other environment variables
should also be set, pointing to where the toolchain has been installed:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
   export ESPRESSIF_TOOLCHAIN_PATH="/path/to/xtensa-esp32s2-elf/"

Flashing
========

The usual ``flash`` target will work with the ``esp32s2`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32s2
   :goals: flash

Refer to :ref:`build_an_application` and :ref:`application_run` for
more details.

It's impossible to determine which serial port the ESP32 board is
connected to, as it uses a generic RS232-USB converter.  The default of
``/dev/ttyUSB0`` is provided as that's often the assigned name on a Linux
machine without any other such converters.

The baud rate of 921600bps is recommended.  If experiencing issues when
flashing, try halving the value a few times (460800, 230400, 115200,
etc).  It might be necessary to change the flash frequency or the flash
mode; please refer to the `esptool documentation`_ for guidance on these
settings.

All flashing options are now handled by the :ref:`west` tool, including flashing
with custom options such as a different serial port.  The ``west`` tool supports
specific options for the ESP32S2 board, as listed here:

  --esp-idf-path ESP_IDF_PATH
                        path to ESP-IDF
  --esp-chip ESP_CHIP
                        soc version to flash, default esp32
  --esp-device ESP_DEVICE
                        serial port to flash, default /dev/ttyUSB0
  --esp-baud-rate ESP_BAUD_RATE
                        serial baud rate, default 921600
  --esp-flash-size ESP_FLASH_SIZE
                        flash size, default "4"
  --esp-flash-freq ESP_FLASH_FREQ
                        flash frequency, default "40m"
  --esp-flash-mode ESP_FLASH_MODE
                        flash mode, default "dio"
  --esp-tool ESP_TOOL   if given, complete path to espidf. default is to
                        search for it in [ESP_IDF_PATH]/components/esptool_py/
                        esptool/esptool.py
  --esp-flash-bootloader ESP_FLASH_BOOTLOADER
                        Bootloader image to flash
  --esp-flash-partition_table ESP_FLASH_PARTITION_TABLE
                        Partition table to flash

For example, to flash to ``/dev/ttyUSB2``, use the following command after
having build the application in the ``build`` directory:


.. code-block:: console

   west flash -d build/ --skip-rebuild --esp-device /dev/ttyUSB2 --esp-chip esp32s2

References
**********

.. _`ESP32-S2 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-s2_technical_reference_manual_en.pdf
.. _`toolchain`: https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-started-setup-toochain
.. _`SDK`: https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-started-get-esp-idf
.. _`Hardware Referecne`: https://esp-idf.readthedocs.io/en/latest/hw-reference/index.html
.. _`esptool documentation`: https://github.com/espressif/esptool/blob/master/README.md
.. _`esptool.py`: https://github.com/espressif/esptool
