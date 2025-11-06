.. zephyr:code-sample:: zbus-proxy-agent-uart
   :name: zbus Proxy agent - UART
   :relevant-api: zbus_apis

   Forward zbus messages between different devices using UART proxy agents.

Overview
********
This sample demonstrates zbus proxy agent communication using UART between separate devices.
The sample implements a request-response pattern where:

- **Device A** acts as a requester, publishing requests on a channel and receiving responses on a shadow channel
- **Device B** acts as a responder, receiving requests on a shadow channel and publishing responses on a channel
- **UART proxy agents** automatically synchronize channel data between devices over UART

Each device defines its own proxy agent and channels.

- The proxy agents are defined with :c:macro:`ZBUS_PROXY_AGENT_DEFINE`
- Channels are defined with :c:macro:`ZBUS_CHAN_DEFINE` and added to proxy agents with :c:macro:`ZBUS_PROXY_ADD_CHAN`
- Shadow channels are defined with :c:macro:`ZBUS_SHADOW_CHAN_DEFINE` and automatically mirror channels from other devices
- The ``common/common.h`` file defines shared data structures used by both devices

This architecture enables message passing between different devices while maintaining zbus's
publish-subscribe structure across device boundaries.

.. note::
   This sample demonstrates the UART backend in isolation. Proxy agent backends
   can be combined - see :ref:`zbus-proxy-agent-backend-combinations` for
   multi-backend configurations.

Building and Running
********************

This sample requires two separate devices connected via UART. Each device runs a different application:

For Device A (Requester):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/proxy_agent/uart/dev_a
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash

For Device B (Responder):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/proxy_agent/uart/dev_b
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash

Hardware Setup
==============

Connect the two devices via UART:

- Device A TX → Device B RX
- Device A RX → Device B TX
- Connect GND between devices

Sample Output
=============

Device A Output (Requester)
****************************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-rc2-139-g2075ad5a54e0 ***
   main: ZBUS Proxy agent UART Forwarder Sample Application
   main: Requesting on channel request_channel, listening on channel response_channel
   main: Published on channel request_channel. Request ID=1, Min=-1, Max=1
   main: Received message on channel response_channel
   main: Response ID: 1, Value: 0
   main: Published on channel request_channel. Request ID=2, Min=-2, Max=2
   main: Received message on channel response_channel
   main: Response ID: 2, Value: 1

Device B Output (Responder)
****************************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-rc2-139-g2075ad5a54e0 ***
   main: ZBUS Proxy agent UART Forwarder Sample Application
   main: Listening for requests on channel request_channel, responding on channel response_channel
   main: Received message on channel request_channel
   main: Request ID: 1, Min: -1, Max: 1
   main: Sending response: ID=1, Value=-1
   main: Response published on channel response_channel
   main: Received message on channel request_channel
   main: Request ID: 2, Min: -2, Max: 2
   main: Sending response: ID=2, Value=1
   main: Response published on channel response_channel
