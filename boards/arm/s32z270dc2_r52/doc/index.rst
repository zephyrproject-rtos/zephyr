.. _s32z270dc2_r52:

NXP X-S32Z27X-DC (DC2)
######################

Overview
********

The X-S32Z27X-DC (DC2) board is based on the NXP S32Z270 Real-Time Processor,
which includes two Real-Time Units (RTU) composed of four ARM Cortex-R52 cores
each, with flexible split/lock configurations.

There is one Zephyr board per RTU:

- ``s32z270dc2_rtu0_r52``, for RTU0
- ``s32z270dc2_rtu1_r52``, for RTU1.

Hardware
********

Information about the hardware and design resources can be found at
`NXP S32Z2 Real-Time Processors website`_.

Supported Features
==================

The boards support the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| Arm GIC   | on-chip    | interrupt_controller                |
+-----------+------------+-------------------------------------+
| Arm Timer | on-chip    | timer                               |
+-----------+------------+-------------------------------------+
| LINFlexD  | on-chip    | serial                              |
+-----------+------------+-------------------------------------+
| MRU       | on-chip    | mbox                                |
+-----------+------------+-------------------------------------+
| NETC      | on-chip    | ethernet                            |
|           |            |                                     |
|           |            | mdio                                |
+-----------+------------+-------------------------------------+
| SIUL2     | on-chip    | pinctrl                             |
|           |            |                                     |
|           |            | gpio                                |
|           |            |                                     |
|           |            | external interrupt controller       |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| SWT       | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The SoC's pads are grouped into ports and pins for consistency with GPIO driver
and the HAL drivers used by this Zephyr port. The following table summarizes
the mapping between pads and ports/pins. This must be taken into account when
using GPIO driver or configuring the pinmuxing for the device drivers.

+-------------------+-------------+
| Pads              | Port/Pins   |
+===================+=============+
| PAD_000 - PAD_015 | PA0 - PA15  |
+-------------------+-------------+
| PAD_016 - PAD_030 | PB0 - PB14  |
+-------------------+-------------+
| PAD_031           | PC15        |
+-------------------+-------------+
| PAD_032 - PAD_047 | PD0 - PD15  |
+-------------------+-------------+
| PAD_048 - PAD_063 | PE0 - PE15  |
+-------------------+-------------+
| PAD_064 - PAD_079 | PF0 - PF15  |
+-------------------+-------------+
| PAD_080 - PAD_091 | PG0 - PG11  |
+-------------------+-------------+
| PAD_092 - PAD_095 | PH12 - PH15 |
+-------------------+-------------+
| PAD_096 - PAD_111 | PI0 - PI15  |
+-------------------+-------------+
| PAD_112 - PAD_127 | PJ0 - PJ15  |
+-------------------+-------------+
| PAD_128 - PAD_143 | PK0 - PK15  |
+-------------------+-------------+
| PAD_144 - PAD_145 | PL0 - PL1   |
+-------------------+-------------+
| PAD_146 - PAD_159 | PM2 - PM15  |
+-------------------+-------------+
| PAD_160 - PAD_169 | PN0 - PN9   |
+-------------------+-------------+
| PAD_170 - PAD_173 | PO10 - PO13 |
+-------------------+-------------+

This board does not include user LED's or switches, which are needed for some
of the samples such as :ref:`blinky-sample` or :ref:`button-sample`.
Follow the steps described in the sample description to enable support for this
board.

System Clock
============

The Cortex-R52 cores are configured to run at 800 MHz.

Serial Port
===========

The SoC has 12 LINFlexD instances that can be used in UART mode. Instance 0
(defined as ``uart0`` in devicetree) is configured for the console and the
remaining are disabled and not configured.

Watchdog
========

The watchdog driver only supports triggering an interrupt upon timer expiration.
Zephyr is currently running from SRAM on this board, thus system reset is not
supported.

Ethernet
========

NETC driver supports to manage the Physical Station Interface (PSI0) and/or a
single Virtual SI (VSI). The rest of the VSI's shall be assigned to different
cores of the system. Refer to :ref:`nxp_s32_netc-samples` to learn how to
configure the Ethernet network controller.

