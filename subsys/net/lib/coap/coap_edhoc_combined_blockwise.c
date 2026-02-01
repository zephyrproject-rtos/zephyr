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
 * @brief Parse all Request-Tag options from a CoAP packet
 *
 * Per RFC 9175 Section 3.2.1, Request-Tag is repeatable and 0-8 bytes each.
 * This function serializes the list as: [len1][bytes1][len2][bytes2]...
 * Absent Request-Tag is distinct from present with 0-length (RFC 9175 Section 3.4).
 *
 * @param request CoAP packet to parse
 * @param out_count Output: number of Request-Tag options found (0 = absent)
 * @param out_data Output: serialized Request-Tag data
 * @param out_data_len Output: length of serialized data
 * @param max_data_len Maximum size of out_data buffer
 * @return 0 on success, negative on error
 */
static int parse_request_tag_list(const struct coap_packet *request,
				   uint8_t *out_count,
				   uint8_t *out_data,
				   size_t *out_data_len,
				   size_t max_data_len)
{
	struct coap_option options[8];
	int num_found;
	size_t data_len = 0;

	if (request == NULL || out_count == NULL || out_data == NULL || out_data_len == NULL) {
		return -EINVAL;
	}

	*out_count = 0;
	*out_data_len = 0;

	/* Find all Request-Tag options (RFC 9175: repeatable) */
	num_found = coap_find_options(request, COAP_OPTION_REQUEST_TAG, options, ARRAY_SIZE(options));
	if (num_found < 0) {
		LOG_ERR("Failed to find Request-Tag options (%d)", num_found);
		return num_found;
	}

	/* Process each Request-Tag option found */
	for (int i = 0; i < num_found; i++) {
		/* RFC 9175 Section 3.2.1: Request-Tag is 0-8 bytes */
		if (options[i].len > 8) {
			LOG_ERR("Request-Tag too long: %u bytes (max 8)", options[i].len);
			return -EINVAL;
		}

		/* Check if we have space for [len][bytes] */
		if (data_len + 1 + options[i].len > max_data_len) {
			LOG_ERR("Request-Tag list too large");
			return -ENOMEM;
		}

		/* Serialize: [len][bytes] */
		out_data[data_len++] = (uint8_t)options[i].len;
		if (options[i].len > 0) {
			memcpy(&out_data[data_len], options[i].value, options[i].len);
			data_len += options[i].len;
		}
	}

	*out_count = (uint8_t)num_found;
	*out_data_len = data_len;
	return 0;
}

/**
 * @brief Compare two Request-Tag lists for equality
 *
 * Per RFC 9175 Section 3.3, Request-Tag lists must match exactly.
 * Absent Request-Tag is distinct from present with 0-length.
 *
 * @return true if lists match exactly, false otherwise
 */
static bool request_tag_lists_equal(uint8_t count_a, const uint8_t *data_a, size_t len_a,
				     uint8_t count_b, const uint8_t *data_b, size_t len_b)
{
	/* Count must match (including 0 = absent) */
	if (count_a != count_b) {
		return false;
	}

	/* If both absent, they match */
	if (count_a == 0) {
		return true;
	}

	/* Length and data must match exactly */
	return (len_a == len_b) && (memcmp(data_a, data_b, len_a) == 0);
}

/**
 * @brief Find outer Block1 cache entry by address, token, and Request-Tag list
 *
 * Per RFC 9175 Section 3.3, Request-Tag is part of the blockwise operation key.
 */
