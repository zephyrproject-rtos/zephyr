.. _lpcxpresso54114_m0:

NXP LPCXPRESSO54114 (M0 Core)
#############################

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

The code for the secondary core is linked into the primary core binary file.
Startup code copies the secondary core's code into an appropriate location
in RAM and starts its execution.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:ref:`ipm-mcux-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_mcux
   :board: lpcxpresso54114_m4
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

   ***** Booting Zephyr OS v1.11.0-764-g4e3007a *****
   Hello World from MASTER! arm
   Received: 1
   ...
   Received: 99

.. _LPCScrypt: https://www.nxp.com/support/developer-resources/software-development-tools/lpc-developer-resources-/lpc-microcontroller-utilities/lpcscrypt-v1.8.2:LPCSCRYPT
