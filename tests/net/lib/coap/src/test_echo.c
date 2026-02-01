/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


#if defined(CONFIG_COAP_SERVER_ECHO)

#include <zephyr/net/coap_service.h>

/* Forward declarations for Echo test helpers */
extern struct coap_echo_entry *coap_echo_cache_find(struct coap_echo_entry *cache,
						    const struct net_sockaddr *addr,
						    net_socklen_t addr_len);
extern int coap_echo_create_challenge(struct coap_echo_entry *cache,
				      const struct net_sockaddr *addr,
				      net_socklen_t addr_len,
				      uint8_t *echo_value, size_t *echo_len);
extern int coap_echo_verify_value(struct coap_echo_entry *cache,
				  const struct net_sockaddr *addr,
				  net_socklen_t addr_len,
				  const uint8_t *echo_value, size_t echo_len);
extern bool coap_echo_is_address_verified(struct coap_echo_entry *cache,
					  const struct net_sockaddr *addr,
					  net_socklen_t addr_len);
extern int coap_echo_build_challenge_response(struct coap_packet *response,
					      const struct coap_packet *request,
					      const uint8_t *echo_value,
					      size_t echo_len,
					      uint8_t *buf, size_t buf_len);
extern bool coap_is_unsafe_method(uint8_t code);
extern int coap_echo_extract_from_request(const struct coap_packet *request,
					  uint8_t *echo_value, size_t *echo_len);

/* Test Echo option length validation per RFC 9175 Section 2.2.1 */
ZTEST(coap, test_echo_option_length_validation)
{
	struct coap_echo_entry cache[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE] = {0};
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = peer_addr,
		.sin6_port = net_htons(5683)
	};
	uint8_t echo_value[41];
	size_t echo_len;
	int ret;

	/* Valid Echo length (1-40 bytes) */
	echo_len = 8;
	ret = coap_echo_create_challenge(cache, (struct net_sockaddr *)&addr,
					 sizeof(addr), echo_value, &echo_len);
	zassert_equal(ret, 0, "Should create challenge with valid length");
	zassert_equal(echo_len, CONFIG_COAP_SERVER_ECHO_MAX_LEN,
		     "Echo length should match config");

	/* Verify with valid length */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, echo_len);
	zassert_equal(ret, 0, "Should verify valid Echo value");

	/* Test invalid length: 0 bytes (caught by extract function) */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, 0);
	zassert_equal(ret, -EINVAL, "Should reject Echo with length 0");

	/* Test invalid length: > 40 bytes */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, 41);
	zassert_equal(ret, -EINVAL, "Should reject Echo with length > 40");
}

/* Test unsafe method freshness requirement per RFC 9175 Section 2.3 */
ZTEST(coap, test_echo_unsafe_method_detection)
{
	/* Test that unsafe methods are correctly identified */
	zassert_true(coap_is_unsafe_method(COAP_METHOD_POST),
		    "POST should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_PUT),
		    "PUT should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_DELETE),
		    "DELETE should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_PATCH),
		    "PATCH should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_IPATCH),
		    "IPATCH should be unsafe");

	/* Test that safe methods are not flagged */
	zassert_false(coap_is_unsafe_method(COAP_METHOD_GET),
		     "GET should be safe");
	zassert_false(coap_is_unsafe_method(COAP_METHOD_FETCH),
		     "FETCH should be safe");
}

/* Test Echo challenge and verification flow */
ZTEST(coap, test_echo_challenge_verification_flow)
{
	struct coap_echo_entry cache[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE] = {0};
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = peer_addr,
		.sin6_port = net_htons(5683)
	};
	uint8_t echo_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];
	size_t echo_len;
	int ret;

	/* Step 1: Create initial challenge */
	ret = coap_echo_create_challenge(cache, (struct net_sockaddr *)&addr,
					 sizeof(addr), echo_value, &echo_len);
	zassert_equal(ret, 0, "Should create challenge");
	zassert_equal(echo_len, CONFIG_COAP_SERVER_ECHO_MAX_LEN,
		     "Echo length should match config");

	/* Step 2: Verify the challenge succeeds */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, echo_len);
	zassert_equal(ret, 0, "Should verify correct Echo value");

	/* Step 3: Verify address is now verified for amplification mitigation */
	bool verified = coap_echo_is_address_verified(cache,
						      (struct net_sockaddr *)&addr,
						      sizeof(addr));
	zassert_true(verified, "Address should be verified after successful Echo");

	/* Step 4: Verify wrong Echo value fails */
	uint8_t wrong_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];

	memset(wrong_value, 0xFF, sizeof(wrong_value));
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), wrong_value, echo_len);
	zassert_equal(ret, -EINVAL, "Should reject incorrect Echo value");
}

