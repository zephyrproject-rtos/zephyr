.. zephyr:board:: frdm_mcxw70

Overview
********

The FRDM-MCXW70 is a compact and scalable development board for rapid
prototyping of the MCX W70 wireless MCU. It offers easy evaluation of the MCX
W70's multiprotocol wireless support for Bluetooth LE, Zigbee, Thread and
Matter. The board includes an on-board MCU-Link debugger, industry standard
headers for easy access to the MCU’s I/Os, an accelerometer, a light sensor and
external SPI flash memory.

The MCX W70x family features a 96 MHz Arm® Cortex®-M33 core coupled with a
multiprotocol radio subsystem supporting Matter, Thread, Zigbee and Bluetooth
LE. The independent radio subsystem, with a dedicated core and memory, offloads
the main CPU, preserving it for the primary application and allowing firmware
updates to support future wireless standards.

Hardware
********

- MCXW70 Arm Cortex-M33 microcontroller running up to 96 MHz
- 1.5 MB on-chip Flash memory unit
- 256 KB TCM RAM
- On-board MCU-Link debugger with CMSIS-DAP

For more information about the MCXW70 SoC and FRDM-MCXW70 board, see:

- `MCXW70 SoC Website`_
- `FRDM-MCXW70 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Fetch Binary Blobs
******************

To support Bluetooth, frdm_mcxw70 requires fetching binary blobs, which can be
achieved by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

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
firmware was changed. To put the board in ``DFU mode`` to program the firmware,
short jumper JP8.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper JP8.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J26) of the board.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J28, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Application Building
====================

Openthread applications
-----------------------

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :board: frdm_mcxw70
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=overlay-ot.conf

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: frdm_mcxw70
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=overlay-ot.conf

Application Flashing
====================

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxw70
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-xxx-xxxx ***
   Hello World! frdm_mcxw70/mcxw706c

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxw70/mcxw70ac
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-xxx-xxxx ***
   Hello World! frdm_mcxw70/mcxw70ac

NBU Flashing
============

BLE functionality requires to fetch binary blobs, so make sure to follow
the ``Fetch Binary Blobs`` section first.

Two images must be written to the board: one for the host (CM33) and one for the NBU (CM3).

- To flash the application (CM33) refer to the ``Application Flashing`` section above.

- To flash the ``NBU Flashing``, follow the instructions below:

   * Install ``blhost`` from NXP's website. This is the tool that will allow you to flash the NBU.
   * Enter ISP mode. To boot the MCU in ISP mode, follow these steps:
      - Disconnect the ``FRDM-MCXW70`` board from all power sources.
      - Keep the ``SW2`` (ISP) button on the board pressed, while connecting the board to the host computer USB port.
      - Release the ``SW2`` (ISP) button. The MCXW70 MCU boots in ISP mode.
      - Reconnect any external power supply, if needed.
   * Use the following command to flash NBU file:

.. tabs::

   .. group-tab:: BLE NBU - Windows

      .. code-block:: console
         :caption: Flash BLE only NBU on Windows

         blhost.exe -p COMxx -- receive-sb-file mcxw70_nbu_ble.sb3

   .. group-tab:: BLE NBU - Linux

      .. code-block:: console
         :caption: Flash BLE only NBU on Linux

         ./blhost -p /dev/ttyxx -- receive-sb-file mcxw70_nbu_ble.sb3

   .. group-tab:: DYN NBU - Windows

      .. code-block:: console
         :caption: Flash Dynamic NBU (BLE + 15.4) on Windows

         blhost.exe -p COMxx -- receive-sb-file mcxw70_nbu_ble_15_4_dyn.sb3

   .. group-tab:: DYN NBU - Linux

      .. code-block:: console
         :caption: Flash Dynamic NBU (BLE + 15.4) on Linux

         ./blhost -p /dev/ttyxx -- receive-sb-file mcxw70_nbu_ble_15_4_dyn.sb3

Please consider changing ``COMxx`` on Windows or ``ttyxx`` on Linux to the serial port used by your board.

The NBU files can be found in : ``<zephyr workspace>/modules/hal/nxp/zephyr/blobs/mcxw70/`` folder.

For more details:

.. _blhost User's Guide:
   https://www.nxp.com/docs/en/user-guide/MCUBLHOSTUG.pdf

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _MCXW70 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-w-series-microcontrollers/mcx-w70x-secure-and-ultra-low-power-mcus-for-matter-thread-zigbee-and-bluetooth-le:MCX-W70X

.. _FRDM-MCXW70 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/frdm-development-board-for-mcx-w70x-wireless-mcus:FRDM-MCXW70
