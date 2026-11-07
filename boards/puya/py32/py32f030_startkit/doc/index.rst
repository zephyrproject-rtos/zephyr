.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: py32f030_startkit

This board has the bare minimum components required to power on the
PY32F030x6 MCU. Most of the GPIOs on the PY32 SoC have been exposed in the
external headers with silk screen labels that match the SoC's pin names.

For practical use, you'll need to add additional components
and circuits using a breadboard, for example.

More information about the board and PY32F030x6 can be found at:

- `PY32F030 reference manual`_
- `PY32F030 data sheet`_

Hardware
********

- PY32F030x6 ARM Cortex-M0+ processor
- 32 KiB of flash memory and 4 KiB of RAM
- 24 MHz quartz crystal
- 1 user LED
- One reset button
- 2-way jumper (BOOT0)
- Serial
- SWD (1x4 male dupont (2.54mm))
- USB port (power only)

Supported Features
==================

The Zephyr py32f030_startkit board configuration supports the following
hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.
Only GPIO port A is described in the initial devicetree.

The default configuration can be found in
:zephyr_file:`boards/puya/py32/py32f030_startkit/py32f030_startkit_defconfig`

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------

- LED : PA11
- USART1 TX/RX : PA9/PA10 (115200 baud, 8 data bits, no parity, 1 stop bit)

UART Console
============

Connect PA9 to the RX input of a 3.3 V USB-to-serial adapter and connect the grounds. This is
sufficient for console output. Build and flash the :zephyr:code-sample:`hello_world` sample,
then open the serial port at 115200 baud:

.. code-block:: console

   west build -b py32f030_startkit -d build/hello_world samples/hello_world
   west flash -d build/hello_world --runner jlink
   python -m serial.tools.miniterm /dev/ttyUSB0 115200

For interrupt-driven reception and line echo, also connect the adapter's TX output to PA10
and use :zephyr:code-sample:`uart`:

.. code-block:: console

   west build -b py32f030_startkit -d build/uart_echo samples/drivers/uart/echo_bot
   west flash -d build/uart_echo --runner jlink

Programming and Debugging
*************************

Applications for the ``py32f030_startkit`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board uses J-Link for flashing and debugging. Install the Puya loaders as
described below before running ``west flash``. Only the 24 MHz HSI clock
configuration is supported. Internal flash can be programmed through J-Link;
the Zephyr runtime flash driver API is not supported.

Flashing an application to PY32F030 Start Kit Board
---------------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: py32f030_startkit
   :goals: build flash

You will see the LED blinking every second.

Flashing with J-Link
-------------------

Install the `SEGGER J-Link software`_ and the Puya device definitions and flash loaders from
`py32f0-template`_. On Linux, install the loaders in the per-user J-Link device directory:

.. code-block:: console

   git clone https://github.com/IOsetting/py32f0-template.git
   mkdir -p "$HOME/.config/SEGGER/JLinkDevices"
   cp -r py32f0-template/Misc/Flash/JLinkDevices/Puya "$HOME/.config/SEGGER/JLinkDevices/"

Connect SWDIO, SWCLK, ground and the target voltage reference to the J-Link probe. The board's
J-Link runner uses reset strategy 0, which performs a software system reset over SWD for the
Puya device definition. The NRST connection is optional. See `J-Link reset strategies`_.

Build and flash the blinky sample with:

.. code-block:: console

   west build -b py32f030_startkit -d build samples/basic/blinky
   west flash -d build --runner jlink

The runner selects ``PY32F030X6``, uses SWD at 1 MHz and resets the target after programming.
It generates the Commander script using the firmware path from the build directory.

For direct use with `J-Link Commander`_, save the following commands as ``flash.jlink`` in the
directory containing ``build``:

.. code-block:: text

   RSetType 0
   h
   loadfile build/zephyr/zephyr.hex 0 noreset
   r
   g
   exit

Run the command file from that directory:

.. code-block:: console

   JLinkExe -device PY32F030X6 -if SWD -speed 1000 -AutoConnect 1 -NoGui 1 \
     -ExitOnError 1 -CommandFile flash.jlink

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: py32f030_startkit
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _PY32F030 SVD reference:
   https://github.com/IOsetting/py32f0-template/blob/main/Misc/SVD/py32f030xx.svd

.. _PY32F030 reference manual:
   https://www.puyasemi.com/download.html?keywords=PY32F030

.. _PY32F030 data sheet:
   https://www.puyasemi.com/en/py32_series.html

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink/

.. _py32f0-template:
   https://github.com/IOsetting/py32f0-template

.. _J-Link reset strategies:
   https://kb.segger.com/J-Link_Reset_Strategies

.. _J-Link Commander:
   https://kb.segger.com/J-Link_Commander
