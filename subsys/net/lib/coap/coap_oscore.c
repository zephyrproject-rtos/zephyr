/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coap_oscore, CONFIG_COAP_LOG_LEVEL);

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_oscore.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/net/net_ip.h>

#include <oscore.h>
#include <common/oscore_edhoc_error.h>
#include "oscore/supported_algorithm.h"
#include "oscore/oscore_coap.h"

#include "coap_oscore_internal.h"

/*
 * Concrete OSCORE context. Publicly this is the opaque struct
 * coap_oscore_context; the uoscore struct context is confined to this file so
 * that no uoscore type leaks into the public CoAP API. The Master Secret and
 * Master Salt buffers are sized by Kconfig (RFC 8613 Section 3.1: the Master
 * Secret is variable-length keying material and the Master Salt is optional).
 */
struct coap_oscore_context {
	struct context ctx;
	uint8_t master_secret[CONFIG_COAP_OSCORE_MASTER_SECRET_MAX_LEN];
	uint8_t master_secret_len;
	uint8_t master_salt[CONFIG_COAP_OSCORE_MASTER_SALT_MAX_LEN];
	uint8_t master_salt_len;
	uint8_t sender_id[MAX_KID_LEN];
	uint8_t sender_id_len;
	uint8_t recipient_id[MAX_KID_LEN];
	uint8_t recipient_id_len;
	uint8_t id_context[MAX_KID_CONTEXT_LEN];
	uint8_t id_context_len;
	struct k_mutex lock;
	atomic_t refcount;
};

static struct coap_oscore_context oscore_ctx_pool[CONFIG_COAP_OSCORE_MAX_CONTEXTS];
static K_MUTEX_DEFINE(oscore_ctx_pool_lock);

static bool coap_oscore_refcount_inc_unless_zero(atomic_t *v)
{
	atomic_val_t old;

	do {
		old = atomic_get(v);

		if (old == 0) {
			return false;
		}

	} while (!atomic_cas(v, old, old + 1));

	return true;
}

static bool coap_oscore_refcount_dec_unless_one(atomic_t *v)
{
	atomic_val_t old;

	do {
		old = atomic_get(v);

		if (old <= 1) {
			return false;
		}

	} while (!atomic_cas(v, old, old - 1));

	return true;
}

bool coap_oscore_msg_has_oscore(const struct coap_packet *cpkt)
{
	struct coap_option option;
	int ret;

	ret = coap_find_options(cpkt, COAP_OPTION_OSCORE, &option, 1);
	return ret > 0;
}

int coap_oscore_validate_msg(const struct coap_packet *cpkt)
{
	uint16_t payload_len;
	const uint8_t *payload;

	if (!coap_oscore_msg_has_oscore(cpkt)) {
		/* Not an OSCORE message, no validation needed */
		return 0;
	}

	payload = coap_packet_get_payload(cpkt, &payload_len);
	if (payload == NULL || payload_len == 0) {
		/* RFC 8613 Section 2: OSCORE option present without payload is malformed */
		LOG_ERR("OSCORE message without payload is malformed (RFC 8613 Section 2)");
		return -EBADMSG;
	}

	return 0;
}

static int coap_oscore_parse_option_identity(const struct coap_option *option, const uint8_t **kid,
					     uint8_t *kid_len, const uint8_t **id_context,
					     uint8_t *id_context_len)
{
	const uint8_t *value;
	uint8_t remaining;
	uint8_t first_byte;
	uint8_t piv_len;

	if (option == NULL || kid == NULL || kid_len == NULL || id_context == NULL ||
	    id_context_len == NULL) {
		return -EINVAL;
	}

	*kid = NULL;
	*kid_len = 0;
	*id_context = NULL;
	*id_context_len = 0;

	if (option->len == 0) {
		return 0;
	}

	value = option->value;
	remaining = option->len;

	first_byte = *value++;
	remaining--;
	piv_len = first_byte & COMP_OSCORE_OPT_PIV_N_MASK;

	if (piv_len > MAX_PIV_LEN || remaining < piv_len) {
		return -EBADMSG;
	}

	value += piv_len;
	remaining -= piv_len;

	if ((first_byte & COMP_OSCORE_OPT_KIDC_H_MASK) != 0U) {
		if (remaining == 0U) {
			return -EBADMSG;
		}

		*id_context_len = *value++;
		remaining--;

		if (remaining < *id_context_len) {
			return -EBADMSG;
		}

		*id_context = value;
		value += *id_context_len;
		remaining -= *id_context_len;
	}

	if ((first_byte & COMP_OSCORE_OPT_KID_K_MASK) != 0U) {
		*kid = value;
		*kid_len = remaining;
	} else if (remaining != 0U) {
		return -EBADMSG;
	}

	return 0;
}

