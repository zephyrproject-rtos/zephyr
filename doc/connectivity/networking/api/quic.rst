.. _quic_transport_interface:

QUIC Transport Interface
########################

.. contents::
    :local:
    :depth: 2

Overview
********

QUIC is a general-purpose, multiplexed transport protocol standardised in
:rfc:`9000`. It operates over UDP and provides ordered, reliable byte-stream
delivery with integrated TLS 1.3 security (:rfc:`9001`), stream multiplexing,
and connection migration. Zephyr's QUIC implementation is available under
:kconfig:option:`CONFIG_QUIC`.

Key properties relevant to embedded use:

* **Stream multiplexing**: Bidirectional and unidirectional streams share a
  single UDP socket, avoiding head-of-line blocking at the transport layer.
* **Integrated TLS 1.3**: The handshake is built into the connection
  establishment; no separate TLS layer is required.
* **Flow control**: Per-stream and per-connection credit-based flow control
  prevents fast senders from overwhelming constrained receivers.
* **Loss recovery**: A Probe Timeout (PTO) mechanism retransmits data
  without relying on ICMP or TCP-style ACK clocks.
* **Socket Integration**: Uses standard Zephyr socket calls like ``zsock_send``,
  ``zsock_recv``, ``zsock_recvmsg``, ``zsock_sendmsg``, ``zsock_close``
  for data transfer.
* **Dual stack**: Supports both IPv4 and IPv6 connections.

.. note::

   QUIC support is currently **experimental** in Zephyr. Enable it with
   ``CONFIG_QUIC=y`` and be aware that APIs and Kconfig options may change
   between releases.


Architecture & Concepts
***********************

To effectively use the library, it is helpful to understand the relationship
between **Connections** and **Streams**.

* **QUIC Connection Socket**: Represents the "tunnel" to the peer. It handles
  the TLS handshake, congestion control, and connection termination.
  You typically do not send application data directly over this socket; you use
  it to create streams.
* **QUIC Stream Socket**: Represents a lightweight data channel inside the connection.
  This is where the actual application data (``send`` or ``recv``) flows.


Public API Reference
********************

The public API is defined in :zephyr_file:`include/zephyr/net/quic.h`.

Connection Management
---------------------

:c:func:`quic_connection_open`

Creates a new QUIC connection context. This serves as the foundation for all
subsequent communication.

.. code-block:: c

   int quic_connection_open(const struct net_sockaddr *remote_addr,
                            const struct net_sockaddr *local_addr);


* **Parameters**:

  * ``remote_addr``: The address of the peer.

    * **Client Mode**: Set this to the server's IP and port.
    * **Server Mode**: Set to ``NULL`` (or unspecified) if binding a listener.

  * ``local_addr``: The local address to bind to.

    * **Client Mode**: Can be ``NULL`` (system will auto-bind).
    * **Server Mode**: Set this to the local interface and port to listen on.

* **Returns**: A file descriptor (socket ID) representing the connection,
  or ``< 0`` on error.

:c:func:`quic_connection_close`

Closes the connection and terminates the TLS session. The ``zsock_close`` could
be used here too.

.. code-block:: c

   int quic_connection_close(int sock);


Stream Management
-----------------

:c:func:`quic_stream_open`

Creates a new stream within an established connection.

.. code-block:: c

   int quic_stream_open(int connection_sock,
                        enum quic_stream_initiator initiator,
                        enum quic_stream_direction direction,
                        uint8_t priority);


* **Parameters**:

  * ``connection_sock``: The socket FD returned by ``quic_connection_open``.
  * ``initiator``: Who is opening the stream?

    * ``QUIC_STREAM_CLIENT``
    * ``QUIC_STREAM_SERVER``

  * ``direction``:

    * ``QUIC_STREAM_BIDIRECTIONAL``: Both sides can read/write.
    * ``QUIC_STREAM_UNIDIRECTIONAL``: Only the initiator can write.

  * ``priority``: Priority level (0-255) for scheduling data.

* **Returns**: A new file descriptor (socket ID) specific to this stream.

:c:func:`quic_stream_close`

Closes a specific stream without closing the underlying connection. The ``zsock_close``
could be used here too.

.. code-block:: c

   int quic_stream_close(int sock);


Application Workflow
********************

The following sections outline how to write a generic Client and Server
application using the library.

Client Application
------------------

A client initiates a connection to a remote server, opens a stream,
sends a request, and reads the response.

**Step 1: Setup Addresses**

.. code-block:: c

   struct net_sockaddr_in remote_addr = {
       .sin_family = NET_AF_INET,
       .sin_port = net_htons(4422),
   };

   zsock_inet_pton(NET_AF_INET, "192.0.2.1", &remote_addr.sin_addr);


**Step 2: Open QUIC Connection**

.. code-block:: c

   int conn_sock = quic_connection_open((struct net_sockaddr *)&remote_addr, NULL);
   if (conn_sock < 0) {
       /* Handle error */
   }

   /* The TLS handshake occurs automatically upon the first stream creation. */


