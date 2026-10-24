.. zephyr:board:: kit_a3g_tc4d7_lite

Overview
********

The AURIX TC4D7 lite kit (KIT_A3G_TC4D7_LITE) carries an Infineon AURIX
TC4D7XP microcontroller (TriCore TC1.8P, six cores at up to 500 MHz, 20 MB
program flash), a 25 MHz crystal and an on-board debugger with a USB virtual
COM port.

Zephyr runs on CPU0.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Clocks
======

The clock controller node ramps the system clock through the ramp oscillator
to the system PLL at 500 MHz (fSRI and fSTM), with fSPB, fCPB and fFSI at
100 MHz, and configures the peripheral PLL for a 200 MHz ASCLIN clock.

Serial Port
===========

The console is ASCLIN0 on P14.0 (TX) and P14.1 (RX), connected to the virtual
COM port of the on-board debugger, at 115200 baud.

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
   :board: kit_a3g_tc4d7_lite/tc4d7xp/cpu0
   :goals: build flash
   :flash-args: --openocd /opt/tooling/tricore-openocd/bin/openocd --openocd-search /opt/tooling/tricore-openocd/share/openocd/scripts

References
**********

.. target-notes::

.. _AURIX TC4D7 lite kit:
   https://www.infineon.com/cms/en/product/evaluation-boards/kit_a3g_tc4d7_lite/

.. _openocd-aurix:
   https://gitlab.com/linumiz/infineon/release/openocd-aurix

.. _DAS:
   https://softwaretools.infineon.com/

.. _libftd2xx:
   https://ftdichip.com/drivers/d2xx-drivers/
