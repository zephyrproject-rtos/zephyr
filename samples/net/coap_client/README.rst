.. _coap-client-sample:

CoAP client
###########

Overview
********

A simple CoAP client showing how to retrieve information from a resource.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed.

This sample will make a GET request with path 'test' to the IPv6
multicast address reserved for CoAP nodes, so the URI can be
represented as:

.. code-block:: none

    coap://[ff02::fd]:5683/test

Building and Running
********************

This project will print all the octets of the response received, more context can
be obtained by using a tool like tcpdump or wireshark.

See the `net-tools`_ project for more details.

It can be built and executed on QEMU as described in :ref:`networking_with_qemu`.

Sample output
=============

.. code-block:: console

  reply: 60 00 00 00 00 24 11 40 fe 80 00 00 00 00 00 00 5c 25 e2 ff fe
  15 01 01 fe 80 00 00 00 00 00 00 5c 25 e2 ff fe 15 01 01 16 33 16 33
  00 24 3d 86 60 40 00 01 ff 54 79 70 65 3a 20 30 0a 43 6f 64 65 3a 20
  31 0a 4d 49 44 3a 20 31 0a (76 bytes)

.. note: The values shown above might differ.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
