.. _stun_interface:

STUN
####

.. contents::
    :local:
    :depth: 2

Overview
********

Session Traversal Utilities for NAT (STUN, :rfc:`8489`) lets a device find out
which address and port it has on the outside of a NAT: it sends a Binding
request to a STUN server, and the server answers with the address the request
came from. STUN is also the message format that ICE (:rfc:`8445`) and TURN
(:rfc:`8656`) are built on.

The library is enabled with :kconfig:option:`CONFIG_STUN` and comes in layers,
each one usable without the ones above it:

.. list-table::
   :header-rows: 1
   :widths: 25 30 45

   * - Layer
     - Kconfig option
     - What it does
   * - Message codec
     - :kconfig:option:`CONFIG_STUN`
     - Parses and builds messages, with the attributes of ICE, and computes
       MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 and FINGERPRINT. No I/O, no
       state: every function works on buffers of the caller.
   * - Binding transaction
     - :kconfig:option:`CONFIG_STUN`
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

   STUN support is **experimental**: the API and the Kconfig options may change
   between releases.

Message integrity is computed through the PSA Crypto API; an application that
uses it calls ``psa_crypto_init()`` first.

Finding out the public address
******************************

:c:func:`stun_client_simple` resolves the server, opens a UDP socket, asks, and
closes the socket:

.. code-block:: c

   #include <zephyr/net/stun_client.h>

   struct net_sockaddr mapped;
   int rc;

   /* Port 0 is the default port of STUN, 3478; wait 5 seconds at most. */
   rc = stun_client_simple("stun.example.org", 0, 5000, &mapped);
   if (rc == 0) {
           /* mapped is the address the server saw the request come from */
   }

The address tells the public IP address of the device. The port belongs to the
socket of that one request, which is closed by the time the call returns. To
learn the address and port that peers can reach a socket of the application at,
ask from that very socket with :c:func:`stun_client_query`:

.. code-block:: c

   rc = stun_client_query(sock, &server, sizeof(struct net_sockaddr_in), 5000, &mapped);

For the duration of the call the socket belongs to the transaction: datagrams
that are not the answer of the server are received and dropped.

The request is retransmitted as :rfc:`8489` section 6.2.1 has it, after 0.5,
1.5, 3.5 seconds and so on, for 39.5 seconds in all unless the caller sets a
shorter time. A server given by host name needs
:kconfig:option:`CONFIG_DNS_RESOLVER`; a literal address does not. The
transaction id of a request has to be unpredictable, so the client depends on a
cryptographically secure random number generator.

The :zephyr:code-sample:`stun-client` sample shows the client in an
application.

Sharing the socket
******************

The Binding transaction is for a socket that carries other traffic too. The
application sends and receives; the transaction says what to send, when to come
back, and whether a datagram that arrived is its answer:

