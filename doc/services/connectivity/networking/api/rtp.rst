.. _rtp_interface:

RTP
###

.. contents::
    :local:
    :depth: 2

Overview
********

The Real-time Transport Protocol (RTP, :rfc:`3550`) carries media streams such
as audio over UDP. Zephyr implements the RTP data plane under
:kconfig:option:`CONFIG_RTP`: session management, packet serialization and
deserialization, and two transport backends. RTCP is not implemented.

An application declares a session with :c:macro:`RTP_SESSION_DEFINE`,
configures it with :c:func:`rtp_session_init` as a source, sink, or both, and
starts it with :c:func:`rtp_session_start`. Payloads are transmitted with
:c:func:`rtp_session_send`; received packets are delivered through a callback
invoked from the network receive context.

Two transports are available:

* **Socket** (:kconfig:option:`CONFIG_RTP_TRANSPORT_SOCKET`): BSD sockets with
  a background socket service for polling. The default.
* **net_pkt** (:kconfig:option:`CONFIG_RTP_TRANSPORT_NET_PKT`): builds raw
  ``net_pkt`` buffers, bypassing the socket layer for minimal overhead.

.. note::

   RTP support is **experimental**: APIs and Kconfig options may change.

Samples
*******

* :zephyr:code-sample:`net-rtp`

API Reference
*************

.. doxygengroup:: rtp
