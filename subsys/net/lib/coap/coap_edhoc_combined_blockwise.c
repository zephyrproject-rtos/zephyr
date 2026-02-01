/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include "coap_edhoc_combined_blockwise.h"
#include "coap_edhoc.h"
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>

/* Forward declaration for sockaddr comparison */
static bool sockaddr_equal(const struct net_sockaddr *a, net_socklen_t a_len,
			   const struct net_sockaddr *b, net_socklen_t b_len);

/**
 * @brief Find outer Block1 cache entry by address and token
 */
static struct coap_edhoc_outer_block_entry *
outer_block_find(struct coap_edhoc_outer_block_entry *cache,
		 size_t cache_size,
		 const struct net_sockaddr *addr,
		 net_socklen_t addr_len,
		 const uint8_t *token,
		 uint8_t tkl)
{
	int64_t now = k_uptime_get();

	for (size_t i = 0; i < cache_size; i++) {
		if (!cache[i].active) {
			continue;
		}

		/* Check if entry has expired */
		if ((now - cache[i].timestamp) > CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_LIFETIME_MS) {
			/* Entry expired, clear it (zeroize for security) */
			memset(&cache[i], 0, sizeof(cache[i]));
			continue;
		}

		/* Check if address and token match */
		if (cache[i].tkl == tkl &&
		    sockaddr_equal(&cache[i].addr, cache[i].addr_len, addr, addr_len) &&
		    memcmp(cache[i].token, token, tkl) == 0) {
			return &cache[i];
		}
	}

	return NULL;
}

/**
 * @brief Allocate or reuse outer Block1 cache entry (LRU eviction)
 */
static struct coap_edhoc_outer_block_entry *
outer_block_get_entry(struct coap_edhoc_outer_block_entry *cache,
		      size_t cache_size,
		      const struct net_sockaddr *addr,
		      net_socklen_t addr_len,
		      const uint8_t *token,
		      uint8_t tkl)
{
	struct coap_edhoc_outer_block_entry *entry;
	struct coap_edhoc_outer_block_entry *oldest = NULL;
	int64_t oldest_time = INT64_MAX;

	/* Try to find existing entry */
	entry = outer_block_find(cache, cache_size, addr, addr_len, token, tkl);
	if (entry != NULL) {
		return entry;
	}

	/* Find empty or oldest entry */
	for (size_t i = 0; i < cache_size; i++) {
		if (!cache[i].active) {
			return &cache[i];
		}
		if (cache[i].timestamp < oldest_time) {
			oldest_time = cache[i].timestamp;
			oldest = &cache[i];
		}
	}

	/* Evict oldest entry (zeroize for security) */
	if (oldest != NULL) {
		memset(oldest, 0, sizeof(*oldest));
	}

	return oldest;
}

/**
 * @brief Clear outer Block1 cache entry
 */
static void outer_block_clear(struct coap_edhoc_outer_block_entry *entry)
{
	if (entry == NULL) {
		return;
	}

	/* Zeroize entry (security-first) */
	memset(entry, 0, sizeof(*entry));
}

/**
 * @brief Compare two socket addresses for equality
 */
static bool sockaddr_equal(const struct net_sockaddr *a, net_socklen_t a_len,
			   const struct net_sockaddr *b, net_socklen_t b_len)
{
	if (a_len != b_len || a->sa_family != b->sa_family) {
		return false;
	}

	if (a->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *a4 = (const struct net_sockaddr_in *)a;
		const struct net_sockaddr_in *b4 = (const struct net_sockaddr_in *)b;

		return a4->sin_port == b4->sin_port &&
		       net_ipv4_addr_cmp(&a4->sin_addr, &b4->sin_addr);
	} else if (a->sa_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *a6 = (const struct net_sockaddr_in6 *)a;
		const struct net_sockaddr_in6 *b6 = (const struct net_sockaddr_in6 *)b;

		return a6->sin6_port == b6->sin6_port &&
		       net_ipv6_addr_cmp(&a6->sin6_addr, &b6->sin6_addr);
	}

	return false;
}

