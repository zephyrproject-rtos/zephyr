.. zephyr:board:: balletto_b1_sk

Overview
********

The Alif Balletto B1 StartKit is a low-cost, single-board development
platform for the low-power, AI enabled B1 series device in the Alif Balletto
family.

The B1 series contains a single CPU cluster:

* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz

The board supports the AB1C1F4M51820PH0 SoC variant, with the board
identifier ``balletto_b1_sk/ab1c1f4m51820ph0``.

More information about the board can be found at the
`Balletto B1 StartKit Product Page`_.

Hardware
********

The Balletto B1 StartKit provides the following hardware components:

- RF path and chip antenna for the BLE 5.3 radio subsystem
- J-Link On-Board debugger with USB-to-UART bridge
- High-Speed USB device interface
- mikroBUS Click board expansion socket
- Arducam camera header
- 2x PDM digital microphones
- Multicolor user LED
- User and reset push-buttons
- GPIO headers for I/O expansion

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Configuring a Console
=====================

The on-board USB-to-UART bridge is shared between the Secure Enclave UART and
the UART2 user serial port, selected by the JP5 jumpers. Move JP5 to the UART2
position (JP5-3-5 and JP5-4-6) to reach the Zephyr console.

Connect a USB cable from your PC to the PRG USB connector, and use the serial
terminal of your choice (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

To use SETOOLS without moving JP5 back, keep JP5 in the UART2 position and
connect a 1.8 V USB-to-serial cable to the SEUART header J10: pin 1 is GND,
pin 2 is the SEUART transmit line (to the cable RXD) and pin 3 is the SEUART
receive line (to the cable TXD). The Zephyr console and the Secure Enclave UART
are then available at the same time.

Flashing
========

The Alif Balletto B1 StartKit is programmed using the Alif Security Toolkit
(SETOOLS). Refer to the `Balletto B1 StartKit Product Page`_ for detailed
instructions on installing and using SETOOLS to program the board.

Building an application for the RTSS-HE core:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: balletto_b1_sk/ab1c1f4m51820ph0
   :goals: build

After building, use SETOOLS to flash the ``build/zephyr/zephyr.bin`` binary to
the board.

After flashing and resetting the board, you should see the following message
on the serial console:

.. code-block:: console

   Hello World! balletto_b1_sk/ab1c1f4m51820ph0

Debugging
=========

The board supports debugging through the on-board J-Link debugger using standard
SEGGER J-Link tools.

References
**********

.. target-notes::

.. _Balletto B1 StartKit Product Page:
   https://alifsemi.com/support/kits/balletto-b1startkit/
