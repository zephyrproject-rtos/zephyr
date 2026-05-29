.. SPDX-License-Identifier: Apache-2.0
..
.. Copyright (c) 2026 Ezurio LLC

Overview
********

The Lyra 24 |Lyra 24 variant| Development Kit provides support for the Ezurio Lyra 24 |Lyra 24 variant| Bluetooth Low Energy v5.4 module.

The module is based on the Silicon Laboratories |SoC| module and includes an EFR32BG24 Arm Cortex-M33 CPU.

The Lyra 24 |Lyra 24 variant| module incorporates the QFN package EFR32BG24 (1024kB Flash, 256kB RAM). The part features
up to |Number of IO| configurable GPIOs and BLE Radio TX Power up to |BLE TX Power|.

The kit features a USB interface, an on-board SEGGER J-Link debugger, one user-LED and button, and
support for hardware add-on boards via a `mikroBUS`_ socket and a `Qwiic`_ connector.

.. note::
   You can find more information about the Lyra 24 family of modules in the `Lyra 24 product brief`_ and on the `Lyra 24 website`_.

   You can find more information about the Lyra 24 |Lyra 24 variant| module in the |Datasheet Ref|.

   You can find more information about the underlying |SoC| module in the |SoC Datasheet Ref|.

   You can find more information about the underlying EFR32BG24 SoC in the `EFR32BG24 datasheet`_
   and `EFR32BG24 reference manual`_.

Hardware
********

The Lyra 24 |Lyra 24 variant| DVK has two crystal oscillators as follows.

* High-frequency 39 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The high-frequency crystal oscillator is fitted within the Lyra 24 |Lyra 24 variant| module.

The low-frequency crystal oscillator is fitted to the DVK and is external to the Lyra 24 |Lyra 24 variant| module.

The module supports an |Antenna Type| Bluetooth Low Energy antenna.

Full details of the DVK can be found in the |User Guide Ref| and |Schematics Ref|.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

The **Direction** column indicates the pin direction from the module perspective, with
I indicating Input, O Output, I/O for both and N/A where not applicable.
