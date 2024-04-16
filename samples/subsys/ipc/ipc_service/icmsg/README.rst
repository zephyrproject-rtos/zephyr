.. zephyr:code-sample:: ipc-icmsg
   :name: IPC service: icmsg backend
   :relevant-api: ipc

   Send messages between two cores using the IPC service and icmsg backend.

Overview
********

This application demonstrates how to use IPC Service and the icmsg backend with
Zephyr. It is designed to demonstrate how to integrate it with Zephyr both
from a build perspective and code.

Building the application for nrf5340dk_nrf5340_cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipc_service/icmsg
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: debug

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is host another is remote:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-323-g468d4ae383c1  ***
   [00:00:00.415,985] <inf> host: IPC-service HOST demo started
   [00:00:00.417,816] <inf> host: Ep bounded
   [00:00:00.417,877] <inf> host: Perform sends for 1000 [ms]
   [00:00:01.417,114] <inf> host: Sent 488385 [Bytes] over 1000 [ms]
   [00:00:01.417,175] <inf> host: Wait 500ms. Let net core finish its sends
   [00:00:01.917,266] <inf> host: Stop network core
   [00:00:01.917,297] <inf> host: Reset IPC service
   [00:00:01.917,327] <inf> host: Run network core
   [00:00:01.924,865] <inf> host: Ep bounded
   [00:00:01.924,896] <inf> host: Perform sends for 1000 [ms]
   [00:00:02.924,194] <inf> host: Sent 489340 [Bytes] over 1000 [ms]
   [00:00:02.924,224] <inf> host: IPC-service HOST demo ended


.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-323-g468d4ae383c1  ***
   [00:00:00.006,256] <inf> remote: IPC-service REMOTE demo started
   [00:00:00.006,378] <inf> remote: Ep bounded
   [00:00:00.006,439] <inf> remote: Perform sends for 1000 [ms]
   [00:00:00.833,160] <inf> sync_rtc: Updated timestamp to synchronized RTC by 1)
   [00:00:01.417,572] <inf> remote: Sent 235527 [Bytes] over 1000 [ms]
   [00:00:01.417,602] <inf> remote: IPC-service REMOTE demo ended
   *** Booting Zephyr OS build zephyr-v3.2.0-323-g468d4ae383c1  ***
   [00:00:00.006,256] <inf> remote: IPC-service REMOTE demo started
   [00:00:00.006,347] <inf> remote: Ep bounded
   [00:00:00.006,378] <inf> remote: Perform sends for 1000 [ms]
   [00:00:01.006,164] <inf> remote: Sent 236797 [Bytes] over 1000 [ms]
   [00:00:01.006,195] <inf> remote: IPC-service REMOTE demo ended
