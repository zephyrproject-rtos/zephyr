.. zephyr:board:: frdm_rw612

Overview
********

The RW612 is a highly integrated, low-power tri-radio wireless MCU with an
integrated 260 MHz ARM Cortex-M33 MCU and Wi-Fi 6 + Bluetooth Low Energy (LE) 5.3 / 802.15.4
radios designed for a broad array of applications, including connected smart home devices,
gaming controllers, enterprise and industrial automation, smart accessories and smart energy.

The RW612 MCU subsystem includes 1.2 MB of on-chip SRAM and a high-bandwidth Quad SPI interface
with an on-the-fly decryption engine for securely accessing off-chip XIP flash.

The advanced design of the RW612 delivers tight integration, low power and highly secure
operation in a space- and cost-efficient wireless MCU requiring only a single 3.3 V power supply.

Hardware
********

- 260 MHz ARM Cortex-M33, tri-radio cores for Wi-Fi 6 + BLE 5.3 + 802.15.4
- 1.2 MB on-chip SRAM
- External Quad SPI NOR flash (64 MB) over FlexSPI
- USB Type-C connector (J10) for power, UART console, and debug
- 10/100 Mbps Ethernet
- Integrated Wi-Fi 6 (802.11ax) radio
- Integrated Bluetooth Low Energy 5.3 radio
- Integrated 802.15.4 radio (for Zigbee/Thread)
- Arduino Uno R3 compatible headers
- mikroBUS header
- On-board MCU-Link debugger (CMSIS-DAP / J-Link)
- Single 3.3 V power supply

For more information about the RW612 SoC and FRDM-RW612 board, see:

