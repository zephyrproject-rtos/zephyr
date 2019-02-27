.. _ethernet_interface:

Ethernet
########

.. contents::
    :local:
    :depth: 2

.. toctree::
   :maxdepth: 1

   vlan.rst
   lldp.rst
   8021Qav.rst

Overview
********

Ethernet is a networking technology commonly used in local area networks (LAN).
For more information, see this
`Ethernet Wikipedia article <https://en.wikipedia.org/wiki/Ethernet>`_.

Zephyr supports following Ethernet features:

* 10, 100 and 1000 Mbit/sec links
* Auto negotiation
* Half/full duplex
* Promiscuous mode
* TX and RX checksum offloading
* MAC address filtering
* :ref:`Virtual LANs <vlan_interface>`
* :ref:`Priority queues <traffic-class-support>`
* :ref:`IEEE 802.1AS (gPTP) <gptp_interface>`
* :ref:`IEEE 802.1Qav (credit based shaping) <8021Qav>`
* :ref:`LLDP (Link Layer Discovery Protocol) <lldp_interface>`

Not all Ethernet device drivers support all of these features. You can
see what is supported by ``net iface`` net-shell command. It will print
currently supported Ethernet features.

API Reference
*************

.. doxygengroup:: ethernet
   :project: Zephyr

.. doxygengroup:: ethernet_mii
   :project: Zephyr
