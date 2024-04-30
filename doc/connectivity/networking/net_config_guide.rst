.. _network_configuration_guide:

Network Configuration Guide
###########################

.. contents::
    :local:
    :depth: 2

This document describes how various network configuration options can be
set according to available resources in the system.

Network Buffer Configuration Options
************************************

The network buffer configuration options control how much data we
are able to either send or receive at the same time.

:kconfig:option:`CONFIG_NET_PKT_RX_COUNT`
  Maximum amount of network packets we can receive at the same time.

:kconfig:option:`CONFIG_NET_PKT_TX_COUNT`
  Maximum amount of network packet sends pending at the same time.

:kconfig:option:`CONFIG_NET_BUF_RX_COUNT`
  How many network buffers are allocated for receiving data.
  Each net_buf contains a small header and either a fixed or variable
  length data buffer. The :kconfig:option:`CONFIG_NET_BUF_DATA_SIZE`
  is used when :kconfig:option:`CONFIG_NET_BUF_FIXED_DATA_SIZE` is set.
  This is the default setting. The default size of the buffer is 128 bytes.

  The :kconfig:option:`CONFIG_NET_BUF_VARIABLE_DATA_SIZE` is an experimental
  setting. There each net_buf data portion is allocated from a memory pool and
  can be the amount of data we have received from the network.
  When data is received from the network, it is placed into net_buf data portion.
  Depending on device resources and desired network usage, user can tweak
  the size of the fixed buffer by setting :kconfig:option:`CONFIG_NET_BUF_DATA_SIZE`, and
  the size of the data pool size by setting :kconfig:option:`CONFIG_NET_PKT_BUF_RX_DATA_POOL_SIZE`
  and :kconfig:option:`CONFIG_NET_PKT_BUF_TX_DATA_POOL_SIZE` if variable size buffers are used.

  When using the fixed size data buffers, the memory consumption of network buffers
  can be tweaked by selecting the size of the data part according to what kind of network
  data we are receiving. If one sets the data size to 256, but only receives packets
  that are 32 bytes long, then we are "wasting" 224 bytes for each packet because we
  cannot utilize the remaining data. One should not set the data size too low because
  there is some overhead involved for each net_buf. For these reasons the default
  network buffer size is set to 128 bytes.

  The variable size data buffer feature is marked as experimental as it has not
  received as much testing as the fixed size buffers. Using variable size data
  buffers tries to improve memory utilization by allocating minimum amount of
  data we need for the network data. The extra cost here is the amount of time
  that is needed when dynamically allocating the buffer from the memory pool.

  For example, in Ethernet the maximum transmission unit (MTU) size is 1500 bytes.
  If one wants to receive two full frames, then the net_pkt RX count should be set to 2,
  and net_buf RX count to (1500 / 128) * 2 which is 24.
  If TCP is being used, then these values need to be higher because we can queue the
  packets internally before delivering to the application.

:kconfig:option:`CONFIG_NET_BUF_TX_COUNT`
  How many network buffers are allocated for sending data. This is similar setting
  as the receive buffer count but for sending.


Connection Options
******************

:kconfig:option:`CONFIG_NET_MAX_CONN`
  This option tells how many network connection endpoints are supported.
  For example each TCP connection requires one connection endpoint. Similarly
  each listening UDP connection requires one connection endpoint.
  Also various system services like DHCP and DNS need connection endpoints to work.
  The network shell command **net conn** can be used at runtime to see the
  network connection information.

:kconfig:option:`CONFIG_NET_MAX_CONTEXTS`
  Number of network contexts to allocate. Each network context describes a network
  5-tuple that is used when listening or sending network traffic. Each BSD socket in the
  system uses one network context.


Socket Options
**************

:kconfig:option:`CONFIG_NET_SOCKETS_POLL_MAX`
  Maximum number of supported poll() entries. One needs to select proper value here depending
  on how many BSD sockets are polled in the system.

:kconfig:option:`CONFIG_POSIX_MAX_FDS`
  Maximum number of open file descriptors, this includes files, sockets, special devices, etc.
  One needs to select proper value here depending on how many BSD sockets are created in
  the system.

