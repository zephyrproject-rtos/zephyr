.. zephyr:board:: rd_rw612_bga

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

- 260 MHz ARM Cortex-M33, tri-radio cores for Wifi 6 + BLE 5.3 + 802.15.4
- 1.2 MB on-chip SRAM

Supported Features
==================

+-----------+------------+-----------------------------------+
| Interface | Controller | Driver/Component                  |
+===========+============+===================================+
| NVIC      | on-chip    | nested vector interrupt controller|
+-----------+------------+-----------------------------------+
| SYSTICK   | on-chip    | systick                           |
+-----------+------------+-----------------------------------+
| MCI_IOMUX | on-chip    | pinmux                            |
+-----------+------------+-----------------------------------+
| GPIO      | on-chip    | gpio                              |
+-----------+------------+-----------------------------------+
| USART     | on-chip    | serial                            |
+-----------+------------+-----------------------------------+
| DMA       | on-chip    | dma                               |
+-----------+------------+-----------------------------------+
| SPI       | on-chip    | spi                               |
+-----------+------------+-----------------------------------+
| I2C       | on-chip    | i2c                               |
+-----------+------------+-----------------------------------+
| FLEXSPI   | on-chip    | flash/memc                        |
+-----------+------------+-----------------------------------+
| TRNG      | on-chip    | entropy                           |
+-----------+------------+-----------------------------------+
| DMIC      | on-chip    | dmic                              |
+-----------+------------+-----------------------------------+
| LCDIC     | on-chip    | mipi-dbi                          |
+-----------+------------+-----------------------------------+
| WWDT      | on-chip    | watchdog                          |
+-----------+------------+-----------------------------------+
| USBOTG    | on-chip    | usb                               |
+-----------+------------+-----------------------------------+
| CTIMER    | on-chip    | counter                           |
+-----------+------------+-----------------------------------+
| SCTIMER   | on-chip    | pwm                               |
+-----------+------------+-----------------------------------+
| MRT       | on-chip    | counter                           |
+-----------+------------+-----------------------------------+
| OS_TIMER  | on-chip    | os timer                          |
+-----------+------------+-----------------------------------+
| PM        | on-chip    | power management; uses SoC Power  |
|           |            | Modes 1 and 2                     |
+-----------+------------+-----------------------------------+
| BLE       | on-chip    | Bluetooth                         |
+-----------+------------+-----------------------------------+
| ADC       | on-chip    | adc                               |
+-----------+------------+-----------------------------------+
| DAC       | on-chip    | dac                               |
+-----------+------------+-----------------------------------+
| ENET      | on-chip    | ethernet                          |
+-----------+------------+-----------------------------------+

The default configuration can be found in the defconfig file:

   :zephyr_file:`boards/nxp/rd_rw612_bga/rd_rw612_bga_defconfig/`

Other hardware features are not currently supported


Display Support
***************

The rd_rw612_bga board supports several in-tree display modules. Setup for
each module is described below:

GoWorld 16880 LCM
=================

This module does not connect directly to the board, and must be connected
via an adapter board and jumper wires. Connections are described in
:zephyr_file:`boards/nxp/rd_rw612_bga/dts/goworld_16880_lcm.overlay`. The
display sample can be built for this board like so:

.. zephyr-app-commands::
   :board: rd_rw612_bga
   :gen-args: -DDTC_OVERLAY_FILE=goworld_16880_lcm.overlay
   :zephyr-app: samples/drivers/display
   :goals: build
   :compact:

Adafruit 2.8 TFT
================

The :ref:`adafruit_2_8_tft_touch_v2` connects to the board's Arduino headers
directly, but some modifications are required (see
:zephyr_file:`boards/shields/adafruit_2_8_tft_touch_v2/boards/rd_rw612_bga.overlay`
for a list). The display sample can be built for this module like so:

.. zephyr-app-commands::
   :board: rd_rw612_bga
   :shield: adafruit_2_8_tft_touch_v2
   :zephyr-app: samples/drivers/display
   :goals: build
   :compact:

NXP LCD_PAR_S035
================

The :ref:`lcd_par_s035` does not connect directly to the board, and must be
connected via jumper wires. Connections and required board changes are
described in
:zephyr_file:`boards/shields/lcd_par_s035/boards/rd_rw612_bga.overlay`. The
display sample can be built for the module like so:

.. zephyr-app-commands::
   :board: rd_rw612_bga
   :shield: lcd_par_s035_8080
   :zephyr-app: samples/drivers/display
   :goals: build
   :compact:

Fetch Binary Blobs
******************

To support Bluetooth, rd_rw612_bga requires fetching binary blobs, which can be
achieved by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the JLink Firmware.

Configuring a Console
=====================

Connect a USB cable from your PC to J7, and use the serial terminal of your choice
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
   :board: rd_rw612_bga
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.4.0 *****
   Hello World! rd_rw612_bga

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`jlink-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rd_rw612_bga
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS zephyr-v3.6.0 *****
   Hello World! rd_rw612_bga

Bluetooth
*********

BLE functionality requires to fetch binary blobs, so make sure to follow
the ``Fetch Binary Blobs`` section first.

rd_rw612_bga platform supports the monolithic feature. The required binary blob
``<zephyr workspace>/modules/hal/nxp/zephyr/blobs/rw61x_sb_ble_a2.bin`` will be linked
with the application image directly, forming one single monolithic image.

Board variants
**************

Ethernet
========

To use ethernet on the RD_RW612_BGA board, you first need to make the following
modifications to the board hardware:

Add resistors:

- R485
- R486
- R487
- R488
- R489
- R491
- R490

Remove resistors:

- R522
- R521
- R520
- R524
- R523
- R508
- R505

Then, build for the board target ``rd_rw612_bga//ethernet``.

Resources
*********

.. target-notes::

.. _RW612 Website:
   https://www.nxp.com/products/wireless-connectivity/wi-fi-plus-bluetooth-plus-802-15-4/wireless-mcu-with-integrated-tri-radiobr1x1-wi-fi-6-plus-bluetooth-low-energy-5-3-802-15-4:RW612
