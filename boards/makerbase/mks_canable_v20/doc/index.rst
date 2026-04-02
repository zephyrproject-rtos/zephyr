.. zephyr:board:: mks_canable_v20

Overview
********

The Makerbase MKS CANable V2.0 board features an ARM Cortex-M4 based STM32G431C8 MCU
with a CAN, USB and debugger connections.
Here are some highlights of the MKS CANable V2.0 board:

- STM32 microcontroller in LQFP48 package
- USB Type-C connector (J1)
- CAN-Bus connector (J2)
- ST-LINK/V3E debugger/programmer header (J4)
- USB VBUS power supply (5 V)
- Three LEDs: red/power_led (D1), blue/stat_led (D2), green/word_led (D3)
- One push-button for RESET
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell.

The LED red/power_led (D1) is connected directly to on-board 3.3 V and not controllable by the MCU.

More information about the board can be found at the `MKS CANable V2.0 website`_.
It is very advisable to take a look in on user manual `MKS CANable V2.0 User Manual`_ and
schematic `MKS CANable V2.0 schematic`_ before start.

More information about STM32G431KB can be found here:

- `STM32G431C8 on www.st.com`_
- `STM32G4 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- CAN_RX/BOOT0 : PB8
- CAN_TX : PB9
- D2 : PA15
- D3 : PA0
- USB_DN : PA11
- USB_DP : PA12
- SWDIO : PA13
- SWCLK : PA14
- NRST : PG10

For more details please refer to `MKS CANable V2.0 schematic`_.

System Clock
------------

The MKS CANable V2.0 system clock is driven by internal high speed oscillator.
By default system clock is driven by PLL clock at 160 MHz,
the PLL is driven by the 16 MHz high speed internal oscillator.

The FDCAN1 peripheral is driven by PLLQ, which has 80 MHz frequency.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

MKS CANable V2.0 board includes an SWDIO debug connector header J4.

.. note::

   The debugger is not the part of the board!

Applications for the ``mks_canable_v20`` board target can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board could be flashed using west.

Flashing an application to MKS CANable V2.0
-------------------------------------------

The debugger shall be wired to MKS CANable V2.0 board's J4 connector
according `MKS CANable V2.0 schematic`_.

Build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mks_canable_v20
   :goals: build flash
   :west-args: -S rtt-console
   :compact:

The argument ``-S rtt-console`` is needed for debug purposes with SEGGER RTT protocol.
This option is optional and may be omitted. Omitting it frees up RAM space but prevents RTT usage.

If option ``-S rtt-console`` is selected, the connection to the target can be established as follows:

.. code-block:: console

   $ telnet localhost 9090

You should see the following message on the console:

.. code-block:: console

   $ Hello World! mks_canable_v20/stm32g431xx

.. note::

   Current OpenOCD config will skip Segger RTT for OpenOCD under 0.12.0.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mks_canable_v20
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _MKS CANable V2.0 website:
   https://github.com/makerbase-mks/CANable-MKS

.. _MKS CANable V2.0 User Manual:
   https://github.com/makerbase-mks/CANable-MKS/blob/main/User%20Manual/CANable%20V2.0/Makerbase%20CANable%20V2.0%20Use%20Manual.pdf

.. _MKS CANable V2.0 schematic:
   https://github.com/makerbase-mks/CANable-MKS/blob/main/Hardware/MKS%20CANable%20V2.0/MKS%20CANable%20V2.0_001%20schematic.pdf

.. _STM32G431C8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431c8.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
