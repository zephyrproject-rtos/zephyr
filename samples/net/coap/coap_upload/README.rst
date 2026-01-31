.. zephyr:code-sample:: coap-upload
   :name: CoAP upload
   :relevant-api: coap

   Use the CoAP client API to upload data via a PUT request

Overview
********

This sample demonstrates the use of the CoAP client API to upload a content to a
CoAP server with PUT requests. The sample showcases various methods of uploading
a content:

  * Short upload with payload pointer,
  * Short upload with payload callback,
  * Block transfer upload with payload pointer,
  * Block transfer upload with payload callback.

The sample is compatible with the :zephyr:code-sample:`coap-server` sample, or
can run against a standalone CoAP server, for example `aiocoap`_.

Requirements
************
- :ref:`networking_with_host`, :ref:`networking_with_native_sim`
- or, a board with hardware networking (tested on nrf7002dk board with ``wifi-ipv4`` snippet)
- Network connection between the board and a host running a CoAP server

Build and Running
*****************
Build the CoAP upload sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/coap_upload
   :board: <board to use>
   :goals: build
   :compact:

The easiest way to run this sample application is to build and run it as a
native_sim application. Some setup is required as described in
:ref:`networking_with_native_sim`.

Download a CoAP server application, for example `aiocoap`_ (Python).

Using ``aiocoap``:

.. code-block:: bash

    python -m pip install "aiocoap[all]"
    mkdir file_root
    aiocoap-fileserver --write file_root

Launch :command:`net-setup.sh` in net-tools:

.. code-block:: bash

    ./net-setup.sh

Build and run the CoAP upload sample application for native_sim like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/coap_upload
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
    [00:00:00.110,000] <inf> net_samples_common: Network connectivity established and IP address assigned
    [00:00:00.110,000] <inf> net_samples_common: Waiting for network...
    [00:00:00.110,000] <inf> coap_upload:
    [00:00:00.110,000] <inf> coap_upload: * Starting CoAP upload using IPv4
    [00:00:00.110,000] <inf> coap_upload:
    [00:00:00.110,000] <inf> coap_upload: ** CoAP upload short
    [00:00:00.180,000] <inf> coap_upload: CoAP upload short done in 70 ms
    [00:00:00.180,000] <inf> coap_upload:
    [00:00:00.180,000] <inf> coap_upload: ** CoAP upload short with callback
    [00:00:00.240,000] <inf> coap_upload: CoAP upload short with callback done in 60 ms
    [00:00:00.240,000] <inf> coap_upload:
    [00:00:00.240,000] <inf> coap_upload: ** CoAP upload blockwise
    [00:00:00.300,000] <inf> coap_upload: CoAP upload blockwise ongoing, sent 128 bytes so far
    [00:00:00.360,000] <inf> coap_upload: CoAP upload blockwise ongoing, sent 256 bytes so far
    [00:00:00.420,000] <inf> coap_upload: CoAP upload blockwise ongoing, sent 384 bytes so far
    [00:00:00.480,000] <inf> coap_upload: CoAP upload blockwise done in 240 ms
    [00:00:00.480,000] <inf> coap_upload:
    [00:00:00.480,000] <inf> coap_upload: ** CoAP upload blockwise with callback
    [00:00:00.540,000] <inf> coap_upload: CoAP upload blockwise with callback ongoing, sent 128 bytes so far
    [00:00:00.600,000] <inf> coap_upload: CoAP upload blockwise with callback ongoing, sent 256 bytes so far
    [00:00:00.660,000] <inf> coap_upload: CoAP upload blockwise with callback ongoing, sent 384 bytes so far
    [00:00:00.720,000] <inf> coap_upload: CoAP upload blockwise with callback done in 240 ms
    [00:00:01.230,000] <inf> coap_upload:
    [00:00:01.230,000] <inf> coap_upload: * Starting CoAP upload using IPv6
    [00:00:01.230,000] <inf> coap_upload:
    [00:00:01.230,000] <inf> coap_upload: ** CoAP upload short
    [00:00:01.320,000] <inf> coap_upload: CoAP upload short done in 90 ms
    [00:00:01.320,000] <inf> coap_upload:
    [00:00:01.320,000] <inf> coap_upload: ** CoAP upload short with callback
    [00:00:01.380,000] <inf> coap_upload: CoAP upload short with callback done in 60 ms
    [00:00:01.380,000] <inf> coap_upload:
    [00:00:01.380,000] <inf> coap_upload: ** CoAP upload blockwise
    [00:00:01.440,000] <inf> coap_upload: CoAP upload blockwise ongoing, sent 128 bytes so far
    [00:00:01.500,000] <inf> coap_upload: CoAP upload blockwise ongoing, sent 256 bytes so far
    [00:00:01.560,000] <inf> coap_upload: CoAP upload blockwise ongoing, sent 384 bytes so far
    [00:00:01.620,000] <inf> coap_upload: CoAP upload blockwise done in 240 ms
    [00:00:01.620,000] <inf> coap_upload:
    [00:00:01.620,000] <inf> coap_upload: ** CoAP upload blockwise with callback
    [00:00:01.680,000] <inf> coap_upload: CoAP upload blockwise with callback ongoing, sent 128 bytes so far
    [00:00:01.740,000] <inf> coap_upload: CoAP upload blockwise with callback ongoing, sent 256 bytes so far
    [00:00:01.800,000] <inf> coap_upload: CoAP upload blockwise with callback ongoing, sent 384 bytes so far
    [00:00:01.860,000] <inf> coap_upload: CoAP upload blockwise with callback done in 240 ms

.. _aiocoap: https://github.com/chrysn/aiocoap
