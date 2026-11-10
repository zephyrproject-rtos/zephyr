.. _ethernet_interface:

Ethernet
########

.. contents::
    :local:
    :depth: 2

.. toctree::
   :maxdepth: 1

   mac_config.rst
   vlan.rst
   ethernet_bridge.rst
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
* :ref:`MAC address configuration <mac_address_config>`
* :ref:`Virtual LANs <vlan_interface>`
* :ref:`Bridging and the Rapid Spanning Tree Protocol <ethernet_bridge_interface>`
* :ref:`Priority queues <traffic-class-support>`
* :ref:`IEEE 802.1AS (gPTP) <gptp_interface>`
* :ref:`IEEE 802.1Qav (credit based shaping) <8021Qav>`
* :ref:`LLDP (Link Layer Discovery Protocol) <lldp_interface>`

Not all Ethernet device drivers support all of these features. You can
see what is supported by ``net iface`` net-shell command. It will print
currently supported Ethernet features.

.. _eth_virtio_net:

Virtio-net
**********

The Virtio-net driver supports multiple pending transmit packets. Configure the limit with
:kconfig:option:`CONFIG_ETH_VIRTIO_NET_TX_BUFFERS`, which defaults to four requests per device.
Each request includes a maximum-size Ethernet frame buffer and a Virtio header, and retains a packet
reference until the device completes transmission. Setting the limit to one reduces buffer memory
and transmit queue size at the cost of concurrency.

Packets with up to sixteen nonempty fragments in the linear RAM mapping are submitted directly,
without copying the payload. The driver copies other packets into a request buffer, including
packets with more fragments or a descriptor chain larger than the device's transmit queue.

The driver requests a power-of-two transmit queue large enough for a header and sixteen payload
fragments per request: 128 descriptors with the default request limit. The queue is capped at the
device's supported size. Devices with a single transmit descriptor use the copy path.

API Reference
*************

.. doxygengroup:: ethernet

.. doxygengroup:: ethernet_mii
