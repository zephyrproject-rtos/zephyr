.. zephyr:code-sample:: coap-server
   :name: CoAP service
   :relevant-api: coap coap_service

   Use the CoAP server subsystem to register CoAP resources.

Overview
********

This sample shows how to register CoAP resources to a main CoAP service.
The CoAP server implementation expects all services and resources to be
available at compile time, as they are put into dedicated sections.

The resource is placed into the correct linker section based on the owning
service's name. A linker file is required, see ``sections-ram.ld`` for an example.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed.

The sample will listen for requests on the default CoAP UDP port
(5683 or 5684 for secure CoAP) in the site-local IPv6 multicast address reserved
for CoAP nodes.

The sample exports the following resources:

.. code-block:: none

   /test
   /seg1/seg2/seg3
   /query
   /separate
   /large
   /location-query
   /large-update

These resources allow a good part of the ETSI test cases to be run
against coap-server.

Building And Running
********************
Build the CoAP server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/coap_server
   :board: <board to use>
   :goals: build
   :compact:

Use :zephyr_file:`overlay-dtls.conf <samples/net/sockets/coap_server/overlay-dtls.conf>`
to build the sample with CoAP secure resources instead.

Use :zephyr_file:`overlay-oscore.conf <samples/net/sockets/coap_server/overlay-oscore.conf>`
to build the sample with OSCORE (Object Security for Constrained RESTful Environments)
support. OSCORE provides end-to-end encryption at the CoAP layer.

This project has no output in case of success, the correct
functionality can be verified by using some external tool such as tcpdump
or wireshark.

See the `net-tools`_ project for more details

This sample can be built and executed on QEMU or native_sim board as
described in :ref:`networking_with_host`.

Use this command on the host to run the `libcoap`_ implementation of
the ETSI test cases:

.. code-block:: console

   sudo ./examples/etsi_coaptest.sh -i tap0 2001:db8::1

To build the version supporting the TI CC2520 radio, use the supplied
prj_cc2520.conf configuration file enabling IEEE 802.15.4.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools

.. _`libcoap`: https://github.com/obgm/libcoap

Wi-Fi
=====

The IPv4 Wi-Fi support can be enabled in the sample with
:ref:`Wi-Fi snippet <snippet-wifi-ipv4>`.

OSCORE (Object Security)
=========================

This sample supports OSCORE (RFC 8613) for end-to-end CoAP message encryption.
OSCORE is compatible with standard CoAP clients like libcoap's coap-client.

Building with OSCORE
---------------------

Build the server with OSCORE support:

.. code-block:: console

   west build -b native_sim -- -DEXTRA_CONF_FILE=overlay-oscore.conf

For IPv4 networking:

.. code-block:: console

   west build -b native_sim -- -DEXTRA_CONF_FILE=overlay-oscore-ipv4.conf

Configuration
-------------

The sample uses pre-configured OSCORE credentials for testing:

- **Master Secret**: ``0102030405060708090a0b0c0d0e0f10`` (hex)
- **Master Salt**: ``9e7ca92223786340`` (hex)
- **Server Sender ID**: ``server`` (ASCII)
- **Server Recipient ID**: ``client`` (ASCII)
- **AEAD Algorithm**: AES-CCM-16-64-128 (10)
- **HKDF Algorithm**: HKDF-SHA-256 (-10)

.. warning::
   These credentials are for **testing only**. Use secure key derivation in production.

Testing with libcoap
--------------------

Create an OSCORE configuration file ``oscore.conf``:

.. code-block:: none

   master_secret,hex,"0102030405060708090a0b0c0d0e0f10"
   master_salt,hex,"9e7ca92223786340"
   sender_id,ascii,"client"
   recipient_id,ascii,"server"
   replay_window,integer,32
   aead_alg,integer,10
   hkdf_alg,integer,-10

Test with coap-client:

.. code-block:: console

   # IPv6
   coap-client -m get coap://[2001:db8::1]/test -O oscore.conf

   # IPv4
   coap-client -m get coap://192.0.2.1/test -O oscore.conf

The server accepts both OSCORE-protected and unprotected requests by default.
Set ``require_oscore`` to ``true`` in the source code to enforce OSCORE for all requests.

Requirements
------------

OSCORE support requires:

- ``CONFIG_COAP_OSCORE=y`` - Enable OSCORE support
- ``CONFIG_UOSCORE=y`` - uoscore-uedhoc module
- ``CONFIG_PSA_CRYPTO=y`` - PSA Crypto API
- ``CONFIG_ZCBOR=y`` - CBOR support

See :zephyr_file:`overlay-oscore.conf <samples/net/sockets/coap_server/overlay-oscore.conf>`
for the complete configuration.
