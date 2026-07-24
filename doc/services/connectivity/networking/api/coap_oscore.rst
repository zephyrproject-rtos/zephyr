.. _coap_oscore_interface:

OSCORE Support (RFC 8613)
#########################

.. contents::
    :local:
    :depth: 2


Overview
========

The Zephyr CoAP library provides support for Object Security for Constrained RESTful
Environments (OSCORE) as specified in :rfc:`8613`. OSCORE provides end-to-end protection
of CoAP messages using COSE (CBOR Object Signing and Encryption).

OSCORE protects CoAP messages at the application layer, providing:

1. **Confidentiality**: Message payloads and sensitive options are encrypted
2. **Integrity**: Messages are authenticated with a MAC
3. **Replay protection**: Sequence numbers prevent replay attacks
4. **Proxy-friendly**: Outer options remain visible for routing

Unlike DTLS, OSCORE provides end-to-end security that survives proxy translation
between different transport protocols (UDP, TCP, HTTP).

Additional OSCORE configuration options:

- :kconfig:option:`CONFIG_COAP_OSCORE_MAX_CONTEXTS`: Maximum number of OSCORE security contexts
- :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE`: Number of OSCORE exchanges to track per service
- :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`: Lifetime of tracked OSCORE exchanges used to protect
   deferred (separate) responses

Configuration
=============

Enable OSCORE support with :kconfig:option:`CONFIG_COAP_OSCORE`. This option depends
on the uoscore-uedhoc module and PSA Crypto support:

.. code-block:: kconfig

   CONFIG_COAP_OSCORE=y
   CONFIG_UOSCORE=y
   CONFIG_PSA_CRYPTO=y

The uoscore module automatically selects required PSA crypto algorithms (AES-CCM,
HKDF-SHA256, etc.).

Server Usage
============

To enable OSCORE on a CoAP service, define the service with
:c:macro:`COAP_SERVICE_DEFINE_OSCORE` (or :c:macro:`COAPS_SERVICE_DEFINE_OSCORE`
for DTLS). The macro statically allocates the per-service OSCORE exchange cache
and binds a *context provider* callback that returns the security context to
use. The context is created through the Zephyr OSCORE API; applications do not
include the underlying uoscore-uedhoc headers directly:

.. code-block:: c

   #include <zephyr/net/coap_oscore.h>
   #include <zephyr/net/coap_service.h>

   /* Key material must remain valid for the lifetime of the context. */
   static const uint8_t master_secret[16] = { /* ... */ };
   static const uint8_t master_salt[8] = { /* ... */ };
   static const uint8_t sender_id[] = { /* ... */ };
   static const uint8_t recipient_id[] = { /* ... */ };

   static struct coap_oscore_context *my_oscore_ctx;

   /* Provider invoked by the CoAP server when OSCORE processing is needed. */
   static struct coap_oscore_context *my_oscore_provider(void)
   {
       return my_oscore_ctx;
   }

   static uint16_t my_service_port = 5683;

   /* Second argument "true" requires OSCORE for all requests. */
   COAP_SERVICE_DEFINE_OSCORE(my_service, NULL, &my_service_port,
                              COAP_SERVICE_AUTOSTART, my_oscore_provider, true);

   int my_service_oscore_init(void)
   {
       struct coap_oscore_init_params params = {
           .master_secret = master_secret,
           .master_secret_len = sizeof(master_secret),
           .sender_id = sender_id,
           .sender_id_len = sizeof(sender_id),
           .recipient_id = recipient_id,
           .recipient_id_len = sizeof(recipient_id),
           .master_salt = master_salt,
           .master_salt_len = sizeof(master_salt),
           .aead_alg = COAP_OSCORE_AEAD_AES_CCM_16_64_128,
           .hkdf = COAP_OSCORE_HKDF_SHA_256,
           .fresh_master_secret_salt = true,
       };

       /* Derive the context once its key material is available. Until the
        * provider returns non-NULL, the service behaves as OSCORE-disabled.
        */
       return coap_oscore_context_init(&params, &my_oscore_ctx);
   }

The number of contexts that can be allocated at once is controlled by
:kconfig:option:`CONFIG_COAP_OSCORE_MAX_CONTEXTS`. Release a context with
``coap_oscore_context_free()`` once it is no longer referenced by any client or
service.

When a service is OSCORE-enabled (its context provider returns a context):

1. **Incoming requests**: The server automatically verifies and decrypts OSCORE-protected
   requests (RFC 8613 Section 8.2). Resource handlers receive decrypted CoAP messages
   with Inner options visible.

