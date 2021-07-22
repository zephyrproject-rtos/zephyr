.. _Multiple_instance_RPMsg_sample:

Multiple instance of RPMsg
##########################

Overview
********

Multiple instance of RPMsg is an abstraction created over OpenAMP.
It simplifies the initialization and endpoint creation process.
This sample demonstrates how to use multi-instance RPMsg in Zephyr.

Building the application for nrf5340dk_nrf5340_cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/rpmsg_multi_instance
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: debug

Open a serial terminal (for example Minicom or PuTTY) and connect the board with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

When you reset the development kit, the following messages (one for master and one for remote) will appear on the corresponding serial ports:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.5.0-3564-gf89886d69a8c  ***
   Starting application thread!

   RPMsg Multiple instance [master no 1] demo started

   RPMsg Multiple instance [master no 2] demo started
   Master [no 1] core received a message: 1
   Master [no 2] core received a message: 1
   Master [no 1] core received a message: 3
   Master [no 2] core received a message: 3
   Master [no 1] core received a message: 5
   Master [no 2] core received a message: 5
   ...
   Master [no 1] core received a message: 99
   RPMsg Multiple instance [no 1] demo ended.
   Master [no 2] core received a message: 99
   RPMsg Multiple instance [no 2] demo ended.


.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.5.0-3564-gf89886d69a8c  ***
   Starting application thread!

   RPMsg Multiple instance [remote no 1] demo started

   RPMsg Multiple instance [remote no 2] demo started
   Remote [no 1] core received a message: 0
   Remote [no 2] core received a message: 0
   Remote [no 1] core received a message: 2
   Remote [no 2] core received a message: 2
   Remote [no 1] core received a message: 4
   Remote [no 2] core received a message: 4
   ...
   Remote [no 1] core received a message: 98
   RPMsg Multiple instance [no 1] demo ended.
   Remote [no 2] core received a message: 98
   RPMsg Multiple instance [no 2] demo ended.
