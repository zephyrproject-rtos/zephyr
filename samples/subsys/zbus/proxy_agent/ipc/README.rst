.. zephyr:code-sample:: zbus-proxy-agent-ipc
   :name: zbus Proxy agent - IPC
   :relevant-api: zbus_apis

   Forward zbus messages between different domains using IPC proxy agents.

Overview
********
This sample demonstrates zbus proxy agent communication on multi-core platforms.
The sample implements a request-response pattern where:

- The **application CPU** acts as a requester, publishing requests on a channel and receiving responses on a shadow channel
- The **remote CPU** acts as a responder, receiving requests on a shadow channel and publishing responses on a channel
- **IPC proxy agents** automatically synchronize channel data between CPU domains

Each domain defines its own proxy agent and channels.

- The proxy agents are defined with :c:macro:`ZBUS_PROXY_AGENT_DEFINE`
- Channels are defined with :c:macro:`ZBUS_CHAN_DEFINE` and added to proxy agents with :c:macro:`ZBUS_PROXY_ADD_CHAN`
- Shadow channels are defined with :c:macro:`ZBUS_SHADOW_CHAN_DEFINE` and automatically mirror channels from other domains
- The ``common/common.h`` file defines shared data structures used by both domains

This architecture enables message passing between different CPU cores while maintaining zbus's
publish-subscribe structure across domain boundaries.

.. note::
   This sample demonstrates the IPC backend in isolation. Proxy agent backends
   can be combined - see :ref:`zbus-proxy-agent-backend-combinations` for
   multi-backend configurations.

Building and Running
********************

This sample uses sysbuild for multi-core platforms. Use the test configurations from sample.yaml:

.. zephyr-app-commands::
   :zephyr-app: .
   :board: nrf54h20dk/nrf54h20/cpuapp
   :goals: build
   :west-args: -T sample.zbus.proxy_agent_ipc.nrf54h20_cpuapp_cpurad

**Supported platforms and CPU combinations:**

  - **nRF54H20**: CPUAPP + CPURAD
  - **nRF54L15**: CPUAPP + CPUFLPR
  - **nRF54LM20**: CPUAPP + CPUFLPR
  - **nRF5340**: CPUAPP + CPUNET

  Use ``-T <test_name>`` with the corresponding test from sample.yaml to build for different configurations.

Sample Output
*************

Application CPU Output
======================

.. code-block:: console

    *** Booting Zephyr OS build v4.3.0-rc2-139-g2075ad5a54e0 ***
    main: ZBUS Proxy agent IPC Forwarder Sample Application
    main: Requesting on channel request_channel, listening on channel response_channel
    main: Published on channel request_channel. Request ID=1, Min=-1, Max=1
    main: Received message on channel response_channel
    main: Response ID: 1, Value: -1
    main: Published on channel request_channel. Request ID=2, Min=-2, Max=2
    main: Received message on channel response_channel
    main: Response ID: 2, Value: 0

Remote CPU Output
=================

.. code-block:: console

    *** Booting Zephyr OS build v4.3.0-rc2-139-g2075ad5a54e0 ***
    main: ZBUS Proxy agent IPC Forwarder Sample Application
    main: Listening for requests on channel request_channel, responding on channel response_channel
    main: Received message on channel request_channel
    main: Request ID: 1, Min: -1, Max: 1
    main: Sending response: ID=1, Value=1
    main: Response published on channel response_channel
    main: Received message on channel request_channel
    main: Request ID: 2, Min: -2, Max: 2
    main: Sending response: ID=2, Value=-1
    main: Response published on channel response_channel
