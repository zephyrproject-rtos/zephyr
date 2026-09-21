.. zephyr:board:: ensemble_e1c_sk

Overview
********

The Alif Ensemble E1C StartKit is a low-cost, single-board development
platform for the low-power, AI-enabled E1C MCU in the Alif Ensemble family.

The E1C series contains a single CPU cluster:

* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz

The board supports the AE1C1F4051920PH0 SoC variant, with the board
identifier ``ensemble_e1c_sk/ae1c1f4051920ph0/rtss_he``.

More information about the board can be found at the
`Ensemble E1C StartKit Product Page`_.

Hardware
********

The Ensemble E1C StartKit provides the following hardware components:

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

The Alif Ensemble E1C StartKit is programmed using the Alif Security Toolkit
(SETOOLS). Refer to the `Ensemble E1C StartKit Product Page`_ for detailed
instructions on installing and using SETOOLS to program the board.

Building an application for the RTSS-HE core:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e1c_sk/ae1c1f4051920ph0/rtss_he
   :goals: build

After building, use SETOOLS to flash the ``build/zephyr/zephyr.bin`` binary to
the board.

After flashing and resetting the board, you should see the following message
on the serial console:

.. code-block:: console

   Hello World! ensemble_e1c_sk/ae1c1f4051920ph0/rtss_he

Debugging
=========

The board supports debugging through the on-board J-Link debugger using standard
SEGGER J-Link tools.

References
**********

.. target-notes::

.. _Ensemble E1C StartKit Product Page:
   https://alifsemi.com/support/kits/ensemble-e1cstartkit/
