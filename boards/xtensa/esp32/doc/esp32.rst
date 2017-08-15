.. _esp32:

ESP32
#####

Overview
********

From Wikipedia:

::

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
``ZEPHYR_GCC_VARIANT`` shell variable.  Two other environment variables
should also be set, pointing to, respectively, the path where ESP-IDF can be
located, and where the toolchain has been installed:

.. code-block:: console

   export ZEPHYR_GCC_VARIANT="espressif"
   export ESP_IDF_PATH="/path/to/esp-idf"
   export ESPRESSIF_TOOLCHAIN_PATH="/path/to/xtensa-esp32-elf/"


Flashing
========

Issuing ``make BOARD=esp32 flash`` should work as usual.  Environment
variables can be set to set the serial port device, baud rate, and
other options.  Please refer to the following table for details.

+----------------+---------------+
| Variable       | Default value |
+================+===============+
| ESP_DEVICE     | /dev/ttyUSB0  |
+----------------+---------------+
| ESP_BAUD_RATE  | 921600        |
+----------------+---------------+
| ESP_FLASH_SIZE | detect        |
+----------------+---------------+
| ESP_FLASH_FREQ | 40m           |
+----------------+---------------+
| ESP_FLASH_MODE | dio           |
+----------------+---------------+
| ESP_TOOL       | espidf        |
+----------------+---------------+

It's impossible to determine which serial port the ESP32 board is
connected to, as it uses a generic RS232-USB converter.  The default of
``/dev/ttyUSB0`` is provided as that's often the assigned name on a Linux
machine without any other such converters.

The baud rate of 921600bps is recommended.  If experiencing issues when
flashing, try halving the value a few times (460800, 230400, 115200,
etc).  It might be necessary to change the flash frequency or the flash
mode; please refer to the `esptool documentation`_ for guidance on these
settings.

If ``ESP_TOOL`` is set to "espidf", the `esptool.py`_ script found within
ESP-IDF will be used.  Otherwise, this variable is handled as a path to
the tool.


Using JTAG
==========

In addition to the SoC vendor and SDK, this board also requires a build of
OpenOCD with patches supporting the SoC.  The source code and build
instructions can be obtained from GitHub:

.. code-block:: console

   git clone https://github.com/espressif/openocd-esp32

After building OpenOCD, one should proceed as usual with the setup process.
Since JTAG adapters may be different, this will most likely require editing
the ``esp32.cfg`` file that's in the ``openocd-esp32`` directory.  For
instance, to use a Flyswatter 2 JTAG adapter, one would modify the file so
that the "source" line would read:

.. code-block:: tcl

   source [find interface/ftdi/flyswatter2.cfg]

It might be a good idea to comment the line setting ``ESP32_RTOS`` and
increasing ``adapter_khz`` to 400.  (Or higher, if using another JTAG
adapter, but this value is known to be stable with the Flyswatter.)

After the file has been properly edited, connect the JTAG pins according to
the table below.  Please consult your JTAG adapter manual for the proper
pinout.  Power to the ESP32 board should be provided by a USB cable.

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

Further documentation can be obtained from the SoC vendor in `JTAG debugging
for ESP32`_.

References
**********

.. [1] https://en.wikipedia.org/wiki/ESP32
.. _`ESP32 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
.. _`JTAG debugging for ESP32`: https://espressif.com/sites/default/files/documentation/jtag_debugging_for_esp32_en.pdf
.. _`toolchain`: https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-started-setup-toochain
.. _`SDK`: https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-started-get-esp-idf
.. _`Hardware Referecne`: https://esp-idf.readthedocs.io/en/latest/hw-reference/index.html
.. _`esptool documentation`: https://github.com/espressif/esptool/blob/master/README.md
.. _`esptool.py`: https://github.com/espressif/esptool
