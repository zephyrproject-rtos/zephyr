.. zephyr:board:: mks_canable_v10

Overview
********

The Makerbase MKS CANable V1.0 board features an ARM Cortex-M0 based STM32F072C8
MCU with a CAN, USB and debugger connections. Here are some highlights of the
MKS CANable V1.0 board:

- STM32 microcontroller in LQFP48 package
- USB Micro-B connector (J1)
- CAN-Bus connector (J3)
- Three LEDs: blue/tx_led (D1), green/rx_led (D2), red/power_led (D3)
- Power-on boot selection for BOOT0 (J5)
- ST-LINK/V3E debugger/programmer header (J2)
- Development support: Serial Wire Debug (SWD)
- USB VBUS power supply (5 V)

The LED red/power_led (D3) is connected directly to on-board 3.3 V
and not controllable by the MCU.

More information about the board can be found at the `MKS CANable V1.0 website`_.
It is very advisable to take a look in on schematic `MKS CANable V1.0 schematic`_
and user manual `MKS CANable V2.0 User Manual`_ before start.

More information about STM32F072C8 can be found here:

- `STM32F072C8 on www.st.com`_
- `STM32F0x1/STM32F0x2/STM32F0x8 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- BOOT : BOOT0
- CAN_RX : PB8
- CAN_TX : PB9
- D1 : PA0
- D2 : PA1
- USB_DN : PA11
- USB_DP : PA12
- SWDIO : PA13
- SWCLK : PA14
- NRST

For more details please refer to `MKS CANable V1.0 schematic`_.

System Clock
------------

The MKS CANable V1.0 system clock is driven by internal high speed oscillator.
By default system clock is driven by PLL clock at 48 MHz, the PLL is driven by
the 8 MHz high speed internal oscillator.

The CAN1 peripheral is driven by PLL, which has 48 MHz frequency.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The MKS CANable V1.0 board includes an SWDIO debug connector header J2.

.. note::

   The debugger is not part of the board!

Applications for the ``mks_canable_v10`` board target can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board could be flashed using west.

Flashing an application to MKS CANable V1.0 via USB DFU
-------------------------------------------------------

If flashing via USB DFU, connect the short-circuit cap to the two pins of
BOOT (J5) when applying power to the MKS CANable V1.0 board in order to enter
the built-in DFU mode.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mks_canable_v10
   :goals: flash

Flashing an application to MKS CANable V1.0 via SWD Debugger
------------------------------------------------------------

The debugger shall be wired to MKS CANable V1.0 board's J2 connector
according `MKS CANable V1.0 schematic`_.

Build and flash an application. Here is an example for using OpenOCD
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mks_canable_v10
   :goals: build flash
   :west-args: -S rtt-console
   :flash-args: --runner openocd
   :compact:

The argument ``-S rtt-console`` is needed for debug purposes with SEGGER RTT
protocol. This option is optional and may be omitted. Omitting it frees up
RAM space but prevents RTT usage.

If option ``-S rtt-console`` is selected, the connection to the target can be
established as follows:

.. code-block:: console

   $ telnet localhost 9090

You should see the following message on the console:

.. code-block:: console

   $ Hello World! mks_canable_v10/stm32f072xb

.. note::

   Current OpenOCD config will skip Segger RTT for OpenOCD under 0.12.0.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mks_canable_v10
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _MKS CANable V1.0 website:
   https://github.com/makerbase-mks/CANable-MKS

.. _MKS CANable V1.0 User Manual:
   https://github.com/makerbase-mks/CANable-MKS/blob/main/User%20Manual/CANable%20V2.0/Makerbase%20CANable%20V2.0%20Use%20Manual.pdf

.. _MKS CANable V1.0 schematic:
   https://github.com/makerbase-mks/CANable-MKS/blob/main/Hardware/MKS%20CANable-V1.0/MKS_CANable-v1.0%20schematic.pdf

.. _MKS CANable V2.0 User Manual:
   https://github.com/makerbase-mks/CANable-MKS/blob/main/User%20Manual/CANable%20V2.0/Makerbase%20CANable%20V2.0%20Use%20Manual.pdf

.. _STM32F072C8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f072c8.html

.. _STM32F0x1/STM32F0x2/STM32F0x8 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
