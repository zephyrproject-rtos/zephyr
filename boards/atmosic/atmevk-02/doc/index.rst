.. _atmevk-02:

Atmosic ATMx2xx
###############

Overview
********
The ATM3201/ATM3221, ATM2201/ATM2221, and ATM3202/ATM2202 chips are part of a family of extreme low-power Bluetooth® 5.0 system-on-a-chip (SoC) solutions. This Bluetooth Low Energy SoC integrates a Bluetooth 5  radio with an ARM® Cortex® M0 processor and state-of-the-art power management to enable maximum lifetime in battery operated devices. The extremely low power ATM2 series SoC is designed with an extensive set of peripherals and flexible I/O to support a wide variety of applications across the consumer, commercial, and industrial Internet of Things (IoT) markets.  

Hardware Features
*****************
- Bluetooth LE 5.0 compliant
- Fully integrated RF front-end
- 16 MHz ARM® Cortex® M0 CPU
- 256 KB ROM, 128 KB RAM, 4 KB OTP
- SWD for interactive debugging
- Integrated Power Management Unit (PMU)
- DC/DC Buck-Boost Converter
- RF Wakeup Receiver
- I2C, SPI, UART, PWM Peripherals
- Configurable GPIOs
- Quad SPI with Execute in Place (XIP)
- Application ADC (10-bit)
- Digital microphone input (PDM)
- Keyboard matrix controller (KSM)
- Quadrature decoder for mouse input (QDEC)
- 16 MHz / 32.768 kHz Crystal Oscillator
- AES 128 hardware
- True random number generator (TRNG)
- Sensor Hub

Zephyr Support
==============

The Zephyr support on the ATMEVK-Mxxxx-02 boards includes the following:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth            |
+-----------+------------+----------------------+
| SPI(M)    | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+

Boards
======
The following table summarizes the boards and some of their key features.

+-----------------+---------+-------------------+---------------+-----------+
| Name            | Part #  | Energy Harvesting | On-chip Flash | Package   |
+=================+=========+===================+===============+===========+
| ATMEVK-M2201-02 | ATM2201 |                   |               | QFN 5x5   |
+-----------------+---------+-------------------+---------------+-----------+
| ATMEVK-M2202-02 | ATM2202 |                   | x             | QFN 5x5   |
+-----------------+---------+-------------------+---------------+-----------+
| ATMEVK-M2221-02 | ATM2221 |                   |               | DRQFN 6x6 |
+-----------------+---------+-------------------+---------------+-----------+
| ATMEVK-M2251-02 | ATM2251 |                   |               | WLCSP     |
+-----------------+---------+-------------------+---------------+-----------+
| ATMEVK-M3201-02 | ATM3201 | x                 |               | QFN 5x5   |
+-----------------+---------+-------------------+---------------+-----------+
| ATMEVK-M3202-02 | ATM3202 | x                 | x             | QFN 5x5   |
+-----------------+---------+-------------------+---------------+-----------+
| ATMEVK-M3221-02 | ATM3221 | x                 |               | DRQFN 6x6 |
+-----------------+---------+-------------------+---------------+-----------+

Pin Multiplexing
================
The following table summarizes the mapping between the pins and the functionality they can support. In the ATM3221/ATM2221 SoC, all 34 programmable IO pins, P0 - P33, support multiple functional signals. The ATM3201/ATM2201 SoC supports 17 programmable IO pins, and pins that are not supported are marked with '#'. The ATM3202/ATM2202 SoC supports 12 programmable IO pins, and pins that are not supported are marked with '#' and '*'.