**Step 3: Open a Stream**

.. code-block:: c

   /* Create a bidirectional stream initiated by the client */
   int stream_sock = quic_stream_open(conn_sock,
                                      QUIC_STREAM_CLIENT,
                                      QUIC_STREAM_BIDIRECTIONAL,
                                      0);


**Step 4: Transfer Data**

Use standard socket calls on the **stream socket**, not the connection socket.

.. code-block:: c

   zsock_send(stream_sock, "Hello Server", 12, 0);

   char buffer[64];
   zsock_recv(stream_sock, buffer, sizeof(buffer), 0);


**Step 5: Cleanup**

.. code-block:: c

   quic_stream_close(stream_sock);
   quic_connection_close(conn_sock);


Server Application
------------------

A server binds to a local port, waits for incoming connections,
accepts streams, and processes data.

**Step 1: Bind Connection Socket**

.. code-block:: c

   struct net_sockaddr_in local_addr = {
      .sin_family = NET_AF_INET,
      .sin_port = net_htons(4422),
      .sin_addr.s_addr = NET_INADDR_ANY,
   };

   /* Create the listening context */
   int conn_sock = quic_connection_open(NULL, (struct net_sockaddr *)&local_addr);


**Step 2: Accept Incoming Streams**

The :c:func:`quic_connection_open` socket acts as a "parent".
Use :c:func:`zsock_accept()` on the **connection socket** to retrieve a file
descriptor for a new stream initiated by a peer.

.. code-block:: c

   struct net_sockaddr_in peer_addr;
   net_socklen_t addrlen = sizeof(peer_addr);

   /* Block until a client opens a stream */
   int stream_sock = zsock_accept(conn_sock, (struct net_sockaddr *)&peer_addr, &addrlen);

   if (stream_sock >= 0) {
       /* Handle the new stream (read/write data) */
       char buf[128];
       int len = zsock_recv(stream_sock, buf, sizeof(buf), 0);

       /* Echo back */
       zsock_send(stream_sock, buf, len, 0);

       /* Close the stream when done, also zsock_close() can be used */
       quic_stream_close(stream_sock);
   }


TLS & Security Configuration
****************************

The QUIC transport uses ``mbedTLS`` and PSA APIs for cryptographic operations.
Security credentials (certificates, keys) are managed via the
Zephyr **TLS Credentials** subsystem.

Managing Certificates
---------------------

Before opening a connection, you must register your certificates
using :c:func:`tls_credential_add`.

1. **CA Certificate**: Required for clients to verify servers.
2. **Server Certificate & Private Key**: Required for servers.

**Example: Loading Credentials**

.. code-block:: c

   #include <zephyr/net/tls_credentials.h>

   /* Tag ID to reference credentials later */
   #define MY_SEC_TAG 1

   static const char server_cert[] = ...; /* PEM or DER data */
   static const char priv_key[] = ...;    /* PEM or DER data */

   void setup_credentials(void) {
       tls_credential_add(MY_SEC_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
                          server_cert, sizeof(server_cert));
       tls_credential_add(MY_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
                          priv_key, sizeof(priv_key));
   }


Applying Credentials to QUIC
----------------------------

You apply credentials to the QUIC socket using :c:func:`zsock_setsockopt` on
the **connection socket** immediately after creation. The credentials
must be set before the stream is created.

.. code-block:: c

   sec_tag_t sec_tag_list[] = { MY_SEC_TAG };

   zsock_setsockopt(conn_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
                    sec_tag_list, sizeof(sec_tag_list));


Applying ALPN to QUIC
---------------------

The Application-Layer Protocol Negotiation (ALPN) is mandatory in QUIC.
The ALPN is used to negotiate the application protocol that is run on top
of two QUIC endpoints. You apply ALPN list to the QUIC socket using
:c:func:`zsock_setsockopt` on the **connection socket** immediately after creation.
The ALPN list must be set before the stream is created.
Note that the list items must be constants, and they cannot be variables.

.. code-block:: c

   const char * const alpn_list[] = {
       "echo-quic",
       NULL
   };

   ret = zsock_setsockopt(quic_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST,
			       alpn_list, sizeof(alpn_list));
   if (ret < 0) {
       LOG_ERR("Failed to set ALPN (%d)", -errno);
   }


Configure Options
*****************

All options below live inside the ``if QUIC`` block and are only visible when
:kconfig:option:`CONFIG_QUIC` is enabled.

Connection and Stream Counts
----------------------------

