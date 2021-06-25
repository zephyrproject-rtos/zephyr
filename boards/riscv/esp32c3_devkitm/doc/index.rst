.. _esp32c3_devkitm:

ESP32-C3
########

Overview
********

ESP32-C3 is a single-core Wi-Fi and Bluetooth 5 (LE) microcontroller SoC,
based on the open-source RISC-V architecture. It strikes the right balance of power,
I/O capabilities and security, thus offering the optimal cost-effective
solution for connected devices.
The availability of Wi-Fi and Bluetooth 5 (LE) connectivity not only makes the device configuration easy,
but it also facilitates a variety of use-cases based on dual connectivity. [1]_

The features include the following:

- 32-bit core RISC-V microcontroller with a maximum clock speed of 160 MHz
- 400 KB of internal RAM
- 802.11b/g/n/e/i
- A Bluetooth LE subsystem that supports features of Bluetooth 5 and Bluetooth mesh
- Various peripherals:

  - 12-bit ADC with up to 18 channels
  - TWAI compatible with CAN bus 2.0
  - Temperature sensor
  - 4x SPI
  - 2x I2S
  - 2x I2C
  - 3x UART
  - LED PWM with up to 16 channels

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)

System requirements
*******************

Build Environment Setup
=======================

Some variables must be exported into the environment prior to building this port.
Find more information at :ref:`env_vars` on how to keep this settings saved in you environment.

.. note::

   In case of manual toolchain installation, set :file:`ESPRESSIF_TOOLCHAIN_PATH` accordingly.
   Otherwise, set toolchain path as below. If necessary.

On Linux and macOS:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
   export ESPRESSIF_TOOLCHAIN_PATH="${HOME}/.espressif/tools/riscv32-esp-elf/1.24.0.123_64eb9ff-8.4.0/riscv32-esp-elf"
   export PATH=$PATH:$ESPRESSIF_TOOLCHAIN_PATH/bin

On Windows:

.. code-block:: console

  # on CMD:
  set ESPRESSIF_TOOLCHAIN_PATH=%USERPROFILE%\.espressif\tools\riscv32-esp-elf\1.24.0.123_64eb9ff-8.4.0\riscv32-esp-elf
  set ZEPHYR_TOOLCHAIN_VARIANT=espressif
  set PATH=%PATH%;%ESPRESSIF_TOOLCHAIN_PATH%\bin

  # on PowerShell
  $env:ESPRESSIF_TOOLCHAIN_PATH="$env:USERPROFILE\.espressif\tools\riscv32-esp-elf\1.24.0.123_64eb9ff-8.4.0\riscv32-esp-elf"
  $env:ZEPHYR_TOOLCHAIN_VARIANT="espressif"
  $env:Path += "$env:ESPRESSIF_TOOLCHAIN_PATH\bin"

Finally, retrieve required submodules to build this port. This might take a while for the first time:

.. code-block:: console

   west espressif update

.. note::

    It is recommended running the command above after :file:`west update` so that submodules also get updated.

Flashing
========

The usual ``flash`` target will work with the ``esp32c3_devkitm`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c3_devkitm
   :goals: flash

Refer to :ref:`build_an_application` and :ref:`application_run` for
more details.

It's impossible to determine which serial port the ESP32 board is
connected to, as it uses a generic RS232-USB converter.  The default of
``/dev/ttyUSB0`` is provided as that's often the assigned name on a Linux
machine without any other such converters.

The baud rate of 921600bps is recommended.  If experiencing issues when
flashing, try halving the value a few times (460800, 230400, 115200,
etc).

All flashing options are now handled by the :ref:`west` tool, including flashing
with custom options such as a different serial port.  The ``west`` tool supports
specific options for the ESP32C3 board, as listed here:

  --esp-idf-path ESP_IDF_PATH
                        path to ESP-IDF
  --esp-device ESP_DEVICE
                        serial port to flash, default /dev/ttyUSB0
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

.. [1] https://www.espressif.com/en/products/socs/esp32-c3
.. _`ESP32C3 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
.. _`ESP32C3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
