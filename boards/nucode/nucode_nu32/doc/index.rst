.. zephyr:board:: nucode_nu32

Overview
********

The NUCODE NU32 is a compact module based on the Nordic Semiconductor
nRF52832 ARM Cortex-M4F SoC. It supports Bluetooth Low Energy (BLE) 5.x
and NFC-A. Unlike the NUCODE NU40, the nRF52832 has no USB device
controller, so firmware is programmed over the SWD debug interface.

More information about the board can be found at the
`NUCODE NU32 website`_.

Hardware
********

The ``nucode_nu32/nrf52832`` board target is built around the nRF52832
(512 KB flash, 64 KB RAM, QFN48). The board uses the nRF52832 internal
low-frequency RC oscillator by default.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

UART
----

* UART0: TX = P0.06, RX = P0.08, RTS = P0.05, CTS = P0.07

NFC
---

* NFC-A antenna pins: P0.09 and P0.10

Reset
-----

* RESET = P0.21 (configured through UICR as the hardware reset pin)

SWD
---

* The SWDCLK and SWDIO debug pads are used for flashing and debugging.

NUCODE_JTAG Adapter
-------------------

The NU32-B-Dev04 carrier can be connected to an external J-Link through a
NUCODE_JTAG Adapter.  The minimum SWD connection is:

.. list-table:: SWD connection
   :header-rows: 1

   * - NUCODE_JTAG Adapter
     - NU32-B-Dev04
     - Description
   * - VTARGET
     - VCC or VTARGET
     - Target voltage reference
   * - GND
     - GND
     - Common ground
   * - SWDCLK
     - SWDCLK
     - SWD clock
   * - SWDIO
     - SWDIO
     - SWD data
   * - nRESET (nRST)
     - RESET (P0.21)
     - Target reset

The target must be powered and VTARGET must be connected before starting a
debug session.  Keep the SWD leads short.  Connect nRESET to P0.21 so the
debug probe can reliably reset and recover the target.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nucode_nu32/nrf52832`` board target can be built
and flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is programmed and debugged using an external
:ref:`debug probe <debug-probes>` (for example a SEGGER J-Link) connected
to the SWD pads.

#. Build and flash the :zephyr:code-sample:`hello_world` sample:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: nucode_nu32/nrf52832
      :goals: build flash
      :flash-args: -r jlink

#. Open a serial terminal (115200 8N1) connected to the UART0 pins and
   reset the board to see the output.

If more than one J-Link is connected to the host, select the probe by appending
``--dev-id <serial-number>`` to the ``west flash`` command.

Debugging
=========

Start a debug session with J-Link:

.. code-block:: console

   $ west debug -r jlink

Refer to the :ref:`application_run` page for more details on debugging.

References
**********

.. target-notes::

.. _NUCODE NU32 website: https://nuworks.io
