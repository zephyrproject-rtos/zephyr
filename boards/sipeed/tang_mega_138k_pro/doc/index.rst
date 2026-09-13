.. zephyr:board:: tang_mega_138k_pro

Overview
********

The `Tang Mega 138K Pro Dock`_ is an FPGA development board from Sipeed based on the Gowin
GW5AST-LV138 device. It combines a large FPGA fabric with an integrated 32-bit RISC-V
`AndesTech AE350`_ SoC and provides interfaces such as DDR3, PCIe Gen3 x4, SFP+, Gigabit
Ethernet, HDMI/DVI, MIPI CSI, and M.2.


Hardware
********

The Tang Mega 138K Pro Dock provides the following hardware features:

- Andes AE350 32-bit RISC-V processor
- 1 GB DDR3 memory
- 2 x 128 Mbit SPI flash
- 1 x 8 Kbit I2C EPROM
- PCIe connector
- 2 x SFP+ connectors
- Gigabit Ethernet
- 2 x DVI TX interface
- 2 x DVI RX interface
- DVP interface
- RGB interface
- 2 x MIPI CSI connectors
- 2 x ADC
- 2 x MS5351 Clock generator
- MicroSD card slot
- M.2 Key-B socket
- 3 x PMOD connectors
- 1 x Customizable USB-C connector
- 40-pin expansion header
- 3.5 mm headphone jack
- Speaker connector
- Mic Array interface
- WS2812 RGB LED and aRGB/WS2812 strip connector
- PWM fan connector
- JTAG and UART debug interfaces
- User LEDs and buttons


Supported Features
==================

.. zephyr:board-supported-hw::

The ``tang_mega_138k_pro/ae350/demo`` configuration supports the peripherals enabled by the
``ae350_demo`` FPGA IP configuration in the `Modified Reference Design`_:

- UART2 as the console
- GPIO for LEDs and buttons
- Watchdog
- RTC
- PIT


Programming and debugging
*************************

.. zephyr:board-supported-runners::


FPGA provisioning
=================

Before using board, write the matching FPGA bitstream to offset zero of the external flash.
For the ``tang_mega_138k_pro/ae350/demo`` variant, build the ``ae350_demo.fs`` image from
the `Modified Reference Design`_ and program it as follows:

.. code-block:: console

   openFPGALoader --board tangmega138k --external-flash --write-flash \
     --offset 0x0 ae350_demo.fs

This step creates the AE350 processor and peripherals described by the selected variant's
devicetree. Repeat it after the FPGA image has been erased or replaced.


Building and flashing Zephyr
============================

You can build applications in the usual way. Here is an example for
the :zephyr:code-sample:`blinky` application.

Zephyr firmware is programmed via the FTDI JTAG adapter connected to the ``JTAG|UART`` interface
on the board. For the XIP configuration, ``west flash`` writes only ``zephyr.bin`` to the
firmware area reserved by the board runner; it does not program the FPGA image.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: tang_mega_138k_pro/ae350/demo
   :goals: build flash


Debugging
=========

The AE350 is debugged via its dedicated JTAG port on the 40-pin connector (J23). A separate
JTAG adapter compatible with OpenOCD is required.

The AE350 JTAG pins are:

.. list-table::
   :header-rows: 1

   * - JTAG Function
     - Pin number
   * - TMS
     - 5
   * - TCK
     - 6
   * - TRST
     - 7
   * - TDO
     - 8
   * - TDI
     - 9

The default ``tang_mega_138k_pro/ae350/demo`` configuration uses XIP. Flash the application
before starting a debug session, then attach without loading it again because the XIP flash
mapping is read-only to the debugger:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: tang_mega_138k_pro/ae350/demo
   :maybe-skip-config:
   :goals: attach

For early-boot debugging, set ``CONFIG_XIP=n`` in the application configuration and build a
non-XIP image. Non-XIP images are linked at the selected SRAM address and ``west debug`` loads
the ELF directly into RAM:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: tang_mega_138k_pro/ae350/demo
   :maybe-skip-config:
   :goals: build debug


References
**********

.. target-notes::

.. _Tang Mega 138K Pro Dock: https://en.wiki.sipeed.com/hardware/en/tang/tang-mega-138k/mega-138k-pro.html

.. _AndesTech AE350: http://www.andestech.com/en/products-solutions/andeshape-platforms/ae350-axi-based-platform-pre-integrated-with-n25f-nx25f-a25-ax25/

.. _Modified Reference Design: https://github.com/soburi/tang_mega_138kpro_ae350_demo
