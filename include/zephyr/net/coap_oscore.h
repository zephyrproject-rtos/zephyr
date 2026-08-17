/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief CoAP OSCORE API
 *
 * A public API for creating OSCORE (RFC 8613) security contexts that can be
 * attached to a CoAP client or service.
 */

#ifndef ZEPHYR_INCLUDE_NET_COAP_OSCORE_H_
#define ZEPHYR_INCLUDE_NET_COAP_OSCORE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/coap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CoAP OSCORE API
 * @defgroup coap_oscore CoAP OSCORE API
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/**
 * @brief Opaque OSCORE security context.
 *
 * Created with coap_oscore_context_add() and used in the internal context pool.
 * The internal representation is private to the CoAP OSCORE implementation.
 *
 * @kconfig_dep{CONFIG_COAP_OSCORE}
 */
struct coap_oscore_context;

/** @brief AEAD algorithm used to protect OSCORE messages (RFC 8613 Section 3.1). */
enum coap_oscore_aead_alg {
	/** AES-CCM mode 128-bit key, 64-bit tag, 13-byte nonce. */
	COAP_OSCORE_AEAD_AES_CCM_16_64_128 = 0,
};

/** @brief HKDF algorithm used to derive OSCORE keys (RFC 8613 Section 3.1). */
enum coap_oscore_hkdf {
	/** HKDF-SHA-256. */
	COAP_OSCORE_HKDF_SHA_256 = 0,
};

/**
 * @brief OSCORE security context derivation parameters (RFC 8613 Section 3.2).
 *
 * @note coap_oscore_context_add() copies all referenced buffers into the
 *       context pool, so the caller may free or reuse them as soon as the call
 *       returns; they need not outlive the derived context.
 *
 * @kconfig_dep{CONFIG_COAP_OSCORE}
 */
struct coap_oscore_init_params {
	/** Master Secret (required, typically 16 bytes for AES-CCM-16-64-128). */
	const uint8_t *master_secret;
	/** Length of @ref master_secret in bytes. */
	size_t master_secret_len;

	/** Sender ID (required). */
	const uint8_t *sender_id;
	/** Length of @ref sender_id in bytes. */
	size_t sender_id_len;

	/** Recipient ID (required). */
	const uint8_t *recipient_id;
	/** Length of @ref recipient_id in bytes. */
	size_t recipient_id_len;

	/** Master Salt (optional, set to NULL if unused). */
	const uint8_t *master_salt;
	/** Length of @ref master_salt in bytes (0 if unused). */
	size_t master_salt_len;

	/** ID Context (optional, set to NULL if unused). */
	const uint8_t *id_context;
	/** Length of @ref id_context in bytes (0 if unused). */
	size_t id_context_len;

	/** AEAD algorithm (use ::COAP_OSCORE_AEAD_AES_CCM_16_64_128 for the default). */
	enum coap_oscore_aead_alg aead_alg;
	/** KDF algorithm (use ::COAP_OSCORE_HKDF_SHA_256 for the default). */
	enum coap_oscore_hkdf hkdf;

	/**
	 * True if the combination of Master Secret and Master Salt is unique at
	 * every boot (e.g. derived with EDHOC). Reuse of the master secret and
	 * master salt is currently not supported. Setting this to false will
	 * result in an error.
	 */
	bool fresh_master_secret_salt;
};

/**
 * @brief Add an OSCORE security context.
 *
 * Derives the common, sender and recipient contexts from @p params
 * (RFC 8613 Section 3.2) and adds them to the pool of contexts that can be used with
 * the CoAP server. Contexts are allocated from a fixed pool sized by
 * @kconfig{CONFIG_COAP_OSCORE_MAX_CONTEXTS} and released with
 * coap_oscore_context_remove(). Each context corresponds to one client identity
 * (Recipient ID); add one context per client to serve multiple clients concurrently.
 *
 * @kconfig_dep{CONFIG_COAP_OSCORE}
 *
 * @param  params Derivation parameters.
 * @param  ctx    On success, set to the newly created context handle.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p params is NULL, a required parameter is missing or has
 *         an invalid size, or the context could not be derived.
 * @retval -ENOTSUP if @c fresh_master_secret_salt is false while
 *         @kconfig{CONFIG_COAP_OSCORE_CONTEXT_REUSE} is disabled.
 * @retval -ENOMEM if no free context slot is available.
 */
int coap_oscore_context_add(const struct coap_oscore_init_params *params,
			    struct coap_oscore_context **ctx);

/**
 * @brief Remove an OSCORE security context.
 *
 * Removes the context from the pool of contexts that can be used with the CoAP
 * server. The context is reference-counted: if an in-flight request, an active
 * exchange, or an observer still references it, the context is retained and
 * -EAGAIN is returned. Retry once those references have been released. Passing
 * NULL returns -EINVAL.
 *
 * @kconfig_dep{CONFIG_COAP_OSCORE}
 *
 * @param ctx Context handle to remove.
 *
 * @retval 0 on success.
 * @retval -EAGAIN if the context is in use and cannot be removed.
 * @retval -EINVAL if @p ctx is NULL or not a valid context handle.
 */
int coap_oscore_context_remove(struct coap_oscore_context *ctx);

/**
 * @brief OSCORE exchange entry.
 *
 * Tracks which server responses need OSCORE protection by matching the client
 * address and request token (RFC 8613 Section 8.3). A CoAP service defined with
 * @ref COAP_SERVICE_DEFINE_OSCORE owns a statically allocated cache of these
 * entries.
 *
 * @kconfig_dep{CONFIG_COAP_OSCORE}
 */
struct coap_oscore_exchange {
	struct net_sockaddr_storage addr;  /**< Client address */
	net_socklen_t addr_len;            /**< Address length */
	uint8_t token[COAP_TOKEN_MAX_LEN]; /**< Token from the request */
	uint8_t tkl;                       /**< Token length */
	int64_t timestamp;                 /**< Creation timestamp */
	struct coap_oscore_context *ctx;   /**< OSCORE context used for this exchange */
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_COAP_OSCORE_H_ */
