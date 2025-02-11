.. _96b_meerkat96:

96Boards Meerkat96
##################

Overview
********

96Boards Meerkat96 board is based on NXP i.MX7 Hybrid multi-core processor,
composed of a dual Cortex®-A7 and a single Cortex®-M4 core.
Zephyr OS is ported to run on the Cortex®-M4 core.

- Board features:

  - RAM: 512 Mbyte
  - Storage:

    - microSD Socket
  - Wireless:

    - WiFi: 2.4GHz IEEE 802.11b/g/n
    - Bluetooth: v4.1 (BR/EDR)
  - USB:

    - Host - 2x type A
    - OTG: - 1x type micro-B
  - HDMI
  - Connectors:

    - 40-Pin Low Speed Header
    - 60-Pin High Speed Header
  - LEDs:

    - 4x Green user LEDs
    - 1x Blue Bluetooth LED
    - 1x Yellow WiFi LED

.. image:: img/96b_meerkat96.jpg
   :align: center
   :alt: 96Boards Meerkat96

More information about the board can be found at the
`96Boards website`_.

Hardware
********

The i.MX7 SoC provides the following hardware capabilities:

- Dual Cortex A7 (800MHz/1.0GHz) core and Single Cortex M4 (200MHz) core

- Memory

  - External DDR memory up to 1 Gbyte
  - Internal RAM -> A7: 256KB SRAM
  - Internal RAM -> M4: 3x32KB (TCML, TCMU, OCRAM_S), 1x128KB (OCRAM) and 1x256MB (DDR)

- Display

  - RGB 1920x1080x24bpp
  - 4-wire Resistive touch

- Multimedia

  - 1x Camera Parallel Interface
  - 1x Analog Audio Line in (Stereo)
  - 1x Analog Audio Mic in (Mono)
  - 1x Analog Audio Headphone out (Stereo)

- Connectivity

  - USB 2.0 OTG (High Speed)
  - USB 2.0 host (High Speed)
  - 10/100 Mbit/s Ethernet PHY
  - 4x I2C
  - 4x SPI
  - 7x UART
  - 1x IrDA
  - 20x PWM
  - Up to 125 GPIO
  - 4x Analog Input (12 Bit)
  - 2x SDIO/SD/MMC (8 Bit)
  - 2x CAN

More information about the i.MX7 SoC can be found here:

- `i.MX 7 Series Website`_
- `i.MX 7 Dual Datasheet`_
- `i.MX 7 Dual Reference Manual`_

Supported Features
==================

The Zephyr 96b_meerkat96 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

	:zephyr_file:`boards/96boards/meerkat96/96b_meerkat96_mcimx7d_m4_defconfig`

Other hardware features are not currently supported by the port.

Connections and IOs
===================

96Boards Meerkat96 board was tested with the following pinmux controller
configuration.

+---------------+-----------------+---------------------------+
| Board Name    | SoC Name        | Usage                     |
+===============+=================+===========================+
| UART_1 RXD    | UART1_TXD       | UART Console              |
+---------------+-----------------+---------------------------+
| UART_1 TXD    | UART1_RXD       | UART Console              |
+---------------+-----------------+---------------------------+
| LED_R1        | GPIO1_IO04      | LED0                      |
+---------------+-----------------+---------------------------+
| LED_R2        | GPIO1_IO05      | LED1                      |
+---------------+-----------------+---------------------------+
| LED_R3        | GPIO1_IO06      | LED2                      |
+---------------+-----------------+---------------------------+
| LED_R4        | GPIO1_IO07      | LED3                      |
+---------------+-----------------+---------------------------+

System Clock
============

The M4 Core is configured to run at a 200 MHz clock speed.

Serial Port
===========

The iMX7D SoC has seven UARTs. UART_1 is configured for the console and
the remaining are not used/tested.

Programming and Debugging
*************************

The 96Boards Meerkat96 board doesn't have QSPI flash for the M4 and it needs
to be started by the A7 core. The A7 core is responsible to load the M4 binary
application into the RAM, put the M4 in reset, set the M4 Program Counter and
Stack Pointer, and get the M4 out of reset. The A7 can perform these steps at
bootloader level or after the Linux system has booted.

The M4 can use up to 5 different RAMs. These are the memory mapping for A7 and M4:

+------------+-----------------------+------------------------+-----------------------+----------------------+
| Region     | Cortex-A7             | Cortex-M4 (System Bus) | Cortex-M4 (Code Bus)  | Size                 |
+============+=======================+========================+=======================+======================+
| DDR        | 0x80000000-0xFFFFFFFF | 0x80000000-0xDFFFFFFF  | 0x10000000-0x1FFEFFFF | 2048MB (less for M4) |
+------------+-----------------------+------------------------+-----------------------+----------------------+
| OCRAM      | 0x00900000-0x0091FFFF | 0x20200000-0x2021FFFF  | 0x00900000-0x0091FFFF | 128KB                |
+------------+-----------------------+------------------------+-----------------------+----------------------+
| TCMU       | 0x00800000-0x00807FFF | 0x20000000-0x20007FFF  |                       | 32KB                 |
+------------+-----------------------+------------------------+-----------------------+----------------------+
| TCML       | 0x007F8000-0x007FFFFF |                        | 0x1FFF8000-0x1FFFFFFF | 32KB                 |
+------------+-----------------------+------------------------+-----------------------+----------------------+
| OCRAM_S    | 0x00180000-0x00187FFF | 0x20180000-0x20187FFF  | 0x00000000-0x00007FFF | 32KB                 |
+------------+-----------------------+------------------------+-----------------------+----------------------+
| QSPI Flash |                       |                        | 0x08000000-0x0BFFFFFF | 64MB                 |
+------------+-----------------------+------------------------+-----------------------+----------------------+