.. list-table::
   :widths: 40 12 48
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - :kconfig:option:`CONFIG_QUIC_MAX_CONTEXTS`
     - 1
     - Maximum number of simultaneous QUIC connections in the system.
       Each context carries its own TLS state, flow-control windows,
       and sent-packet history.  Memory scales linearly with this value.
   * - :kconfig:option:`CONFIG_QUIC_MAX_STREAMS_BIDI`
     - 3–5
     - Maximum bidirectional streams **per connection**.  Streams are
       allocated at build time; unused stream slots still consume their
       TX/RX buffer allocation.
   * - :kconfig:option:`CONFIG_QUIC_MAX_STREAMS_UNI`
     - 0
     - Maximum unidirectional streams per connection.  Set to ``0`` to
       disable unidirectional streams entirely and recover the associated
       buffer RAM.
   * - :kconfig:option:`CONFIG_QUIC_MAX_ENDPOINTS`
     - 2–3
     - Number of UDP endpoint sockets.  A client connecting to a single
       server over one IP version needs 2; dual-stack server roles need 3.
       Consider enabling :kconfig:option:`CONFIG_QUIC_ENDPOINT_USE_IPV4_MAPPING_TO_IPV6`
       to share a single socket for both IPv4 and IPv6.
   * - :kconfig:option:`CONFIG_QUIC_MAX_PEER_CIDS`
     - 4
     - Number of peer Connection IDs tracked per connection.  Reduce to
       1 on very constrained devices if connection migration is not needed.

QUIC Transport Parameters (Flow Control)
----------------------------------------

These values are advertised to the remote peer during the handshake
(see :rfc:`9000` Section 18.2).  They govern how much data the **peer is
permitted to send** before receiving a window update.  Each value should
match the corresponding local receive buffer size; advertising a window
larger than the receive buffer causes buffer overflows, while advertising
a window smaller than the buffer under-utilises available memory.

.. list-table::
   :widths: 44 12 44
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - :kconfig:option:`CONFIG_QUIC_INITIAL_MAX_DATA`
     - 16384
     - Connection-level receive credit in bytes.  Must be greater than or
       equal to any single stream's ``INITIAL_MAX_STREAM_DATA`` value,
       otherwise the connection window becomes the bottleneck.  A good
       starting point is ``streams_bidi × INITIAL_MAX_STREAM_DATA_BIDI_LOCAL``.
   * - :kconfig:option:`CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL`
     - 16384
     - Initial receive window for locally initiated bidirectional streams.
       Set equal to :kconfig:option:`CONFIG_QUIC_STREAM_RX_BUFFER_SIZE`.
   * - :kconfig:option:`CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE`
     - 16384
     - Initial receive window for peer-initiated bidirectional streams.
       Set equal to :kconfig:option:`CONFIG_QUIC_STREAM_RX_BUFFER_SIZE`.
   * - :kconfig:option:`CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_UNI`
     - 16384
     - Initial receive window for unidirectional streams opened by the peer.
       Only relevant when :kconfig:option:`CONFIG_QUIC_MAX_STREAMS_UNI` > 0.
   * - :kconfig:option:`CONFIG_QUIC_INITIAL_MAX_STREAMS_BIDI`
     - =QUIC_MAX_STREAMS_BIDI
     - Maximum number of bidirectional streams the peer may open before
       receiving a ``MAX_STREAMS`` frame.  Defaults to the local build-time
       limit.
   * - :kconfig:option:`CONFIG_QUIC_INITIAL_MAX_STREAMS_UNI`
     - =QUIC_MAX_STREAMS_UNI
     - Maximum number of unidirectional streams the peer may open.
   * - :kconfig:option:`CONFIG_QUIC_STREAM_RX_WINDOW_UPDATE_THRESHOLD`
     - 25
     - Percentage of stream receive buffer consumed before a
       ``MAX_STREAM_DATA`` frame is sent to refill the peer's window.
       Lower values (e.g. 15 %) send updates more frequently, which
       improves throughput on high-latency links at the cost of
       additional control frames.

Buffer Sizes and RAM Allocation
-------------------------------

The following options directly control how much RAM the QUIC subsystem
allocates at initialisation.  See `Memory Model`_ for a detailed
breakdown of how these interact.