/**
 * @brief Send 2.31 Continue response for intermediate Block1
 *
 * Per RFC 7959, the server acknowledges each block with a 2.31 Continue
 * response containing a Block1 option indicating the next expected block.
 */
static int send_continue_response(const struct coap_service *service,
				  const struct coap_packet *request,
				  struct coap_block_context *block_ctx,
				  const struct net_sockaddr *client_addr,
				  net_socklen_t client_addr_len)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	int ret;

	ret = coap_packet_init(&response, buf, sizeof(buf),
			       COAP_VERSION_1, type, tkl, token,
			       COAP_RESPONSE_CODE_CONTINUE, id);
	if (ret < 0) {
		LOG_ERR("Failed to init 2.31 Continue response (%d)", ret);
		return ret;
	}

	/* Add Block1 option acknowledging received block */
	ret = coap_append_block1_option(&response, block_ctx);
	if (ret < 0) {
		LOG_ERR("Failed to add Block1 option to Continue response (%d)", ret);
		return ret;
	}

	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

/**
 * @brief Send error response and clear cache entry
 */
static int send_error_and_clear(const struct coap_service *service,
				const struct coap_packet *request,
				uint8_t error_code,
				const struct net_sockaddr *client_addr,
				net_socklen_t client_addr_len,
				struct coap_edhoc_outer_block_entry *entry)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	int ret;

	/* Clear cache entry first (security-first) */
	if (entry != NULL) {
		outer_block_clear(entry);
	}

	ret = coap_packet_init(&response, buf, sizeof(buf),
			       COAP_VERSION_1, type, tkl, token,
			       error_code, id);
	if (ret < 0) {
		return ret;
	}

	/* For 4.13 Request Entity Too Large, add Size1 option */
	if (error_code == COAP_RESPONSE_CODE_REQUEST_TOO_LARGE) {
		ret = coap_append_option_int(&response, COAP_OPTION_SIZE1,
					     CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_MAX_LEN);
		if (ret < 0) {
			LOG_WRN("Failed to add Size1 option (%d)", ret);
			/* Continue anyway */
		}
	}

	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

