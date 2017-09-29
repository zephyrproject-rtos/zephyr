.. _zoap-server-sample:

CoAP Server
###########

Overview
********

A simple CoAP server showing how to expose a simple resource.

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
against zoap-server.

Building And Running
********************

This project has no output in case of success, the correct
functionality can be verified by using some external tool like tcpdump
or wireshark.

See the `net-tools`_ project for more details

It can be built and executed on QEMU as follows:

.. code-block:: console

    make run


Use this command on the host to run the`libcoap`_ implementation of
the ETSI test cases:

.. code-block:: console

   sudo ./examples/etsi_coaptest.sh -i tap0 2001:db8::1

To build the version supporting the TI CC2520 radio, use the supplied
configuration file enabling IEEE 802.15.4:

.. code-block:: console

    make CONF_FILE=prj_cc2520.conf run


.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools

.. _`libcoap`: https://github.com/obgm/libcoap
