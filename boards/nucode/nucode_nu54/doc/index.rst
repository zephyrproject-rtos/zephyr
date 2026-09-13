.. zephyr:board:: nucode_nu54

Overview
********

The NUCODE NU54V DK is a development board for the NU54V module, which is
based on the Nordic Semiconductor nRF54L15. The module provides 1.5 MB of
RRAM, 256 KB of RAM, and a 2.4 GHz radio supporting Bluetooth Low Energy and
IEEE 802.15.4. The board includes four user LEDs, four push buttons, a Qwiic
connector, a Cortex 10-pin debug connector, and an onboard DAPLink CMSIS-DAP
debug probe.

This board definition targets the NU54V DK with the nRF54L15. The NU54 module
family also contains nRF54L05 and nRF54L10 variants, which are not covered by
this board definition.

The pin assignments are based on the `NUCODE NU54 DK board files`_,
`NUCODE NU54 DK pinout`_, and the current `NUCODE NU54 DK schematic`_.
See the `NUCODE NU54 software page`_ and `NUCODE NU54V DK product page`_ for
more information.

Hardware
********

- Nordic Semiconductor nRF54L15 Arm Cortex-M33 application processor
- 1.5 MB RRAM and 256 KB RAM
- 2.4 GHz multiprotocol radio
- Four user LEDs and four user push buttons
- Qwiic I2C connector
- Onboard DAPLink CMSIS-DAP probe with two CDC ACM UART interfaces
- Cortex 10-pin SWD connector for an external probe

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

* LED0 = P2.09 (active high)
* LED1 = P1.10 (active high)
* LED2 = P2.07 (active high)
* LED3 = P1.14 (active high)

Push Buttons
------------

* ``button0``/``sw0`` = board signal SW1 = component SW2 = P1.13
  (active low, pull-up)
* ``button1``/``sw1`` = board signal SW2 = component SW3 = P1.09
  (active low, pull-up)
* ``button2``/``sw2`` = board signal SW3 = component SW4 = P1.08
  (active low, pull-up)
* ``button3``/``sw3`` = board signal SW4 = component SW5 = P0.04
  (active low, pull-up)

Component SW1 is the two-pole ``DISABLE_SWD``/``DISABLE_UART`` switch, not a
user push button. The ``sw0`` through ``sw3`` names above are Zephyr aliases.

UART Console
------------

The board schematic routes the primary onboard debugger UART path to UART20.
The default console speed is 115200 baud. RTS and CTS are routed to the debug
interface, but hardware flow control is not enabled by default.

* TX = P1.04
* RX = P1.05
* RTS = P1.06
* CTS = P1.07

UART30 is available to the CPUAPP as a disabled optional configuration on
P0.00 through P0.03.

Both UART paths pass through the onboard debug interface. Probe firmware may
expose them as two CDC ACM interfaces named ``UART0`` and ``UART1``. If there
is no console output, verify that the ``DISABLE_UART`` half of SW1 allows the
UART level shifters to drive the target. UART20 also depends on SB9 through
SB12; UART30 depends on SB5 through SB8. The switch direction, solder-bridge
population, and onboard probe firmware must be checked against the actual
board revision. A CDC ACM interface appearing on the host does not by itself
confirm that the probe firmware is forwarding the corresponding target UART.

Qwiic I2C
---------

The Qwiic connector uses I2C22.

* SCL = P1.03
* SDA = P1.02

Pin Sharing
-----------

P1.02 and P1.03 are shared with NFC, so NFCT is disabled by default. P1.10 is
shared by ``led1`` and ``pwm-led0``. P2.07 can be connected to SWO through
SB13. P1.08 and P0.04 are connected to buttons which short the pins to ground
when pressed. Do not route GRTC clock outputs to these pins unless the physical
button paths have been isolated.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The onboard probe enumerates as a DAPLink CMSIS-DAP device. Build
and flash a sample with pyOCD as follows. The board's USB-C connection powers
the board and carries CMSIS-DAP, mass-storage, and CDC ACM interfaces; a second
USB-UART cable is not required by the board design.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucode_nu54/nrf54l15/cpuapp
   :goals: build flash

The board does not have a pyOCD board-ID entry, so the board runner explicitly
selects the built-in ``nrf54l`` target. It also disables pyOCD's automatic
unlock operation so that connecting to a protected target fails instead of
implicitly mass-erasing it. An external J-Link can also be used through the
Cortex 10-pin connector by selecting the J-Link runner.

On Linux, pyOCD needs permission to access the onboard probe's composite USB
device. If the probe is listed only when running pyOCD as root, install the
following udev rule as ``root``:

.. code-block:: console

   # echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0d28", ATTR{idProduct}=="0204", MODE="0666"' > /etc/udev/rules.d/50-nucode-nu54-cmsis-dap.rules

Reload the rules, and then unplug and reconnect the board:

.. code-block:: console

   $ sudo udevadm control --reload-rules

The supported board target is ``nucode_nu54/nrf54l15/cpuapp``. When using an
external probe, isolate the onboard SWD interface before driving the same
signals.

Hardware Configuration Notes
****************************

The internal HFXO and LFXO load-capacitance values and the DCDC mode follow
NUCODE's current board files. The board's oscillator path depends on its BOM
and the SB18 through SB21 population. If an external LFXO is populated, update
the devicetree configuration as described in the
`NUCODE NU54 DK hardware notes`_. Verify these settings when supporting a board
revision with a different BOM or solder-bridge population.

References
**********

.. target-notes::

.. _NUCODE NU54 software page: https://nuworks.io/wiki/software/NU54
.. _NUCODE NU54 DK board files: https://github.com/Nucode01/NU54DK_Zephyr_DTS
.. _NUCODE NU54 DK pinout:
   https://github.com/Nucode01/NU54DK_Zephyr_DTS/blob/main/00_Docs/03_PINOUT.md
.. _NUCODE NU54 DK hardware notes:
   https://github.com/Nucode01/NU54DK_Zephyr_DTS/blob/main/00_Docs/04_HARDWARE_NOTES.md
.. _NUCODE NU54 DK schematic:
   https://github.com/Nucode01/NU54DK_Zephyr_DTS/blob/main/NU54-DK%20Schematic.pdf
.. _NUCODE NU54V DK product page:
   https://nucode.store/product/nu-54v-dk-nucode-nrf54l15-ble-60-mcu-kcfcccemic/36/