static uint8_t oscore_err_to_coap_code(enum err oscore_err)
{
	switch (oscore_err) {
	case ok:
		return COAP_RESPONSE_CODE_VALID;

	/* Failure to parse/decompress the OSCORE option or decode the COSE
	 * object => 4.02 Bad Option (RFC 8613 Section 8.2, step 2).
	 */
	case not_oscore_pkt:
	case not_valid_input_packet:
	case too_many_options:
	case oscore_valuelen_to_long_error:
	case oscore_inpkt_invalid_tkl:
	case oscore_inpkt_invalid_option_delta:
	case oscore_inpkt_invalid_optionlen:
	case oscore_inpkt_invalid_piv:
	case oscore_unknown_hkdf:
	case oscore_invalid_algorithm_aead:
	case oscore_invalid_algorithm_hkdf:
		return COAP_RESPONSE_CODE_BAD_OPTION;

	/* Security context not found, replay detected or Echo/freshness
	 * validation failure => 4.01 Unauthorized (RFC 8613 Section 8.2,
	 * step 3 and Appendix B.1.2).
	 */
	case oscore_kid_recipient_id_mismatch:
	case token_mismatch:
	case first_request_after_reboot:
	case echo_validation_failed:
	case echo_val_mismatch:
	case no_echo_option:
	case oscore_replay_window_protection_error:
	case oscore_replay_notification_protection_error:
		return COAP_RESPONSE_CODE_UNAUTHORIZED;

	/* Decryption/MAC verification failure and any other error => stop
	 * processing and respond with 4.00 Bad Request (RFC 8613 Section 8.2,
	 * step 4).
	 */
	default:
		return COAP_RESPONSE_CODE_BAD_REQUEST;
	}
}

/* RFC 8613 Appendix B.1.2: the recipient replay window is lost on reboot when the
 * context is reused. These errors mean the server must answer with an Echo challenge
 * to re-synchronize the replay window rather than a plain error response.
 */
static bool oscore_err_needs_echo_challenge(enum err oscore_err)
{
	switch (oscore_err) {
	case first_request_after_reboot:
	case echo_validation_failed:
		return true;
	default:
		return false;
	}
}

