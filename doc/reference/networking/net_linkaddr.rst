.. _net_linkaddr_interface:

Link Layer Address Handling
###########################

.. contents::
    :local:
    :depth: 2

Overview
********

The link layer addresses are set for network interfaces so that L2
connectivity works correctly in the network stack. Typically the link layer
addresses are 6 bytes long like in Ethernet but for IEEE 802.15.4 the link
layer address length is 8 bytes.

API Reference
*************

.. doxygengroup:: net_linkaddr
   :project: Zephyr