+------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Name | Function                                                                                                                                                                                         |
+======+============+============+============+============+============+============+============+============+============+============+============+============+============+============+============+
| P0 * | GPIO_0     | KSI_19     | KSO_19     | KSO_8      |            | QDZ_1      |            |            |            | SHUB1_CLK  |            | SPI1_CLK   |            | XPAON      |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P1 * | GPIO_1     | KSI_18     | KSO_18     | KSO_9      |            | QDZ_0      |            |            |            | SHUB1_CS   |            | SPI1_CS    |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P2 * | GPIO_2     | KSI_17     | KSO_17     | KSO_10     | KSO_8      | QDY_1      |            |            |            | SHUB1_MOSI |            | SPI1_MOSI  |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P3 # | GPIO_3     | KSI_16     | KSO_16     | KSO_11     | KSO_9      | QDY_0      | UART1_RTS  | I2C0_SCK   | SHUB0_SCK  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P4 # | GPIO_4     | KSI_15     | KSO_15     | KSO_12     | KSO_10     |            | UART1_RX   | I2C0_SDA   | SHUB0_SDA  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P5 # | GPIO_5     | KSI_14     | KSO_14     | KSO_13     | KSO_11     |            | UART1_TX   | PWM_0      |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P6 # | GPIO_6     | KSI_13     | KSO_13     | KSO_14     | KSO_12     |            | UART1_CTS  | SPI1_CLK   |            |            | SHUB1_CLK  |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P7 # | GPIO_7     | KSI_12     | KSO_12     | KSO_15     | KSO_13     |            | PWM_0      | SPI1_CS    |            |            | SHUB1_CS   |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P8 # | GPIO_8     | KSI_11     | KSO_11     | KSO_16     | KSO_14     |            | PWM_1      | SPI1_MOSI  |            |            | SHUB1_MOSI |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P9   | GPIO_9     | KSI_10     | KSO_10     | KSO_17     | KSO_15     |            | PWM_2      | SPI1_MISO  | SHUB0_CLK  | SHUB0_SCK  | SHUB1_MISO | SPI0_CLK   | PDM_CLK    |            | I2C0_SCK   |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P10  | GPIO_10    | KSI_9      | KSO_9      | KSO_18     | KSO_16     | UART0_RX   | PWM_1      | SPI0_CLK   | SHUB0_CS   | SHUB0_SDA  | SHUB0_CLK  | SPI0_CS    | PDM_IN     | XPAON      | I2C0_SDA   |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P11  | GPIO_11    | KSI_8      | KSO_8      | KSO_16     | KSO_17     | UART0_TX   | PWM_2      | SPI0_CS    | SHUB0_MOSI | SHUB1_SCK  | SHUB0_CS   | SPI0_MOSI  | UART0_CTS  | XPAON      | I2C1_SCK   |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P12# | GPIO_12    | KSI_7      |            |            | KSO_18     | QDZ_1      | SPI0_MOSI  | I2C1_SCK   |            | SHUB1_SCK  | SHUB0_MOSI |            | UART0_CTS  |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P13  | GPIO_13    | KSI_6      |            |            | KSO_19     | QDZ_0      | SPI0_MISO  | I2C1_SDA   |            | SHUB0_MISO | SHUB1_SDA  |            |            | UART0_RTS  |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P14# | QSPI_CLK   |            |            |            |            |            |            |            | QSPI_D3    |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P15# | QSPI_CS    |            |            |            |            |            |            |            |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P16# | QSPI_D0    |            |            |            |            |            |            |            |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P17* | QSPI_D1    |            |            | QSPI_CLK   |            |            |            |            |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P18# | QSPI_D2    | KSI_5      | GPIO_14    |            |            |            |            |            |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P19* | QSPI_D3    | KSI_4      | GPIO_15    | QSPI_CS    |            |            |            |            |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P20  | GPIO_16    | KSI_3      |            | QSPI_D0    |            |            | PWM_3      | SPI0_CLK   | SHUB0_CLK  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P21# | GPIO_17    | KSI_2      | UART0_CTS  |            |            |            | PWM_4      | SPI0_CS    | SHUB0_CS   |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P22  | GPIO_18    | KSI_1      |            | QSPI_D1    |            | UART0_RX   | PWM_5      | SPI0_MOSI  | SHUB0_MOSI |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P23  | GPIO_19    | KSI_0      |            | QSPI_D2    |            | UART0_TX   | PWM_6      | SPI0_MISO  | SHUB0_MISO |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P24  | GPIO_20    | KSO_7      | UART0_RTS  | QSPI_D3    | PDM_CLK    | QDY_1      | PWM_3      | SPI1_CLK   | SHUB1_CLK  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P25  | GPIO_21    | KSO_6      | UART0_RX   |            | PDM_IN     | QDY_0      | PWM_4      | SPI1_CS    | SHUB1_CS   |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P26# | GPIO_22    | KSO_5      |            |            |            | QDX_1      | PWM_5      | SPI1_MOSI  | SHUB1_MOSI |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P27# | GPIO_23    | KSO_4      |            |            |            | QDX_0      | PWM_6      | SPI1_MISO  | SHUB1_MISO |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P28# | GPIO_24    | KSO_3      |            |            |            |            | PWM_7      |            |            |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P29# | GPIO_25    | KSO_2      |            |            |            |            | I2C0_SCK   | UART1_RTS  | SHUB0_SCK  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P30  | GPIO_26    | KSO_1      |            |            |            | UART0_TX   | I2C0_SDA   | UART1_TX   | SHUB0_SDA  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P31# | GPIO_27    | KSO_0      |            |            |            |            | I2C1_SCK   | UART1_RX   | SHUB1_SCK  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P32  | GPIO_28    | UART1_RX   | KSI_17     | KSO_17     |            | QDX_1      | I2C1_SDA   | UART1_CTS  | SHUB1_SDA  |            |            |            |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+
| P33  | GPIO_29    | UART1_TX   |            |            |            | QDX_0      |            |            |            | SHUB1_MISO |            | SPI1_MISO  |            |            |            |
+------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+------------+

Programming and Debugging
*************************

Applications for the ATMEVK-Mxxxx-02 boards can be built, flashed, and debugged in the usual way. See `Building`_ and `Flashing`_ for more details on building and running.

