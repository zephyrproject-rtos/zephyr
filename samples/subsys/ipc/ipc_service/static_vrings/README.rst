.. zephyr:code-sample:: ipc-static-vrings
   :name: IPC service: static vrings backend
   :relevant-api: ipc

   Send messages between two cores using the IPC service and static vrings backend.

Overview
********

This application demonstrates how to use IPC Service and the static vrings
backend with Zephyr. It is designed to demonstrate how to integrate it with
Zephyr both from a build perspective and code.

Building the application for nrf5340dk/nrf5340/cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipc_service/static_vrings
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: debug
   :west-args: --sysbuild

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is host another is remote:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-2383-g147aee22f273  ***
   IPC-service HOST [INST 0 - ENDP A] demo started
   IPC-service HOST [INST 0 - ENDP B] demo started
   IPC-service HOST [INST 1] demo started
   HOST [1]: 1
   HOST [1]: 3
   HOST [1]: 5
   HOST [1]: 7
   HOST [1]: 9
   ...


.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-2383-g147aee22f273  ***
   IPC-service REMOTE [INST 0 - ENDP A] demo started
   IPC-service REMOTE [INST 0 - ENDP B] demo started
   IPC-service REMOTE [INST 1] demo started
   REMOTE [1]: 0
   REMOTE [1]: 2
   REMOTE [1]: 4
   REMOTE [1]: 6
   REMOTE [1]: 8
   ...