int coap_oscore_protect(uint8_t *coap_msg, uint32_t coap_msg_len, uint8_t *oscore_msg,
			uint32_t *oscore_msg_len, struct coap_oscore_context *ctx)
{
	enum err result;

	if (coap_msg == NULL || oscore_msg == NULL || oscore_msg_len == NULL || ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	result = coap2oscore(coap_msg, coap_msg_len, oscore_msg, oscore_msg_len, &ctx->ctx);
	k_mutex_unlock(&ctx->lock);
	if (result != ok) {
		LOG_ERR("OSCORE protection failed: %d", result);
		return -EACCES;
	}

	LOG_DBG("OSCORE protected message: %u -> %u bytes", coap_msg_len, *oscore_msg_len);
	return 0;
}

int coap_oscore_verify(const struct coap_packet *request, uint8_t *oscore_msg,
		       uint32_t oscore_msg_len, uint8_t *coap_msg, uint32_t *coap_msg_len,
		       struct coap_oscore_context **ctx, uint8_t *error_code,
		       bool *needs_echo_challenge)
{
	enum err result;
	struct coap_option option;
	const uint8_t *kid;
	const uint8_t *id_context;
	uint8_t kid_len;
	uint8_t id_context_len;
	int ret;
	bool identity_match_found = false;
	bool context_candidates[CONFIG_COAP_OSCORE_MAX_CONTEXTS] = {false};

	result = unexpected_result_from_ext_lib;

	if (request == NULL || ctx == NULL || oscore_msg == NULL || coap_msg == NULL ||
	    coap_msg_len == NULL) {
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		}
		return -EINVAL;
	}

	*ctx = NULL;

	ret = coap_find_options(request, COAP_OPTION_OSCORE, &option, 1);
	if (ret <= 0) {
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		}
		return -EINVAL;
	}

	ret = coap_oscore_parse_option_identity(&option, &kid, &kid_len, &id_context,
						&id_context_len);
	if (ret < 0) {
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_BAD_OPTION;
		}
		return -EACCES;
	}

	if (needs_echo_challenge != NULL) {
		*needs_echo_challenge = false;
	}

	if (oscore_msg == NULL || coap_msg == NULL || coap_msg_len == NULL || ctx == NULL) {
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		}
		return -EINVAL;
	}

	/* Recipient IDs and ID Contexts can be short/low-entropy, so several
	 * configured contexts may share the same identity. Phase 1 pins every context
	 * whose identity matches (under the pool lock); phase 2 then runs the
	 * expensive AEAD verification on each candidate with the pool lock released,
	 * since only the AEAD tag can disambiguate colliding identities. The pin taken
	 * in phase 1 keeps each candidate alive after the pool lock is dropped, and
	 * distinct contexts therefore decrypt concurrently.
	 */
	k_mutex_lock(&oscore_ctx_pool_lock, K_FOREVER);
	for (int i = 0; i < CONFIG_COAP_OSCORE_MAX_CONTEXTS; i++) {
		if (!coap_oscore_refcount_inc_unless_zero(&oscore_ctx_pool[i].refcount)) {
			continue;
		}

		if (oscore_ctx_pool[i].recipient_id_len == kid_len &&
		    oscore_ctx_pool[i].id_context_len == id_context_len &&
		    (kid_len == 0U || memcmp(oscore_ctx_pool[i].recipient_id, kid, kid_len) == 0) &&
		    (id_context_len == 0U ||
		     memcmp(oscore_ctx_pool[i].id_context, id_context, id_context_len) == 0)) {
			identity_match_found = true;
			context_candidates[i] = true;
		} else {
			/* Not a match: drop the pin taken above. */
			(void)coap_oscore_context_dec_refcount(&oscore_ctx_pool[i]);
		}
	}
	k_mutex_unlock(&oscore_ctx_pool_lock);

	for (int i = 0; i < CONFIG_COAP_OSCORE_MAX_CONTEXTS; i++) {
		if (!context_candidates[i]) {
			continue;
		}

		if (*ctx != NULL) {
			/* Winner already found; release remaining pinned candidates. */
			(void)coap_oscore_context_dec_refcount(&oscore_ctx_pool[i]);
			continue;
		}

		k_mutex_lock(&oscore_ctx_pool[i].lock, K_FOREVER);
		result = oscore2coap(oscore_msg, oscore_msg_len, coap_msg, coap_msg_len,
				     &oscore_ctx_pool[i].ctx);
		k_mutex_unlock(&oscore_ctx_pool[i].lock);

		if (result != ok && !oscore_err_needs_echo_challenge(result)) {
			(void)coap_oscore_context_dec_refcount(&oscore_ctx_pool[i]);
			continue;
		}

		*ctx = &oscore_ctx_pool[i];
	}

	if (*ctx == NULL && !identity_match_found) {
		LOG_DBG("No OSCORE context found returning 4.01 Unauthorized");
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_UNAUTHORIZED;
		}
		return -ENOENT;
	}

	if (*ctx == NULL || result != ok) {
		LOG_DBG("OSCORE verification failed: %d", result);
		if (error_code != NULL) {
			*error_code = oscore_err_to_coap_code(result);
		}
		if (needs_echo_challenge != NULL) {
			*needs_echo_challenge = oscore_err_needs_echo_challenge(result);
		}
		return -EACCES;
	}

	LOG_DBG("OSCORE verified message: %u -> %u bytes", oscore_msg_len, *coap_msg_len);
	return 0;
}

int coap_oscore_context_add(const struct coap_oscore_init_params *params,
			    struct coap_oscore_context **ctx)
{
	struct coap_oscore_context *slot = NULL;
	enum err result;

	if (params == NULL || ctx == NULL ||
	    params->aead_alg != COAP_OSCORE_AEAD_AES_CCM_16_64_128 ||
	    params->hkdf != COAP_OSCORE_HKDF_SHA_256) {
		return -EINVAL;
	}

	/* Master Secret is mandatory and must be non-empty (RFC 8613 Section 3.1). */
	if (params->master_secret == NULL || params->master_secret_len == 0) {
		return -EINVAL;
	}