Programming and Debugging
*************************

Applications for the ``s32z270dc2_rtu0_r52`` and ``s32z270dc2_rtu1_r52`` boards
can be built in the usual way as documented in :ref:`build_an_application`.

Currently is only possible to load and execute a Zephyr application binary on
this board from the internal SRAM, using `Lauterbach TRACE32`_ development
tools and debuggers.

.. note::
   Currently, the start-up scripts executed with ``west flash`` and
   ``west debug`` commands perform the same steps to initialize the SoC and
   load the application to SRAM. The difference is that ``west flash`` hide the
   Lauterbach TRACE32 interface, executes the application and exits.

Install Lauterbach TRACE32 Software
===================================

Follow the steps described in :ref:`lauterbach-trace32-debug-host-tools` to
install and set-up Lauterbach TRACE32 software.

Set-up the Board
================

Connect the Lauterbach TRACE32 debugger to the board's JTAG connector (``J134``)
and to the host computer.

For visualizing the serial output, connect the board's USB/UART port (``J119``) to
the host computer and run your favorite terminal program to listen for output.
For example, using the cross-platform `pySerial miniterm`_ terminal:

.. code-block:: console

   python -m serial.tools.miniterm <port> 115200

Replace ``<port>`` with the port where the board can be found. For example,
under Linux, ``/dev/ttyUSB0``.

Flashing
========

For example, you can build and run the :ref:`hello_world` sample for the board
``s32z270dc2_rtu0_r52`` with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32z270dc2_rtu0_r52
   :goals: build flash

You should see the following message in the terminal:

.. code-block:: console

   Hello World! s32z270dc2_rtu0_r52

Debugging
=========

To enable debugging using Lauterbach TRACE32 software, run instead:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32z270dc2_rtu0_r52
   :goals: build debug

Step through the application in your debugger, and you should see the following
message in the terminal:

.. code-block:: console

   Hello World! s32z270dc2_rtu0_r52

RTU and Core Configuration
==========================

This Zephyr port can only run single core in any of the Cortex-R52 cores,
either in lock-step or split-lock mode. By default, Zephyr runs on the first
core of the RTU chosen and in lock-step mode (which is the reset
configuration).

To build for split-lock mode, the :kconfig:option:`CONFIG_DCLS` must be
disabled from your application Kconfig file.

Additionally, to run in a different core or with a different core
configuration than the default, extra parameters must be provided to the runner
as follows:

.. code-block:: console

   west <command> --startup-args elfFile=<elf_path> rtu=<rtu_id> \
      core=<core_id> lockstep=<yes/no>

Where:

- ``<command>`` is ``flash`` or ``debug``
- ``<elf_path>`` is the path to the Zephyr application ELF in the output
  directory
- ``<rtu_id>`` is the zero-based RTU index (0 for ``s32z270dc2_rtu0_r52``
  and 1 for ``s32z270dc2_rtu1_r52``)
- ``<core_id>`` is the zero-based core index relative to the RTU on which to
  run the Zephyr application (0, 1, 2 or 3)
- ``<yes/no>`` can be ``yes`` to run in lock-step, or ``no`` to run in
  split-lock.

For example, to build the :ref:`hello_world` sample for the board
``s32z270dc2_rtu0_r52`` with split-lock core configuration:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32z270dc2_rtu0_r52
   :goals: build
   :gen-args: -DCONFIG_DCLS=n

To execute this sample in the second core of RTU0 in split-lock mode:

.. code-block:: console

   west flash --startup-args elfFile=build/zephyr/zephyr.elf \
      rtu=0 core=1 lockstep=no

References
**********

.. target-notes::

.. _NXP S32Z2 Real-Time Processors website:
   https://www.nxp.com/products/processors-and-microcontrollers/s32-automotive-platform/s32z-and-s32e-real-time-processors/s32z2-safe-and-secure-high-performance-real-time-processors:S32Z2

.. _Lauterbach TRACE32:
   https://www.lauterbach.com

.. _pySerial miniterm:
   https://pyserial.readthedocs.io/en/latest/tools.html#module-serial.tools.miniterm