:kconfig:option:`CONFIG_NET_SOCKETPAIR_BUFFER_SIZE`
  This option is used by socketpair() function. It sets the size of the
  internal intermediate buffer, in bytes. This sets the limit how large
  messages can be passed between two socketpair endpoints.


TLS Options
***********

:kconfig:option:`CONFIG_NET_SOCKETS_TLS_MAX_CONTEXTS`
  Maximum number of TLS/DTLS contexts. Each TLS/DTLS connection needs one context.

:kconfig:option:`CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS`
  This variable sets maximum number of TLS/DTLS credentials that can be
  used with a specific socket.

:kconfig:option:`CONFIG_NET_SOCKETS_TLS_MAX_CIPHERSUITES`
  Maximum number of TLS/DTLS ciphersuites per socket.
  This variable sets maximum number of TLS/DTLS ciphersuites that can
  be used with specific socket, if set explicitly by socket option.
  By default, all ciphersuites that are available in the system are
  available to the socket.

:kconfig:option:`CONFIG_NET_SOCKETS_TLS_MAX_APP_PROTOCOLS`
  Maximum number of supported application layer protocols.
  This variable sets maximum number of supported application layer
  protocols over TLS/DTLS that can be set explicitly by a socket option.
  By default, no supported application layer protocol is set.

:kconfig:option:`CONFIG_NET_SOCKETS_TLS_MAX_CLIENT_SESSION_COUNT`
  This variable specifies maximum number of stored TLS/DTLS sessions,
  used for TLS/DTLS session resumption.

:kconfig:option:`CONFIG_TLS_MAX_CREDENTIALS_NUMBER`
   Maximum number of TLS credentials that can be registered.
   Make sure that this value is high enough so that all the
   certificates can be loaded to the store.


IPv4/6 Options
**************

:kconfig:option:`CONFIG_NET_IF_MAX_IPV4_COUNT`
   Maximum number of IPv4 network interfaces in the system.
   This tells how many network interfaces there will be in the system
   that will have IPv4 enabled.
   For example if you have two network interfaces, but only one of them
   can use IPv4 addresses, then this value can be set to 1.
   If both network interface could use IPv4, then the setting should be
   set to 2.

:kconfig:option:`CONFIG_NET_IF_MAX_IPV6_COUNT`
   Maximum number of IPv6 network interfaces in the system.
   This is similar setting as the IPv4 count option but for IPv6.


TCP Options
***********

:kconfig:option:`CONFIG_NET_TCP_TIME_WAIT_DELAY`
  How long to wait in TCP *TIME_WAIT* state (in milliseconds).
  To avoid a (low-probability) issue when delayed packets from
  previous connection get delivered to next connection reusing
  the same local/remote ports,
  `RFC 793 <https://www.rfc-editor.org/rfc/rfc793>`_ (TCP) suggests
  to keep an old, closed connection in a special *TIME_WAIT* state for
  the duration of 2*MSL (Maximum Segment Lifetime). The RFC
  suggests to use MSL of 2 minutes, but notes

  *This is an engineering choice, and may be changed if experience indicates
  it is desirable to do so.*

  For low-resource systems, having large MSL may lead to quick
  resource exhaustion (and related DoS attacks). At the same time,
  the issue of packet misdelivery is largely alleviated in the modern
  TCP stacks by using random, non-repeating port numbers and initial
  sequence numbers. Due to this, Zephyr uses much lower value of 1500ms
  by default. Value of 0 disables *TIME_WAIT* state completely.