	/* Sender ID and Recipient ID are mandatory inputs; their value may be the
	 * empty byte string, so only the pointers are required, not a non-zero
	 * length. For the optional Master Salt and ID Context, reject a non-zero
	 * length paired with a NULL buffer.
	 */
	if (params->sender_id == NULL || params->recipient_id == NULL) {
		return -EINVAL;
	}

	if ((params->master_salt == NULL && params->master_salt_len != 0) ||
	    (params->id_context == NULL && params->id_context_len != 0)) {
		return -EINVAL;
	}

	if (params->master_secret_len > CONFIG_COAP_OSCORE_MASTER_SECRET_MAX_LEN ||
	    params->master_salt_len > CONFIG_COAP_OSCORE_MASTER_SALT_MAX_LEN ||
	    params->sender_id_len > MAX_KID_LEN || params->recipient_id_len > MAX_KID_LEN ||
	    params->id_context_len > MAX_KID_CONTEXT_LEN) {
		return -EINVAL;
	}

#if !defined(CONFIG_COAP_OSCORE_CONTEXT_REUSE)
	if (!params->fresh_master_secret_salt) {
		LOG_ERR("Context reuse disabled: fresh_master_secret_salt must be true");
		return -ENOTSUP;
	}
#endif

	k_mutex_lock(&oscore_ctx_pool_lock, K_FOREVER);

	for (int i = 0; i < CONFIG_COAP_OSCORE_MAX_CONTEXTS; i++) {
		if (atomic_get(&oscore_ctx_pool[i].refcount) == 0) {
			slot = &oscore_ctx_pool[i];
			break;
		}
	}

	if (slot == NULL) {
		k_mutex_unlock(&oscore_ctx_pool_lock);
		LOG_ERR("No free OSCORE context slots (CONFIG_COAP_OSCORE_MAX_CONTEXTS=%d)",
			CONFIG_COAP_OSCORE_MAX_CONTEXTS);
		return -ENOMEM;
	}

	k_mutex_init(&slot->lock);

	slot->master_secret_len = params->master_secret_len;
	memcpy(slot->master_secret, params->master_secret, params->master_secret_len);
	slot->master_salt_len = params->master_salt_len;
	if (params->master_salt_len > 0) {
		memcpy(slot->master_salt, params->master_salt, params->master_salt_len);
	}
	slot->sender_id_len = params->sender_id_len;
	memcpy(slot->sender_id, params->sender_id, params->sender_id_len);
	slot->recipient_id_len = params->recipient_id_len;
	memcpy(slot->recipient_id, params->recipient_id, params->recipient_id_len);
	slot->id_context_len = params->id_context_len;
	if (params->id_context_len > 0) {
		memcpy(slot->id_context, params->id_context, params->id_context_len);
	}

	/* Map the Zephyr parameters onto the uoscore initialization parameters.
	 * uoscore keeps pointers to master_secret, master_salt, id_context and
	 * sender_id, hence the lifetime requirement documented in the header.
	 * uoscore's struct byte_array uses a non-const ptr but only reads these
	 * buffers during context derivation, so cast away const here to preserve the
	 * const-correct public API without discarded-qualifier warnings at callers.
	 */
	struct oscore_init_params uo_params = {
		.master_secret = {.len = slot->master_secret_len, .ptr = slot->master_secret},
		.sender_id = {.len = slot->sender_id_len, .ptr = slot->sender_id},
		.recipient_id = {.len = slot->recipient_id_len, .ptr = slot->recipient_id},
		.master_salt = {.len = slot->master_salt_len, .ptr = slot->master_salt},
		.id_context = {.len = slot->id_context_len, .ptr = slot->id_context},
		.aead_alg =
			OSCORE_AES_CCM_16_64_128, /* Only one AEAD algorithm supported for now */
		.hkdf = OSCORE_SHA_256,           /* Only one HKDF algorithm supported for now */
		.fresh_master_secret_salt = params->fresh_master_secret_salt,
	};

	result = oscore_context_init(&uo_params, &slot->ctx);
	if (result != ok) {
		LOG_ERR("oscore_context_init failed: %d", result);
		memset(slot, 0, sizeof(*slot));
		k_mutex_unlock(&oscore_ctx_pool_lock);
		return -EINVAL;
	}

