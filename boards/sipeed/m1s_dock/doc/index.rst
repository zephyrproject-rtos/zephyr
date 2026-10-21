.. zephyr:board:: m1s_dock

Overview
********

The Sipeed M1S Dock is a development board based on the Bouffalo Lab BL808
tri-core SoC.  The BL808 integrates three RISC-V CPUs, Wi-Fi, BLE 5.0, and a
hardware security engine.

The M1S Dock includes a 16 MB flash (W25Q128JV), USB-C connector, two user
buttons, one user LED, and exposes GPIO pins for peripheral access.

Hardware
********

- SoC: BL808 tri-core RISC-V

  - **M0**: T-Head E907 (RV32IMAFCP, 320 MHz)
  - **D0**: T-Head C906 (RV64IMAFCV, 480 MHz)
  - **LP**: T-Head E902 (RV32EMC, 80 MHz)

- RAM:

   - 224 KB SRAM for M0
   - 512 KB SRAM for D0
   - 48 KB SRAM for LP
   - 64 MB of shared ultra-fast PSRAM

- Flash: 16 MB SPI NOR (W25Q128JV)
- LED: 1 user LED on GPIO8
- Buttons: 2 user buttons on GPIO22 and GPIO23
- Console:

   - MM_UART at 115200 baud for D0 (Adapter's first port, GPIO16 and 17)
   - UART0 at 115200 baud for M0 (Adapter's second port, GPIO14 and 15)
   - UART1 at 115200 for LP (GPIO30 and 31)

For more information about the Bouffalo Lab BL808 SoC:

- `Bouffalo Lab BL808 Datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The BL808 default clock configuration:

- M0 (E907): 320 MHz (WIFIPLL)
- D0 (C906): 480 MHz (CPUPLL)
- LP (E902): 80 MHz (BCLK)

Serial Port
===========

The ``m1s_dock`` board uses UART0 as the default serial port, accessible via
the USB-C connector through the second port of the onboard BL702-based adapter.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

#. Build and flash the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: m1s_dock/bl808c09q2i/m0
      :goals: build flash

#. Run your favorite terminal program to listen for output. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB1 -o

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release the RST button:

   .. code-block:: console

      *** Booting Zephyr OS build v4.5.0 ***
      Hello World! m1s_dock/bl808c09q2i/m0


Asymmetric Multiprocessing
==========================

#. Build the first firmware (LP CPU):

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: m1s_dock/bl808c09q2i/lp
      :goals: build

#. The flash tool has limited ability to flash multiple firmware, start from the highest flash address and provide it with the target address minus bootheader size (0x2000):

   .. code-block:: console

      bflb-mcu-tool-uart --chipname bl808 --port /dev/ttyUSB1 --addr 0x7FE000  --firmware build/zephyr/zephyr.bin

#. Build the second firmware (D0 CPU):

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: m1s_dock/bl808c09q2i/d0
      :goals: build

#. Flash it:

   .. code-block:: console

      bflb-mcu-tool-uart --chipname bl808 --port /dev/ttyUSB1 --addr 0x3FE000  --firmware build/zephyr/zephyr.bin

#. Build the main firmware (M0 CPU), enabling the other CPUs, and flash it normally:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: m1s_dock/bl808c09q2i/m0
      :gen-args: -DCONFIG_SOC_SERIES_BL808_D0_START=y -DCONFIG_SOC_SERIES_BL808_LP_START=y
      :goals: build flash

.. _Bouffalo Lab BL808 Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL808_DS/en