.. code-block:: c

   static struct stun_binding txn;

   /* Do what the transaction wants done now. */
   static void run(void)
   {
           struct stun_txn_step step;

           while (stun_binding_advance(&txn, k_uptime_get_32(), &step) == 0) {
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
           if (stun_binding_start(&txn, server, txid, "my-app", NULL,
                                  k_uptime_get_32(), &step) == 0) {
                   send_datagram(step.tx, step.tx_len);
                   run();
           }
   }

   /* For every datagram, before the other protocols on the socket see it. */
   bool on_datagram(const struct net_sockaddr *from, const uint8_t *buf, size_t len)
   {
           if (!stun_binding_owns(&txn, buf, len)) {
                   return false;
           }
           (void)stun_binding_on_datagram(&txn, from, buf, len);
           run();
           return true;
   }

A response is accepted only if it comes from the address the request went to
and carries the transaction id of the request; whatever else arrives is
counted in ``rx_rejected`` and changes nothing.

Parsing and building messages
*****************************

:c:func:`stun_is_message` tells a STUN message from the other protocols that
may share its port, and :c:func:`stun_parse` checks a message and finds the
attributes the codec understands. A message is built with a
``struct stun_builder``. This is how an agent answers a Binding request that
is protected with a short-term credential (:rfc:`8489` section 9.1.3): the
request is authenticated with MESSAGE-INTEGRITY-SHA256 when it carries that
attribute and with MESSAGE-INTEGRITY otherwise, and the response is protected
the same way the request was:

.. code-block:: c

   struct stun_builder b;
   struct stun_msg msg;
   uint8_t out[128];
   int len;

   if (!stun_is_message(buf, n) || stun_parse(buf, n, &msg) != 0) {
           return;
   }
   if (msg.type != STUN_BINDING_REQUEST) {
           return;
   }
   if (msg.has_integrity_sha256) {
           if (stun_verify_integrity_sha256(buf, n, &msg, key, key_len, 32) != 0) {
                   return;
           }
   } else if (stun_verify_integrity(buf, n, &msg, key, key_len) != 0) {
           return;
   }

   stun_builder_init(&b, out, sizeof(out), STUN_BINDING_SUCCESS_RESPONSE, msg.txid);
   if (stun_add_xor_mapped_address(&b, from) != 0) {
           return;
   }
   if (msg.has_integrity_sha256) {
           if (stun_add_integrity_sha256(&b, key, key_len) != 0 ||
               stun_add_fingerprint(&b) != 0) {
                   return;
           }
           len = stun_builder_len(&b);
   } else {
           len = stun_finish(&b, key, key_len);
   }
   if (len > 0) {
           send_datagram(out, len);
   }

The example drops a request it cannot authenticate. A server that answers
strangers sends the error responses of :rfc:`8489` section 9.1.3 instead
(400 when the credential is missing, 401 when it does not match); the codec
builds them with :c:func:`stun_add_error_code`. The test
``tests/net/lib/stun/src/example.c`` runs this very example against the rules
of section 9.1.3; keep the two the same.

A builder that has run out of room remembers it: every later call, and
:c:func:`stun_finish` at the end, returns the same error. An argument that is
refused is reported by the call that was given it and leaves the message as it
was.

The parser follows :rfc:`8489` to the letter where the protocol depends on it:

* The message has to fill the datagram: the header says how long it is.
* Of an attribute that appears more than once, the first one counts.
* Attributes after MESSAGE-INTEGRITY are not covered by it and are ignored,
  except MESSAGE-INTEGRITY-SHA256 and FINGERPRINT.
* FINGERPRINT, if present, has to be the last attribute.
* An attribute the codec understands that has a length it cannot have makes
  the message malformed.
* Comprehension-required attributes that nobody understands are reported by
  :c:func:`stun_unknown_attrs`, for the caller to reject the message as the
  RFC requires.

Integrity values are compared in constant time.

Logging
*******

A message that is refused gives its caller ``-EBADMSG`` whichever rule it broke;
the reason goes to the log. The level of the library is set with
:kconfig:option:`CONFIG_STUN_LOG_LEVEL_DBG` and its siblings, which
:kconfig:option:`CONFIG_NET_LOG` makes available; by default it is the level of
the system, :kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL`.

What the network makes the library do is logged as debug records only, so that
traffic cannot fill the log at the levels a product runs with. Error records
are for faults of the device itself: a PSA backend that fails, or a random
number generator that does. Keys, integrity values and the contents of
attributes are never logged.

What is not included
********************

* TURN (:rfc:`8656`) and ICE (:rfc:`8445`). The codec has the attributes of
  ICE, and :c:func:`stun_add_attr`, :c:func:`stun_find_attr` and
  :c:func:`stun_get_xor_address` handle those of other STUN usages.
* The long-term credential mechanism. The parser checks the form of REALM and
  NONCE; the key derivation, USERHASH and PASSWORD-ALGORITHMS are left to the
  caller.
* STUN over TCP and TLS.
* Demultiplexing of STUN, DTLS and RTP on one socket (:rfc:`7983`);
  :c:func:`stun_is_message` is the STUN part of it.

Shell commands
**************

With :kconfig:option:`CONFIG_STUN_SHELL` the :ref:`network shell <net_shell>`
has two more commands:

.. code-block:: console

   uart:~$ net stun server stun.example.org
   STUN server: stun.example.org port 3478
   uart:~$ net stun query
   Mapped address: 203.0.113.7:54321

``net stun server`` takes a host name or address and, optionally, a port;
without arguments it shows the server that is set, and an empty name (``""``)
unsets it. The server the commands start with is
:kconfig:option:`CONFIG_STUN_SHELL_SERVER`, empty by default.
``net stun query`` takes the longest time to wait, in milliseconds, and waits
5 seconds if it is not given.

API Reference
*************

.. doxygengroup:: stun

.. doxygengroup:: stun_client