Here is an example for the ``samples/hello_world`` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ screen <tty_device> 115200

Replace :code:`<tty_device>` with the port where the UART1 interface of the ATMEVK-Mxxxx-02 board is connected. For example, under Linux, :code:`/dev/ttyUSB0`.

The bootloader and the application can be built the usual way. Refer to the 'Boards' section above for the values that can be used to replace <BOARD> below.


.. _build_an_application:

Building
========

From the root of the zephyr workspace (``WEST_TOPDIR``):

Build MCUboot
-------------

.. code-block:: console

   $ west build -s bootloader/mcuboot/boot/zephyr -b <BOARD> -d build/<BOARD>/bootloader/mcuboot/boot/zephyr

Build the Application
---------------------

.. code-block:: console

   $ west build -s zephyr/samples/hello_world -b <BOARD> -d build/<BOARD>/zephyr/samples/hello_world -- -DCONFIG_BOOTLOADER_MCUBOOT=y '-DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE="bootloader/mcuboot/root-rsa-2048.pem"'



.. _application_run:

Flashing
========

Flash MCUboot
-------------

.. code-block:: console

   $ west flash --build-dir build/<BOARD>/bootloader/mcuboot/boot/zephyr --device=<ATRDIxxxx> --verify --noreset


Flash the Application
---------------------

.. code-block:: console

   $ west flash --build-dir build/<BOARD>/zephyr/samples/hello_world --device=<ATRDIxxxx> --verify


Note on UART1 RX
================

In order to use P32 for UART1 RX the MODE2 switch on the EVK (labelled SW1) needs to be in the OFF position.
This switch is connected to a 1k pull-down resistor and when enabled the FTDI interface board is unable to drive the UART1 RX line.
In order to connect the ATMx2 part and the FTDI interface board the J3 jumper on the FTDI interface board must also be connecting pins 5 and 6 (third position from the right); however, when loading the application, the jumper on J3 should be removed because otherwise benign boot will prevent the chip from booting into the application.

Atmosic In-System Programming (ISP) Tool
****************************************

This SDK ships with a tool called Atmosic In-System Programming Tool
(ISP) for bundling all three types of binaries -- OTP NVDS, flash NVDS, and
flash -- into a single binary archive.

+---------------+-----------------------------------------------------+
|  Binary Type  |  Description                                        |
+---------------+-----------------------------------------------------+
|   .bin        |  binary file, contains flash or nvds data only.     |
+---------------+-----------------------------------------------------+

The ISP tool, which is also shipped as a stand-alone package, can then be used
to unpack the components of the archive and download them on a device.

west atm_arch commands
======================

atm isp archive tool
  -atm_isp_path ATM_ISP_PATH, --atm_isp_path ATM_ISP_PATH
                        specify atm_isp exe path path
  -d, --debug           debug enabled, default false
  -s, --show            show archive
  -b, --burn            burn archive
  -a, --append          append to input atm file
  -i INPUT_ATM_FILE, --input_atm_file INPUT_ATM_FILE
                        input atm file path
  -o OUTPUT_ATM_FILE, --output_atm_file OUTPUT_ATM_FILE
                        output atm file path
  -p PARTITION_INFO_FILE, --partition_info_file PARTITION_INFO_FILE
                        partition info file path
  -nvds_file NVDS_FILE, --nvds_file NVDS_FILE
                        nvds file path
  -spe_file SPE_FILE, --spe_file SPE_FILE
                        spe file path
  -app_file APP_FILE, --app_file APP_FILE
                        application file path
  -mcuboot_file MCUBOOT_FILE, --mcuboot_file MCUBOOT_FILE
                        mcuboot file path
  -atmwstk_file ATMWSTK_FILE, --atmwstk_file ATMWSTK_FILE
                        atmwstk file path
  -openocd_pkg_root OPENOCD_PKG_ROOT, --openocd_pkg_root OPENOCD_PKG_ROOT
                        Path to directory where openocd and its scripts are found

Support Linux and Windows currently. The ``--atm_isp_path`` option should be specifiec accordingly.

On Linux::
  the ``--atm_isp_path`` option should be modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

On Windows::
  the ``--atm_isp_path`` option should be modules/hal/atmosic_lib/tools/atm_arch/bin/Windows_NT/atm_isp.exe

Generate atm isp file
---------------------

Without MCUBOOT::
  west atm_arch -o <BOARD>_beacon.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

With MCUBOOT::
  west atm_arch -o <BOARD>_beacon.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.signed.bin \
    --mcuboot_file build/<BOARD>/<MCUBOOT>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

Show atm isp file
-----------------

  west atm_arch -i <BOARD>_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp \
    --show

Flash atm isp file
------------------

  west atm_arch -i <BOARD>_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp \
    --openocd_pkg_root openair/modules/hal_atmosic \
    --burn
