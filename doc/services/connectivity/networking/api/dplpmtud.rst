.. _net_dplpmtud:

Datagram PLPMTUD API
####################

.. contents::
    :local:
    :depth: 2

Overview
********

Zephyr provides a generic **Datagram Packetization Layer Path MTU Discovery**
(DPLPMTUD) implementation based on :rfc:`8899`. It is intended for UDP-based
transports (QUIC, CoAP over UDP, custom protocols) that must discover how large
a datagram payload can be without relying on local IP fragmentation.

The subsystem splits responsibilities as follows:

**Generic stack** (``subsys/net/ip/dplpmtud.c``)

* Per-destination search state: validated PLPMTU, probe size, retry count, bounds
* Binary search between the base PLPMTU (1200 bytes) and the path ceiling
* Integration with the existing PMTU destination cache (ICMP PTB in, validated
  size out)
* Black-hole handling when the PMTU cache reports an MTU below the base PLPMTU

**Transport consumer**

* Construct probe datagrams at the size returned by
  :c:func:`net_dplpmtud_get_path_probe_size()`
* Send probes with don't fragment enabled (see :ref:`ip_socket_options` and
  :c:macro:`ZSOCK_IP_DONTFRAG` / :c:macro:`ZSOCK_IPV6_DONTFRAG`)
* Map transport ACK/loss to :c:func:`net_dplpmtud_on_path_probe_acked()` and
  :c:func:`net_dplpmtud_on_path_probe_lost()`

:ref:`QUIC <quic_dplpmtud>` is the first in-tree consumer. Other transports can
use the same API without duplicating the RFC 8899 state machine.

Configuration
*************

Enable DPLPMTUD per address family:

* :kconfig:option:`CONFIG_NET_IPV4_PMTU` and
  :kconfig:option:`CONFIG_NET_IPV4_PMTU_DPLPMTUD` for IPv4
* :kconfig:option:`CONFIG_NET_IPV6_PMTU` and
  :kconfig:option:`CONFIG_NET_IPV6_PMTU_DPLPMTUD` for IPv6

Both families also require :kconfig:option:`CONFIG_NET_UDP`. The umbrella option
:kconfig:option:`CONFIG_NET_PMTU_DPLPMTUD` is selected when either family
option is enabled.

ICMP-based PMTU reduction (Packet Too Big) remains available through
:kconfig:option:`CONFIG_NET_IPV4_PMTU_PTB` and
:kconfig:option:`CONFIG_NET_IPV6_PMTU_PTB`. DPLPMTUD complements PTB by actively
probing for larger sizes when the path allows it.

The number of concurrent paths tracked matches
:kconfig:option:`CONFIG_NET_IPV4_PMTU_DESTINATION_CACHE_ENTRIES` and
:kconfig:option:`CONFIG_NET_IPV6_PMTU_DESTINATION_CACHE_ENTRIES` respectively.

Usage model
***********

Initialize one :c:struct:`net_dplpmtud_path` per remote destination (or reuse the
handle for the lifetime of a connection):

.. code-block:: c

   struct net_dplpmtud_path path;
   uint16_t max_plpmtu = 1452U; /* transport / peer limit, or 0 if uncapped */
   int mtu;
   int probe;
   int ret;

   ret = net_dplpmtud_init_path(&path, &remote_addr, max_plpmtu);
   if (ret < 0) {
       /* handle error */
   }

   mtu = net_dplpmtud_get_path_mtu(&path);
   if (mtu < 0) {
       mtu = NET_DPLPMTUD_BASE_PLPMTU;
   }

   /* Application data must fit in @a mtu bytes (transport framing excluded). */

Probe lifecycle:

1. Call :c:func:`net_dplpmtud_get_path_probe_size()`. A return value of ``0``
   means no probe is needed. The path must already be initialized; getters do
   not create cache entries.
2. If :c:func:`net_dplpmtud_path_probe_in_flight()` is false, build and send a
   probe datagram of that size with don't fragment set (socket option or
   per-datagram control message in :c:func:`zsock_sendmsg()`).
3. Call :c:func:`net_dplpmtud_on_path_probe_sent()` to reserve the probe in the
   generic state machine, then transmit the probe datagram.
4. If the send fails, call :c:func:`net_dplpmtud_on_path_probe_lost()` so core
   and transport state stay aligned.
5. On transport confirmation, call :c:func:`net_dplpmtud_on_path_probe_acked()`
   or :c:func:`net_dplpmtud_on_path_probe_lost()`.

Update :c:func:`net_dplpmtud_set_path_max_plpmtu()` when the transport learns a
new ceiling (for example a QUIC ``max_udp_payload_size`` transport parameter).

Call :c:func:`net_dplpmtud_note_path_blackhole()` if the transport detects a
black hole independent of ICMP. PTB updates below the base PLPMTU are applied
automatically through the PMTU cache.

API Reference
*************

.. doxygengroup:: net_dplpmtud
