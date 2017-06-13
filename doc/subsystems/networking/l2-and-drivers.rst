.. _l2_and_drivers:

L2 Stack and Drivers
####################

The L2 stack is designed to hide the whole networking link-layer part
and the related device drivers from the higher IP stack. This is made
through a unique object known as the "network interface object":
:c:type:`struct net_if` declared in `include/net/net_if.h`.

The IP layer is unaware of implementation details beyond the net_if
object and the generic API provided by the L2 layer in
`include/net/net_l2.h` as :c:type:`struct net_l2`.

Only the L2 layer can talk to the device driver, linked to the net_if
object. The L2 layer dictates the API provided by the device driver,
specific for that device, and optimized for working together.

Currently, there are L2 layers for Ethernet, IEEE 802.15.4 Soft-MAC,
Bluetooth IPSP, and a dummy one, which is a generic layer example that
can be used as a template for writing a new one.

L2 layer API
************

In order to create an L2 layer, or even a driver for a specific L2
layer, one needs to understand how the IP layer interacts with it and
how the L2 layer is supposed to behave. The generic L2 API has 3
functions:

- recv: All device drivers, once they receive a packet which they put
  into a :c:type:`struct net_pkt`, will push this buffer to the IP
  core stack via :c:func:`net_recv_data()`. At this point, the IP core
  stack does not know what to do with it. Instead, it passes the
  buffer along to the L2 stack's recv() function for handling. The L2
  stack does what it needs to do with the packet, for example, parsing
  the link layer header, or handling link-layer only packets. The
  recv() function will return NET_DROP in case of an erroneous packet,
  NET_OK if the packet was fully consumed by the L2, or NET_CONTINUE
  if the IP stack should then handle it as an IP packet.

- reserve: Prior to creating any network buffer content, the Zephyr
  core stack needs to know how much dedicated buffer space is needed
  for the L2 layer (for example, space for the link layer header). This
  reserve function returns the number of bytes needed.

- send: Similar to recv, the IP core stack will call this function to
  actually send a packet. All relevant link-layer content will be
  generated and added by this function.  As for recv, send returns a
  verdict and can decide to drop the packet via NET_DROP if something
  wrong happened, or will return NET_OK.

Network Device drivers
**********************

Network device drivers fully follows Zephyr device driver model as a
basis. Please refer to :ref:`device_drivers`.

There are, however, two differences:

- the driver_api pointer must point to a valid :c:type:`struct
  net_if_api` pointer.

- The network device driver must use NET_DEVICE_INIT_INSTANCE(). This
  macro will call the DEVICE_AND_API_INIT() macro, and also
  instantiate a unique :c:type:`struct net_if` related to the created
  device driver instance.

Implementing a network device driver depends on the L2 stack it
belongs to: Ethernet, IEEE 802.15.4, etc. In the next section, we will
describe how a device driver should behave when receiving or sending a
packet. The rest is really hardware dependent and thus does not need
to be detailed here.

Ethernet device driver
======================

On reception, it is up to the device driver to fill-in the buffer with
as many data fragments as required. The buffer itself is a
:c:type:`struct net_pkt` and should be allocated through
:c:func:`net_pkt_get_reserve_rx(0)`. Then all fragments will be
allocated through :c:func:`net_pkt_get_reserve_data(0)`. Of course
the amount of required fragments depends on the size of the received
packet and on the size of a fragment, which is given by
`CONFIG_NET_PKT_DATA_SIZE`.

Note that it is not up to the device driver to decide on the
link-layer space to be reserved in the buffer. Hence the 0 given as
parameter here. The Ethernet L2 layer will update such information
once the packet's Ethernet header has been successfully parsed.

In case :c:func:`net_recv_data()` call fails, it will be up to the
device driver to unreference the buffer via
:c:func:`net_pkt_unref()`.

On sending, it is up to the device driver to send the buffer all at
once, with all the fragments.

In case of a fully successful packet transmission only, the device
driver must unreference the buffer via `net_pkt_unref()`.

Each Ethernet device driver will need, in the end, to call
`NET_DEVICE_INIT_INSTANCE()` like this:

.. code-block:: c

   NET_DEVICE_INIT_INSTANCE(...,
                            CONFIG_ETH_INIT_PRIORITY
			    &the_valid_net_if_api_instance,
			    ETHERNET_L2,
			    NET_L2_GET_CTX_TYPE(ETHERNET_L2), 1500);

IEEE 802.15.4 device driver
===========================

Device drivers for IEEE 802.15.4 L2 work basically the same as for
Ethernet.  What has been described above, especially for recv, applies
here as well.  There are two specific differences however:

- It requires a dedicated device driver API: :c:type:`struct
  ieee802154_radio_api`, which overloads :c:type:`struct
  net_if_api`. This is because 802.15.4 L2 needs more from the device
  driver than just send and recv functions.  This dedicated API is
  declared in `include/net/ieee802154_radio.h`. Each and every IEEE
  802.15.4 device driver must provide a valid pointer on such
  relevantly filled-in API structure.

- Sending a packet is slightly particular. IEEE 802.15.4 sends
  relatively small frames, 127 bytes all inclusive: frame header,
  payload and frame checksum.  Buffer fragments are meant to fit such
  frame size limitation.  But a buffer containing an IPv6/UDP packet
  might have more than one fragment. In the Ethernet device driver, it
  is up to the driver to handle all fragments. IEEE 802.15.4 drivers
  handle only one fragment at a time.  This is why the :c:type:`struct
  ieee802154_radio_api` requires a tx function pointer which differs
  from the :c:type:`struct net_if_api` send function pointer.
  Instead, the IEEE 802.15.4 L2, provides a generic
  :c:func:`ieee802154_radio_send()` meant to be given as
  :c:type:`struct net_if` send function. It turn, the implementation
  of :c:func:`ieee802154_radio_send()` will ensure the same behavior:
  sending one fragment at a time through :c:type:`struct
  ieee802154_radio_api` tx function, and unreferencing the buffer
  only when all the transmission were successful.

Each IEEE 802.15.4 device driver, in the end, will need to call
`NET_DEVICE_INIT_INSTANCE()` that way:

.. code-block:: c

   NET_DEVICE_INIT_INSTANCE(...,
                            the_device_init_prio,
			    &the_valid_ieee802154_radio_api_instance,
			    IEEE802154_L2,
			    NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
