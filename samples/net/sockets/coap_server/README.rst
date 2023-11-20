.. zephyr:code-sample:: coap-server
   :name: CoAP service
   :relevant-api: coap coap_service udp

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

The sample will listen for requests in the CoAP UDP port (5683) in the
site-local IPv6 multicast address reserved for CoAP nodes.

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
