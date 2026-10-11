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

The STUN library is a message codec. It parses received messages and builds
messages to send, with the attributes that ICE adds, and it computes and
verifies the MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 and FINGERPRINT
attributes. The library does no I/O and keeps no state: every function works on
buffers of the caller. The application owns the socket, generates the
transaction ids and retransmits its requests.

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

Logging
   :kconfig:option:`CONFIG_STUN_LOG_LEVEL_DBG` and its siblings, available
   under :kconfig:option:`CONFIG_NET_LOG`. Why a received message was refused
   is logged at the debug level only, so that traffic from the network cannot
   fill the log. Errors are logged for failures of the PSA Crypto backend. Keys
   and integrity values are never logged.

Usage
*****

Receiving a message
===================

:c:func:`stun_has_valid_header` tells a STUN message from the other protocols
that may share its port, such as DTLS and RTP (:rfc:`7983`).
:c:func:`stun_parse` then checks the whole message and fills a
:c:struct:`stun_msg` with the header fields and the attributes the codec
understands. The structure refers to the buffer that was parsed, which has to
stay unchanged while the structure is in use.

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

   stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
   if (stun_add_username(&b, username) != 0 ||
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
           if (stun_verify_integrity_sha256(buf, n, &msg, key, key_len,
                                            STUN_INTEGRITY_SHA256_MAX_LEN) != 0) {
                   return;
           }
   } else if (stun_verify_integrity(buf, n, &msg, key, key_len) != 0) {
           return;
   }
   /* Unknown attributes are looked at after the authentication. */
   if (stun_unknown_attrs(buf, n, &msg, NULL, 0, NULL, 0) != 0) {
           return;
   }

   stun_builder_init(&b, out, sizeof(out), STUN_BINDING_SUCCESS_RESPONSE, msg.txid);
   if (stun_add_xor_mapped_address(&b, from) != 0) {
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

Limitations
***********

The following is not part of the library:

* Transport. The library does not send or receive datagrams, retransmit
  requests or match responses to them.
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
sample messages of :rfc:`5769` and of :rfc:`8489` Appendix B.1.

API Reference
*************

.. doxygengroup:: stun