:kconfig:option:`CONFIG_NET_TCP_RETRY_COUNT`
  Maximum number of TCP segment retransmissions.
  The following formula can be used to determine the time (in ms)
  that a segment will be be buffered awaiting retransmission:

  .. math::

     \sum_{n=0}^{\mathtt{NET\_TCP\_RETRY\_COUNT}} \bigg(1 \ll n\bigg)\times
     \mathtt{NET\_TCP\_INIT\_RETRANSMISSION\_TIMEOUT}

  With the default value of 9, the IP stack will try to
  retransmit for up to 1:42 minutes.  This is as close as possible
  to the minimum value recommended by
  `RFC 1122 <https://www.rfc-editor.org/rfc/rfc1122>`_ (1:40 minutes).
  Only 5 bits are dedicated for the retransmission count, so accepted
  values are in the 0-31 range.  It's highly recommended to not go
  below 9, though.

  Should a retransmission timeout occur, the receive callback is
  called with :code:`-ETIMEDOUT` error code and the context is dereferenced.

:kconfig:option:`CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE`
  Maximum sending window size to use.
  This value affects how the TCP selects the maximum sending window
  size. The default value 0 lets the TCP stack select the value
  according to amount of network buffers configured in the system.
  Note that if there are multiple active TCP connections in the system,
  then this value might require finetuning (lowering), otherwise multiple
  TCP connections could easily exhaust net_buf pool for the queued TX data.

:kconfig:option:`CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE`
  Maximum receive window size to use.
  This value defines the maximum TCP receive window size. Increasing
  this value can improve connection throughput, but requires more
  receive buffers available in the system for efficient operation.
  The default value 0 lets the TCP stack select the value
  according to amount of network buffers configured in the system.

:kconfig:option:`CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT`
  How long to queue received data (in ms).
  If we receive out-of-order TCP data, we queue it. This value tells
  how long the data is kept before it is discarded if we have not been
  able to pass the data to the application. If set to 0, then receive
  queueing is not enabled. The value is in milliseconds.

  Note that we only queue data sequentially in current version i.e.,
  there should be no holes in the queue. For example, if we receive
  SEQs 5,4,3,6 and are waiting SEQ 2, the data in segments 3,4,5,6 is
  queued (in this order), and then given to application when we receive
  SEQ 2. But if we receive SEQs 5,4,3,7 then the SEQ 7 is discarded
  because the list would not be sequential as number 6 is be missing.


Traffic Class Options
*********************

It is possible to configure multiple traffic classes (queues) when receiving
or sending network data. Each traffic class queue is implemented as a thread
with different priority. This means that higher priority network packet can
be placed to a higher priority network queue in order to send or receive it
faster or slower. Because of thread scheduling latencies, in practice the
fastest way to send a packet out, is to directly send the packet without
using a dedicated traffic class thread. This is why by default the
:kconfig:option:`CONFIG_NET_TC_TX_COUNT` option is set to 0 if userspace is
not enabled. If userspace is enabled, then the minimum TX traffic class
count is 1. Reason for this is that the userspace application does not
have enough permissions to deliver the message directly.

In receiving side, it is recommended to have at least one receiving traffic
class queue. Reason is that typically the network device driver is running
in IRQ context when it receives the packet, in which case it should not try
to deliver the network packet directly to the upper layers, but to place
the packet to the traffic class queue. If the network device driver is not
running in IRQ context when it gets the packet, then the RX traffic class
option :kconfig:option:`CONFIG_NET_TC_RX_COUNT` could be set to 0.


Stack Size Options
******************

There several network specific threads in a network enabled system.
Some of the threads might depend on a configure option which can be
used to enable or disable a feature. Each thread stack size is optimized
to allow normal network operations.

The network management API is using a dedicated thread by default. The thread
is responsible to deliver network management events to the event listeners that
are setup in the system if the :kconfig:option:`CONFIG_NET_MGMT` and
:kconfig:option:`CONFIG_NET_MGMT_EVENT` options are enabled.
If the options are enabled, the user is able to register a callback function
that the net_mgmt thread is calling for each network management event.
By default the net_mgmt event thread stack size is rather small.
The idea is that the callback function does minimal things so that new
events can be delivered to listeners as fast as possible and they are not lost.
The net_mgmt event thread stack size is controlled by
:kconfig:option:`CONFIG_NET_MGMT_EVENT_QUEUE_SIZE` option. It is recommended
to not do any blocking operations in the callback function.

The network thread stack utilization can be monitored from kernel shell by
the **kernel threads** command.
