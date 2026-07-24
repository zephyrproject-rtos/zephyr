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

#include "coap_oscore_internal.h"

/*
 * Concrete OSCORE context. Publicly this is the opaque struct
 * coap_oscore_context; the uoscore struct context is confined to this file so
 * that no uoscore type leaks into the public CoAP API.
 */
struct coap_oscore_context {
	struct context ctx;
	bool in_use;
};

static struct coap_oscore_context oscore_ctx_pool[CONFIG_COAP_OSCORE_MAX_CONTEXTS];
static K_MUTEX_DEFINE(oscore_ctx_pool_lock);

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

int coap_oscore_protect(uint8_t *coap_msg, uint32_t coap_msg_len, uint8_t *oscore_msg,
			uint32_t *oscore_msg_len, struct coap_oscore_context *ctx)
{
	enum err result;

	if (coap_msg == NULL || oscore_msg == NULL || oscore_msg_len == NULL || ctx == NULL) {
		return -EINVAL;
	}

	result = coap2oscore(coap_msg, coap_msg_len, oscore_msg, oscore_msg_len, &ctx->ctx);

	if (result != ok) {
		LOG_ERR("OSCORE protection failed: %d", result);
		return -EACCES;
	}

	LOG_DBG("OSCORE protected message: %u -> %u bytes", coap_msg_len, *oscore_msg_len);
	return 0;
}

int coap_oscore_verify(uint8_t *oscore_msg, uint32_t oscore_msg_len, uint8_t *coap_msg,
		       uint32_t *coap_msg_len, struct coap_oscore_context *ctx, uint8_t *error_code)
{
	enum err result;

	if (oscore_msg == NULL || coap_msg == NULL || coap_msg_len == NULL || ctx == NULL) {
		if (error_code != NULL) {
			*error_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		}
		return -EINVAL;
	}

	result = oscore2coap(oscore_msg, oscore_msg_len, coap_msg, coap_msg_len, &ctx->ctx);

	if (result != ok) {
		LOG_ERR("OSCORE verification failed: %d", result);
		if (error_code != NULL) {
			*error_code = oscore_err_to_coap_code(result);
		}
		return -EACCES;
	}

	LOG_DBG("OSCORE verified message: %u -> %u bytes", oscore_msg_len, *coap_msg_len);
	return 0;
}

int coap_oscore_context_init(struct coap_oscore_init_params *params,
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

	/* Currently session reuse is not supported */
	if (!params->fresh_master_secret_salt) {
		LOG_ERR("Session reuse is not supported; fresh_master_secret_salt must be true");
		return -ENOTSUP;
	}

	/* Map the Zephyr parameters onto the uoscore initialization parameters.
	 * uoscore keeps pointers to master_secret, master_salt, id_context and
	 * sender_id, hence the lifetime requirement documented in the header.
	 * uoscore's struct byte_array uses a non-const ptr but only reads these
	 * buffers during context derivation, so cast away const here to preserve the
	 * const-correct public API without discarded-qualifier warnings at callers.
	 */
	struct oscore_init_params uo_params = {
		.master_secret = {.len = params->master_secret_len,
				  .ptr = (uint8_t *)params->master_secret},
		.sender_id = {.len = params->sender_id_len, .ptr = (uint8_t *)params->sender_id},
		.recipient_id = {.len = params->recipient_id_len,
				 .ptr = (uint8_t *)params->recipient_id},
		.master_salt = {.len = params->master_salt_len,
				.ptr = (uint8_t *)params->master_salt},
		.id_context = {.len = params->id_context_len, .ptr = (uint8_t *)params->id_context},
		.aead_alg =
			OSCORE_AES_CCM_16_64_128, /* Only one AEAD algorithm supported for now */
		.hkdf = OSCORE_SHA_256,           /* Only one HKDF algorithm supported for now */
		.fresh_master_secret_salt = params->fresh_master_secret_salt,
	};

	k_mutex_lock(&oscore_ctx_pool_lock, K_FOREVER);

	for (int i = 0; i < CONFIG_COAP_OSCORE_MAX_CONTEXTS; i++) {
		if (!oscore_ctx_pool[i].in_use) {
			slot = &oscore_ctx_pool[i];
			slot->in_use = true;
			break;
		}
	}

	k_mutex_unlock(&oscore_ctx_pool_lock);

	if (slot == NULL) {
		LOG_ERR("No free OSCORE context slots (CONFIG_COAP_OSCORE_MAX_CONTEXTS=%d)",
			CONFIG_COAP_OSCORE_MAX_CONTEXTS);
		return -ENOMEM;
	}

	result = oscore_context_init(&uo_params, &slot->ctx);
	if (result != ok) {
		LOG_ERR("oscore_context_init failed: %d", result);
		k_mutex_lock(&oscore_ctx_pool_lock, K_FOREVER);
		memset(slot, 0, sizeof(*slot));
		k_mutex_unlock(&oscore_ctx_pool_lock);
		return -EINVAL;
	}

	*ctx = slot;
	return 0;
}

void coap_oscore_context_free(struct coap_oscore_context *ctx)
{
	if (ctx == NULL) {
		return;
	}

	if (!PART_OF_ARRAY(oscore_ctx_pool, ctx)) {
		LOG_ERR("Invalid OSCORE context handle");
		return;
	}

	k_mutex_lock(&oscore_ctx_pool_lock, K_FOREVER);
	memset(ctx, 0, sizeof(*ctx));
	k_mutex_unlock(&oscore_ctx_pool_lock);
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
			net_socklen_t addr_len, const uint8_t *token, uint8_t tkl)
{
	struct coap_oscore_exchange *entry;
	int64_t now = k_uptime_get();

	if (tkl > COAP_TOKEN_MAX_LEN) {
		return -EINVAL;
	}

	entry = coap_oscore_exchange_find(cache, addr, addr_len, token, tkl);
	if (entry != NULL) {
		entry->timestamp = now;
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
	memcpy(&entry->addr, addr, addr_len);
	entry->addr_len = addr_len;
	memcpy(entry->token, token, tkl);
	entry->tkl = tkl;
	entry->timestamp = now;

	return 0;
}

void coap_oscore_exchange_remove(struct coap_oscore_exchange *cache, const struct net_sockaddr *addr,
			    net_socklen_t addr_len, const uint8_t *token, uint8_t tkl)
{
	struct coap_oscore_exchange *entry;

	entry = coap_oscore_exchange_find(cache, addr, addr_len, token, tkl);
	if (entry != NULL) {
		memset(entry, 0, sizeof(*entry));
	}
}