For more information about memory mapping see the
`i.MX 7 Dual Reference Manual`_  (section 2.1.2 and 2.1.3), and the
`Toradex Wiki`_.

At compilation time you have to choose which RAM will be used. This
configuration is done in the file :zephyr_file:`boards/96boards/meerkat96/96b_meerkat96_mcimx7d_m4.dts`
with "zephyr,flash" (when CONFIG_XIP=y) and "zephyr,sram" properties.
The available configurations are:

.. code-block:: none

   "zephyr,flash"
   - &ddr_code
   - &tcml_code
   - &ocram_code
   - &ocram_s_code
   - &ocram_pxp_code
   - &ocram_epdc_code

   "zephyr,sram"
   - &ddr_sys
   - &tcmu_sys
   - &ocram_sys
   - &ocram_s_sys
   - &ocram_pxp_sys
   - &ocram_epdc_sys


Below you will find the instructions to load and run Zephyr on M4 from
A7 using u-boot.

Copy the compiled zephyr.bin to the first FAT partition of the SD card and
plug into the board. Power it up and stop the u-boot execution.
Set the u-boot environment variables and run the zephyr.bin from the
appropriated memory configured in the Zephyr compilation:

.. code-block:: console

   setenv bootm4 'fatload mmc 0:1 $m4addr $m4fw && dcache flush && bootaux $m4addr'
   # TCML
   setenv m4tcml 'setenv m4fw zephyr.bin; setenv m4addr 0x007F8000'
   setenv bootm4tcml 'run m4tcml && run bootm4'
   run bootm4tcml
   # TCMU
   setenv m4tcmu 'setenv m4fw zephyr.bin; setenv m4addr 0x00800000'
   setenv bootm4tcmu 'run m4tcmu && run bootm4'
   run bootm4tcmu
   # OCRAM
   setenv m4ocram 'setenv m4fw zephyr.bin; setenv m4addr 0x00900000'
   setenv bootm4ocram 'run m4ocram && run bootm4'
   run bootm4ocram
   # OCRAM_S
   setenv m4ocrams 'setenv m4fw zephyr.bin; setenv m4addr 0x00180000'
   setenv bootm4ocrams 'run m4ocrams && run bootm4'
   run bootm4ocrams
   # DDR
   setenv m4ddr 'setenv m4fw zephyr.bin; setenv m4addr 0x80000000'
   setenv bootm4ddr 'run m4ddr && run bootm4'
   run bootm4ddr

Debugging
=========

96Boards Meerkat96 board can be debugged by connecting an external JLink
JTAG debugger to the J4 debug connector. Then download and install
`J-Link Tools`_ and `NXP iMX7D Connect CortexM4.JLinkScript`_.

To run Zephyr Binary using J-Link create the following script in order to
get the Program Counter and Stack Pointer from zephyr.bin.

get-pc-sp.sh:
.. code-block:: console

   #!/bin/sh

   firmware=$1

   pc=$(od -An -N 8 -t x4 $firmware | awk '{print $2;}')
   sp=$(od -An -N 8 -t x4 $firmware | awk '{print $1;}')

   echo pc=$pc
   echo sp=$sp


Get the SP and PC from firmware binary: ``./get-pc-sp.sh zephyr.bin``
.. code-block:: console

   pc=00900f01
   sp=00905020

Plug in the J-Link into the board and PC and run the J-Link command line tool:

.. code-block:: console

   /usr/bin/JLinkExe -device Cortex-M4 -if JTAG -speed 4000 -autoconnect 1 -jtagconf -1,-1 -jlinkscriptfile iMX7D_Connect_CortexM4.JLinkScript

The following steps are necessary to run the zephyr.bin:

1. Put the M4 core in reset
2. Load the binary in the appropriate addr (TMCL, TCMU, OCRAM, OCRAM_S or DDR)
3. Set PC (Program Counter)
4. Set SP (Stack Pointer)
5. Get the M4 core out of reset

Issue the following commands inside J-Link commander:

.. code-block:: console

   w4 0x3039000C 0xAC
   loadfile zephyr.bin,0x00900000
   w4 0x00180000 00900f01
   w4 0x00180004 00905020
   w4 0x3039000C 0xAA

With these mechanisms, applications for the ``96b_meerkat96`` board
configuration can be built and debugged in the usual way (see
:ref:`build_an_application` and :ref:`application_run` for more details).

References
==========

- `Loading Code on Cortex-M4 from Linux for the i.MX 6SoloX and i.MX 7Dual/7Solo Application Processors`_
- `J-Link iMX7D Instructions`_

.. _96Boards website:
   https://www.96boards.org/product/imx7-96/

.. _i.MX 7 Series Website:
   https://www.nxp.com/products/processors-and-microcontrollers/applications-processors/i.mx-applications-processors/i.mx-7-processors:IMX7-SERIES?fsrch=1&sr=1&pageNum=1

.. _i.MX 7 Dual Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMX7DCEC.pdf

.. _i.MX 7 Dual Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMX7DRM

.. _J-Link Tools:
   https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack

.. _NXP iMX7D Connect CortexM4.JLinkScript:
   https://wiki.segger.com/images/8/86/NXP_iMX7D_Connect_CortexM4.JLinkScript

.. _Loading Code on Cortex-M4 from Linux for the i.MX 6SoloX and i.MX 7Dual/7Solo Application Processors:
   https://www.nxp.com/docs/en/application-note/AN5317.pdf

.. _J-Link iMX7D Instructions:
   https://wiki.segger.com/IMX7D

.. _Toradex Wiki:
   https://developer.toradex.com/knowledge-base/freertos-on-the-cortex-m4-of-a-colibri-imx7#Memory_areas