.. list-table::
   :widths: 44 12 44
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - :kconfig:option:`CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN`
     - 1500
     - Receive staging buffer per endpoint in bytes.  Should equal the
       network MTU.  Combined with :kconfig:option:`CONFIG_QUIC_PKT_COUNT`,
       total endpoint RX staging = ``endpoints × pkt_count × this value``.
   * - :kconfig:option:`CONFIG_QUIC_TX_BUFFER_SIZE`
     - 1500
     - Output packet assembly buffer per endpoint.  Must be at least the
       MTU (minimum 1280 for IPv6 compliance, maximum 1500).
   * - :kconfig:option:`CONFIG_QUIC_CRYPTO_RX_BUFFER_SIZE`
     - 4096
     - Shared CRYPTO frame reassembly buffer per endpoint, used during the
       TLS handshake.  Since QUIC progresses sequentially through encryption
       levels (Initial → Handshake → Application), a single buffer is reused
       across all levels.  4096 bytes is the minimum for interoperability
       with browsers: Chrome and Firefox may split a ClientHello across 10 or
       more CRYPTO frame fragments totalling up to ~4 KiB.  Reduce to 2048
       for embedded-to-embedded deployments with small certificates.
       Range: 1024–8192.
   * - :kconfig:option:`CONFIG_QUIC_CRYPTO_OOO_SLOTS`
     - 8
     - Number of out-of-order CRYPTO frame metadata slots per endpoint.
       Each slot (approximately 8 bytes) records the offset and length of one
       out-of-order fragment until the gap before it is filled.  8 slots
       are needed for browser interoperability; reduce to 4 for
       embedded-only deployments.  Range: 4–16.
   * - :kconfig:option:`CONFIG_QUIC_STREAM_TX_BUFFER_SIZE`
     - 8192
     - Per-stream buffer holding sent but unacknowledged data.  The QUIC
       sender cannot advance beyond the unacknowledged window, so this
       is the primary throughput-limiting parameter on high-latency links.
       Size to at least the bandwidth-delay product:
       ``throughput_bytes_per_ms × rtt_ms``.
   * - :kconfig:option:`CONFIG_QUIC_STREAM_RX_BUFFER_SIZE`
     - =QUIC_STREAM_TX_BUFFER_SIZE
     - Per-stream receive buffer.  Defaults to the TX buffer size, which
       is optimal for symmetric request/response patterns.  Can be
       reduced independently for asymmetric workloads (e.g. download-only
       streams).
   * - :kconfig:option:`CONFIG_QUIC_STREAM_OOO_SLOTS`
     - 4
     - Number of out-of-order STREAM frame slots per stream.  Each slot
       holds up to :kconfig:option:`CONFIG_QUIC_STREAM_OOO_SEG_SIZE` bytes.
       Set to 2 on loss-free local networks; increase to 8 or more on
       WAN links with significant reordering.
   * - :kconfig:option:`CONFIG_QUIC_STREAM_OOO_SEG_SIZE`
     - 1280
     - Maximum size of one buffered out-of-order segment.  Setting this
       to the MTU avoids fragmentation of incoming frames.  Can be
       reduced to 512 bytes when OOO delivery is uncommon.
   * - :kconfig:option:`CONFIG_QUIC_SENT_PKT_HISTORY_SIZE`
     - 64
     - Ring buffer depth of recently sent packet records.  Each entry
       is approximately 24 bytes.  Must be large enough to cover all
       in-flight packets simultaneously: at least
       ``ceil(STREAM_TX_BUFFER_SIZE / TX_BUFFER_SIZE) × 2``.
       Increase to 128–256 on lossy or high-latency WAN links.
   * - :kconfig:option:`CONFIG_QUIC_TLS_TRANSCRIPT_BUF_LEN`
     - 4096
     - Per-connection buffer accumulating the TLS 1.3 handshake transcript
       used for ``Finished`` MAC computation and HKDF key derivation.
       4096 bytes covers typical TLS 1.3 handshakes with RSA-2048 and
       EC P-256 certificates.
       Increase to 6144–8192 for longer certificate chains or RSA-4096.
       Total RAM consumed equals this value multiplied by
       :kconfig:option:`CONFIG_QUIC_MAX_CONTEXTS`.


Timeout Options
---------------

.. list-table::
   :widths: 44 12 44
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - :kconfig:option:`CONFIG_QUIC_MAX_IDLE_TIMEOUT`
     - 30000
     - Milliseconds of inactivity after which the connection is silently
       closed.  Both endpoints advertise their value; the smaller of the
       two takes effect.  Set to 0 to disable idle timeout entirely.
       Recommended values: 5000 ms (loopback), 10000 ms (LAN),
       30000 ms (WAN).
   * - :kconfig:option:`CONFIG_QUIC_CONNECT_TIMEOUT`
     - 3000
     - Milliseconds to wait for the handshake to complete before the
       ``connect()`` call fails.  Should be at least 10× the expected
       RTT to accommodate packet loss during the handshake.
   * - :kconfig:option:`CONFIG_QUIC_MAX_PTO_TIMEOUT_MS`
     - 10000
     - Upper bound on the exponential PTO backoff in milliseconds.
       The PTO timer doubles on each consecutive probe timeout:
       ``PTO_n = PTO_base × 2^n``.  This cap prevents excessively long
       retransmission delays.  Should not exceed
       :kconfig:option:`CONFIG_QUIC_MAX_IDLE_TIMEOUT`.

Service Thread Options
----------------------

.. list-table::
   :widths: 44 12 44
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - :kconfig:option:`CONFIG_QUIC_SERVICE_THREAD_PRIO`
     - NUM_PREEMPT_PRIORITIES
     - Thread priority of the QUIC service dispatcher.  Values ≥ 0 are
       preemptive (0 = highest); values < 0 are cooperative.
   * - :kconfig:option:`CONFIG_QUIC_SERVICE_STACK_SIZE`
     - 4096
     - Stack size in bytes for the QUIC service thread.  4096 bytes is the
       default and is sufficient for mbedTLS handshake operations.  Reduce
       only if RAM is extremely constrained and profiling confirms the stack
       headroom is not needed.
   * - :kconfig:option:`CONFIG_QUIC_PKT_COUNT`
     - =QUIC_MAX_ENDPOINTS
     - Number of simultaneous pending packet receive operations.  Defaults
       to the number of endpoints so that one packet per endpoint can be
       in flight concurrently.  Increase (e.g. 2× endpoints) for
       higher-throughput scenarios.


