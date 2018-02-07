.. _ipm-mcux-sample:

Sample mailbox application
##########################

Overview
********

The :ref:`lpcxpresso54114` board has two core processors (Cortex-M4F
and Cortex-M0+). This sample application uses a mailbox to send messages
from one processor core to the other.

Requirements
************

- :ref:`lpcxpresso54114` board

Building and Running
********************

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
