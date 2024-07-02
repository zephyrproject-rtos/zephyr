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
through a :c:struct:`net_if` declared in
:zephyr_file:`include/zephyr/net/net_if.h`.

The upper layers are unaware of implementation details beyond the net_if
object and the generic API provided by the L2 layer in
:zephyr_file:`include/zephyr/net/net_l2.h` as :c:struct:`net_l2`.

Only the L2 layer can talk to the device driver, linked to the net_if
object. The L2 layer dictates the API provided by the device driver,
specific for that device, and optimized for working together.

Currently, there are L2 layers for :ref:`Ethernet <ethernet_interface>`,
:ref:`IEEE 802.15.4 Soft-MAC <ieee802154_interface>`, :ref:`CANBUS <can_api>`,
:ref:`OpenThread <thread_protocol_interface>`, Wi-Fi, and a dummy layer example
that can be used as a template for writing a new one.

L2 layer API
************

In order to create an L2 layer, or a driver for a specific L2 layer,
one needs to understand how the L3 layer interacts with it and
how the L2 layer is supposed to behave.
See also :ref:`network stack architecture <network_stack_architecture>` for
more details. The generic L2 API has these functions:

- ``recv()``: All device drivers, once they receive a packet which they put
  into a :c:struct:`net_pkt`, will push this buffer to the network
  stack via :c:func:`net_recv_data`. At this point, the network
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
basis. Please refer to :ref:`device_model_api`.

There are, however, two differences:

- The driver_api pointer must point to a valid :c:struct:`net_if_api`
  pointer.

- The network device driver must use :c:macro:`NET_DEVICE_INIT_INSTANCE()`
  or :c:macro:`ETH_NET_DEVICE_INIT()` for Ethernet devices. These
  macros will call the :c:macro:`DEVICE_DEFINE()` macro, and also
  instantiate a unique :c:struct:`net_if` related to the created
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
:c:struct:`net_pkt` and should be allocated through
:c:func:`net_pkt_rx_alloc_with_buffer`. Then all data buffers will be
automatically allocated and filled by :c:func:`net_pkt_write`.

After all the network data has been received, the device driver needs to
call :c:func:`net_recv_data`. If that call fails, it will be up to the
device driver to unreference the buffer via :c:func:`net_pkt_unref`.

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
Ethernet. What has been described above, especially for ``recv()``, applies
here as well. There are two specific differences however:

- It requires a dedicated device driver API: :c:struct:`ieee802154_radio_api`,
  which overloads :c:struct:`net_if_api`. This is because 802.15.4 L2 needs more from the device
  driver than just ``send()`` and ``recv()`` functions.  This dedicated API is
  declared in :zephyr_file:`include/zephyr/net/ieee802154_radio.h`. Each and every
  IEEE 802.15.4 device driver must provide a valid pointer on such
  relevantly filled-in API structure.

- Sending a packet is slightly different than in Ethernet. Most IEEE 802.15.4
  PHYs support relatively small frames only, 127 bytes all inclusive: frame
  header, payload and frame checksum. Buffers to be sent over the radio will
  often not fit this frame size limitation, e.g. a buffer containing an IPv6
  packet will often have to be split into several fragments and IP6 packet headers
  and fragments need to be compressed using a protocol like 6LoWPAN before being
  passed on to the radio driver. Additionally the IEEE 802.15.4 standard defines
  medium access (e.g. CSMA/CA), frame retransmission, encryption and other pre-
  processing procedures (e.g. addition of information elements) that individual
  radio drivers should not have to care about. This is why the
  :c:struct:`ieee802154_radio_api` requires a tx function pointer which differs
  from the :c:struct:`net_if_api` send function pointer. Zephyr's native
  IEEE 802.15.4 L2 implementation provides a generic :c:func:`ieee802154_send`
  instead, meant to be given as :c:type:`net_if` send function. The implementation
  of :c:func:`ieee802154_send` takes care of IEEE 802.15.4 standard packet
  preparation procedures, splitting the packet into possibly compressed,
  encrypted and otherwise pre-processed fragment buffers, sending one buffer
  at a time through :c:struct:`ieee802154_radio_api` tx function and unreferencing
  the network packet only when the transmission as a whole was either successful
  or failed.

Interaction between IEEE 802.15.4 radio device drivers and L2 is bidirectional:

- L2 -> L1: Methods as :c:func:`ieee802154_send` and several IEEE 802.15.4 net
  management calls will call into the driver, e.g. to send a packet over the
  radio link or re-configure the driver at runtime. These incoming calls will
  all be handled by the methods in the :c:struct:`ieee802154_radio_api`.

- L1 -> L2: There are several situations in which the driver needs to initiate
  calls into the L2/MAC layer. Zephyr's IEEE 802.15.4 L1 -> L2 adaptation API
  employs an "inversion-of-control" pattern in such cases avoids duplication of
  complex logic across independent driver implementations and ensures
  implementation agnostic loose coupling and clean separation of concerns between
  MAC (L2) and PHY (L1) whenever reverse information transfer or close co-operation
  between hardware and L2 is required. During driver initialization, for example,
  the driver calls :c:func:`ieee802154_init` to pass the interface's MAC address
  as well as other hardware-related configuration to L2. Similarly, drivers may
  indicate performance or timing critical radio events to L2 that require close
  integration with the hardware (e.g. :c:func:`ieee802154_handle_ack`). Calls
  from L1 into L2 are not implemented as methods in :c:struct:`ieee802154_radio_api`
  but are standalone functions declared and documented as such in
  :zephyr_file:`include/zephyr/net/ieee802154_radio.h`. The API documentation will
  clearly state which functions must be implemented by all L2 stacks as part
  of the L1 -> L2 "inversion-of-control" adaptation API.

Note: Standalone functions in :zephyr_file:`include/zephyr/net/ieee802154_radio.h`
that are not explicitly documented as callbacks are considered to be helper functions
within the PHY (L1) layer implemented independently of any specific L2 stack, see for
example :c:func:`ieee802154_is_ar_flag_set`.

As all net interfaces, IEEE 802.15.4 device driver implementations will have to call
``NET_DEVICE_INIT_INSTANCE()`` in the end:

.. code-block:: c

   NET_DEVICE_INIT_INSTANCE(...,
                            the_device_init_prio,
			    &the_valid_ieee802154_radio_api_instance,
			    IEEE802154_L2,
			    NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);

API Reference
*************

.. doxygengroup:: net_l2
