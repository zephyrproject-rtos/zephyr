.. zephyr:board:: kit_a2g_tc375_lite

Overview
********

The AURIX TC375 lite kit (KIT_A2G_TC375_LITE) carries an Infineon AURIX
TC375TP microcontroller (TriCore TC1.6.2P, three cores at up to 300 MHz,
6 MB program flash), a 20 MHz crystal and an on-board debugger with a USB
virtual COM port.

Zephyr runs on CPU0.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Clocks
======

The clock controller node configures the system PLL to 300 MHz (fSRI) with
fSPB, fFSI and fSTM at 100 MHz, and the peripheral PLL for a 200 MHz ASCLIN
clock.

Serial Port
===========

The console is ASCLIN0 on P14.0 (TX) and P14.1 (RX), connected to the virtual
COM port of the on-board debugger, at 115200 baud.

CAN
===

MCMCAN0 node 0 is on P20.8 (TX) and P20.7 (RX), connected to the on-board
TLE9251V transceiver, with the standby input on P20.6. It runs at 500 kbit/s
from an 80 MHz CAN clock.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing uses OpenOCD with AURIX support, which reaches the on-board
debugger through the Infineon TAS server. Neither the upstream OpenOCD nor
the one in the Zephyr SDK supports AURIX yet; build it from
`openocd-aurix`_:

.. code-block:: console

   git clone https://gitlab.com/linumiz/infineon/release/openocd-aurix.git
   cd openocd-aurix
   ./bootstrap
   mkdir build && cd build
   ../configure --prefix=/opt/tooling/tricore-openocd \
                --enable-tas-client --enable-ftdi
   make -j$(nproc) && sudo make install

OpenOCD talks to the ``tas_server`` daemon of the Infineon `DAS`_ package,
which needs FTDI's `libftd2xx`_ in its directory. Start it once per boot:

.. code-block:: console

   cd <path-to-tas_server>
   LD_LIBRARY_PATH=. ./tas_server &

Build and flash the :zephyr:code-sample:`hello_world` sample with that
OpenOCD:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_a2g_tc375_lite/tc375tp/cpu0
   :goals: build flash
   :flash-args: --openocd /opt/tooling/tricore-openocd/bin/openocd --openocd-search /opt/tooling/tricore-openocd/share/openocd/scripts

References
**********

.. target-notes::

.. _AURIX TC375 lite kit:
   https://www.infineon.com/cms/en/product/evaluation-boards/kit_a2g_tc375_lite/

.. _openocd-aurix:
   https://gitlab.com/linumiz/infineon/release/openocd-aurix

.. _DAS:
   https://softwaretools.infineon.com/

.. _libftd2xx:
   https://ftdichip.com/drivers/d2xx-drivers/