Memory Model
************

The table below shows how each RAM region scales with the configuration
parameters.  All sizes are in bytes unless noted.

.. list-table::
   :widths: 36 64
   :header-rows: 1

   * - Memory Region
     - Formula
   * - Endpoint RX staging
     - ``QUIC_MAX_ENDPOINTS × QUIC_PKT_COUNT × QUIC_ENDPOINT_PENDING_DATA_LEN``
   * - Endpoint TX buffers
     - ``QUIC_MAX_ENDPOINTS × QUIC_TX_BUFFER_SIZE``
   * - Crypto RX buffers
     - ``QUIC_MAX_ENDPOINTS × QUIC_CRYPTO_RX_BUFFER_SIZE``
   * - Crypto OOO slots
     - ``QUIC_MAX_ENDPOINTS × QUIC_CRYPTO_OOO_SLOTS × 8``
   * - Stream TX buffers
     - ``(QUIC_MAX_CONTEXTS × (QUIC_MAX_STREAMS_BIDI + QUIC_MAX_STREAMS_UNI)) × QUIC_STREAM_TX_BUFFER_SIZE``
   * - Stream RX buffers
     - ``(QUIC_MAX_CONTEXTS × (QUIC_MAX_STREAMS_BIDI + QUIC_MAX_STREAMS_UNI)) × QUIC_STREAM_RX_BUFFER_SIZE``
   * - Stream OOO buffers
     - ``total_streams × QUIC_STREAM_OOO_SLOTS × QUIC_STREAM_OOO_SEG_SIZE``
   * - Sent-packet history
     - ``QUIC_MAX_CONTEXTS × QUIC_SENT_PKT_HISTORY_SIZE × 24``
   * - TLS transcript buffers
     - ``QUIC_MAX_CONTEXTS × QUIC_TLS_TRANSCRIPT_BUF_LEN``
   * - TLS context (mbedTLS)
     - ~8192 × ``QUIC_MAX_CONTEXTS`` (estimated; depends on ciphersuites)
   * - Connection state
     - ~512 × ``QUIC_MAX_CONTEXTS``
   * - Stream state
     - ~128 × ``total_streams``

**Total estimated QUIC RAM** (rough minimum for 1 connection, 3 bidi streams,
1500-byte stream buffers, LAN client defaults):

.. code-block:: none

   Endpoint RX staging :  2 × 1 × 1500 = 3 000 B
   Endpoint TX         :  2 × 1500      = 3 000 B
   Crypto RX buffers   :  2 × 2048      = 4 096 B  (embedded client, LAN)
   Crypto OOO slots    :  2 × 4 × 8     =   256 B
   Stream TX           :  3 × 1500      = 4 500 B
   Stream RX           :  3 × 1500      = 4 500 B
   OOO buffers         :  3 × 2 × 512   = 3 072 B
   Sent-pkt history    :  1 × 16 × 24   =   384 B
   TLS transcript      :  1 × 4096      = 4 096 B
   TLS context         :  1 × 8192      = 8 192 B
   State overhead      :  512 + 3×128   =   896 B
   ─────────────────────────────────────────────
   Approximate total                   ≈ 35 992 B (~35 KiB)

The dominant cost at low stream counts is the mbedTLS context per connection.
At higher stream counts, the stream TX/RX buffers become dominant.


Kconfig Optimizer
*****************

Choosing buffer sizes that are both memory-efficient and able to sustain
target throughput requires balancing several interdependent parameters.  To
assist with this, Zephyr provides a Python optimizer script:

.. code-block:: none

   scripts/net/quic-kconfig-optimizer.py

The script accepts a description of the deployment scenario — connection
counts, MTU, network type, expected RTT, and a RAM budget — and outputs a
complete set of recommended ``CONFIG_`` values together with an estimated
RAM breakdown and a ready-to-paste ``prj.conf`` snippet.

Prerequisites
-------------

The script requires Python 3.8 or later with no additional dependencies
beyond the standard library.

.. code-block:: bash

   python3 --version   # must be 3.8+

Usage: Interactive Mode
-----------------------

Run the script without arguments to enter the guided interactive mode.
Each prompt shows its default in square brackets; press :kbd:`Enter` to
accept it.

.. code-block:: bash

   python3 scripts/net/quic-kconfig-optimizer.py

Example session:

