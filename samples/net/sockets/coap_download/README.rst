.. zephyr:code-sample:: coap-download
   :name: CoAP download
   :relevant-api: coap

   Use the CoAP client API to download data via a GET request

Overview
********

This sample demonstrates the use of the CoAP client API to make GET requests to
a CoAP server. If the data to be fetched is larger than a single CoAP packet,
a blockwise transfer will be used to receive the data.

Once the transfer is complete, the sample prints the amount of data received
and the time taken.

Requirements
************
- :ref:`networking_with_host`, :ref:`networking_with_native_sim`
- or, a board with hardware networking (tested on nucleo_h753zi)
- Network connection between the board and host running a CoAP server

Build and Running
*****************
Build the CoAP download sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/coap_download
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

The easiest way to run this sample application is to build and run it as a
native_sim application. Some setup is required as described in
:ref:`networking_with_native_sim`.

Download a CoAP server application, for example `aiocoap`_ (Python), or
`Eclipse Californium`_ (Java) has a demo `Simple File Server`_ application.

Using ``aiocoap``:

.. code-block:: bash

    python -m pip install "aiocoap[all]"
    mkdir file_root
    echo "some test data" > file_root/test.txt
    aiocoap-fileserver file_root

You can also change the name of the CoAP resource to request via Kconfig:

.. code-block:: cfg

    CONFIG_NET_SAMPLE_COAP_RESOURCE_PATH="resource_name_to_request"

Launch :command:`net-setup.sh` in net-tools:

.. code-block:: bash

   ./net-setup.sh

Build and run the coap_download sample application for native_sim like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/coap_download
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Sample output
=============

.. code-block:: console

    [00:00:00.000,000] <inf> net_config: Initializing network
    [00:00:00.000,000] <inf> net_config: IPv4 address: 192.0.2.1
    [00:00:00.110,000] <inf> net_config: IPv6 address: 2001:db8::1
    [00:00:00.110,000] <inf> net_config: IPv6 address: 2001:db8::1
    [00:00:00.110,000] <inf> coap_download: Network L4 is connected
    [00:00:00.110,000] <inf> coap_download: Starting CoAP download using IPv4
    [00:00:00.180,000] <inf> coap_download: CoAP response, result_code=69, offset=0, len=100
    [00:00:00.180,000] <inf> coap_download: CoAP download done, got 100 bytes in 70 ms
    [00:00:00.180,000] <inf> coap_download: Starting CoAP download using IPv6
    [00:00:00.300,000] <inf> coap_download: CoAP response, result_code=69, offset=0, len=100
    [00:00:00.300,000] <inf> coap_download: CoAP download done, got 100 bytes in 120 ms

.. _aiocoap: https://github.com/chrysn/aiocoap
.. _Eclipse Californium: https://github.com/eclipse-californium/californium
.. _Simple File Server: https://github.com/eclipse-californium/californium/tree/main/demo-apps/cf-simplefile-server
