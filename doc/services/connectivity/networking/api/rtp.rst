.. _rtp_interface:

RTP and SRTP
############

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

Secure RTP
**********

Secure RTP (SRTP, :rfc:`3711`) adds confidentiality, message authentication,
and replay protection to RTP packets. Zephyr implements SRTP natively under
:kconfig:option:`CONFIG_SRTP`, on top of the PSA Crypto API, so hardware
crypto acceleration is used where a platform provides it.

Supported transforms:

* AES-128 in counter mode (AES-CM) with HMAC-SHA1 authentication using 80-bit
  or 32-bit tags, the :rfc:`3711` mandatory-to-implement suites.
* AES-128-GCM and AES-256-GCM (:rfc:`7714`), where authentication is part of
  the AEAD transform; AES-256-GCM session keys are derived with the
  AES_256_CM_PRF key derivation function (:rfc:`6188`).
* NULL cipher (authentication only), and NULL authentication with an explicit
  opt-in.

The implementation includes the AES-CM key derivation function with a
configurable key derivation rate, rollover counter tracking with packet index
estimation, a sliding replay protection window of
:kconfig:option:`CONFIG_SRTP_REPLAY_WINDOW_SIZE` packets, and enforcement of
the 2^48 packets-per-master-key limit.

Master keys are provisioned by the application through
:c:struct:`srtp_policy`. A master key identifier (MKI) of up to
:c:macro:`SRTP_MKI_MAX_LEN` bytes can be configured per stream; it is appended
to protected packets and matched on received ones. One master key per stream
is kept, so the MKI does not select among multiple keys and rekeying via MKI
is not supported. Key exchange mechanisms (DTLS-SRTP, SDES, MIKEY) are out of
scope, as is SRTCP (RTCP is not implemented in Zephyr).

To protect an RTP session, install SRTP state between initializing and
starting the session:

.. code-block:: c

   static struct srtp_session_ctx srtp_ctx;

   static const struct srtp_policy policy = {
           .cipher = SRTP_CIPHER_AES_128_CM,
           .auth = SRTP_AUTH_HMAC_SHA1_80,
           .master_key = master_key,
           .master_key_len = SRTP_AES_128_KEY_LEN,
           .master_salt = master_salt,
           .master_salt_len = SRTP_MASTER_SALT_LEN,
   };

   rtp_session_init(&session, ...);
   rtp_session_set_srtp(&session, &policy, &policy, &srtp_ctx);
   rtp_session_start(&session);

Transmitted packets are protected with a stream keyed by the session's SSRC.
Receive streams are late-bound per source SSRC, and packets that fail
authentication or replay checks are dropped before the application callback
is invoked. See :c:func:`rtp_session_set_srtp` for the session lifecycle
rules and transport-specific constraints.

The SRTP core (:c:func:`srtp_protect` and :c:func:`srtp_unprotect`) operates
in place on serialized RTP packets and can also be used standalone, without an
RTP session.

.. note::

   Unprotecting packets runs in the network receive context; budget for the
   per-packet cipher and authentication cost there.

Samples
*******

* :zephyr:code-sample:`net-rtp`

API Reference
*************

.. doxygengroup:: rtp

.. doxygengroup:: srtp
