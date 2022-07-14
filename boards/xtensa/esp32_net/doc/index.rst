.. _esp32_net:

ESP32-NET
#########

Overview
********

ESP32_NET is a board configuration to allow zephyr application building
targeted to ESP32 APP_CPU only, please refer ESP32 board to a more complete
list of features.

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

The usual ``flash`` target will work with the ``esp32`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32_net
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

.. _`ESP32 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
.. _`JTAG debugging for ESP32`: http://esp-idf.readthedocs.io/en/latest/api-guides/jtag-debugging/index.html
.. _`toolchain`: https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-started-setup-toochain
.. _`SDK`: https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-started-get-esp-idf
.. _`Hardware Reference`: https://esp-idf.readthedocs.io/en/latest/hw-reference/index.html
.. _`esptool documentation`: https://github.com/espressif/esptool/blob/master/README.md
.. _`esptool.py`: https://github.com/espressif/esptool
.. _`ESP-WROVER-32 V3 Getting Started Guide`: https://dl.espressif.com/doc/esp-idf/latest/get-started/get-started-wrover-kit.html
.. _`installing prerequisites`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-1-install-prerequisites
.. _`set up the tools`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-3-set-up-the-tools
.. _`set up environment variables`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-4-set-up-the-environment-variables
.. _`ESP32 Toolchain`: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#xtensa-esp32-elf
.. _`OpenOCD for ESP32 download`: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#openocd-esp32
