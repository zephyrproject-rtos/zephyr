.. zephyr:board:: dect_nr_plus_usb_dongle

Overview
********

The Norik DECT NR+ USB Dongle is a compact development platform built around the
Nordic Semiconductor nRF9151 System-in-Package (SiP). It is intended for
evaluating and prototyping DECT NR+ (also known as DECT-2020 New Radio)
applications, and can operate either as a child node or as a gateway.

The dongle plugs directly into a USB-A port and requires no external programmer
or cables: an onboard Raspberry Pi RP2040 acts as a CMSIS-DAP debug probe and a
USB-to-serial bridge for the nRF9151.

More information about the board can be found on the `DECT NR+ USB Dongle Product Page`_
and in the `DECT NR+ USB Dongle Datasheet`_.

Hardware
********

- Nordic Semiconductor nRF9151 SiP

  - Arm Cortex-M33 running at up to 64 MHz
  - 1 MB flash and 256 KB RAM
  - Arm TrustZone and Arm CryptoCell security

- DECT NR+ radio with support for bands 1, 2, 9 and 22
- Onboard omnidirectional chip antenna
- Raspberry Pi RP2040 interface MCU providing:

  - CMSIS-DAP debug access to the nRF9151 over SWD
  - USB-to-serial bridge to the nRF9151 UART
  - USB Device Firmware Upgrade (DFU) of the interface MCU

- USB-A connector for power, programming and communication
- One user LED
- USB powered, operating supply voltage 4.75 V to 5.25 V
- Operating temperature range -40 to +85 degrees Celsius
- Board dimensions: 53.35 mm x 21.59 mm

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED0 (green) = P0.20

UART
----

The nRF9151 ``uart0`` is connected to the onboard RP2040 interface MCU, which
exposes it to the USB host as a serial port (CDC ACM). It is used as the Zephyr
console and shell backend at 115200 baud (8N1).

* UART0 TX = P0.01
* UART0 RX = P0.00

Security
********

The nRF9151 supports Arm TrustZone. By default the ``dect_nr_plus_usb_dongle/nrf9151``
board target builds a single Secure image, while the ``/ns`` variant builds a
Non-Secure application that runs alongside Trusted Firmware-M (TF-M) as the
Secure firmware. Most DECT NR+ and modem applications require the ``/ns``
variant, since the modem firmware and its security services are only reachable
from Non-Secure firmware through TF-M.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The DECT NR+ USB Dongle integrates a Raspberry Pi RP2040 acting as a CMSIS-DAP
debug probe, so no external debugger is required. Simply plug the dongle into a
USB-A port; the nRF9151 can then be programmed and debugged over the onboard
probe, and its console is available on the serial port presented by the dongle.

Building an application
=======================

Applications that use the modem and DECT NR+ features must be built for the
Non-Secure board target ``dect_nr_plus_usb_dongle/nrf9151/ns``. Applications
that do not require Non-Secure execution can be built for the Secure target
``dect_nr_plus_usb_dongle/nrf9151``.

Flashing
========

Use the :zephyr:code-sample:`blinky` sample to verify that Zephyr runs correctly
on the board. Build and flash it with:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: dect_nr_plus_usb_dongle/nrf9151
   :goals: build flash

To connect to the console, open the serial port presented by the dongle (for
example ``/dev/ttyACM0`` on Linux) at 115200 baud.

.. note::

   Holding the ``BOOT`` button while plugging in the dongle puts the RP2040
   interface MCU into its USB mass-storage (UF2) bootloader, which is used to
   update the interface firmware itself. It does not affect the nRF9151
   application.

Debugging
=========

The onboard CMSIS-DAP probe can also be used for debugging. After building an
application, start a debug session with:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: dect_nr_plus_usb_dongle/nrf9151
   :goals: debug

References
**********

.. target-notes::

.. _DECT NR+ USB Dongle Product Page: https://www.norik.com/dect-nr-usb-dongle/
.. _DECT NR+ USB Dongle Datasheet: https://www.norik.com/wp-content/uploads/2025/04/Norik_DECT_NR_USB_Dongle_V1.1.pdf
