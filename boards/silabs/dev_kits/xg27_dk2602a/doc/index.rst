.. zephyr:board:: xg27_dk2602a

Silicon Labs xG27-DK2602A is a Dev Kit using the EFR32BG27 SoC. The kit
consists of the EFR32BG27 +8 dBm Dev Kit Board (BRD2602A).

Hardware
********

- EFR32BG27 Blue Gecko Wireless SoC with up to 76.8 MHz operating frequency
- ARM® Cortex® M33 core with 64 kB RAM and 768 kB Flash
- Macronix ultra low power 8-Mbit SPI flash (MX25R8035F)
- 2.4 GHz ceramic antenna for wireless transmission
- Silicon Labs Si7021 relative humidity and temperature sensor
- Vishay VEML6035 low power, high sensitivity ambient light Sensor
- Silicon Labs Si7210 hall effect sensor
- TDK InvenSense ICM-20689 6-axis inertial sensor
- Pair of PDM microphones
- One LED and one push button
- Power enable signals and isolation switches for ultra low power operation
- On-board SEGGER J-Link debugger for easy programming and debugging, which
  includes a USB virtual COM port and Packet Trace Interface (PTI)
- Mini Simplicity connector for access to energy profiling and advanced wireless
  network debugging
- Breakout pads for GPIO access and connection to external hardware
- Reset button
- CR2032 coin cell holder and external battery connector

For more information, refer to these documents:

- `xG27 Dev Kit User's Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Flashing
========

The xG27 Dev Kit includes an embedded `J-Link`_ adapter built around
EFM32GG12 microcontroller (not user-programmable).
The adapter provides:

- SWD interface to EFR32BG27 for flashing and debugging.
- SWO trace interface to EFR32BG27 for tracing.
- UART interface to EFR32BG27 for console access.
- A USB connection to the host computer, which exposes CDC-ACM Serial Port
  endpoints for access to the console UART interface and proprietary J-Link
  endpoints for access to the SWD and SWO interfaces.

UART functionality of the adapter is accessible via standard CDC-ACM USB driver
present in most desktop operating systems and any standard serial port terminal
program e.g. `picocom`_.

SWD and SWO functionality is accessible via `Simplicity Commander`_.

The simplest way to flash the board is by using West, which runs Simplicity
Commander in unattended mode and passes all the necessary arguments to it.

- If Simplicity Commander is installed in the system and the directory in
  which ``commander`` executable is located is present in the :envvar:`PATH` environment
  variable:

  .. code-block:: console

   west flash

- Otherwise, one should specify full path to the ``commander`` executable:

  .. code-block:: console

   west flash --commander <path_to_commander_directory>/commander

- In case several J-Link adapters are connected, you must specify serial number
  of the adapter which should be used for flashing:

  .. code-block:: console

   west flash --dev-id <J-Link serial number>

Programming and Debugging
=========================

.. zephyr:board-supported-runners::

The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xg27_dk2602a
   :goals: build

Connect your device to your host computer using the USB port and you
should see a USB connection. Use ``west``'s flash command

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! xg27_dk2602a

.. _picocom:
   https://github.com/npat-efault/picocom

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html

.. _Simplicity Commander:
   https://www.silabs.com/developers/mcu-programming-options

.. _xG27 Dev Kit User's Guide:
   https://www.silabs.com/documents/public/user-guides/ug554-brd2602a-user-guide.pdf
