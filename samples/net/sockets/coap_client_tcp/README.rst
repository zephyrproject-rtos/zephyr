.. zephyr:code-sample:: coap-client-tcp
   :name: CoAP TCP Client
   :relevant-api: coap coap_client_tcp

   Use the CoAP over TCP client to communicate with a CoAP server.

Overview
********

This sample demonstrates how to use the CoAP over TCP client API to communicate
with a CoAP server using reliable transports as specified in RFC 8323.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed.

The sample shows:

- Connecting to a CoAP TCP server
- Waiting for CSM exchange completion using event callbacks
- Sending GET requests over TCP
- Using Ping/Pong signals with event-based waiting
- Graceful session release
- Event callback for server signals (Release, Abort)

Requirements
************

- :ref:`networking_with_host`, :ref:`networking_with_native_sim`
- Network connection between the board and host running a CoAP server
- A CoAP server with TCP support, such as `aiocoap`_

Building and Running
********************

The easiest way to run this sample application is to build and run it as a
native_sim application. Some setup is required as described in
:ref:`networking_with_native_sim`.

A CoAP server with TCP support must be running on the host. Using
``aiocoap``:

.. code-block:: bash

    python -m pip install "aiocoap[all]"
    mkdir file_root
    echo "Hello from CoAP TCP!" > file_root/test
    aiocoap-fileserver --bind "coap+tcp://[::]:5683" file_root

Launch :command:`net-setup.sh` in net-tools:

.. code-block:: bash

   ./net-setup.sh

Build and run the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/coap_client_tcp
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Configuration
*************

The server address and port can be modified via Kconfig and in ``src/main.c``:

- :kconfig:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR`: Server IP address
- ``PEER_PORT``: Server port (default: 5683)
- ``RESOURCE_PATH``: Resource path to request (default: "test")

The following Kconfig options are relevant:

- :kconfig:option:`CONFIG_COAP_OVER_RELIABLE_TRANSPORT`: Enable CoAP over TCP support
- :kconfig:option:`CONFIG_COAP_CLIENT_TCP`: Enable CoAP TCP client
- :kconfig:option:`CONFIG_COAP_CLIENT_MESSAGE_SIZE`: Maximum message size

See Also
********

- :zephyr:code-sample:`coap-client`
- :zephyr:code-sample:`coap-server`
- `RFC 8323 - CoAP over TCP, TLS, and WebSockets <https://tools.ietf.org/html/rfc8323>`_
- `aiocoap`_

.. _aiocoap: https://github.com/chrysn/aiocoap
