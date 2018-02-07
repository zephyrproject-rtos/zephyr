.. _rpmsg-lite-sample:

Sample RPMsg-Lite application
#############################

Overview
********

The :ref:`lpcxpresso54114` board has two core processors (Cortex-M4F
and Cortex-M0+). This sample application uses RPMsg-Lite library to send
messages from one processor core to the other.

Requirements
************

- :ref:`lpcxpresso54114` board

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/rpmsg_lite
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

   ***** Booting Zephyr OS v1.14.0-rc1-1300-g0ddf34590df3 *****
   ===== app started ========

   RPMsg demo starts
   Primary core received a msg
   Message: Size=4, DATA = 1
   Primary core received a msg
   Message: Size=4, DATA = 3
   ...
   Primary core received a msg
   Message: Size=4, DATA = 99
   Primary core received a msg
   Message: Size=4, DATA = 101

   RPMsg demo ends
