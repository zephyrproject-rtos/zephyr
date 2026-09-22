.. zephyr:board:: ubx_evkninab5

Overview
********

The EVK-NINA-B5 is a compact and scalable development board for rapid
prototyping of the NINA-B5 wireless module. It offers easy evaluation of the
NINA-B5's multiprotocol wireless support for Bluetooth LE, Zigbee, Thread and
Matter. The board includes an on-board J-link-Link debugger and industry
standard headers for easy access to the MCU’s I/Os.

The NINA-B5 module is built on the MCX W71x 96 MHz Arm® Cortex®-M33 from NXP.

Hardware
********

- MCXW71 Arm Cortex-M33 microcontroller running up to 96 MHz
- 1MB on-chip Flash memory unit
- 128 KB TCM RAM
- On-board MCU-Link debugger with CMSIS-DAP

For more information about the NINA-B5 module and EVK-NINA-B5 board, see:

- `NINA-B5 product page`_
- `EVK-NINA-B5 product page`_

Supported Features
==================

.. zephyr:board-supported-hw::

Fetch Binary Blobs
******************

To support Bluetooth, ubx_evkninab5 requires fetching binary blobs, which can be
achieved by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

Note: The EVK-NINA-B5 is preflashed with NBU files.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.


Using J-Link
------------

The onboard debug circuit is a Segger J-Link debugger

The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J12) of the board.

For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to the USB port, and use the serial terminal
of your choice (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Application Building
====================

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: ubx_evkninab5
   :goals: build
   :gen-args:


Application Flashing
====================

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ubx_evkninab5
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-xxx-xxxx ***
   Hello World! ubx_evkninab5

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ubx_evkninab5
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-xxx-xxxx ***
   Hello World! ubx_evkninab5

NBU Flashing
============

BLE functionality requires to fetch binary blobs, so make sure to follow
the ``Fetch Binary Blobs`` section first.

Two images must be written to the board: one for the host (CM33) and one for the NBU (CM3).

- To flash the application (CM33) refer to the ``Application Flashing`` section above.

- To flash the ``NBU Flashing``, follow the instructions below in the NINA-B5 system
  integration manual available on the `NINA-B5 product page`_.

The NBU files can be found in : ``<zephyr workspace>/modules/hal/nxp/zephyr/blobs/mcxw71/`` folder.

For more details:

.. _MCXW71 In-System Programming Utility:
   https://docs.nxp.com/bundle/AN14427/page/topics/introduction.html

.. _blhost Website:
   https://www.nxp.com/search?keyword=blhost&start=0

References
**********

.. target-notes::

.. _NINA-B5 product page:
   https://www.u-blox.com/en/product/nina-b50-series-open-cpu

.. _EVK-NINA-B5 product page:
   https://www.u-blox.com/en/product/evk-nina-b5?legacy=Current#Documentation-&-resources
