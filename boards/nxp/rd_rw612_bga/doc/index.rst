.. _rd_rw612_bga:

NXP RD-RW612-BGA
################

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
| FLEXSPI   | on-chip    | flash                             |
+-----------+------------+-----------------------------------+
| TRNG      | on-chip    | entropy                           |
+-----------+------------+-----------------------------------+
| DMIC      | on-chip    | dmic                              |
+-----------+------------+-----------------------------------+
| WWDT      | on-chip    | watchdog                          |
+-----------+------------+-----------------------------------+
| CTIMER    | on-chip    | counter                           |
+-----------+------------+-----------------------------------+
| MRT       | on-chip    | counter                           |
+-----------+------------+-----------------------------------+


The default configuration can be found in the defconfig file:

   :zephyr_file:`boards/nxp/rd_rw612_bga/rd_rw612_bga_defconfig/`

Other hardware features are not currently supported

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

Here is an example for the :ref:`hello_world` application. This example uses the
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

Here is an example for the :ref:`hello_world` application. This example uses the
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


Resources
=========

.. _RW612 Website:
   https://www.nxp.com/products/wireless-connectivity/wi-fi-plus-bluetooth-plus-802-15-4/wireless-mcu-with-integrated-tri-radiobr1x1-wi-fi-6-plus-bluetooth-low-energy-5-3-802-15-4:RW612