/* Test Echo challenge response format per RFC 9175 Section 2.4 item 3 */
ZTEST(coap, test_echo_challenge_response_format)
{
	uint8_t request_buf[COAP_BUF_SIZE];
	uint8_t response_buf[COAP_BUF_SIZE];
	struct coap_packet request;
	struct coap_packet response;
	uint8_t echo_value[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	uint8_t token[4] = {0xAA, 0xBB, 0xCC, 0xDD};
	int ret;

	/* Test CON request -> ACK response with Echo */
	ret = coap_packet_init(&request, request_buf, sizeof(request_buf),
			      COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			      COAP_METHOD_PUT, coap_next_id());
	zassert_equal(ret, 0, "Should init CON request");

	ret = coap_echo_build_challenge_response(&response, &request,
						echo_value, sizeof(echo_value),
						response_buf, sizeof(response_buf));
	zassert_equal(ret, 0, "Should build challenge response");

	/* Verify response is ACK type per RFC 9175 */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_ACK,
		     "CON request should get ACK response");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_UNAUTHORIZED,
		     "Should be 4.01 Unauthorized");

	/* Verify Echo option is present */
	struct coap_option option;

	ret = coap_find_options(&response, COAP_OPTION_ECHO, &option, 1);
	zassert_equal(ret, 1, "Should find Echo option");
	zassert_equal(option.len, sizeof(echo_value), "Echo length should match");
	zassert_mem_equal(option.value, echo_value, sizeof(echo_value),
			 "Echo value should match");

	/* Test NON request -> NON response with Echo */
	ret = coap_packet_init(&request, request_buf, sizeof(request_buf),
			      COAP_VERSION_1, COAP_TYPE_NON_CON, sizeof(token), token,
			      COAP_METHOD_PUT, coap_next_id());
	zassert_equal(ret, 0, "Should init NON request");

	ret = coap_echo_build_challenge_response(&response, &request,
						echo_value, sizeof(echo_value),
						response_buf, sizeof(response_buf));
	zassert_equal(ret, 0, "Should build challenge response");

	/* Verify response is NON type per RFC 9175 */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_NON_CON,
		     "NON request should get NON response");
}

/* Test Echo cache management (LRU eviction) */
ZTEST(coap, test_echo_cache_lru_eviction)
{
	struct coap_echo_entry cache[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE] = {0};
	struct net_sockaddr_in6 addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE + 1];
	uint8_t echo_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];
	size_t echo_len;
	int ret;

	/* Fill the cache */
	for (int i = 0; i < CONFIG_COAP_SERVER_ECHO_CACHE_SIZE; i++) {
		addrs[i].sin6_family = NET_AF_INET6;
		addrs[i].sin6_addr = dummy_addr.sin6_addr;
		addrs[i].sin6_port = net_htons(5683 + i);

		ret = coap_echo_create_challenge(cache, (struct net_sockaddr *)&addrs[i],
						sizeof(addrs[i]), echo_value, &echo_len);
		zassert_equal(ret, 0, "Should create challenge %d", i);

		/* Small delay to ensure different timestamps */
		k_msleep(1);
	}

	/* Verify all entries are in cache */
	for (int i = 0; i < CONFIG_COAP_SERVER_ECHO_CACHE_SIZE; i++) {
		struct coap_echo_entry *entry = coap_echo_cache_find(
			cache, (struct net_sockaddr *)&addrs[i], sizeof(addrs[i]));
		zassert_not_null(entry, "Entry %d should be in cache", i);
	}

	/* Add one more entry - should evict the oldest (first) */
	addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE].sin6_family = NET_AF_INET6;
	addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE].sin6_addr = dummy_addr.sin6_addr;
	addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE].sin6_port =
		net_htons(5683 + CONFIG_COAP_SERVER_ECHO_CACHE_SIZE);

	ret = coap_echo_create_challenge(
		cache,
		(struct net_sockaddr *)&addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE],
		sizeof(addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE]),
		echo_value, &echo_len);
	zassert_equal(ret, 0, "Should create challenge for new entry");

	/* Verify first entry was evicted */
	struct coap_echo_entry *entry = coap_echo_cache_find(
		cache, (struct net_sockaddr *)&addrs[0], sizeof(addrs[0]));
	zassert_is_null(entry, "Oldest entry should be evicted");

	/* Verify new entry is in cache */
	entry = coap_echo_cache_find(
		cache,
		(struct net_sockaddr *)&addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE],
		sizeof(addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE]));
	zassert_not_null(entry, "New entry should be in cache");
}

/* Test Echo option extraction from request */
ZTEST(coap, test_echo_extract_from_request)
{
	uint8_t request_buf[COAP_BUF_SIZE];
	uint8_t request_buf2[COAP_BUF_SIZE];
	struct coap_packet request;
	uint8_t echo_value_in[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	uint8_t echo_value_out[40];
	size_t echo_len_out;
	int ret;

	/* Initialize buffers to avoid parsing uninitialized memory */
	memset(request_buf, 0, sizeof(request_buf));
	memset(request_buf2, 0, sizeof(request_buf2));

	/* Create request with Echo option */
	ret = coap_packet_init(&request, request_buf, sizeof(request_buf),
			      COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			      COAP_METHOD_PUT, coap_next_id());
	zassert_equal(ret, 0, "Should init request");

	ret = coap_packet_append_option(&request, COAP_OPTION_ECHO,
				       echo_value_in, sizeof(echo_value_in));
	zassert_equal(ret, 0, "Should append Echo option");

	/* Extract Echo option */
	ret = coap_echo_extract_from_request(&request, echo_value_out, &echo_len_out);
	zassert_equal(ret, 0, "Should extract Echo option");
	zassert_equal(echo_len_out, sizeof(echo_value_in), "Echo length should match");
	zassert_mem_equal(echo_value_out, echo_value_in, sizeof(echo_value_in),
			 "Echo value should match");

	/* Test request without Echo option - use fresh buffer */
	ret = coap_packet_init(&request, request_buf2, sizeof(request_buf2),
			      COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			      COAP_METHOD_GET, coap_next_id());
	zassert_equal(ret, 0, "Should init request");

	ret = coap_echo_extract_from_request(&request, echo_value_out, &echo_len_out);
	zassert_equal(ret, -ENOENT, "Should return -ENOENT for missing Echo, got %d", ret);
}

#endif /* CONFIG_COAP_SERVER_ECHO */