int coap_edhoc_outer_block_process(const struct coap_service *service,
				    struct coap_packet *request,
				    const uint8_t *buf,
				    size_t received,
				    const struct net_sockaddr *client_addr,
				    net_socklen_t client_addr_len,
				    uint8_t *reconstructed_buf,
				    size_t *reconstructed_len)
{
	struct coap_edhoc_outer_block_entry *entry;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;
	uint16_t payload_len;
	const uint8_t *payload;
	int ret;
	bool is_first_block;
	bool has_edhoc_option;

	if (service == NULL || request == NULL || buf == NULL ||
	    reconstructed_buf == NULL || reconstructed_len == NULL) {
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* Extract token */
	tkl = coap_header_get_token(request, token);
	if (tkl == 0) {
		/* RFC 7959: Block1 requires token for tracking */
		LOG_ERR("Block1 request missing token");
		(void)send_error_and_clear(service, request,
					   COAP_RESPONSE_CODE_BAD_REQUEST,
					   client_addr, client_addr_len, NULL);
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* Get Block1 option - returns block size in bytes, or negative on error */
	bool has_more;
	uint32_t block_number;
	ret = coap_get_block1_option(request, &has_more, &block_number);
	if (ret < 0) {
		LOG_ERR("Failed to get Block1 option (%d)", ret);
		(void)send_error_and_clear(service, request,
					   COAP_RESPONSE_CODE_BAD_REQUEST,
					   client_addr, client_addr_len, NULL);
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* ret is the block size in bytes */
	int block_size_bytes = ret;
	enum coap_block_size block_size_szx = coap_bytes_to_block_size(block_size_bytes);
	if (block_size_szx < 0) {
		LOG_ERR("Invalid block size: %d", block_size_bytes);
		(void)send_error_and_clear(service, request,
					   COAP_RESPONSE_CODE_BAD_REQUEST,
					   client_addr, client_addr_len, NULL);
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* Check if EDHOC option is present (basic check first) */
	has_edhoc_option = coap_edhoc_msg_has_edhoc(request);

	/* Look for existing reassembly */
	entry = outer_block_find(service->data->outer_block_cache,
				 CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
				 client_addr, client_addr_len, token, tkl);

	is_first_block = (block_number == 0);

	/* RFC 9668 Section 3.3.2: Start condition - EDHOC option present AND Block1 present */
	if (is_first_block && !has_edhoc_option) {
		/* Not a combined request with Block1 - let normal processing handle it */
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* RFC 9668 Section 3.1 + RFC 7252 Section 5.4.5: Validate EDHOC option occurrences
	 * Only validate if this is a combined request (EDHOC option present on first block)
	 */
	if (is_first_block && has_edhoc_option) {
		bool edhoc_present;
		ret = coap_edhoc_validate_option(request, &edhoc_present);
		if (ret < 0) {
			/* Multiple EDHOC options - RFC 7252 Section 5.4.5 + 5.4.1 violation */
			LOG_ERR("Repeated EDHOC option in Block1 request");
			
			/* Send 4.02 Bad Option for CON, silently drop for NON */
			if (coap_header_get_type(request) == COAP_TYPE_CON) {
				(void)send_error_and_clear(service, request,
							   COAP_RESPONSE_CODE_BAD_OPTION,
							   client_addr, client_addr_len, NULL);
			}
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}
	}

	/* Continuation condition: Block1 present AND cache match (even without EDHOC option) */
	if (!is_first_block && entry == NULL) {
		/* Block NUM > 0 but no matching reassembly - not a combined request */
		/* Let normal Block1 processing handle it */
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* Get payload */
	payload = coap_packet_get_payload(request, &payload_len);
	if (payload == NULL || payload_len == 0) {
		LOG_ERR("Block1 request missing payload");
		(void)send_error_and_clear(service, request,
					   COAP_RESPONSE_CODE_BAD_REQUEST,
					   client_addr, client_addr_len, entry);
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* Handle first block */
	if (is_first_block) {
		/* Allocate new cache entry */
		entry = outer_block_get_entry(service->data->outer_block_cache,
					      CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					      client_addr, client_addr_len, token, tkl);
		if (entry == NULL) {
			LOG_ERR("Failed to allocate outer Block1 cache entry");
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_INTERNAL_ERROR,
						   client_addr, client_addr_len, NULL);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}

		/* Initialize entry */
		memset(entry, 0, sizeof(*entry));
		memcpy(&entry->addr, client_addr, client_addr_len);
		entry->addr_len = client_addr_len;
		memcpy(entry->token, token, tkl);
		entry->tkl = tkl;
		/* Initialize block context */
		entry->block_ctx.block_size = block_size_szx;
		entry->block_ctx.current = 0;
		entry->block_ctx.total_size = 0; /* Will be set from Size1 if present */
		entry->timestamp = k_uptime_get();
		entry->active = true;

		/* Save header template (everything up to payload marker) */
		size_t header_len = request->offset - payload_len - 1; /* -1 for 0xFF marker */
		if (header_len > sizeof(entry->header_template)) {
			LOG_ERR("Header template too large (%zu > %zu)",
				header_len, sizeof(entry->header_template));
			outer_block_clear(entry);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_BAD_REQUEST,
						   client_addr, client_addr_len, NULL);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}
		memcpy(entry->header_template, buf, header_len);
		entry->header_template_len = header_len;

		/* Copy first block payload */
		if (payload_len > sizeof(entry->reassembly_buf)) {
			LOG_ERR("First block payload too large (%u > %zu)",
				payload_len, sizeof(entry->reassembly_buf));
			outer_block_clear(entry);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_REQUEST_TOO_LARGE,
						   client_addr, client_addr_len, NULL);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}
		memcpy(entry->reassembly_buf, payload, payload_len);
		entry->accumulated_len = payload_len;
		/* Set current to the byte offset for the next expected block */
		entry->block_ctx.current = (block_number + 1) * block_size_bytes;

		LOG_DBG("Started outer Block1 reassembly: block_size=%u, NUM=%u, M=%d",
			block_size_bytes, block_number, has_more);
	} else {
		/* Continuation block - validate */

		/* Check block size consistency (RFC 7959) */
		if (block_size_szx != entry->block_ctx.block_size) {
			LOG_ERR("Block size changed: %u -> %u",
				coap_block_size_to_bytes(entry->block_ctx.block_size),
				block_size_bytes);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_BAD_REQUEST,
						   client_addr, client_addr_len, entry);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}

		/* Check expected block number */
		uint32_t expected_num = (entry->block_ctx.current / block_size_bytes);
		if (block_number != expected_num) {
			LOG_ERR("Unexpected block NUM: expected %u, got %u",
				expected_num, block_number);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_BAD_REQUEST,
						   client_addr, client_addr_len, entry);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}

		/* Check if adding this block would exceed limit */
		if (entry->accumulated_len + payload_len > CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_MAX_LEN) {
			LOG_ERR("Reassembled payload would exceed limit (%zu + %u > %d)",
				entry->accumulated_len, payload_len,
				CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_MAX_LEN);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_REQUEST_TOO_LARGE,
						   client_addr, client_addr_len, entry);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}

		/* Append payload */
		memcpy(entry->reassembly_buf + entry->accumulated_len, payload, payload_len);
		entry->accumulated_len += payload_len;
		/* Update current to the byte offset for the next expected block */
		entry->block_ctx.current = (block_number + 1) * block_size_bytes;

		LOG_DBG("Continued outer Block1 reassembly: NUM=%u, M=%d, accumulated=%zu",
			block_number, has_more, entry->accumulated_len);
	}

	/* Check if this is the last block */
	if (!has_more) {
		/* Last block received - reconstruct full request */
		LOG_DBG("Last outer Block1 received, reconstructing full request");

		/* Reconstruct: header_template + 0xFF + reassembled_payload */
		size_t total_len = entry->header_template_len + 1 + entry->accumulated_len;
		if (total_len > CONFIG_COAP_SERVER_MESSAGE_SIZE) {
			LOG_ERR("Reconstructed request too large (%zu > %d)",
				total_len, CONFIG_COAP_SERVER_MESSAGE_SIZE);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_REQUEST_TOO_LARGE,
						   client_addr, client_addr_len, entry);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}

		/* Copy header template */
		memcpy(reconstructed_buf, entry->header_template, entry->header_template_len);
		/* Add payload marker */
		reconstructed_buf[entry->header_template_len] = 0xFF;
		/* Copy reassembled payload */
		memcpy(reconstructed_buf + entry->header_template_len + 1,
		       entry->reassembly_buf, entry->accumulated_len);

		*reconstructed_len = total_len;

		/* Clear cache entry (security-first) */
		outer_block_clear(entry);

		LOG_DBG("Outer Block1 reassembly complete: %zu bytes", *reconstructed_len);
		return COAP_EDHOC_OUTER_BLOCK_COMPLETE;
	}

	/* Not last block - send 2.31 Continue */
	ret = send_continue_response(service, request, &entry->block_ctx,
				     client_addr, client_addr_len);
	if (ret < 0) {
		LOG_ERR("Failed to send 2.31 Continue response (%d)", ret);
		outer_block_clear(entry);
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	LOG_DBG("Sent 2.31 Continue for Block1 NUM=%u", block_number);
	return COAP_EDHOC_OUTER_BLOCK_WAITING;
}

#if defined(CONFIG_ZTEST)
struct coap_edhoc_outer_block_entry *
coap_edhoc_outer_block_find(struct coap_edhoc_outer_block_entry *cache,
			     size_t cache_size,
			     const struct net_sockaddr *addr,
			     net_socklen_t addr_len,
			     const uint8_t *token,
			     uint8_t tkl)
{
	return outer_block_find(cache, cache_size, addr, addr_len, token, tkl);
}

void coap_edhoc_outer_block_clear(struct coap_edhoc_outer_block_entry *entry)
{
	outer_block_clear(entry);
}
#endif /* CONFIG_ZTEST */
