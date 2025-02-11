.. _ipc_multi_endpoint_sample:

IPC Service - Multi-endpoint Sample Application
###############################################

This application demonstrates how to use IPC Service with multiple endpoints.
By default, it uses the ``icmsg_me`` backend.
You can also configure it to use the ``icbmsg`` backend.

Building the application for nrf5340dk/nrf5340/cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipc_service/multi_endpoint
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: debug

Open a serial terminal (for example Minicom or PuTTY) and connect the board with the following settings:

* Speed: 115200
* Data: 8 bits
* Parity: None
* Stop bits: 1

After resetting the board, the following message will appear on the corresponding
serial port:

.. code-block:: console

   *** Booting Zephyr OS build v3.4.0-rc1-108-gccfbac8b0721 ***
   IPC-service HOST [INST 0 - ENDP A] demo started
   IPC-service HOST [INST 0 - ENDP B] demo started
   IPC-service HOST [INST 1] demo started
   HOST [0A]: 1
   HOST [0A]: 3
   HOST [0B]: 1
   HOST [1]: 1
   ...
   HOST [0A]: 99
   IPC-service HOST [INST 0 - ENDP A] demo ended.
   HOST [0B]: 99
   IPC-service HOST [INST 0 - ENDP B] demo ended.
   HOST [1]: 99
   IPC-service HOST [INST 1] demo ended.

.. code-block:: console

   *** Booting Zephyr OS build v3.4.0-rc1-108-gccfbac8b0721 ***
   IPC-service REMOTE [INST 0 - ENDP A] demo started
   IPC-service REMOTE [INST 0 - ENDP B] demo started
   IPC-service REMOTE [INST 1] demo started
   REMOTE [0A]: 0
   REMOTE [0A]: 2
   ...
   REMOTE [0A]: 98
   IPC-service REMOTE [INST 0 - ENDP A] demo ended.
   REMOTE [0B]: 98
   IPC-service REMOTE [INST 0 - ENDP B] demo ended.
   REMOTE [1]: 98
   IPC-service REMOTE [INST 1] demo ended.


Changing the backend
********************

To change the backend to ``icbmsg``, switch the devicetree
overlay files as follows:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp --sysbuild -- \
   -DDTC_OVERLAY_FILE=boards/nrf5340dk_nrf5340_cpuapp_icbmsg.overlay \
   -Dremote_DTC_OVERLAY_FILE=boards/nrf5340dk_nrf5340_cpunet_icbmsg.overlay
