.. _IPC_Service_sample:

IPC Service
###########

Overview
********

IPC Service is an abstraction layer.
It needs the corresponding backend registered for proper operation.
You can use any IPC mechanism as a backend.
In this sample, it is a multiple instance of RPMsg.
This sample demonstrates how to integrate IPC Service with Zephyr.

Building the application for nrf5340dk_nrf5340_cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipc_service
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: debug

Open a serial terminal (for example Minicom or PuTTY) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

When you reset the development kit, the following messages (one for master and one for remote) will appear on the corresponding serial ports:

.. code-block:: console

   *** Booting Zephyr OS build v2.6.0-rc3-5-g026dfb6f1b71  ***

   IPC Service [master 1] demo started

   IPC Service [master 2] demo started
   Master [1] received a message: 1
   Master [2] received a message: 1
   Master [1] received a message: 3
   Master [2] received a message: 3

   ...
   Master [1] received a message: 99
   IPC Service [master 1] demo ended.
   Master [2] received a message: 99
   IPC Service [master 2] demo ended.


.. code-block:: console

   *** Booting Zephyr OS build v2.6.0-rc3-5-g026dfb6f1b71  ***

   IPC Service [remote 1] demo started

   IPC Service [remote 2] demo started
   Remote [1] received a message: 0
   Remote [2] received a message: 0
   Remote [1] received a message: 2
   Remote [2] received a message: 2
   Remote [1] received a message: 4
   Remote [2] received a message: 4
   ...
   Remote [1] received a message: 98
   IPC Service [remote 1] demo ended.
   Remote [2] received a message: 98
   IPC Service [remote 2] demo ended.
