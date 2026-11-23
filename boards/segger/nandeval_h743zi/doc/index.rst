.. zephyr:board:: nandeval_h743zi

Overview
********

The SEGGER NAND Flash Evaluator (``nandeval_h743zi``) is a platform for
evaluating parallel NAND flash devices. It consists of a baseboard, built
around an ST STM32H743ZI Arm® Cortex®‑M7 microcontroller and an adapter board
that carries the NAND flash device. The adapter board plugs into the baseboard
through a set of pin headers and accepts NAND flash devices in TSOP48 and
VFBGA63 packages, with an 8- or 16-bit data bus. The bundled adapter board
provides a TSOP48 socket, so a NAND flash device can be swapped in and out
without soldering.

The NAND flash device is wired to the microcontroller's dedicated Flexible
Memory Controller (FMC) NAND interface for fast data transfers. The board can
be powered either from the micro-USB connector or from the debug connector,
and a jumper selects a 3.3 V or 1.8 V supply for both the microcontroller and
the NAND flash device.

Key Features

- ST STM32H743ZI Arm Cortex-M7 microcontroller in LQFP144 package
- Three 8-pin headers for connecting the NAND flash adapter board
- Bi-color (green/red) user LED
- 20-pin standard Cortex Debug+ETM connector
- Full-Speed USB interface
- Micro-USB connector for power supply and data transfer
- Selectable 3.3 V / 1.8 V supply voltage

More information about the board can be found at the `SEGGER NAND Flash
Evaluator website`_.

Hardware
********

The NAND Flash Evaluator baseboard is based on the STM32H743ZI and provides
the following hardware components:

- STM32H743ZI in LQFP144 package
- ARM 32-bit Cortex-M7 CPU with FPU
- 480 MHz max CPU frequency
- 2 MB Flash
- 1 MB SRAM
- FMC NAND flash interface
- USB OTG Full Speed
- GPIO with external interrupt capability

For more details on the microcontroller refer to the `STM32H743ZI on
www.st.com`_ and the `STM32H743 reference manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
------------

The NAND Flash Evaluator baseboard has a 12 MHz crystal. By
default, the System clock is driven by the PLL clock at 480 MHz, derived from
the 12 MHz high-speed external (HSE) crystal oscillator.

Serial Console
--------------

The Zephyr console output is provided through SEGGER RTT (Real Time Transfer)
over the debug interface.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The NAND Flash Evaluator baseboard is programmed and debugged through its
20-pin Cortex Debug+ETM connector using a debug probe. In addition to the debug
signals, the connector exposes ITM and DWT trace data as well as ETM instruction
trace on the ``TRACEDATA[0..3]`` and ``TRACECLK`` pins, so ETM can be used in
either Serial Wire (SWD) or JTAG mode.

Applications for the ``nandeval_h743zi`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Connect a compatible JTAG debug probe to your host computer and to the Cortex
Debug+ETM connector of the NAND Flash Evaluator baseboard, then build and
flash an application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nandeval_h743zi
   :goals: build flash

Because the console is routed over SEGGER RTT, open an RTT terminal to view
the output:

.. code-block:: console

   $ west rtt

You should see the following message on the console:

.. code-block:: console

   $ Hello World! nandeval_h743zi

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nandeval_h743zi
   :maybe-skip-config:
   :goals: debug

.. _SEGGER NAND Flash Evaluator website:
   https://www.segger.com/evaluate-our-software/segger/nand-flash-eval-board/

.. _STM32H743ZI on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h743zi.html

.. _STM32H743 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00314099.pdf