static struct coap_edhoc_outer_block_entry *
outer_block_find(struct coap_edhoc_outer_block_entry *cache,
		 size_t cache_size,
		 const struct net_sockaddr *addr,
		 net_socklen_t addr_len,
		 const uint8_t *token,
		 uint8_t tkl,
		 uint8_t request_tag_count,
		 const uint8_t *request_tag_data,
		 size_t request_tag_data_len)
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

		/* Check if address, token, and Request-Tag list match */
		if (cache[i].tkl == tkl &&
		    sockaddr_equal(&cache[i].addr, cache[i].addr_len, addr, addr_len) &&
		    memcmp(cache[i].token, token, tkl) == 0 &&
		    request_tag_lists_equal(cache[i].request_tag_count,
					    cache[i].request_tag_data,
					    cache[i].request_tag_data_len,
					    request_tag_count,
					    request_tag_data,
					    request_tag_data_len)) {
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
		      uint8_t tkl,
		      uint8_t request_tag_count,
		      const uint8_t *request_tag_data,
		      size_t request_tag_data_len)
{
	struct coap_edhoc_outer_block_entry *entry;
	struct coap_edhoc_outer_block_entry *oldest = NULL;
	int64_t oldest_time = INT64_MAX;

	/* Try to find existing entry */
	entry = outer_block_find(cache, cache_size, addr, addr_len, token, tkl,
				 request_tag_count, request_tag_data, request_tag_data_len);
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
#if !defined(CONFIG_ZTEST)
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
#else
	/* In test mode, skip actual sending (service not properly registered) */
	(void)service;
	(void)request;
	(void)block_ctx;
	(void)client_addr;
	(void)client_addr_len;
	return 0;
#endif
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
	/* Clear cache entry first (security-first) */
	if (entry != NULL) {
		outer_block_clear(entry);
	}

#if !defined(CONFIG_ZTEST)
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
#else
	/* In test mode, skip actual sending (service not properly registered) */
	(void)service;
	(void)request;
	(void)error_code;
	(void)client_addr;
	(void)client_addr_len;
	return 0;
#endif
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

	/* Parse Request-Tag list (RFC 9175 Section 3.3: part of operation key) */
	uint8_t request_tag_count = 0;
	uint8_t request_tag_data[64];
	size_t request_tag_data_len = 0;

	ret = parse_request_tag_list(request, &request_tag_count,
				      request_tag_data, &request_tag_data_len,
				      sizeof(request_tag_data));
	if (ret < 0) {
		LOG_ERR("Failed to parse Request-Tag list (%d)", ret);
		(void)send_error_and_clear(service, request,
					   COAP_RESPONSE_CODE_BAD_REQUEST,
					   client_addr, client_addr_len, NULL);
		return COAP_EDHOC_OUTER_BLOCK_ERROR;
	}

	/* Look for existing reassembly (includes Request-Tag in key) */
	entry = outer_block_find(service->data->outer_block_cache,
				 CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
				 client_addr, client_addr_len, token, tkl,
				 request_tag_count, request_tag_data, request_tag_data_len);

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
		/* Block NUM > 0 but no matching reassembly
		 * RFC 9175 Section 3.3: Check if there's an entry with same addr+token but different Request-Tag
		 * If so, clear it (fail-closed policy for mismatched Request-Tag)
		 */
		int64_t now = k_uptime_get();
		for (size_t i = 0; i < CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE; i++) {
			struct coap_edhoc_outer_block_entry *e = &service->data->outer_block_cache[i];
			if (!e->active) {
				continue;
			}
			if ((now - e->timestamp) > CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_LIFETIME_MS) {
				memset(e, 0, sizeof(*e));
				continue;
			}
			if (e->tkl == tkl &&
			    sockaddr_equal(&e->addr, e->addr_len, client_addr, client_addr_len) &&
			    memcmp(e->token, token, tkl) == 0) {
				/* Found entry with same addr+token but different Request-Tag */
				LOG_ERR("Request-Tag mismatch on continuation block (expected count=%u, got count=%u)",
					e->request_tag_count, request_tag_count);
				(void)send_error_and_clear(service, request,
							   COAP_RESPONSE_CODE_BAD_REQUEST,
							   client_addr, client_addr_len, e);
				return COAP_EDHOC_OUTER_BLOCK_ERROR;
			}
		}
		/* No matching reassembly - not a combined request */
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
					      client_addr, client_addr_len, token, tkl,
					      request_tag_count, request_tag_data, request_tag_data_len);
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
		/* Store Request-Tag list (RFC 9175 Section 3.3: part of operation key) */
		if (request_tag_data_len > sizeof(entry->request_tag_data)) {
			LOG_ERR("Request-Tag list too large (%zu > %zu)",
				request_tag_data_len, sizeof(entry->request_tag_data));
			outer_block_clear(entry);
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_BAD_REQUEST,
						   client_addr, client_addr_len, NULL);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}
		entry->request_tag_count = request_tag_count;
		entry->request_tag_data_len = request_tag_data_len;
		if (request_tag_data_len > 0) {
			memcpy(entry->request_tag_data, request_tag_data, request_tag_data_len);
		}
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

		/* RFC 9175 Section 3.3: Request-Tag list must match exactly */
		if (!request_tag_lists_equal(entry->request_tag_count,
					     entry->request_tag_data,
					     entry->request_tag_data_len,
					     request_tag_count,
					     request_tag_data,
					     request_tag_data_len)) {
			LOG_ERR("Request-Tag mismatch on continuation block (expected count=%u, got count=%u)",
				entry->request_tag_count, request_tag_count);
			/* RFC 9175 Section 3.3: different Request-Tag = different operation */
			/* Fail closed: send error and clear entry */
			(void)send_error_and_clear(service, request,
						   COAP_RESPONSE_CODE_BAD_REQUEST,
						   client_addr, client_addr_len, entry);
			return COAP_EDHOC_OUTER_BLOCK_ERROR;
		}

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
	/* Test-only API: find by addr+token only (ignoring Request-Tag)
	 * This allows tests to find entries even with mismatched Request-Tag
	 */
	int64_t now = k_uptime_get();

	for (size_t i = 0; i < cache_size; i++) {
		if (!cache[i].active) {
			continue;
		}

		/* Check if entry has expired */
		if ((now - cache[i].timestamp) > CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_LIFETIME_MS) {
			memset(&cache[i], 0, sizeof(cache[i]));
			continue;
		}

		/* Match on address and token only */
		if (cache[i].tkl == tkl &&
		    sockaddr_equal(&cache[i].addr, cache[i].addr_len, addr, addr_len) &&
		    memcmp(cache[i].token, token, tkl) == 0) {
			return &cache[i];
		}
	}

	return NULL;
}

void coap_edhoc_outer_block_clear(struct coap_edhoc_outer_block_entry *entry)
{
	outer_block_clear(entry);
}
#endif /* CONFIG_ZTEST */
