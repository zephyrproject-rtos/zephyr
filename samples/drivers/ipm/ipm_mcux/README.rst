.. zephyr:code-sample:: ipm-mcux
   :name: IPM on NXP LPC
   :relevant-api: ipm_interface

   Implement inter-processor mailbox (IPM) on NXP LPC family.

Overview
********

Some NXP microcontrollers from LPC family are dual-core, this
sample application uses a mailbox to send messages from one
processor core to the other.

This sample applies to the following boards:
 -  :ref:`lpcxpresso54114`, two core processors (Cortex-M4F and Cortex-M0+)
 -  :ref:`lpcxpresso55s69`, two core processors (dual Cortex-M33)

Requirements
************

- :ref:`lpcxpresso54114` board
- :ref:`lpcxpresso55s69` board

Building the application for lpcxpresso54114_m4
***********************************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/ipm/ipm_mcux
   :board: lpcxpresso54114_m4
   :goals: debug


Building the application for lpcxpresso55s69_cpu0
*************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/ipm/ipm_mcux
   :board: lpcxpresso55s69_cpu0
   :goals: debug

Running
*******
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
