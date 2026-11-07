.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: py32f003_minimal

These reference circuits run bare PY32F003W16S6TU and PY32F003L16S6TU microcontrollers
from the factory-calibrated 24 MHz internal HSI oscillator. Neither circuit needs an
external crystal. Both parts have 32 KiB flash and 4 KiB SRAM.

The ``w16s6tu`` variant uses the SOP16 package and provides a bidirectional UART console.
The ``l16s6tu`` variant uses the SOP8 package and provides console output only, preserving
the SWD pins for programming and debugging.

Hardware
********

Supply 3.3 V between VCC and VSS and connect a 100 nF bypass capacitor close to those pins.
Connect the debugger's target voltage reference to VCC and share ground with the debugger
and serial adapter. Use a 3.3 V logic-level serial adapter.

Pin numbers below follow the top-view package diagrams in the `PY32F003 datasheet`_.
Check the package orientation before wiring.

.. list-table:: Minimal connections
   :header-rows: 1

   * - Signal
     - W16S6TU (SOP16)
     - L16S6TU (SOP8)
   * - VCC
     - Pin 16
     - Pin 1
   * - VSS
     - Pin 1
     - Pin 8
   * - SWDIO (PA13)
     - Pin 3
     - Pin 6
   * - SWCLK (PA14)
     - Pin 4
     - Pin 7
   * - NRST
     - Pin 5
     - Pin 5
   * - USART1 TX (PA2), to adapter RX
     - Pin 8
     - Pin 4
   * - USART1 RX (PA3), from adapter TX
     - Pin 9
     - Not configured

Leave the default reset and debug option bytes intact. NRST is optional when using
the configured J-Link software reset strategy.

On the SOP8 part, PA10 shares physical pin 6 with PA13/SWDIO. The default pin configuration
leaves that pad assigned to SWDIO instead of UART RX. Do not connect the serial adapter's
TX output to it. Pin 7 is also shared between PB5 and PA14/SWCLK. GPIO ports B and F are
disabled in this variant to avoid using aliases of the debug and reset pads.

On the SOP16 part, pin 4 is shared between PF1 and PA14/SWCLK, and pin 5 is shared between
PF0 and PF2/NRST. Do not configure those GPIO aliases while using SWD and reset.

Programming and debugging
************************

Install J-Link and the Puya device definitions as described for
:zephyr:board:`py32f030_startkit`. The runner selects ``PY32F003X6`` and uses SWD at 1 MHz.

Build and flash console output for the SOP8 circuit:

.. code-block:: console

   west build -b py32f003_minimal/py32f003x6/l16s6tu -d build/f003_l samples/hello_world
   west flash -d build/f003_l --runner jlink
   python -m serial.tools.miniterm /dev/ttyUSB0 115200

For the SOP16 circuit, build the interrupt-driven UART echo sample:

.. code-block:: console

   west build -b py32f003_minimal/py32f003x6/w16s6tu -d build/f003_w samples/drivers/uart/echo_bot
   west flash -d build/f003_w --runner jlink

The SOP8 configuration deliberately omits ``zephyr,shell-uart`` and does not support
the UART echo sample without changing its debug-preserving pin configuration.

Optional LED
============

For :zephyr:code-sample:`blinky`, connect PA0 through a current-limiting resistor to an
LED anode and connect its cathode to VSS. PA0 is pin 6 on SOP16 and pin 2 on SOP8.
Enable the supplied overlay from the Zephyr repository root:

.. code-block:: console

   west build -b py32f003_minimal/py32f003x6/l16s6tu -d build/f003_led samples/basic/blinky -- \
     -DEXTRA_DTC_OVERLAY_FILE="$PWD/boards/puya/py32/py32f003_minimal/overlays/led.overlay"

When building elsewhere, replace ``$PWD`` with the Zephyr repository path.

.. _PY32F003 datasheet:
   https://www.puyasemi.com/download_path/%E6%95%B0%E6%8D%AE%E6%89%8B%E5%86%8C/MCU%20%E5%BE%AE%E5%A4%84%E7%90%86%E5%99%A8/PY32F003_Datasheet_V1.9.pdf
