.. _coap_oscore_interface:

OSCORE Support (:rfc:`8613`)
############################

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
- :kconfig:option:`CONFIG_COAP_OSCORE_CONTEXT_REUSE`: Enable OSCORE support for context reuse across reboots
- :kconfig:option:`CONFIG_COAP_OSCORE_MASTER_SECRET_MAX_LEN`: Maximum OSCORE Master Secret length in bytes
- :kconfig:option:`CONFIG_COAP_OSCORE_MASTER_SALT_MAX_LEN`: Maximum OSCORE Master Salt length in bytes

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
for DTLS). The macro statically allocates the per-service OSCORE exchange cache.
Security contexts are created separately through the Zephyr OSCORE API and added
to a shared pool; applications do not include the underlying uoscore-uedhoc
headers directly. Incoming requests are matched to the correct context by their
Recipient ID and ID Context, so a single service can serve multiple clients, each
with its own context:

.. code-block:: c

   #include <zephyr/net/coap_oscore.h>
   #include <zephyr/net/coap_service.h>

   static struct coap_oscore_context *my_oscore_ctx;

   static uint16_t my_service_port = 5683;

   /* Final argument "true" requires OSCORE for all requests. */
   COAP_SERVICE_DEFINE_OSCORE(my_service, NULL, &my_service_port,
                              COAP_SERVICE_AUTOSTART, true);

   int my_service_oscore_init(void)
   {
       /* coap_oscore_context_add() copies the key material, so these
        * buffers need not outlive the call and can live on the stack.
        */
       const uint8_t master_secret[16] = { /* ... */ };
       const uint8_t master_salt[8] = { /* ... */ };
       const uint8_t sender_id[] = { /* ... */ };
       const uint8_t recipient_id[] = { /* ... */ };

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

       /* Add the context to the shared pool once its key material is
        * available. Call once per client identity (Recipient ID).
        */
       return coap_oscore_context_add(&params, &my_oscore_ctx);
   }

The number of contexts that can be allocated at once is controlled by
:kconfig:option:`CONFIG_COAP_OSCORE_MAX_CONTEXTS`. Release a context with
``coap_oscore_context_remove()``. Contexts are reference-counted: while an
in-flight request, an active exchange, or an observer still references a context,
``coap_oscore_context_remove()`` returns ``-EAGAIN`` and the context is retained.
Retry the removal once those references have been released.

When a service is OSCORE-enabled (created with an ``OSCORE`` macro and at least one context is added
to the pool):

1. **Incoming requests**: The server automatically verifies and decrypts OSCORE-protected
   requests (:rfc:`8613` Section 8.2). Resource handlers receive decrypted CoAP messages
   with Inner options visible.

2. **Outgoing responses**: The server automatically OSCORE-protects responses and
   notifications that originate from an OSCORE exchange (:rfc:`8613` Section 8.3).
   Whether a given outgoing message must be protected is decided as follows:

   - **Synchronous responses**, produced while the request is being handled, are matched
      against the per-service exchange cache and protected when a matching entry is found.
      These entries expire after :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`.
      On a mixed service (OSCORE and non-OSCORE clients), sending a synchronous response
      after the exchange cache entry has expired will result in a plaintext response.
   - **Observe notifications** are protected based on the observer's stored OSCORE
      state, which lives for the duration of the observation.
   - **Deferred (separate) responses**, produced after the request handler has
      returned, are matched against the per-service exchange cache and protected
      when a matching entry is found. These entries expire after
      :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`. On a mixed service
      (OSCORE and non-OSCORE clients), sending a deferred response after the exchange
      cache entry has expired will result in a plaintext response.

3. **Error handling**: OSCORE verification errors are sent as simple CoAP responses
    **without** OSCORE processing (:rfc:`8613` Section 8.2):
    - COSE decode failure → 4.02 Bad Option
    - Security context not found → 4.01 Unauthorized
    - Decryption failure → 4.00 Bad Request

4. **Required OSCORE**: If the service is defined with the ``_oscore_required`` argument
    set to true, unprotected requests are rejected with 4.01 Unauthorized.

5. **Fail-closed behavior**: If OSCORE protection of a response fails, the server
   does not fall back to sending a plaintext response. On an OSCORE-required
   service, an outgoing response that cannot be matched to any OSCORE state is
   also dropped rather than sent in the clear. Observe notifications are never
   downgraded. On a mixed service, any response (synchronous or deferred) whose
   exchange cache entry has expired can no longer be matched and is sent unprotected.
   In practice this affects deferred (separate) responses, because a synchronous
   response is matched against an entry that was just created while the request was
   handled (see item 2 and :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`).


Known Limitations
============================

1. **Mixed-Service Expired-Exchange Plaintext**: On a mixed service (one that serves
   both OSCORE and non-OSCORE clients), a response whose exchange cache entry has
   expired can no longer be matched to its OSCORE state and is sent as plaintext. This
   affects both synchronous and deferred (separate) responses (see
   :kconfig:option:`CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS`). For any service that
   carries sensitive data, define it as OSCORE-required (pass the ``_oscore_required``
   argument as true, e.g. via :c:macro:`COAP_SERVICE_DEFINE_OSCORE`) instead of running
   a mixed service. An OSCORE-required service rejects unprotected requests and drops
   responses that cannot be protected, so it never downgrades to plaintext.

Security Context Derivation
============================

OSCORE security contexts are derived from a small set of parameters (:rfc:`8613` Section 3):

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

3. **Persistence across reboots**: If the same master secret is reused after a reboot
   (i.e., master secrets are not re-derived, e.g., via EDHOC), the sender sequence
   number must be persisted to non-volatile memory to prevent nonce reuse, which would
   break confidentiality and integrity. The receiver's replay window does not need to
   be persisted: it is kept in memory and, after a reboot, is re-synchronized using the
   Echo option as described in :rfc:`8613` Appendix B.1.2.

Handling OSCORE When Not Supported
-----------------------------------

When OSCORE support is not enabled (:kconfig:option:`CONFIG_COAP_OSCORE` is not set),
the Zephyr CoAP stack implements fail-closed behavior for the OSCORE option per
:rfc:`7252` Section 5.4.1:

**Server behavior** (when ``CONFIG_COAP_OSCORE=n``):

- **CON requests** with OSCORE option: Returns **4.02 (Bad Option)** response
- **NON requests** with OSCORE option: Silently rejects (drops) the message
- **Responses** with OSCORE option: Sends RST for CON, silently drops NON/ACK

API Reference
=============

.. doxygengroup:: coap_oscore
