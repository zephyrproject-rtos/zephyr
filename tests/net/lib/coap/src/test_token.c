/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


ZTEST(coap, test_packet_init_invalid_token_len)
{
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t token[COAP_TOKEN_MAX_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	int r;

	/* Test with token_len = 9 (reserved per RFC 7252 Section 3) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 9, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 9");

	/* Test with token_len = 15 (reserved per RFC 7252 Section 3) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 15, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 15");

	/* Test with token_len > 15 */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 255, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 255");
}

ZTEST(coap, test_packet_init_null_token_with_nonzero_len)
{
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	int r;

	/* Test with token_len > 0 but token = NULL */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 4, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len > 0 with NULL token");

	/* Test with token_len = 1 but token = NULL */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 1, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 1 with NULL token");

	/* Test with token_len = 8 but token = NULL */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 8 with NULL token");
}

ZTEST(coap, test_packet_init_valid_token_len)
{
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t token[COAP_TOKEN_MAX_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	int r;

	/* Test with token_len = 0 and token = NULL (valid) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 0 with NULL token");

	/* Test with token_len = 0 and token != NULL (valid, token is ignored) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 0 with non-NULL token");

	/* Test with token_len = 1 and valid token */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 1, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 1 with valid token");

	/* Test with token_len = 8 and valid token */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 8 with valid token");

	/* Test with token_len = 4 and valid token */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 4, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 4 with valid token");
}

ZTEST(coap, test_packet_parse_rejects_invalid_tkl)
{
	/* Test that parsing a packet with TKL=9 returns -EBADMSG */
	uint8_t pdu_with_tkl_9[] = {
		0x49, /* Ver=1, Type=CON, TKL=9 (reserved) */
		0x01, /* Code=GET */
		0x12, 0x34 /* Message ID */
	};
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	int r;

	memcpy(data, pdu_with_tkl_9, sizeof(pdu_with_tkl_9));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu_with_tkl_9), NULL, 0);
	zassert_equal(r, -EBADMSG, "Should reject packet with TKL=9");

	/* Test with TKL=15 (also reserved) */
	uint8_t pdu_with_tkl_15[] = {
		0x4F, /* Ver=1, Type=CON, TKL=15 (reserved) */
		0x01, /* Code=GET */
		0x12, 0x34 /* Message ID */
	};

	memcpy(data, pdu_with_tkl_15, sizeof(pdu_with_tkl_15));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu_with_tkl_15), NULL, 0);
	zassert_equal(r, -EBADMSG, "Should reject packet with TKL=15");
}

ZTEST(coap, test_next_token_is_sequence_and_unique)
{
	/* Test RFC9175-compliant sequence-based token generation */
	uint8_t token1[COAP_TOKEN_MAX_LEN], token2[COAP_TOKEN_MAX_LEN], token3[COAP_TOKEN_MAX_LEN];
	uint8_t *token_ptr;
	uint32_t prefix, seq1, seq2, seq3;

	/* Reset token generator with a known prefix for deterministic testing */
	coap_token_generator_reset(0x12345678);

	/* Get first token and copy it (coap_next_token returns pointer to static buffer) */
	token_ptr = coap_next_token();
	zassert_not_null(token_ptr, "Token should not be NULL");
	memcpy(token1, token_ptr, COAP_TOKEN_MAX_LEN);

	/* Extract prefix and sequence from token1 (big-endian encoding) */
	prefix = (token1[0] << 24) | (token1[1] << 16) | (token1[2] << 8) | token1[3];
	seq1 = (token1[4] << 24) | (token1[5] << 16) | (token1[6] << 8) | token1[7];

	/* Verify prefix is correct */
	zassert_equal(prefix, 0x12345678, "Token prefix should match reset value");

	/* Verify sequence starts at 0 (RFC9175 §4.2: "starting at zero") */
	zassert_equal(seq1, 0, "First token sequence should be 0");

	/* Get second token and copy it */
	token_ptr = coap_next_token();
	zassert_not_null(token_ptr, "Token should not be NULL");
	memcpy(token2, token_ptr, COAP_TOKEN_MAX_LEN);

	/* Extract sequence from token2 */
	seq2 = (token2[4] << 24) | (token2[5] << 16) | (token2[6] << 8) | token2[7];

	/* Verify sequence increments */
	zassert_equal(seq2, 1, "Second token sequence should be 1");

	/* Verify tokens are unique */
	zassert_true(memcmp(token1, token2, COAP_TOKEN_MAX_LEN) != 0,
		     "Tokens should be unique");

	/* Get third token and copy it */
	token_ptr = coap_next_token();
	memcpy(token3, token_ptr, COAP_TOKEN_MAX_LEN);
	seq3 = (token3[4] << 24) | (token3[5] << 16) | (token3[6] << 8) | token3[7];

	/* Verify sequence continues to increment */
	zassert_equal(seq3, 2, "Third token sequence should be 2");

	/* Verify all three tokens are unique */
	zassert_true(memcmp(token1, token3, COAP_TOKEN_MAX_LEN) != 0,
		     "Token 1 and 3 should be unique");
	zassert_true(memcmp(token2, token3, COAP_TOKEN_MAX_LEN) != 0,
		     "Token 2 and 3 should be unique");
}

