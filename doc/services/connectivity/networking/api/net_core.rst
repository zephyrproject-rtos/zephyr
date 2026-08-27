.. _net_core_interface:

Network Core Helpers
####################

.. contents::
    :local:
    :depth: 2

Overview
********

The network subsystem contains two functions for sending and receiving
data from the network. The ``net_recv_data()`` is typically used by network
device driver when the received network data needs to be pushed up in the
network stack for further processing. All the data is received via a network
interface which is typically created by the device driver.

For sending, the ``net_send_data()`` can be used. Typically applications do not
call this function directly as there is the :ref:`bsd_sockets_interface` API
for sending and receiving network data.

API Reference
*************

.. doxygengroup:: net_core