.. code-block:: none

   ============================================================
     Zephyr QUIC Kconfig Optimizer -- Interactive Mode
   ============================================================
     Press Enter to accept [default] values.

   Max QUIC connections (QUIC_MAX_CONTEXTS) [1]: 2
   Bidirectional streams per connection [3]:
   Unidirectional streams per connection [0]:
   Network MTU in bytes [1500]:
   RAM budget in bytes (0 = no limit) [0]: 131072
   Device role [client] (client/server/both):
   IP version(s) [ipv4] (ipv4/ipv6/both):
   Expected RTT in ms [10]: 50
   Expected packet loss rate (%) [0.0]:
   Typical application message size (bytes) [1024]: 4096
   Expect out-of-order delivery? [no] (yes/no):
   Network type [lan] (lan/wan/loopback):

Usage: Command-Line Mode
------------------------

All parameters can be supplied on the command line.  When every required
parameter is present, the script skips interactive prompts entirely, which
makes it suitable for CI pipelines and scripted board bringup.

.. code-block:: bash

   python3 scripts/net/quic-kconfig-optimizer.py \
       --contexts        2          \
       --streams-bidi    3          \
       --streams-uni     0          \
       --mtu             1500       \
       --ram-budget      131072     \
       --role            client     \
       --ip-version      ipv4       \
       --rtt-ms          50         \
       --loss-rate       0.0        \
       --app-message-size 4096      \
       --ooo-expected    no         \
       --network-type    lan

If only some parameters are provided, the script falls back to interactive
mode for the missing ones.

.. code-block:: bash

   # Pre-fill the constraints, fill the rest interactively
   python3 scripts/net/quic-kconfig-optimizer.py \
       --contexts 1 --streams-bidi 3 --ram-budget 65536

Input Parameters
----------------

.. list-table::
   :widths: 28 12 12 48
   :header-rows: 1

   * - Parameter
     - CLI flag
     - Default
     - Description
   * - Max connections
     - ``--contexts``
     - 1
     - Maps directly to :kconfig:option:`CONFIG_QUIC_MAX_CONTEXTS`.
       Memory for TLS contexts, connection state, and sent-packet history
       scales linearly with this value.
   * - Bidi streams
     - ``--streams-bidi``
     - 3
     - Bidirectional streams per connection.  Stream TX/RX buffers are
       allocated for every stream across every connection, so
       ``contexts × streams_bidi × 2 × stream_buf`` bytes are reserved
       at boot.
   * - Uni streams
     - ``--streams-uni``
     - 0
     - Unidirectional streams per connection.  Set to 0 if not needed to
       avoid allocating the associated receive buffers.
   * - MTU
     - ``--mtu``
     - 1500
     - Network maximum transmission unit in bytes.  Sets the floor for
       :kconfig:option:`CONFIG_QUIC_TX_BUFFER_SIZE` and
       :kconfig:option:`CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN`.
       Use 1280 for networks where IPv6 minimum MTU compliance is required
       and no Path MTU Discovery is available.
   * - RAM budget
     - ``--ram-budget``
     - 0 (unlimited)
     - Hard upper bound on estimated QUIC RAM usage in bytes.  The script
       aborts with an error if the computed allocation exceeds this value.
       Set to 0 to skip the budget check and simply report the estimate.
   * - Device role
     - ``--role``
     - ``client``
     - ``client``, ``server``, or ``both``.  Affects the recommended
       number of endpoints: servers and dual-stack devices typically
       need one extra endpoint.
   * - IP version
     - ``--ip-version``
     - ``ipv4``
     - ``ipv4``, ``ipv6``, or ``both``.  When ``both`` is selected and
       :kconfig:option:`CONFIG_QUIC_ENDPOINT_USE_IPV4_MAPPING_TO_IPV6` is
       not enabled, an additional endpoint is needed.
   * - Expected RTT
     - ``--rtt-ms``
     - 10
     - Round-trip time in milliseconds between the device and the remote
       peer.  Used to size :kconfig:option:`CONFIG_QUIC_CONNECT_TIMEOUT`,
       :kconfig:option:`CONFIG_QUIC_MAX_PTO_TIMEOUT_MS`, and the
       window-update threshold.
   * - Packet loss rate
     - ``--loss-rate``
     - 0.0
     - Expected packet loss as a percentage (0–100).  Loss ≥ 2 % doubles
       stream buffer sizes to keep the pipeline full; loss ≥ 5 % forces
       the sent-packet history to at least 128 entries for effective
       loss detection.
   * - Message size
     - ``--app-message-size``
     - 1024
     - Typical application payload in bytes.  Stream buffers are sized to
       hold at least 2× this value so that one complete message can be
       in transit while the previous one is being acknowledged.
   * - OOO expected
     - ``--ooo-expected``
     - ``no``
     - Whether out-of-order packet delivery is expected (``yes``/``no``).
       When ``no``, OOO slots are reduced to 2 and segment size to 512 B,
       saving ``total_streams × (default_slots − 2) × seg_size`` bytes.
   * - Network type
     - ``--network-type``
     - ``lan``
     - ``lan``, ``wan``, or ``loopback``.  Controls idle timeout, PTO
       ceiling, and whether stream buffers are doubled to fill the
       larger bandwidth-delay product of a WAN link.