- `RW612 SoC Website`_
- `RW612 Datasheet`_
- `RW612 Reference Manual`_
- `FRDM-RW612 Website`_
- `FRDM-RW612 User Guide`_
- `FRDM-RW612 Board User Manual`_
- `FRDM-RW612 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   Power modes 1, 2 and 3 are supported when using System Power Management.

Shields for Supported Features
==============================

Some features in the table above are tested with Zephyr shields.  These shields
are tested on this board:

- :ref:`lcd_par_s035` - supports the Display interface.  This board uses the
  SPI interface of the shield, connected to the LCDIC on-chip peripheral.

Power management
****************

SVC voltage initialization
==========================

RW6xx SoC clock initialization reads the chip's SVC and package data from
OTP before switching the CPU to 260 MHz. It configures BUCK levels and
voltage-sensor thresholds with the power HAL on cold boot and PM3 recovery.
Applications do not need to duplicate this initialization.

Low-power configuration
=======================

Enable :kconfig:option:`CONFIG_PM` for system power management.
:kconfig:option:`CONFIG_PM_DEVICE` and
:kconfig:option:`CONFIG_PM_DEVICE_SYSTEM_MANAGED` enable device suspend and
resume around system sleep.

The optional :kconfig:option:`CONFIG_SOC_SERIES_RW6XX_T3_CAU_POWER_SAVE` setting applies
to RW610 and RW612 and is disabled by default. It disables T3 at the end of
the default SoC clock initialization on cold boot and PM3 recovery. DTRNG,
GDET and peripherals requiring T3 clocks cannot be used. The option excludes
Flexcomm I2C/SPI, CTIMER, ELS/PKC and
clock monitoring drivers. CAU sleep power-down is enabled for RC32K or
XTAL32K; CAU is retained when CLK32K uses NCO32K.

The board uses iBuck supplies and retains the CAU reference clock needed by
LPOSC. PM1 and PM2 use the OS Timer for timed wakeup. PM3 requires the RTC
high-resolution companion for timed wakeup and time compensation.

Power management test
=====================

Build the standard test with::

   west build -p always -b frdm_rw612 tests/subsys/pm/power_mgmt_soc

The JP9 measurements below used this test with the following configuration:

* :kconfig:option:`CONFIG_SOC_SERIES_RW6XX_T3_CAU_POWER_SAVE` is enabled, with
  immediate logging and a 1024-byte idle stack. UART output remains at
  115200 baud.
* The board test configuration disables unused CTIMER, MRT and PINT drivers.
  The test overlay disables the watchdog and GPIO controller 1.
  Board pinctrl keeps the RGB LEDs off using pull-ups and high sleep output
  levels on GPIO0, GPIO1 and GPIO12.
* The RTC high-resolution companion remains enabled. The measured board
  used RC32K, whose accuracy affects sleep duration.

Selecting a mode
----------------

The standard test uses the board's enabled CPU power states. Its "light"
and "deep" messages describe short and long intervals, not fixed hardware
modes. Check the PM state counters to identify the selected mode.

For a separate measurement of one mode, restrict ``cpu-power-states``
in :zephyr_file:`tests/subsys/pm/power_mgmt_soc/boards/frdm_rw612.overlay`.
For example, select PM2 with::

   &cpu0 {
           cpu-power-states = <&suspend>;
   };

Use ``<&idle>`` for PM1 or ``<&standby>`` for PM3, then rebuild.
Without this restriction, long sleeps normally select PM3.

JP9 current measurements
------------------------

The sleep currents in the following table were measured at JP9.

.. list-table:: Stable JP9 sleep current
   :header-rows: 1
   :widths: 10 35 15 40

   * - Mode
     - Zephyr state
     - Current
     - Timed wakeup
   * - PM1
     - ``PM_STATE_RUNTIME_IDLE``
     - 17.795 mA
     - OS Timer
   * - PM2
     - ``PM_STATE_SUSPEND_TO_IDLE``
     - 7.425 mA
     - OS Timer
   * - PM3
     - ``PM_STATE_STANDBY``
     - 0.818 mA
     - RTC high-resolution counter

PM1--PM3 used the common board configuration above, with only the corresponding
CPU state selected. Each passed all three ztests in three reset runs.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the JLink Firmware.

Configuring a Console
=====================

Connect a USB cable from your PC to J10, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`jlink-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_rw612
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.6.0 *****
   Hello World! frdm_rw612

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`jlink-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_rw612
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS zephyr-v3.6.0 *****
   Hello World! frdm_rw612

SRAM Bus Access Partitioning
****************************

RW612 supports shared access of the SRAM from both the code bus and data bus.
The bus used to access the SRAM is determined using two separate memory mapped address spaces.
The application can configure the partitioning of the SRAM access regions by a devicetree overlay.
For example, below is part of an overlay to change the whole SRAM to be used for data.

.. code-block:: devicetree

   &sram_data {
        reg = <0x0 DT_SIZE_K(1216)>;
   };


NAND Flash (Board Rework)
*************************

The FlexSPI PortB1 on FRDM-RW612 is pin-compatible with SPI NAND flash.
A Winbond W25N01GV (1 Gbit) can be fitted at U12 by board rework, providing
128 MB of NAND storage accessible via the FlexSPI controller.

The ``w25n01gv`` device tree node is present in the board DTS but disabled
by default. Enable it with a devicetree overlay:

.. code-block:: devicetree

   &w25n01gv {
       status = "okay";
   };

The :zephyr:code-sample:`littlefs` sample provides ready-made board files
(``boards/frdm_rw612_nand.*``) that run LittleFS over Dhara FTL on this NAND.

Wireless Connectivity Support
*****************************

Fetch Binary Blobs
==================

To support Bluetooth or Wi-Fi, frdm_rw612 requires fetching binary blobs, which can be
achieved by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

Bluetooth
=========

BLE functionality requires to fetch binary blobs, so make sure to follow
the ``Fetch Binary Blobs`` section first.

frdm_rw612 platform supports the monolithic feature. The required binary blob
``<zephyr workspace>/modules/hal/nxp/zephyr/blobs/rw61x_sb_ble_a2.bin`` will be linked
with the application image directly, forming one single monolithic image.

Wi-Fi
=====

Wi-Fi functionality requires to fetch binary blobs, so make sure to follow
the ``Fetch Binary Blobs`` section first.

frdm_rw612 platform supports the monolithic feature. The required binary blob
``<zephyr workspace>/modules/hal/nxp/zephyr/blobs/rw61x_sb_wifi_a2.bin`` will be linked
with the application image directly, forming one single monolithic image.

RTC Sub-Second Counter
======================

To use the RTC sub-second counter which is clocked at a 32kHZ rate, make the
following modifications to the board hardware:

1. Move the short on SJ21 from 1 and 2 to short 2 and 3.
2. Move the short on SJ22 from 1 and 2 to short 2 and 3.

After this change, the ENET will stop functioning on the board.

.. include:: ../../common/board-footer.rst.inc

.. _RW612 SoC Website:
   https://www.nxp.com/products/wireless-connectivity/wi-fi-plus-bluetooth-plus-802-15-4/wireless-mcu-with-integrated-tri-radiobr1x1-wi-fi-6-plus-bluetooth-low-energy-5-3-802-15-4:RW612

.. _RW612 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/RW612.pdf

.. _RW612 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=RM00278

.. _FRDM-RW612 Website:
   https://www.nxp.com/design/design-center/development-boards/general-purpose-mcus/freedom-development-board-for-rw612-wireless-mcu:FRDM-RW612

.. _FRDM-RW612 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-rw612:GS-FRDM-RW612

.. _FRDM-RW612 Board User Manual:
   https://www.nxp.com/webapp/Download?colCode=UM12160

.. _FRDM-RW612 Schematics:
   https://www.nxp.com/webapp/Download?colCode=FRDM-RW612-SCH
