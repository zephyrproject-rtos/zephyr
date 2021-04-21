.. _esp32:

ESP32
#####

Overview
********


ESP32 is a series of low cost, low power system on a chip microcontrollers
with integrated Wi-Fi & dual-mode Bluetooth.  The ESP32 series employs a
Tensilica Xtensa LX6 microprocessor in both dual-core and single-core
variations.  ESP32 is created and developed by Espressif Systems, a
Shanghai-based Chinese company, and is manufactured by TSMC using their 40nm
process. [1]_

The features include the following:

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
   export ESPRESSIF_TOOLCHAIN_PATH="${HOME}/.espressif/tools/xtensa-esp32-elf/esp-2020r3-8.4.0/xtensa-esp32-elf"
   export PATH=$PATH:$ESPRESSIF_TOOLCHAIN_PATH/bin

On Windows:

.. code-block:: console

  # on CMD:
  set ESPRESSIF_TOOLCHAIN_PATH=%USERPROFILE%\.espressif\tools\xtensa-esp32-elf\esp-2020r3-8.4.0\xtensa-esp32-elf
  set ZEPHYR_TOOLCHAIN_VARIANT=espressif
  set PATH=%PATH%;%ESPRESSIF_TOOLCHAIN_PATH%\bin

  # on PowerShell
  $env:ESPRESSIF_TOOLCHAIN_PATH="$env:USERPROFILE\.espressif\tools\xtensa-esp32-elf\esp-2020r3-8.4.0\xtensa-esp32-elf"
  $env:ZEPHYR_TOOLCHAIN_VARIANT="espressif"
  $env:Path += "$env:ESPRESSIF_TOOLCHAIN_PATH\bin"

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
   :board: esp32
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

Using JTAG
==========

As with much custom hardware, the ESP-32 modules require patches to
OpenOCD that are not upstream.  Espressif maintains their own fork of
the project here.  By convention they put it in ``~/esp`` next to the
installations of their toolchain and SDK:

.. code-block:: console

   cd ~/esp

   git clone https://github.com/espressif/openocd-esp32

   cd openocd-esp32
   ./bootstrap
   ./configure
   make

On the ESP-WROVER-KIT board, the JTAG pins are connected internally to
a USB serial port on the same device as the console.  These boards
require no external hardware and are debuggable as-is.  The JTAG
signals, however, must be jumpered closed to connect the internal
controller (the default is to leave them disconnected).  The jumper
headers are on the right side of the board as viewed from the power
switch, next to similar headers for SPI and UART.  See
`ESP-WROVER-32 V3 Getting Started Guide`_ for details.

On the ESP-WROOM-32 DevKitC board, the JTAG pins are not run to a
standard connector (e.g. ARM 20-pin) and need to be manually connected
to the external programmer (e.g. a Flyswatter2):

+------------+-----------+
| ESP32 pin  | JTAG pin  |
+============+===========+
| 3V3        | VTRef     |
+------------+-----------+
| EN         | nTRST     |
+------------+-----------+
| IO14       | TMS       |
+------------+-----------+
| IO12       | TDI       |
+------------+-----------+
| GND        | GND       |
+------------+-----------+
| IO13       | TCK       |
+------------+-----------+
| IO15       | TDO       |
+------------+-----------+

Once the device is connected, you should be able to connect with (for
a DevKitC board, replace with esp32-wrover.cfg for WROVER):

.. code-block:: console

    cd ~/esp/openocd-esp32
    src/openocd -f interface/ftdi/flyswatter2.cfg -c 'set ESP32_ONLYCPU 1' -c 'set ESP32_RTOS none' -f board/esp-wroom-32.cfg -s tcl

The ESP32_ONLYCPU setting is critical: without it OpenOCD will present
only the "APP_CPU" via the gdbserver, and not the "PRO_CPU" on which
Zephyr is running.  It's currently unexplored as to whether the CPU
can be switched at runtime or if breakpoints can be set for
either/both.

Now you can connect to openocd with gdb and point it to the OpenOCD
gdbserver running (by default) on localhost port 3333.  Note that you
must use the gdb distributed with the ESP-32 SDK.  Builds off of the
FSF mainline get inexplicable protocol errors when connecting.

.. code-block:: console

    ~/esp/xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb outdir/esp32/zephyr.elf
    (gdb) target remote localhost:3333

Further documentation can be obtained from the SoC vendor in `JTAG debugging
for ESP32`_.

References
**********

.. [1] https://en.wikipedia.org/wiki/ESP32
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
