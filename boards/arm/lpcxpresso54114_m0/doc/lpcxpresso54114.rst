.. _lpcxpresso54114_m0:

NXP LPCXPRESSO54114
#####################

Overview
********

See :ref:`lpcxpresso54114` for a general overview of the LPCXpresso54114 board.
The Cortex-M0+ is a secondary core on the board's SoC.

Programming and Debugging
*************************

The LPCXpresso54114 includes the LPC-Link2 serial and debug adapter built into
the board to provide debugging, flash programming, and serial communication
over USB. LPC-Link2 can be configured with Segger J-Link or CMSIS-DAP firmware
variants to support corresponding debug tools. Currently only the Segger J-Link
tools are supported for this board in Zephyr, therefore you should use the
Segger J-Link firmware variant.

Before you start using Zephyr on the LPCXpresso54114, download and run
`LPCScrypt`_ to update the LPC-Link2 firmware to the latest version, currently
``Firmware_JLink_LPC-Link2_20160923.bin``. Serial communication problems, such
as dropping characters, have been observed with older versions of the firmware.

The code for the secondary core is linked into primary core binary file.
Startup code copies secondary core's code into appropriate location in RAM
and starts its execution.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:ref:`ipm_mcux` application.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_mcux
   :board: lpcxpresso54114
   :goals: debug

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.10.99 - BUILD: Feb  7 2018 20:32:27 *****
   Hello World from MASTER! arm
   Received: 1
   Received: 2
   Received: 3
   Received: 4
   Received: 5
   Received: 6
   Received: 7
   Received: 8
   Received: 9
   Received: 10
   Received: 11
   Received: 12
   Received: 13
   Received: 14
   Received: 15
   Received: 16
   Received: 17
   Received: 18
   Received: 19
   Received: 20
   Received: 21
   Received: 22
   Received: 23
   Received: 24
   Received: 25
   Received: 26
   Received: 27
   Received: 28
   Received: 29
   Received: 30
   Received: 31
   Received: 32
   Received: 33
   Received: 34
   Received: 35
   Received: 36
   Received: 37
   Received: 38
   Received: 39
   Received: 40
   Received: 41
   Received: 42
   Received: 43
   Received: 44
   Received: 45
   Received: 46
   Received: 47
   Received: 48
   Received: 49
   Received: 50
   Received: 51
   Received: 52
   Received: 53
   Received: 54
   Received: 55
   Received: 56
   Received: 57
   Received: 58
   Received: 59
   Received: 60
   Received: 61
   Received: 62
   Received: 63
   Received: 64
   Received: 65
   Received: 66
   Received: 67
   Received: 68
   Received: 69
   Received: 70
   Received: 71
   Received: 72
   Received: 73
   Received: 74
   Received: 75
   Received: 76
   Received: 77
   Received: 78
   Received: 79
   Received: 80
   Received: 81
   Received: 82
   Received: 83
   Received: 84
   Received: 85
   Received: 86
   Received: 87
   Received: 88
   Received: 89
   Received: 90
   Received: 91
   Received: 92
   Received: 93
   Received: 94
   Received: 95
   Received: 96
   Received: 97
   Received: 98
   Received: 99