Output Sections
---------------

The optimizer prints five sections.

**Input Parameters** — echoes all supplied values for verification.

**Recommended Kconfig Values** — the computed ``CONFIG_*`` values grouped
by category (connection counts, transport parameters, buffer sizes, timeouts).

**Estimated RAM Usage** — a table showing each memory region, its size, and
a proportional ASCII bar chart.  If a RAM budget was given, the utilisation
percentage is shown.

**prj.conf Snippet** — a copy-pasteable block of Kconfig options
ready to drop into a ``prj.conf`` or board-specific ``.conf`` overlay.

**Additional Notes** — deployment-specific warnings and suggestions derived
from the input, for example if the service stack is likely to be too small,
or if the connection-level flow-control window may become a bottleneck.

Example Output
--------------

The following shows the output for a minimal client configuration: one
connection, three bidirectional streams, Ethernet LAN, 10 ms RTT, 1 KiB
messages, no OOO.

.. code-block:: none

   ========================================================================
            QUIC Kconfig Optimizer -- Recommended Settings
   ========================================================================

   ------------------------------------------------------------------------
     INPUT PARAMETERS
   ------------------------------------------------------------------------
     Max connections (QUIC_MAX_CONTEXTS)            1
     Bidi streams per connection                    3
     Uni  streams per connection                    0
     MTU (bytes)                                    1500
     RAM budget                                     unlimited
     Device role                                    client
     IP version                                     ipv4
     Expected RTT (ms)                              10
     Expected packet loss (%)                       0.0
     Typical message size (bytes)                   1024
     Out-of-order delivery expected                 no
     Network type                                   lan

   ------------------------------------------------------------------------
     RECOMMENDED Kconfig VALUES
   ------------------------------------------------------------------------

     [Connection / stream counts]
       CONFIG_QUIC_MAX_CONTEXTS                              1
       CONFIG_QUIC_MAX_STREAMS_BIDI                         3
       CONFIG_QUIC_MAX_STREAMS_UNI                          0
       CONFIG_QUIC_MAX_ENDPOINTS                            2
       CONFIG_QUIC_PKT_COUNT                                2

     [QUIC transport parameters (flow control)]
       CONFIG_QUIC_INITIAL_MAX_DATA                         6144
       CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL       2048
       CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE      2048
       CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_UNI              16384
       CONFIG_QUIC_INITIAL_MAX_STREAMS_BIDI                 3
       CONFIG_QUIC_INITIAL_MAX_STREAMS_UNI                  0
       CONFIG_QUIC_STREAM_RX_WINDOW_UPDATE_THRESHOLD        25

     [Buffer sizes (RAM)]
       CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN                1500
       CONFIG_QUIC_TX_BUFFER_SIZE                           1500
       CONFIG_QUIC_CRYPTO_RX_BUFFER_SIZE                    2048
       CONFIG_QUIC_CRYPTO_OOO_SLOTS                         4
       CONFIG_QUIC_STREAM_TX_BUFFER_SIZE                    2048
       CONFIG_QUIC_STREAM_RX_BUFFER_SIZE                    2048
       CONFIG_QUIC_STREAM_OOO_SLOTS                         2
       CONFIG_QUIC_STREAM_OOO_SEG_SIZE                      512
       CONFIG_QUIC_SENT_PKT_HISTORY_SIZE                    16
       CONFIG_QUIC_TLS_TRANSCRIPT_BUF_LEN                   4096

     [Timeouts]
       CONFIG_QUIC_MAX_IDLE_TIMEOUT                         10000
       CONFIG_QUIC_CONNECT_TIMEOUT                          3000
       CONFIG_QUIC_MAX_PTO_TIMEOUT_MS                       5000

   ------------------------------------------------------------------------
     ESTIMATED RAM USAGE
   ------------------------------------------------------------------------
     Endpoint RX staging              9 KiB  ######
     Endpoint TX buffers              3 KiB  ##
     Crypto RX buffers                4 KiB  ###
     Crypto OOO slots                  64 B
     Stream TX buffers                6 KiB  ####
     Stream RX buffers                6 KiB  ####
     Stream OOO buffers               2 KiB  #
     Stream state overhead            0 KiB
     Sent-packet history              0 KiB
     TLS context overhead             8 KiB  #####
     Connection state overhead        0 KiB
     TLS transcript buffers           4 KiB  ##
     TOTAL                           42 KiB  ######################## <-- TOTAL

   ------------------------------------------------------------------------
     prj.conf SNIPPET
   ------------------------------------------------------------------------

   CONFIG_QUIC_MAX_CONTEXTS=1
   CONFIG_QUIC_MAX_STREAMS_BIDI=3
   ...

Minimal IoT sensor (client, IPv4 only, tight RAM budget)
--------------------------------------------------------

