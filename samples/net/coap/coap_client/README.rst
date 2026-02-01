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

This sample will make a GET request with path 'test' to the IPv6
multicast address reserved for CoAP nodes, so the URI can be
represented as:

.. code-block:: none

    coap://[ff02::fd]:5683/test

Building and Running
********************

This project will print all the octets of the response received, more context can
be obtained by using a tool such as tcpdump or wireshark.

See the `net-tools`_ project for more details.

This sample can be built and executed on QEMU or native_sim board as described
in :ref:`networking_with_host`.

Building with OSCORE
=====================

Build the client with OSCORE support:

.. code-block:: console

   west build -b native_sim -- -DEXTRA_CONF_FILE=overlay-oscore.conf

For IPv4 networking:

.. code-block:: console

   west build -b native_sim -- -DEXTRA_CONF_FILE=overlay-oscore-ipv4.conf

The OSCORE-enabled build uses an alternative implementation
(``src/coap-client-oscore.c``) that leverages the CoAP client API with
OSCORE context initialization.

Sample output
=============

.. code-block:: console

  reply: 60 00 00 00 00 24 11 40 fe 80 00 00 00 00 00 00 5c 25 e2 ff fe
  15 01 01 fe 80 00 00 00 00 00 00 5c 25 e2 ff fe 15 01 01 16 33 16 33
  00 24 3d 86 60 40 00 01 ff 54 79 70 65 3a 20 30 0a 43 6f 64 65 3a 20
  31 0a 4d 49 44 3a 20 31 0a (76 bytes)

.. note: The values shown above might differ.

OSCORE (Object Security)
=========================

This sample supports OSCORE (RFC 8613) for end-to-end CoAP message encryption.

Configuration
-------------

The OSCORE-enabled client uses pre-configured credentials for testing:

- **Master Secret**: ``0102030405060708090a0b0c0d0e0f10`` (hex)
- **Master Salt**: ``9e7ca92223786340`` (hex)
- **Client Sender ID**: ``client`` (ASCII)
- **Client Recipient ID**: ``server`` (ASCII)
- **AEAD Algorithm**: AES-CCM-16-64-128 (10)
- **HKDF Algorithm**: HKDF-SHA-256 (-10)

These match the server configuration and are compatible with libcoap's OSCORE implementation.

.. warning::
   These credentials are for **testing only**. Use secure key derivation in production.

Testing
-------

1. Start the OSCORE-enabled server:

   .. code-block:: console

      cd samples/net/sockets/coap_server
      west build -b native_sim -- -DEXTRA_CONF_FILE=overlay-oscore.conf
      west build -t run

2. Run the OSCORE-enabled client in another terminal:

   .. code-block:: console

      cd samples/net/sockets/coap_client
      west build -b native_sim -- -DEXTRA_CONF_FILE=overlay-oscore.conf
      west build -t run

The client will automatically send OSCORE-protected GET and POST requests to the server.

Requirements
------------

OSCORE support requires:

- ``CONFIG_COAP_OSCORE=y`` - Enable OSCORE support
- ``CONFIG_COAP_CLIENT=y`` - Enable CoAP client API
- ``CONFIG_UOSCORE=y`` - uoscore-uedhoc module
- ``CONFIG_PSA_CRYPTO=y`` - PSA Crypto API
- ``CONFIG_ZCBOR=y`` - CBOR support

See :zephyr_file:`overlay-oscore.conf <samples/net/sockets/coap_client/overlay-oscore.conf>`
for the complete configuration.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
