.. zephyr:board:: frdm_mcxw72

Overview
********

The FRDM-MCXW72

The MCX W72x family features a 96 MHz Arm® Cortex®-M33 core coupled with a
multiprotocol radio subsystem supporting Matter, Thread, Zigbee and
Bluetooth LE. The independent radio subsystem, with a dedicated core and
memory, offloads the main CPU, preserving it for the primary application and
allowing firmware updates to support future wireless standards.

Hardware
********

- MCXW72 Arm Cortex-M33 microcontroller running up to 96 MHz
- 2MB on-chip Flash memory unit
- 256 KB TCM RAM
- On-board MCU-Link debugger with CMSIS-DAP

For more information about the MCXW72 SoC and FRDM-MCXW72 board, see:

- `MCXW72 SoC Website`_
- `FRDM-MCXW72 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Fetch Binary Blobs
******************

To support Bluetooth, frdm_mcxw72 requires fetching binary blobs, which can be
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
short jumper JP5.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper JP5.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J12) of the board.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J14, and use the serial terminal of your choice
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
   :board: frdm_mcxw72/mcxw727c/cpu0
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=overlay-ot.conf

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: frdm_mcxw72/mcxw727c/cpu0
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=overlay-ot.conf

Application Flashing
====================

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxw72/mcxw727c/cpu0
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-xxx-xxxx ***
   Hello World! frdm_mcxw72/mcxw727c/cpu0

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxw72/mcxw727c/cpu0
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-xxx-xxxx ***
   Hello World! frdm_mcxw72/mcxw727c/cpu0

NBU Flashing
============

BLE functionality requires to fetch binary blobs, so make sure to follow
the ``Fetch Binary Blobs`` section first.

Two images must be written to the board: one for the host (CM33) and one for the NBU (CM3).

- To flash the application (CM33) refer to the ``Application Flashing`` section above.

- To flash the ``NBU Flashing``, follow the instructions below:

   * Install ``blhost`` from NXP's website. This is the tool that will allow you to flash the NBU.
   * Enter ISP mode. To boot the MCU in ISP mode, follow these steps:
      - Disconnect the ``FRDM-MCXW72`` board from all power sources.
      - Keep the ``SW4`` and ``SW1`` buttons on the board pressed, while connecting the board to the host computer USB port.
      - Release the ``SW4`` and ``SW1`` buttons. The MCXW72 MCU boots in ISP mode.
      - Reconnect any external power supply, if needed.
   * Use the following command to flash NBU file:

.. tabs::

   .. group-tab:: DYN NBU - Windows

      .. code-block:: console
         :caption: Flash Dynamic NBU (BLE + 15.4) on Windows

         blhost.exe -p COMxx flash-erase-all 0
         blhost.exe -p COMxx flash-erase-all 2
         blhost.exe -p COMxx write-memory 0x48800000 <nbu-firmware.bin>

   .. group-tab:: DYN NBU - Linux

      .. code-block:: console
         :caption: Flash Dynamic NBU (BLE + 15.4) on Linux

         ./blhost -p /dev/ttyxx flash-erase-all 0
         /blhost -p /dev/ttyxx flash-erase-all 2
         /blhost -p /dev/ttyxx write-memory 0x48800000 <nbu-firmware.bin>

Please consider changing ``COMxx`` on Windows or ``ttyxx`` on Linux to the serial port used by your board.

The NBU files can be found in : ``<zephyr workspace>/modules/hal/nxp/zephyr/blobs/mcxw72/`` folder.

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _MCXW72 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-w-series-microcontrollers/mcx-w72x-secure-and-ultra-low-power-mcus-for-matter-thread-zigbee-and-bluetooth-le:MCX-W72X

.. _FRDM-MCXW72 Website:
