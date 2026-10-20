.. zephyr:board:: frdm_mcxa577

Overview
********

FRDM-MCXA577 is a compact and scalable development board for rapid prototyping of MCX A577
MCUs. They offer industry standard headers for easy access to the MCUs input/output (I/O),
integrated open-standard serial interfaces, external flash memory and an onboard MCU-Link
debugger.

Hardware
********

- MCX-A577 Arm Cortex-M33 microcontroller running at 12MHz
- 2048KB dual-bank on chip Flash
- 640 KB RAM
- 2x FlexCAN with FD, 1x RGB LED, 3x SW buttons
- Ethernet: on-SoC 10BASE-T1S PHY, or 10/100 Mbit external PHY over MII
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, SmartDMA/Camera Header, mikroBUS

For more information about the MCX-A577 SoC and FRDM-MCXA577 board, see:

- `MCX-A577 SoC Website`_
- `FRDM-MCXA577 Website`_
- `FRDM-MCXA577 User Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Shields for Supported Features
==============================

Some features in the table above are tested with Zephyr shields.  These shields
are tested on this board:

- :ref:`lcd_par_s035` - supports the Display interface.  This board uses the
  MIPI_DBI interface of the shield, connected to the FlexIO on-chip peripheral.

Display Support
***************

The frdm_mcxa577 board supports the following in-tree display module(s).

NXP LCD_PAR_S035
================

The :ref:`lcd_par_s035` connects to the board's LCD socket, and is driven over
the MIPI_DBI 8080 parallel interface using the FlexIO on-chip peripheral. The
display sample can be built for this module like so:

.. zephyr-app-commands::
   :board: frdm_mcxa577
   :shield: lcd_par_s035_8080
   :zephyr-app: samples/drivers/display
   :goals: build
   :compact:

Connections and IOs
===================

The MCX-A577 SoC has 5 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| PIO0_2     | UART            | UART RX                    |
+------------+-----------------+----------------------------+
| PIO0_3     | UART            | UART TX                    |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-A577 SoC is configured to use FRO LF running at 12MHz as a source for
the system clock.

Serial Port
===========

The FRDM-MCXA577 SoC has 6 LPUART  interfaces for serial communication.
LPUART0 is configured as UART for the console.

Ethernet
===========

Support for two Ethernet options: the on-SoC 10BASE-T1S PHY with its on-board PMD transceiver,
and a 10/100 Mbit external PHY over MII.

10BASE-T1S (default)
--------------------

The default target ``frdm_mcxa577/mcxa577`` uses the on-SoC 10BASE-T1S PHY:

.. zephyr-app-commands::
   :zephyr-app: samples/net/zperf
   :board: frdm_mcxa577
   :goals: build

10BASE-T1S Board-to-Board Setup
-------------------------------

Two boards can be connected directly with a twisted pair between their 10BASE-T1S connectors.
Exactly one node on the segment must be the PLCA coordinator (node ID 0), and the boards need
different MAC addresses. The board devicetree uses PLCA node ID 1 and a random MAC address,
which is identical on every board when ``CONFIG_TEST_RANDOM_GENERATOR`` is enabled, as in
:zephyr:code-sample:`zperf`.

Build the first board with the default configuration (``192.0.2.1``). For the second board,
save the following overlay as ``samples/net/zperf/t1s_node0.overlay``:

.. code-block:: dts

   &enet {
       /delete-property/ zephyr,random-mac-address;
       local-mac-address = [00 04 9f 00 00 01];
   };

   &t1s {
       plca-enable;
       plca-node-id = <0>;
       plca-node-count = <2>;
       plca-burst-count = <0>;
       plca-burst-timer = <0x80>;
       plca-to-timer = <0x20>;
       status = "okay";
   };

and the following as ``samples/net/zperf/t1s_node0.conf`` to swap the IP addresses:

.. code-block:: cfg

   CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8::2"
   CONFIG_NET_CONFIG_PEER_IPV6_ADDR="2001:db8::1"
   CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.0.2.2"
   CONFIG_NET_CONFIG_PEER_IPV4_ADDR="192.0.2.1"

Then build the second board with both files:

.. zephyr-app-commands::
   :zephyr-app: samples/net/zperf
   :board: frdm_mcxa577
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE=t1s_node0.overlay -DEXTRA_CONF_FILE=t1s_node0.conf
   :goals: build

After flashing, ``net iface`` must show a different ``Link addr`` on each board, and
``net ping 192.0.2.2`` from the first board must succeed.

External PHY over MII
---------------------

The ``frdm_mcxa577/mcxa577/mii`` variant uses the external 10/100 Mbit PHY:

.. zephyr-app-commands::
   :zephyr-app: samples/net/zperf
   :board: frdm_mcxa577/mcxa577/mii
   :goals: build

MII needs P1_8/P1_9 for ENET0_TXD2/TXD3, which are also the LPUART1 MCU-Link VCOM pins.
The variant therefore disables LPUART1 and moves the console/shell to LPUART2 (Arduino D0/D1 header,
P2_11/P2_10); connect a USB-to-serial adapter there to get console output.

The MII signals reach the external PHY only after the following solder-jumper
rework (this rework is not applicable to 10BASE-T1S mode):

- Short 2-3 (and disconnect 1-2) of SJ13 (TXD2), SJ14 (TXD3), SJ16 (RXD2),
  SJ17 (RXD3), SJ18 (COL), SJ19 (CRS).
- Disconnect 1-2 and 2-3 of SJ15 (RX_CLK) and SJ44 (RXER).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.

Using LinkServer
----------------

Linkserver is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``ISP mode`` to program the firmware,
short jumper JP4.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``ISP mode``
by shortening the jumper JP4.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J11) of the board. Additionally, the jumper JP6 must
be shorted.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J13, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa577
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa577/mcxa577

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa577/mcxa577
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa577/mcxa577

10BASE-T1S Throughput Tuning
============================

10BASE-T1S is a shared bus arbitrated by PLCA, so the usable UDP throughput on
the default ``frdm_mcxa577/mcxa577`` target is lower than the 10 Mbps line rate.
Near that limit the default network buffer sizing can be exhausted faster than
the driver drains it, and packets are dropped with ``-EBUSY``.

Increasing the net_buf fragment size, the packet and buffer pools, and the
network TX/RX thread stacks avoids the drops:

.. code-block:: cfg

   CONFIG_NET_BUF_DATA_SIZE=512
   CONFIG_NET_PKT_RX_COUNT=48
   CONFIG_NET_PKT_TX_COUNT=48
   CONFIG_NET_BUF_RX_COUNT=96
   CONFIG_NET_BUF_TX_COUNT=96
   CONFIG_NET_TX_STACK_SIZE=2048
   CONFIG_NET_RX_STACK_SIZE=2048

These raise RAM usage, so they are applied per application rather than enabled
by default. For :zephyr:code-sample:`zperf` they are set in
:zephyr_file:`samples/net/zperf/boards/frdm_mcxa577_mcxa577.conf`.

If more headroom is needed, the ENET QoS driver's descriptor rings and RX thread
can be enlarged as well:

.. code-block:: cfg

   CONFIG_ETH_NXP_ENET_QOS_TX_BUFFER_DESCRIPTORS=32
   CONFIG_ETH_NXP_ENET_QOS_RX_BUFFER_DESCRIPTORS=32
   CONFIG_ETH_NXP_ENET_QOS_RX_THREAD_STACK_SIZE=2048
   CONFIG_ETH_NXP_ENET_QOS_RX_THREAD_PRIORITY=2

Raising the priority of the application thread generating the traffic reduces
inter-packet jitter. For :zephyr:code-sample:`zperf`:

.. code-block:: cfg

   CONFIG_ZPERF_WORK_Q_THREAD_PRIORITY=7
   CONFIG_ZPERF_WORK_Q_STACK_SIZE=4096


Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

.. _MCX-A577 SoC Website:
   https://www.nxp.com/products/MCX-A57X

.. _FRDM-MCXA577 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXA577

.. _FRDM-MCXA577 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa577:GS-FRDM-MCXA577
