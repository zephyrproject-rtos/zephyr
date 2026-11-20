.. _stun_interface:

STUN
####

.. contents::
    :local:
    :depth: 2

Overview
********

Session Traversal Utilities for NAT (STUN, :rfc:`8489`) is a protocol that lets
a device learn which address and port it has on the outside of a NAT: it sends
a Binding request to a server, and the server answers with the address the
request came from. Interactive Connectivity Establishment (ICE, :rfc:`8445`)
and TURN (:rfc:`8656`) use the same messages.

The base of the STUN library is a message codec. It parses received messages
and builds messages to send, with the attributes that ICE adds, and it computes
and verifies the MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 and FINGERPRINT
attributes. The codec does no I/O and keeps no state: every function works on
buffers of the caller. An application that uses the codec alone owns the
socket, generates the transaction ids and retransmits its requests.

The library comes in layers. The codec is usable alone; the Binding transaction
and the socket client are enabled together:

.. list-table::
   :header-rows: 1
   :widths: 25 30 45

   * - Layer
     - Kconfig option
     - What it does
   * - Message codec
     - :kconfig:option:`CONFIG_STUN`
     - Parses and builds messages, with the attributes of ICE, and computes
       MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 and FINGERPRINT.
   * - Binding transaction
     - :kconfig:option:`CONFIG_STUN_CLIENT`
     - One Binding request, its retransmissions and the response. No I/O and
       no clock either: the caller sends what it is told to send, feeds in what
       it receives and says what time it is.
   * - Socket client
     - :kconfig:option:`CONFIG_STUN_CLIENT`
     - Blocking calls that run a transaction over a UDP socket and return the
       address the server saw.
   * - Shell commands
     - :kconfig:option:`CONFIG_STUN_SHELL`
     - ``net stun server`` and ``net stun query``.

An application that only wants to know its public address uses the socket
client. A protocol that shares its socket with STUN, as ICE does, uses the
transaction and the codec and does the I/O itself.

.. note::

   This library is marked :ref:`experimental <api_lifecycle_experimental>`. The
   API and the Kconfig options may change between releases.

Configuration
*************

Enable the library with :kconfig:option:`CONFIG_STUN`; it selects what it
needs, so that a minimal ``prj.conf`` is:

.. code-block:: kconfig

   CONFIG_NETWORKING=y
   CONFIG_STUN=y