A sensor that opens one QUIC connection to a cloud endpoint, uses a single
bidirectional stream to report telemetry, and has 48 KiB available for QUIC:

.. code-block:: bash

   python3 scripts/net/quic-kconfig-optimizer.py \
       --contexts 1 --streams-bidi 1 --streams-uni 0 \
       --mtu 1280 --ram-budget 49152 \
       --role client --ip-version ipv4 \
       --rtt-ms 80 --loss-rate 0.5 \
       --app-message-size 256 \
       --ooo-expected no --network-type wan

The script will size stream buffers to 1500 B (minimum, since
``2 × 256 = 512 < 1500``), set OOO slots to 2, and keep the sent-packet
history at 16 entries — all chosen to minimise RAM while still handling
the expected 0.5 % loss on the WAN link.

Dual-stack server (IPv4 + IPv6, multiple clients)
-------------------------------------------------

An edge gateway accepting up to four simultaneous client connections over
both IPv4 and IPv6, with 256 KiB available for QUIC:

.. code-block:: bash

   python3 scripts/net/quic-kconfig-optimizer.py \
       --contexts 4 --streams-bidi 3 --streams-uni 1 \
       --mtu 1500 --ram-budget 262144 \
       --role server --ip-version both \
       --rtt-ms 5 --loss-rate 0.0 \
       --app-message-size 8192 \
       --ooo-expected no --network-type lan

The script will recommend three endpoints (two for the listening sockets,
one for each client family), and size stream buffers to 16 KiB (to hold
2 × 8192 B messages), which with 4 contexts and 4 streams each will
dominate the RAM budget.

High-throughput WAN link with packet loss
-----------------------------------------

A data aggregator connected over a cellular WAN with measurable packet
loss, requiring sustained throughput for firmware-update payloads:

.. code-block:: bash

   python3 scripts/net/quic-kconfig-optimizer.py \
       --contexts 1 --streams-bidi 2 --streams-uni 0 \
       --mtu 1400 --ram-budget 0 \
       --role client --ip-version ipv4 \
       --rtt-ms 150 --loss-rate 3.0 \
       --app-message-size 32768 \
       --ooo-expected yes --network-type wan

Here the script doubles stream buffers (loss ≥ 2 %) and selects a window
update threshold of 15 % (high-latency WAN) to keep the peer's send window
open, maximising pipeline utilisation.  It also sets OOO slots to 8 with
1280-byte segments to handle cellular reordering.

Tuning Guidelines
*****************

**Start with the optimizer defaults, then measure.**  The script's heuristics
are conservative.  Use the Zephyr network statistics (``CONFIG_NET_STATISTICS``)
to observe actual stream window stalls and OOO events before deciding to
increase buffer sizes.

**Stream buffers are the biggest RAM cost.**  On multi-connection devices,
``contexts × streams × 2 × stream_buf`` dominates.  Halving the stream buffer
size saves proportionally.  Only increase beyond the BDP if profiling shows
persistent sender stalls.

**The connection-level window must not be the bottleneck.**
:kconfig:option:`CONFIG_QUIC_INITIAL_MAX_DATA` must be greater than or equal
to the largest per-stream window; otherwise the connection cap limits
throughput even when individual stream windows are open.

**CRYPTO buffers scale with endpoints, not connections.**
:kconfig:option:`CONFIG_QUIC_CRYPTO_RX_BUFFER_SIZE` and
:kconfig:option:`CONFIG_QUIC_CRYPTO_OOO_SLOTS` are allocated once per
endpoint, so their RAM cost is ``QUIC_MAX_ENDPOINTS × buffer_size``.
The 4096-byte default is required for browser interoperability (Chrome
and Firefox fragment ClientHello into many small CRYPTO frames); reduce
to 2048 for embedded-only deployments where the peer is also a Zephyr
device.

**IPv4-mapped IPv6.**  When both IPv4 and IPv6 are needed, enabling
:kconfig:option:`CONFIG_QUIC_ENDPOINT_USE_IPV4_MAPPING_TO_IPV6` allows one
UDP socket to serve both address families, reducing
:kconfig:option:`CONFIG_QUIC_MAX_ENDPOINTS` by one and saving the
associated buffer RAM.

**Disable unused stream directions.**  If the application only uses
client-initiated streams, set
:kconfig:option:`CONFIG_QUIC_MAX_STREAMS_UNI` to 0.  The optimizer
accounts for this automatically.

**OOO memory is per-stream.**
``QUIC_STREAM_OOO_SLOTS × QUIC_STREAM_OOO_SEG_SIZE × total_streams``
can be significant.  On a reliable LAN, setting
:kconfig:option:`CONFIG_QUIC_STREAM_OOO_SLOTS` to 2 and
:kconfig:option:`CONFIG_QUIC_STREAM_OOO_SEG_SIZE` to 512 is safe and
saves several KiB per connection.


API Reference
*************

.. doxygengroup:: quic