	/* Needs to be last since after this the slot will be visible as live */
	atomic_set(&slot->refcount, 1);

	*ctx = slot;
	k_mutex_unlock(&oscore_ctx_pool_lock);
	return 0;
}

int coap_oscore_context_remove(struct coap_oscore_context *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (!PART_OF_ARRAY(oscore_ctx_pool, ctx)) {
		LOG_ERR("Invalid OSCORE context handle");
		return -EINVAL;
	}

	k_mutex_lock(&oscore_ctx_pool_lock, K_FOREVER);
	/* Counter should be 1, meaning that the context is not in use by any observer or exchange.
	 * If it is not 1, then it is in use and cannot be removed.
	 */
	if (atomic_cas(&ctx->refcount, 1, 0) == false) {
		LOG_ERR("OSCORE context is in use, cannot remove");
		k_mutex_unlock(&oscore_ctx_pool_lock);
		return -EAGAIN;
	}
	memset(ctx, 0, sizeof(*ctx));
	k_mutex_unlock(&oscore_ctx_pool_lock);
	return 0;
}

int coap_oscore_context_inc_refcount(struct coap_oscore_context *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (!coap_oscore_refcount_inc_unless_zero(&ctx->refcount)) {
		return -EINVAL;
	}
	return 0;
}

int coap_oscore_context_dec_refcount(struct coap_oscore_context *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (!coap_oscore_refcount_dec_unless_one(&ctx->refcount)) {
		return -EINVAL;
	}
	return 0;
}

struct coap_oscore_exchange *coap_oscore_exchange_find(struct coap_oscore_exchange *cache,
						       const struct net_sockaddr *addr,
						       net_socklen_t addr_len, const uint8_t *token,
						       uint8_t tkl)
{
	int64_t now = k_uptime_get();

	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		if (cache[i].addr_len == 0) {
			continue;
		}

		if ((now - cache[i].timestamp) > CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS) {
			(void)coap_oscore_context_dec_refcount(cache[i].ctx);
			memset(&cache[i], 0, sizeof(cache[i]));
			continue;
		}

		if (cache[i].tkl == tkl && net_sockaddr_cmp(net_sad(&cache[i].addr), addr) &&
		    memcmp(cache[i].token, token, tkl) == 0) {
			return &cache[i];
		}
	}

	return NULL;
}

int coap_oscore_exchange_add(struct coap_oscore_exchange *cache, const struct net_sockaddr *addr,
			     net_socklen_t addr_len, const uint8_t *token, uint8_t tkl,
			     struct coap_oscore_context *ctx)
{
	struct coap_oscore_exchange *entry;
	int64_t now = k_uptime_get();

	if (tkl > COAP_TOKEN_MAX_LEN) {
		return -EINVAL;
	}

	entry = coap_oscore_exchange_find(cache, addr, addr_len, token, tkl);
	if (entry != NULL) {
		entry->timestamp = now;
		(void)coap_oscore_context_dec_refcount(entry->ctx);
		entry->ctx = ctx;
		if (coap_oscore_context_inc_refcount(entry->ctx) != 0) {
			/* old context removed but potentially new one could not be added */
			memset(entry, 0, sizeof(*entry));
			return -EINVAL;
		}
		return 0;
	}

	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		if (cache[i].addr_len == 0) {
			entry = &cache[i];
			break;
		}
	}

	if (entry == NULL) {
		return -ENOMEM;
	}

	if (addr_len > sizeof(entry->addr)) {
		return -EINVAL;
	}

	entry->ctx = ctx;
	if (coap_oscore_context_inc_refcount(entry->ctx) != 0) {
		return -EINVAL;
	}

	memcpy(&entry->addr, addr, addr_len);
	entry->addr_len = addr_len;
	memcpy(entry->token, token, tkl);
	entry->tkl = tkl;
	entry->timestamp = now;

	return 0;
}

void coap_oscore_exchange_remove(struct coap_oscore_exchange *cache,
				 const struct net_sockaddr *addr, net_socklen_t addr_len,
				 const uint8_t *token, uint8_t tkl)
{
	struct coap_oscore_exchange *entry;

	entry = coap_oscore_exchange_find(cache, addr, addr_len, token, tkl);
	if (entry != NULL) {
		(void)coap_oscore_context_dec_refcount(entry->ctx);
		memset(entry, 0, sizeof(*entry));
	}
}