Message integrity
   :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`,
   :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HMAC`,
   :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (MESSAGE-INTEGRITY) and
   :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` (MESSAGE-INTEGRITY-SHA256),
   all on by default. The library initializes the PSA Crypto API itself.

Address families
   :kconfig:option:`CONFIG_NET_IPV6` for IPv6 addresses. Without it the address
   functions return ``-EAFNOSUPPORT`` for such an address, and the message
   still parses.

Binding transaction and socket client
   :kconfig:option:`CONFIG_STUN_CLIENT`, which needs
   :kconfig:option:`CONFIG_NET_SOCKETS` and :kconfig:option:`CONFIG_NET_UDP`.
   The transaction id of a request has to be unpredictable, so the client
   depends on a cryptographically secure random number generator. A server
   given by host name needs :kconfig:option:`CONFIG_DNS_RESOLVER`; a literal
   address does not. :kconfig:option:`CONFIG_STUN_CLIENT_SOFTWARE` is what the
   requests carry in SOFTWARE.

Retransmission schedule
   The parameters of :rfc:`8489` section 6.2.1 for every Binding transaction of
   the build: :kconfig:option:`CONFIG_STUN_BINDING_RTO_MS`, the initial
   retransmission timeout (500 ms by default, 500 to 60000),
   :kconfig:option:`CONFIG_STUN_BINDING_MAX_TRANSMITS`, Rc, the number of
   times the request is sent (7 by default, 1 to 16), and
   :kconfig:option:`CONFIG_STUN_BINDING_FINAL_WAIT_RTO`, Rm, the wait after
   the last transmission in initial timeouts (16 by default, 1 to 255).

Shell commands
   :kconfig:option:`CONFIG_STUN_SHELL`, with
   :kconfig:option:`CONFIG_STUN_CLIENT` and :kconfig:option:`CONFIG_NET_SHELL`.

Logging
   :kconfig:option:`CONFIG_STUN_LOG_LEVEL_DBG` and its siblings, available
   under :kconfig:option:`CONFIG_NET_LOG`. Why a received message was refused
   is logged at the debug level only, so that traffic from the network cannot
   fill the log. Errors are logged for failures of the PSA Crypto backend and of
   the random number generator the client takes its transaction ids from. Keys
   and integrity values are never logged.

Usage
*****

Finding out the public address
==============================

:c:func:`stun_client_simple` resolves the server, opens a UDP socket, connects
it to the server, so that the socket receives only the datagrams of the server,
asks, and closes the socket:

.. code-block:: c

   #include <zephyr/net/stun_client.h>

   struct net_sockaddr_storage mapped;
   int rc;

   /* Port 0 is the default port of STUN, 3478; wait 5 seconds at most. */
   rc = stun_client_simple("stun.example.org", 0, 5000, &mapped);
   if (rc == 0) {
           /* mapped is the address the server saw the request come from */
   }

A name that resolves to several addresses, an IPv4 and an IPv6 one for example,
is asked at one address after another until one of them answers. The time is
for the whole call: each address gets an equal part of what is left of it, so an
address that fails at once leaves its part to the others. With no time given,
0, each address is asked for as long as the retransmission schedule lasts.

The address tells the public IP address of the device. The port belongs to the
socket of that one request, which is closed by the time the call returns. To
learn the address and port that peers can reach a socket of the application at,
ask from that very socket with :c:func:`stun_client_query`:

.. code-block:: c

   rc = stun_client_query(sock, &server, sizeof(struct net_sockaddr_in), 5000,
                          &mapped);

For the duration of the call the socket belongs to the transaction: datagrams
that are not the answer of the server are received and dropped.

The request is retransmitted as :rfc:`8489` section 6.2.1 has it: with the
default configuration after 0.5, 1.5, 3.5 seconds and so on, for 39.5 seconds in
all unless the caller sets a shorter time.

The :zephyr:code-sample:`stun-client` sample shows the client in an
application.

Sharing the socket
==================

The Binding transaction is for a socket that carries other traffic too. The
application sends and receives; the transaction says what to send, when to come
back, and whether a datagram that arrived is its answer:

.. code-block:: c

   static struct stun_binding txn;

   /* Do what the transaction wants done now. */
   static void run(void)
   {
           struct stun_txn_step step;

           while (stun_binding_next_step(&txn, k_uptime_get_32(), &step) == 0) {
                   switch (step.action) {
                   case STUN_TXN_SEND:
                           send_datagram(step.tx, step.tx_len);
                           continue;
                   case STUN_TXN_WAIT:
                           /* run() is to be called again at that time */
                           set_timer(step.wait_until_ms);
                           return;
                   case STUN_TXN_DONE:
                           /* txn.mapped is the address the server saw */
                           return;
                   default:
                           /* STUN_TXN_FAILED: txn.fail says why */
                           return;
                   }
           }
   }

   void start(const struct net_sockaddr *server)
   {
           uint8_t txid[STUN_TXID_SIZE];
           struct stun_txn_step step;

           if (sys_csrand_get(txid, sizeof(txid)) != 0) {
                   return;
           }
           if (stun_binding_start(&txn, server, txid, "my-app", k_uptime_get_32(),
                                  &step) == 0) {
                   send_datagram(step.tx, step.tx_len);
                   run();
           }
   }

   /* For every datagram, before the other protocols on the socket see it. */
   bool on_datagram(const struct net_sockaddr *from, const uint8_t *buf, size_t len)
   {
           if (!stun_binding_is_response(&txn, buf, len)) {
                   return false;
           }
           (void)stun_binding_on_datagram(&txn, from, buf, len);
           run();
           return true;
   }

A response is accepted only if it comes from the address the request went to
and carries the transaction id of the request; whatever else arrives changes
nothing.

The library keeps nothing of a transaction outside its ``struct stun_binding``:
no timer and no reference to it. To give up a transaction that is in flight,
zero the structure; it is then ready for :c:func:`stun_binding_start` again.

Receiving a message
===================

:c:func:`stun_has_valid_header` tells a STUN message from the other protocols
that may share its port, such as DTLS and RTP (:rfc:`7983`).
:c:func:`stun_parse` then checks the whole message and fills a
:c:struct:`stun_msg` with the header fields and the attributes the codec
understands. The structure refers to the buffer that was parsed, which has to
stay unchanged while the structure is in use: the functions that take the
structure read the message from it. Finish with the structure before the buffer
receives the next datagram or a response is built in it.

A received message is processed in the order of :rfc:`8489` section 6.3:

#. :c:func:`stun_parse` refuses a malformed message. Its description lists the
   rules the parser applies.
#. :c:func:`stun_verify_fingerprint` checks FINGERPRINT, if the STUN usage
   requires the attribute.
#. :c:func:`stun_verify_integrity` or :c:func:`stun_verify_integrity_sha256`
   authenticates the message, if the STUN usage protects it. The parser cannot
   do it, because it has no key.
#. :c:func:`stun_unknown_attrs` reports the attributes the receiver is required
   to understand and does not.

An attribute of another STUN usage is read with :c:func:`stun_find_attr`. An
attribute in the XOR-MAPPED-ADDRESS format, such as XOR-PEER-ADDRESS of TURN, is
decoded with :c:func:`stun_get_xor_address`. One in the MAPPED-ADDRESS format,
such as ALTERNATE-SERVER, is decoded by the application from the value
:c:func:`stun_find_attr` returns: the XOR function would decode it without
complaint and get the address wrong.

Building a message
==================

A message is built in a buffer of the caller with a :c:struct:`stun_builder`.
The attributes are added in the order of the calls, and the closing attributes
come last: MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256, then FINGERPRINT. This
is the Binding request of an ICE connectivity check (:rfc:`8445` section 7.2.4):

.. code-block:: c

   #include <zephyr/net/stun.h>
   #include <zephyr/random/random.h>

   struct stun_builder b;
   uint8_t txid[STUN_TXID_SIZE];
   uint8_t out[128];
   int len;

   /* The transaction id of a request has to be unpredictable. */
   if (sys_csrand_get(txid, sizeof(txid)) != 0) {
           return;
   }

   if (stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid) != 0 ||
       stun_add_username(&b, username) != 0 ||
       stun_add_priority(&b, priority) != 0 ||
       stun_add_ice_controlling(&b, tiebreaker) != 0) {
           return;
   }

   /* MESSAGE-INTEGRITY with the password of the peer, then FINGERPRINT. */
   len = stun_finish(&b, key, key_len);
   if (len > 0) {
           send_datagram(out, len);
   }

The example checks every return value, because the builder remembers only some
of the failures: :c:struct:`stun_builder` tells which. :c:func:`stun_finish`
signs with MESSAGE-INTEGRITY alone. A message with no integrity attribute, with
MESSAGE-INTEGRITY-SHA256 or with both is closed with :c:func:`stun_finish_plain`
instead, as in the next example.

Answering a request
===================

This is how an agent answers a Binding request that is protected with a
short-term credential (:rfc:`8489` section 9.1.3). USERNAME names the
credential; the key of the integrity check is its password, processed with the
OpaqueString profile (:rfc:`8265`) as :rfc:`8489` section 9.1.1 requires. The
example's ``lookup_password()`` stands for the credential store of the
application and returns the password in that form. The request is authenticated
with MESSAGE-INTEGRITY-SHA256 if it carries that attribute and with
MESSAGE-INTEGRITY otherwise, and the response is protected the same way the
request was:

.. code-block:: c

   struct stun_builder b;
   struct stun_msg msg;
   const uint8_t *key;
   size_t key_len;
   uint8_t out[128];
   int len;

   if (!stun_has_valid_header(buf, n) || stun_parse(buf, n, &msg) != 0) {
           return;
   }
   if (msg.type != STUN_BINDING_REQUEST) {
           return;
   }
   /* No USERNAME is a 400, a name the agent does not know is a 401. */
   if (msg.username == NULL ||
       lookup_password(msg.username, msg.username_len, &key, &key_len) != 0) {
           return;
   }
   if (msg.has_integrity_sha256) {
           if (stun_verify_integrity_sha256(&msg, key, key_len,
                                            STUN_INTEGRITY_SHA256_MAX_LEN) != 0) {
                   return;
           }
   } else if (stun_verify_integrity(&msg, key, key_len) != 0) {
           return;
   }
   /* Unknown attributes are looked at after the authentication. */
   if (stun_unknown_attrs(&msg, NULL, 0, NULL, 0) != 0) {
           return;
   }

   if (stun_builder_init(&b, out, sizeof(out), STUN_BINDING_SUCCESS_RESPONSE,
                         msg.txid) != 0 ||
       stun_add_xor_mapped_address(&b, from) != 0) {
           return;
   }
   if (msg.has_integrity_sha256) {
           if (stun_add_integrity_sha256(&b, key, key_len) != 0) {
                   return;
           }
           len = stun_finish_plain(&b);
   } else {
           len = stun_finish(&b, key, key_len);
   }
   if (len > 0) {
           send_datagram(out, len);
   }

The example drops a request it cannot process. A server that answers strangers
sends an error response instead: 400 (Bad Request) if USERNAME or the integrity
attribute is missing, 401 (Unauthenticated) if the name is unknown or the HMAC
does not match, and 420 (Unknown Attribute) with UNKNOWN-ATTRIBUTES if
:c:func:`stun_unknown_attrs` reported any. The checks run in this order, so that
a request which fails more than one of them gets the first of these codes. The
codec builds the responses with :c:func:`stun_add_error_code` and
:c:func:`stun_add_unknown_attributes`.

Shell commands
==============

With :kconfig:option:`CONFIG_STUN_SHELL` the :ref:`network shell <net_shell>`
has two more commands:

.. code-block:: console

   uart:~$ net stun server stun.example.org
   STUN server: stun.example.org port 3478
   uart:~$ net stun query
   Mapped address: 203.0.113.7:54321

``net stun server`` takes a host name or address and, optionally, a port;
without arguments it shows the server that is set, and an empty name (``""``)
unsets it. The commands start with no server set.
``net stun query`` takes the longest time to wait, in milliseconds, and waits
5 seconds if it is not given; a server name that resolves to several addresses
has that time shared between them. With ``0`` each address is asked for as long
as the request is retransmitted, with the default configuration 39.5 seconds if
it does not answer.

Limitations
***********

The following is not part of the library:

* Transactions other than a Binding request without credentials. The
  transaction builds its request with SOFTWARE and FINGERPRINT only; a request
  that carries USERNAME and MESSAGE-INTEGRITY, such as an ICE connectivity
  check, is built with the codec and retransmitted by the application.
* Retransmission parameters of a single transaction. The initial timeout, Rc
  and Rm are set with Kconfig options only, for all the transactions of the
  build; a transaction cannot be started with its own, and the timeout is
  neither measured nor remembered per server.
* ICE and TURN. The codec has the attributes of ICE; :c:func:`stun_add_attr` and
  :c:func:`stun_find_attr` carry those of other STUN usages as raw values, and
  :c:func:`stun_add_xor_address` and :c:func:`stun_get_xor_address` the ones in
  the XOR-MAPPED-ADDRESS format.
* The long-term credential mechanism. The parser limits the length of REALM and
  NONCE; the key derivation, USERHASH and PASSWORD-ALGORITHM are left to the
  application.
* The OpaqueString profile (:rfc:`8265`), which USERNAME and the password of a
  short-term credential have to be processed with before they are given to the
  library. ICE credentials are ASCII and need no processing.
* The framing of STUN over TCP and TLS.

Testing
*******

The tests under :zephyr_file:`tests/net/lib/stun/` check the codec against the
sample messages of :rfc:`5769` and of :rfc:`8489` Appendix B.1, and the Binding
transaction against the retransmission schedule of :rfc:`8489` section 6.2.1.
The tests under :zephyr_file:`tests/net/lib/stun_client/` run the socket client
and the shell commands against a server on the loopback interface.

API Reference
*************

.. doxygengroup:: stun

.. doxygengroup:: stun_client
