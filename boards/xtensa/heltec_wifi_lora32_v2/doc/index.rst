.. _heltec_wifi_lora32_v2:

Heltec WiFi LoRa 32 (V2)
########################

Overview
********

Heltec WiFi LoRa 32 is a classic IoT dev-board designed & produced by Heltec Automation(TM), it’s a highly
integrated product based on ESP32 + SX127x, it has Wi-Fi, BLE, LoRa functions, also Li-Po battery management
system, 0.96" OLED are also included. [1]_

The features include the following:

- Microprocessor: ESP32 (dual-core 32-bit MCU + ULP core)
- LoRa node chip SX1276/SX1278
- Micro USB interface with a complete voltage regulator, ESD protection, short circuit protection,
  RF shielding, and other protection measures
- Onboard SH1.25-2 battery interface, integrated lithium battery management system
- Integrated WiFi, LoRa, Bluetooth three network connections, onboard Wi-Fi, Bluetooth dedicated 2.4GHz
   metal 3D antenna, reserved IPEX (U.FL) interface for LoRa use
- Onboard 0.96-inch 128*64 dot matrix OLED display
- Integrated CP2102 USB to serial port chip

System requirements
*******************

Prerequisites
=============

The ESP32 toolchain :file:`xtensa-esp32-elf` is required to build this port.
The toolchain installation can be performed in two ways:

#. Automatic installation

   .. code-block:: console

      west espressif install

   .. note::

      By default, the toolchain will be downloaded and installed under $HOME/.espressif directory
      (%USERPROFILE%/.espressif on Windows).

#. Manual installation

   Follow the `ESP32 Toolchain`_ link to download proper OS package version.
   Unpack the toolchain file to a known location as it will be required for environment path configuration.

Build Environment Setup
=======================

Some variables must be exported into the environment prior to building this port.
Find more information at :ref:`env_vars` on how to keep this settings saved in you environment.

.. note::

   In case of manual toolchain installation, set :file:`ESPRESSIF_TOOLCHAIN_PATH` accordingly.
   Otherwise, set toolchain path as below. If necessary, update the version folder path as in :file:`esp-2020r3-8.4.0`.

On Linux and macOS:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
   export ESPRESSIF_TOOLCHAIN_PATH="${HOME}/.espressif/tools/zephyr"

On Windows:

.. code-block:: console

  # on CMD:
  set ESPRESSIF_TOOLCHAIN_PATH=%USERPROFILE%\.espressif\tools\zephyr
  set ZEPHYR_TOOLCHAIN_VARIANT=espressif

  # on PowerShell
  $env:ESPRESSIF_TOOLCHAIN_PATH="$env:USERPROFILE\.espressif\tools\zephyr"
  $env:ZEPHYR_TOOLCHAIN_VARIANT="espressif"

Finally, retrieve required submodules to build this port. This might take a while for the first time:

.. code-block:: console

   west espressif update

.. note::

    It is recommended running the command above after :file:`west update` so that submodules also get updated.

Flashing
========

The usual ``flash`` target will work with the ``heltec_wifi_lora32_v2`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: heltec_wifi_lora32_v2
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
specific options for the ESP32 board, as listed here:

  --esp-idf-path ESP_IDF_PATH
                        path to ESP-IDF
  --esp-device ESP_DEVICE
                        serial port to flash, default $ESPTOOL_PORT if defined.
                        If not, esptool will loop over available serial ports until
                        it finds ESP32 device to flash.
  --esp-baud-rate ESP_BAUD_RATE
                        serial baud rate, default 921600
  --esp-flash-size ESP_FLASH_SIZE
                        flash size, default "detect"
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

   west flash -d build/ --skip-rebuild --esp-device /dev/ttyUSB2


References
**********

.. [1] https://heltec.org/project/wifi-lora-32/
.. _`Heltec WiFi LoRa (v2) Pinout Diagram`: https://resource.heltec.cn/download/WiFi_LoRa_32/WIFI_LoRa_32_V2.pdf
.. _`Heltec WiFi LoRa (v2) Schematic Diagrams`: https://resource.heltec.cn/download/WiFi_LoRa_32/V2
.. _`ESP32 Toolchain`: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#xtensa-esp32-elf
.. _`esptool documentation`: https://github.com/espressif/esptool/blob/master/README.md
