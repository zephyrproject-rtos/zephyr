.. zephyr:code-sample:: zbus-ipc-forwarder
   :name: zbus Proxy agent - IPC forwarder
   :relevant-api: zbus_apis

   Forward zbus messages between different domains using IPC.

Overview
********
This sample demonstrates zbus inter-domain communication on multi-core platforms.
The sample implements a request-response pattern where:

- The **application CPU** acts as a requester, publishing requests on a master channel and receiving responses on a shadow channel
- The **remote CPU** acts as a responder, receiving requests on a shadow channel and publishing responses on a master channel
- **IPC forwarders** automatically synchronize channel data between CPU domains using the zbus proxy functionality

The ``common/common.h`` file defines shared channels using :c:macro:`ZBUS_MULTIDOMAIN_CHAN_DEFINE` with conditional
compilation to create master channels on one domain and shadow channels on the other.

This architecture enables message passing between different CPU cores while maintaining zbus's
publish-subscribe structure across domain boundaries.

Building and Running
********************

Use sysbuild to build this sample for multi-core platforms:

For nRF54H20 with CPURAD:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/ipc_forwarder
   :board: nrf54h20dk/nrf54h20/cpuapp
   :goals: build
   :west-args: --sysbuild -S nordic-log-stm

It can also be built for the other cores in the nRF54H20 using sysbuild configurations:

.. code-block::

   SB_CONFIG_REMOTE_BOARD_NRF54H20_CPURAD=y # Default
   SB_CONFIG_REMOTE_BOARD_NRF54H20_CPUPPR=y
   SB_CONFIG_REMOTE_BOARD_NRF54H20_CPUPPR_XIP=y
   SB_CONFIG_REMOTE_BOARD_NRF54H20_CPUFLPR_XIP=y

For nRF5340:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/ipc_forwarder
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

Sample Output
*************

Application CPU Output
======================

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1157-gaaa3626d8206 ***
   <inf> main: ZBUS Multidomain IPC Forwarder Sample Application
   <inf> main: Channel request_channel is a master channel
   <inf> main: Channel response_channel is a shadow channel
   <inf> main: Published on channel request_channel. Request ID=1, Min=-1, Max=1
   <inf> main: Received message on channel response_channel
   <inf> main: Response ID: 1, Value: 0
   <inf> main: Published on channel request_channel. Request ID=2, Min=-2, Max=2
   <inf> main: Received message on channel response_channel
   <inf> main: Response ID: 2, Value: -1

Remote CPU Output
=================

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1157-gaaa3626d8206 ***
   <inf> main: ZBUS Multidomain IPC Forwarder Sample Application
   <inf> main: Channel request_channel is a shadow channel
   <inf> main: Channel response_channel is a master channel
   <inf> main: Received message on channel request_channel
   <inf> main: Request ID: 1, Min: -1, Max: 1
   <inf> main: Sending response: ID=1, Value=0
   <inf> main: Response published on channel response_channel
   <inf> main: Received message on channel request_channel
   <inf> main: Request ID: 2, Min: -2, Max: 2
   <inf> main: Sending response: ID=2, Value=-1
   <inf> main: Response published on channel response_channel
