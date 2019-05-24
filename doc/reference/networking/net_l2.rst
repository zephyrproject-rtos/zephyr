.. _net_l2_interface:

L2 Layer Management
###################

.. contents::
    :local:
    :depth: 2

Overview
********

The L2 stack is designed to hide the whole networking link-layer part
and the related device drivers from the upper network stack. This is made
through a :c:type:`struct net_if` declared in
:zephyr_file:`include/net/net_if.h`.

The upper layers are unaware of implementation details beyond the net_if
object and the generic API provided by the L2 layer in
:zephyr_file:`include/net/net_l2.h` as :c:type:`struct net_l2`.

Only the L2 layer can talk to the device driver, linked to the net_if
object. The L2 layer dictates the API provided by the device driver,
specific for that device, and optimized for working together.

Currently, there are L2 layers for :ref:`Ethernet <ethernet_interface>`,
:ref:`IEEE 802.15.4 Soft-MAC <ieee802154_interface>`,
:ref:`Bluetooth IPSP <bluetooth-ipsp-sample>`, :ref:`CANBUS <can_interface>`,
:ref:`OpenThread <thread_protocol_interface>`, Wi-Fi, and a dummy layer
example that can be used as a template for writing a new one.

L2 layer API
************

In order to create an L2 layer, or a driver for a specific L2 layer,
one needs to understand how the L3 layer interacts with it and
how the L2 layer is supposed to behave.
See also :ref:`network stack architecture <network_stack_architecture>` for
more details. The generic L2 API has these functions:

- ``recv()``: All device drivers, once they receive a packet which they put
  into a :c:type:`struct net_pkt`, will push this buffer to the network
  stack via :c:func:`net_recv_data()`. At this point, the network
  stack does not know what to do with it. Instead, it passes the
  buffer along to the L2 stack's ``recv()`` function for handling.
  The L2 stack does what it needs to do with the packet, for example, parsing
  the link layer header, or handling link-layer only packets. The ``recv()``
  function will return ``NET_DROP`` in case of an erroneous packet,
  ``NET_OK`` if the packet was fully consumed by the L2, or ``NET_CONTINUE``
  if the network stack should then handle it.

- ``send()``: Similar to receive function, the network stack will call this
  function to actually send a network packet. All relevant link-layer content
  will be generated and added by this function.
  The ``send()`` function returns the number of bytes sent, or a negative
  error code if there was a failure sending the network packet.

- ``enable()``: This function is used to enable/disable traffic over a network
  interface. The function returns ``<0`` if error and ``>=0`` if no error.

- ``get_flags()``: This function will return the capabilities of an L2 driver,
  for example whether the L2 supports multicast or promiscuous mode.

Network Device drivers
**********************

Network device drivers fully follows Zephyr device driver model as a
basis. Please refer to :ref:`device_drivers`.

There are, however, two differences:

- The driver_api pointer must point to a valid :c:type:`struct net_if_api`
  pointer.

- The network device driver must use ``NET_DEVICE_INIT_INSTANCE()``
  or ``ETH_NET_DEVICE_INIT()`` for Ethernet devices. These
  macros will call the ``DEVICE_AND_API_INIT()`` macro, and also
  instantiate a unique :c:type:`struct net_if` related to the created
  device driver instance.

Implementing a network device driver depends on the L2 stack it
belongs to: :ref:`Ethernet <ethernet_interface>`,
:ref:`IEEE 802.15.4 <ieee802154_interface>`, etc.
In the next section, we will describe how a device driver should behave when
receiving or sending a network packet. The rest is hardware dependent
and is not detailed here.

Ethernet device driver
======================

On reception, it is up to the device driver to fill-in the network packet with
as many data buffers as required. The network packet itself is a
:c:type:`struct net_pkt` and should be allocated through
:c:func:`net_pkt_rx_alloc_with_buffer()`. Then all data buffers will be
automatically allocated and filled by :c:func:`net_pkt_write()`.

After all the network data has been received, the device driver needs to
call :c:func:`net_recv_data()`. If that call fails, it will be up to the
device driver to unreference the buffer via :c:func:`net_pkt_unref()`.

On sending, the device driver send function will be called, and it is up to
the device driver to send the network packet all at once, with all the buffers.

Each Ethernet device driver will need, in the end, to call
``ETH_NET_DEVICE_INIT()`` like this:

.. code-block:: c

   ETH_NET_DEVICE_INIT(..., CONFIG_ETH_INIT_PRIORITY,
                       &the_valid_net_if_api_instance, 1500);

IEEE 802.15.4 device driver
===========================

Device drivers for IEEE 802.15.4 L2 work basically the same as for
Ethernet.  What has been described above, especially for ``recv()``, applies
here as well.  There are two specific differences however:

- It requires a dedicated device driver API: :c:type:`struct
  ieee802154_radio_api`, which overloads :c:type:`struct
  net_if_api`. This is because 802.15.4 L2 needs more from the device
  driver than just ``send()`` and ``recv()`` functions.  This dedicated API is
  declared in :zephyr_file:`include/net/ieee802154_radio.h`. Each and every
  IEEE 802.15.4 device driver must provide a valid pointer on such
  relevantly filled-in API structure.

- Sending a packet is slightly different than in Ethernet. IEEE 802.15.4 sends
  relatively small frames, 127 bytes all inclusive: frame header,
  payload and frame checksum.  Buffers are meant to fit such
  frame size limitation.  But a buffer containing an IPv6/UDP packet
  might have more than one fragment. IEEE 802.15.4 drivers
  handle only one buffer at a time.  This is why the :c:type:`struct
  ieee802154_radio_api` requires a tx function pointer which differs
  from the :c:type:`struct net_if_api` send function pointer.
  Instead, the IEEE 802.15.4 L2, provides a generic
  :c:func:`ieee802154_radio_send()` meant to be given as
  :c:type:`struct net_if` send function. It turn, the implementation
  of :c:func:`ieee802154_radio_send()` will ensure the same behavior:
  sending one buffer at a time through :c:type:`struct
  ieee802154_radio_api` tx function, and unreferencing the network packet
  only when all the transmission were successful.

Each IEEE 802.15.4 device driver, in the end, will need to call
``NET_DEVICE_INIT_INSTANCE()`` that way:

.. code-block:: c

   NET_DEVICE_INIT_INSTANCE(...,
                            the_device_init_prio,
			    &the_valid_ieee802154_radio_api_instance,
			    IEEE802154_L2,
			    NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);

API Reference
*************

.. doxygengroup:: net_l2
   :project: Zephyr