ZTEST(coap, test_token_generator_rekey)
{
	/* Test that rekey generates new prefix and resets sequence */
	uint8_t token1[COAP_TOKEN_MAX_LEN], token2[COAP_TOKEN_MAX_LEN];
	uint8_t *token_ptr;
	uint32_t prefix1, prefix2, seq1, seq2;

	/* First rekey */
	coap_token_generator_rekey();
	token_ptr = coap_next_token();
	memcpy(token1, token_ptr, COAP_TOKEN_MAX_LEN);
	prefix1 = (token1[0] << 24) | (token1[1] << 16) | (token1[2] << 8) | token1[3];
	seq1 = (token1[4] << 24) | (token1[5] << 16) | (token1[6] << 8) | token1[7];

	/* Sequence should start at 0 after rekey */
	zassert_equal(seq1, 0, "Sequence should be 0 after rekey");

	/* Second rekey */
	coap_token_generator_rekey();
	token_ptr = coap_next_token();
	memcpy(token2, token_ptr, COAP_TOKEN_MAX_LEN);
	prefix2 = (token2[0] << 24) | (token2[1] << 16) | (token2[2] << 8) | token2[3];
	seq2 = (token2[4] << 24) | (token2[5] << 16) | (token2[6] << 8) | token2[7];

	/* Sequence should reset to 0 after rekey */
	zassert_equal(seq2, 0, "Sequence should reset to 0 after rekey");

	/* Prefixes should be different (with very high probability) */
	zassert_true(prefix1 != prefix2,
		     "Rekey should generate different prefix (may fail rarely due to randomness)");
}

ZTEST(coap, test_request_tag_generation_not_recycled)
{
	/* Test that Request-Tags are not recycled (use sequence-based generation) */
	uint8_t tag1[COAP_TOKEN_MAX_LEN], tag2[COAP_TOKEN_MAX_LEN], tag3[COAP_TOKEN_MAX_LEN];
	uint8_t *tag_ptr;

	/* Reset token generator for deterministic testing */
	coap_token_generator_reset(0xAABBCCDD);

	/* Generate multiple Request-Tags (using coap_next_token which is used for Request-Tag) */
	tag_ptr = coap_next_token();
	memcpy(tag1, tag_ptr, COAP_TOKEN_MAX_LEN);
	
	tag_ptr = coap_next_token();
	memcpy(tag2, tag_ptr, COAP_TOKEN_MAX_LEN);
	
	tag_ptr = coap_next_token();
	memcpy(tag3, tag_ptr, COAP_TOKEN_MAX_LEN);

	/* Verify all tags are unique (never recycled) */
	zassert_true(memcmp(tag1, tag2, COAP_TOKEN_MAX_LEN) != 0,
		     "Request-Tags should not be recycled");
	zassert_true(memcmp(tag1, tag3, COAP_TOKEN_MAX_LEN) != 0,
		     "Request-Tags should not be recycled");
	zassert_true(memcmp(tag2, tag3, COAP_TOKEN_MAX_LEN) != 0,
		     "Request-Tags should not be recycled");

	/* Verify they follow sequence pattern */
	uint32_t seq1 = (tag1[4] << 24) | (tag1[5] << 16) | (tag1[6] << 8) | tag1[7];
	uint32_t seq2 = (tag2[4] << 24) | (tag2[5] << 16) | (tag2[6] << 8) | tag2[7];
	uint32_t seq3 = (tag3[4] << 24) | (tag3[5] << 16) | (tag3[6] << 8) | tag3[7];

	zassert_equal(seq2, seq1 + 1, "Request-Tags should follow sequence");
	zassert_equal(seq3, seq2 + 1, "Request-Tags should follow sequence");
}
