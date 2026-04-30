.. zephyr:board:: sl2619_rdk

Overview
********

`SL2619 RDK` enables easy and rapid prototyping of multimodal AI-native IoT applications. A flexible design
approach supports a core compute module, an I/O base board, debug, and programmable I/O. It's powered by the
Synaptics SL2619 SoC in `SL2610 Product Line`_

Hardware
********

- Core modules:
  - Synaptics SL2619 SoC
  - Storage (eMMC 5.1), memory (DRAM), PMIC,
  - SD card slot
  - Audio Input/Output
  - Digital microphones (DMICs)
  - RGMII-TX/RX

- Daughter card interface options:
  - MIPI-DSI and MIPI-CSI interface
  - JTAG, 40-pin header
  - 4-pin PoE+

- I/O board:
  - HDMI Type-A Tx
  - M.2 E-key 2230 slot for SDIO and UART
  - USB 3.0 Type-A: 4 ports, host mode
  - USB 2.0 Type-C: OTG host or peripheral mode
  - Push buttons: USB-BOOT selection and system RESET
  - 2pin Header: SD-BOOT selection
  - Type-C power supply with 15V @ 1.8A


Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging (A55)
*************************

.. zephyr:board-supported-runners::

There are multiple methods to program and debug Zephyr. The convenient method would be using U-Boot Command, see the following documentation:

Building and Flashing
=====================

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: sl2619_rdk/sl2619/a55
   :goals: build

Copy the compiled ``zephyr.bin`` to the tftpboot directory.

.. code-block:: console

   => tftpboot 0x1000000 zephyr.bin
   => go 0x10000000

You should see the following output on the serial console:

.. code-block:: text

   *** Booting Zephyr OS build vx.x.x-xxx-gxxxxxxxxxxxx ***
   Hello World! sl2619_rdk/sl2619/a55

.. _SL2610 Product Line:
   https://www.synaptics.com/products/embedded-processors/sl2610-product-line
