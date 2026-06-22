.. zephyr:code-sample:: coap-client
   :name: CoAP client
   :relevant-api: coap

   Use the CoAP library to implement a client that fetches a resource.

Overview
********

This sample is a simple CoAP client showing how to retrieve information
from a resource.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed.

This sample connects to a CoAP server selected with
``CONFIG_NET_SAMPLE_COAP_CLIENT_PEER``. The peer value may be an IPv4
literal, an IPv6 literal, or a hostname. If the port is omitted, the
sample uses the default CoAP port 5683.

With the default configuration, the URI can be represented as:

.. code-block:: none

    coap://[2001:db8::1]:5683/test

Other valid peer strings include:

.. code-block:: none

    192.0.2.1:5683
    192.0.2.42
    [2001:db8::1]:5683
    [2001:db8::2]
    example.com:5683
    example.com

Optional reply timeout and block-wise retry behavior can be configured
with ``CONFIG_NET_SAMPLE_COAP_CLIENT_REPLY_TIMEOUT_MS`` and
``CONFIG_NET_SAMPLE_COAP_CLIENT_BLOCKWISE_MAX_RETRIES``.

For example, an IPv4 peer endpoint can be set with:

.. code-block:: none

    CONFIG_NET_SAMPLE_COAP_CLIENT_PEER="192.0.2.1:5683"

For hostname-based testing with DNS enabled:

.. code-block:: none

    CONFIG_NET_SAMPLE_COAP_CLIENT_PEER="example.com:5683"

See :ref: ``coap_sample_server.py`` in ``net-tools\coap-test-server\`` for a simple helper server that can
be used for manual interoperability testing.

Building and Running
********************

This project will print all the octets of the response received, more context can
be obtained by using a tool such as tcpdump or wireshark.

See the `net-tools`_ project for more details.

See the ``net-tools\coap-test-server\README.md`` project for more details about test server script.

This sample can be built and executed on QEMU or native_sim board as described
in :ref:`networking_with_host`.

Sample output
=============

.. code-block:: console

  reply: 60 00 00 00 00 24 11 40 fe 80 00 00 00 00 00 00 5c 25 e2 ff fe
  15 01 01 fe 80 00 00 00 00 00 00 5c 25 e2 ff fe 15 01 01 16 33 16 33
  00 24 3d 86 60 40 00 01 ff 54 79 70 65 3a 20 30 0a 43 6f 64 65 3a 20
  31 0a 4d 49 44 3a 20 31 0a (76 bytes)

.. note: The values shown above might differ.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