2. **Outgoing responses**: The server automatically OSCORE-protects responses and
   notifications that originate from an OSCORE exchange (RFC 8613 Section 8.3).
   Whether a given outgoing message must be protected is decided as follows:

   - **Synchronous responses** (produced while the request is being handled) are
      protected based on the OSCORE status of the request currently being
      processed. This decision is authoritative and does **not** depend on the
      exchange cache, so it can never be lost to cache expiry.
   - **Observe notifications** are protected based on the observer's stored OSCORE
      state, which lives for the duration of the observation.
   - **Deferred (separate) responses**, produced after the request handler has
      returned, are matched against the per-service exchange cache and protected
      when a matching entry is found. These entries expire after
      :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`. On a mixed service
      (OSCORE and non-OSCORE clients), sending a deferred response after the exchange
      cache entry has expired will result in a plaintext response.

3. **Error handling**: OSCORE verification errors are sent as simple CoAP responses
    **without** OSCORE processing (RFC 8613 Section 8.2):
    - COSE decode failure → 4.02 Bad Option
    - Security context not found → 4.01 Unauthorized
    - Decryption failure → 4.00 Bad Request

4. **Required OSCORE**: If the service is defined with the ``_oscore_required`` argument
    set to true, unprotected requests are rejected with 4.01 Unauthorized.

5. **Fail-closed behavior**: If OSCORE protection of a response fails, the server
   does not fall back to sending a plaintext response. On an OSCORE-required
   service, an outgoing response that cannot be matched to any OSCORE state is
   also dropped rather than sent in the clear. Synchronous responses and Observe
   notifications are never downgraded. On a mixed service the only case that can
   still yield an unprotected response is a deferred (separate) response produced
   after its exchange entry has expired (see item 2 and
   :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`).


Known Limitations
============================

1. **Single Context**: The current implementation does not support per-client OSCORE
   contexts within a service. Each service can use only one OSCORE context at a time,
   so all clients share that context. This is sufficient for many use cases, but it
   does not allow separate contexts per client.
2. **Context Reuse Across Reboots**: Only freshly derived security contexts are
   supported, i.e. contexts created with ``fresh_master_secret_salt = true`` where the
   Master Secret / Master Salt combination is unique at every boot (for example, derived
   via EDHOC). Applications that cannot guarantee a fresh context at every boot
   must not enable OSCORE for the time being.
3. **Reboot Replay Recovery (Echo)**: The Echo-based freshness exchange for restored
   contexts (RFC 8613 Appendix B.1.2, RFC 9175) is not implemented. When uoscore reports
   the first request after a reboot, the server rejects it with 4.01 Unauthorized instead
   of answering with an Echo challenge, so a client cannot automatically re-synchronize.
   This is fail-closed meaning no unprotected or replayed request is accepted but a
   reused or restored context cannot recover after a reboot without re-establishing the
   context out of band.

Security Context Derivation
============================

OSCORE security contexts are derived from a small set of parameters (RFC 8613 Section 3):

**Required parameters**:

- **Master Secret**: Shared secret (typically 16 bytes for AES-CCM-16-64-128)
- **Sender ID**: Unique identifier for the sender
- **Recipient ID**: Unique identifier for the recipient

**Optional parameters**:

- **Master Salt**: Additional entropy (recommended, typically 8 bytes)
- **ID Context**: Additional context identifier
- **AEAD Algorithm**: Defaults to AES-CCM-16-64-128
- **KDF**: Defaults to HKDF-SHA-256

These parameters are typically established through:

1. **Pre-shared keys**: Configured at device provisioning
2. **EDHOC**: Ephemeral Diffie-Hellman Over COSE (see uoscore-uedhoc module)

Security Considerations
=======================

1. **Sequence number overflow**: The sender sequence number (SSN) must not exceed 2^23-1
   for AES-CCM-16-64-128. The uoscore library enforces this limit.

2. **Master secret protection**: Master secrets must be stored securely (e.g., in
   secure storage or derived from EDHOC).

3. **Replay window**: The uoscore library maintains a replay window to detect and reject
   replayed requests. This does not protect against replays across reboots, so the SSN
   must be persisted to non-volatile memory if the same master secret is reused after a
   reboot.

4. **Fresh master secrets**: If master secrets are not re-derived after reboot (e.g.,
   using EDHOC), the sender sequence number must be persisted to non-volatile memory
   to prevent reuse.

Handling OSCORE When Not Supported
-----------------------------------

When OSCORE support is not enabled (:kconfig:option:`CONFIG_COAP_OSCORE` is not set),
the Zephyr CoAP stack implements fail-closed behavior for the OSCORE option per
RFC 7252 Section 5.4.1:

**Server behavior** (when ``CONFIG_COAP_OSCORE=n``):

- **CON requests** with OSCORE option: Returns **4.02 (Bad Option)** response
- **NON requests** with OSCORE option: Silently rejects (drops) the message
- **Responses** with OSCORE option: Sends RST for CON, silently drops NON/ACK

API Reference
=============

.. doxygengroup:: coap_oscore
