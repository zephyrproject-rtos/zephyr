.. _net_offload_interface:

Network Traffic Offloading
==========================

.. contents::
    :local:
    :depth: 2

Network Offloading
##################

Overview
********

The network offloading API provides hooks that a device vendor can use
to provide an alternate implementation for an IP stack. This means that the
actual network connection creation, data transfer, etc., is done in the vendor
HAL instead of the Zephyr network stack.

API Reference
*************

.. doxygengroup:: net_offload

.. _net_socket_offloading:

Socket Offloading
#################

Overview
********

In addition to the network offloading API, Zephyr allows offloading of networking
functionality at the socket API level. With this approach, vendors who provide an
alternate implementation of the networking stack, exposing socket API for their
networking devices, can easily integrate it with Zephyr.

See :zephyr_file:`drivers/wifi/simplelink/simplelink_sockets.c` for a sample
implementation on how to integrate network offloading at socket level.
