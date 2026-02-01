/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"

#if defined(CONFIG_COAP_Q_BLOCK)

/**
 * @brief Test Q-Block option constants
 *
 * Verifies RFC 9177 Section 12.1 Table 4 option numbers and Section 12.3 Table 5 content-format.
 */
ZTEST(coap, test_q_block_constants)
{
	/* RFC 9177 Section 12.1 Table 4: Q-Block1 = 19, Q-Block2 = 31 */
	zassert_equal(COAP_OPTION_Q_BLOCK1, 19, "Q-Block1 option number must be 19");
	zassert_equal(COAP_OPTION_Q_BLOCK2, 31, "Q-Block2 option number must be 31");

	/* RFC 9177 Section 12.3 Table 5: application/missing-blocks+cbor-seq = 272 */
	zassert_equal(COAP_CONTENT_FORMAT_APP_MISSING_BLOCKS_CBOR_SEQ, 272,
		      "Missing blocks content-format must be 272");
}

/**
 * @brief Test Q-Block1 option encode/decode
 *
 * Tests RFC 9177 Section 4.2 Q-Block option structure (NUM/M/SZX).
 */
ZTEST(coap, test_q_block1_option_encode_decode)
{
	struct coap_packet cpkt;
	uint8_t buf[128];
	uint8_t token[] = { 0x42 };
	bool has_more;
	uint32_t block_number;
	int ret;
	int block_size;

	/* Initialize packet */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append Q-Block1 option: NUM=5, M=1, SZX=2 (64 bytes) */
	ret = coap_append_q_block1_option(&cpkt, 5, true, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Failed to append Q-Block1 option");

	/* Decode and verify */
	block_size = coap_get_q_block1_option(&cpkt, &has_more, &block_number);
	zassert_equal(block_size, 64, "Block size should be 64");
	zassert_true(has_more, "More flag should be set");
	zassert_equal(block_number, 5, "Block number should be 5");

	/* Test without more flag */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_q_block1_option(&cpkt, 10, false, COAP_BLOCK_256);
	zassert_equal(ret, 0, "Failed to append Q-Block1 option");

	block_size = coap_get_q_block1_option(&cpkt, &has_more, &block_number);
	zassert_equal(block_size, 256, "Block size should be 256");
	zassert_false(has_more, "More flag should not be set");
	zassert_equal(block_number, 10, "Block number should be 10");
}

/**
 * @brief Test Q-Block2 option encode/decode
 *
 * Tests RFC 9177 §4.2 Q-Block option structure (NUM/M/SZX).
 */
ZTEST(coap, test_q_block2_option_encode_decode)
{
	struct coap_packet cpkt;
	uint8_t buf[128];
	uint8_t token[] = { 0x43 };
	bool has_more;
	uint32_t block_number;
	int ret;
	int block_size;

	/* Initialize packet */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_ACK, sizeof(token), token,
			       COAP_RESPONSE_CODE_CONTENT, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append Q-Block2 option: NUM=3, M=1, SZX=4 (256 bytes) */
	ret = coap_append_q_block2_option(&cpkt, 3, true, COAP_BLOCK_256);
	zassert_equal(ret, 0, "Failed to append Q-Block2 option");

	/* Decode and verify */
	block_size = coap_get_q_block2_option(&cpkt, &has_more, &block_number);
	zassert_equal(block_size, 256, "Block size should be 256");
	zassert_true(has_more, "More flag should be set");
	zassert_equal(block_number, 3, "Block number should be 3");
}

/**
 * @brief Test Block/Q-Block mixing validation
 *
 * Tests RFC 9177 §4.1: MUST NOT mix Block and Q-Block in same packet.
 */
ZTEST(coap, test_block_q_block_mixing_validation)
{
	struct coap_packet cpkt;
	uint8_t buf[128];
	uint8_t token[] = { 0x44 };
	int ret;

	/* Test 1: Only Block1 - should be valid */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_option_int(&cpkt, COAP_OPTION_BLOCK1, 0x08); /* NUM=0, M=1, SZX=0 */
	zassert_equal(ret, 0, "Failed to append Block1");

	ret = coap_validate_block_q_block_mixing(&cpkt);
	zassert_equal(ret, 0, "Only Block1 should be valid");

	/* Test 2: Only Q-Block1 - should be valid */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_q_block1_option(&cpkt, 0, true, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Failed to append Q-Block1");

	ret = coap_validate_block_q_block_mixing(&cpkt);
	zassert_equal(ret, 0, "Only Q-Block1 should be valid");

	/* Test 3: Block1 + Q-Block1 - should be invalid */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1236);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_option_int(&cpkt, COAP_OPTION_BLOCK1, 0x08);
	zassert_equal(ret, 0, "Failed to append Block1");

	ret = coap_append_q_block1_option(&cpkt, 0, true, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Failed to append Q-Block1");

	ret = coap_validate_block_q_block_mixing(&cpkt);
	zassert_equal(ret, -EINVAL, "Block1 + Q-Block1 should be invalid");

	/* Test 4: Block2 + Q-Block2 - should be invalid */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_ACK, sizeof(token), token,
			       COAP_RESPONSE_CODE_CONTENT, 0x1237);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_option_int(&cpkt, COAP_OPTION_BLOCK2, 0x18); /* NUM=1, M=1, SZX=0 */
	zassert_equal(ret, 0, "Failed to append Block2");

	ret = coap_append_q_block2_option(&cpkt, 1, true, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Failed to append Q-Block2");

	ret = coap_validate_block_q_block_mixing(&cpkt);
	zassert_equal(ret, -EINVAL, "Block2 + Q-Block2 should be invalid");

	/* Test 5: Block1 + Q-Block2 - should be invalid */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1238);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_option_int(&cpkt, COAP_OPTION_BLOCK1, 0x08);
	zassert_equal(ret, 0, "Failed to append Block1");

	ret = coap_append_q_block2_option(&cpkt, 0, true, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Failed to append Q-Block2");

	ret = coap_validate_block_q_block_mixing(&cpkt);
	zassert_equal(ret, -EINVAL, "Block1 + Q-Block2 should be invalid");
}

#if defined(CONFIG_ZCBOR)
/**
 * @brief Test CBOR Sequence encoding for missing blocks
 *
 * Tests RFC 9177 §5 missing-blocks payload encoding.
 */
ZTEST(coap, test_missing_blocks_cbor_encode)
{
	uint8_t payload[64];
	size_t encoded_len;
	int ret;

	/* Test 1: Encode single missing block */
	uint32_t missing1[] = { 3 };
	ret = coap_encode_missing_blocks_cbor_seq(payload, sizeof(payload),
						  missing1, ARRAY_SIZE(missing1),
						  &encoded_len);
	zassert_equal(ret, 0, "Failed to encode single missing block");
	zassert_true(encoded_len > 0, "Encoded length should be > 0");
	zassert_true(encoded_len < sizeof(payload), "Encoded length should fit in buffer");

	/* Test 2: Encode multiple missing blocks in ascending order */
	uint32_t missing2[] = { 1, 5, 7, 10 };
	ret = coap_encode_missing_blocks_cbor_seq(payload, sizeof(payload),
						  missing2, ARRAY_SIZE(missing2),
						  &encoded_len);
	zassert_equal(ret, 0, "Failed to encode multiple missing blocks");
	zassert_true(encoded_len > 0, "Encoded length should be > 0");

	/* Test 3: Non-ascending order should fail */
	uint32_t missing3[] = { 5, 3, 7 };
	ret = coap_encode_missing_blocks_cbor_seq(payload, sizeof(payload),
						  missing3, ARRAY_SIZE(missing3),
						  &encoded_len);
	zassert_equal(ret, -EINVAL, "Non-ascending order should fail");

	/* Test 4: Empty list */
	ret = coap_encode_missing_blocks_cbor_seq(payload, sizeof(payload),
						  NULL, 0, &encoded_len);
	zassert_equal(ret, 0, "Empty list should succeed");
	zassert_equal(encoded_len, 0, "Empty list should have 0 length");
}

/**
 * @brief Test CBOR Sequence decoding for missing blocks
 *
 * Tests RFC 9177 §5 missing-blocks payload decoding.
 */
ZTEST(coap, test_missing_blocks_cbor_decode)
{
	uint8_t payload[64];
	uint32_t missing_in[] = { 2, 4, 6, 8 };
	uint32_t missing_out[10];
	size_t encoded_len;
	size_t decoded_count;
	int ret;

	/* Encode a list of missing blocks */
	ret = coap_encode_missing_blocks_cbor_seq(payload, sizeof(payload),
						  missing_in, ARRAY_SIZE(missing_in),
						  &encoded_len);
	zassert_equal(ret, 0, "Failed to encode");

	/* Decode and verify */
	ret = coap_decode_missing_blocks_cbor_seq(payload, encoded_len,
						  missing_out, ARRAY_SIZE(missing_out),
						  &decoded_count);
	zassert_equal(ret, 0, "Failed to decode");
	zassert_equal(decoded_count, ARRAY_SIZE(missing_in), "Decoded count mismatch");

	for (size_t i = 0; i < decoded_count; i++) {
		zassert_equal(missing_out[i], missing_in[i],
			      "Decoded block %zu mismatch: expected %u, got %u",
			      i, missing_in[i], missing_out[i]);
	}

	/* Test empty payload */
	ret = coap_decode_missing_blocks_cbor_seq(NULL, 0,
						  missing_out, ARRAY_SIZE(missing_out),
						  &decoded_count);
	zassert_equal(ret, 0, "Empty payload should succeed");
	zassert_equal(decoded_count, 0, "Empty payload should have 0 count");
}

/**
 * @brief Test CBOR Sequence decode with duplicates
 *
 * Tests RFC 9177 §5: client ignores duplicates.
 */
ZTEST(coap, test_missing_blocks_cbor_decode_duplicates)
{
	uint8_t payload[64];
	uint32_t missing_out[10];
	size_t decoded_count;
	int ret;

	/* Manually create CBOR Sequence with duplicates: 1, 3, 3, 5 */
	/* CBOR encoding: uint 1 = 0x01, uint 3 = 0x03, uint 5 = 0x05 */
	payload[0] = 0x01; /* 1 */
	payload[1] = 0x03; /* 3 */
	payload[2] = 0x03; /* 3 (duplicate) */
	payload[3] = 0x05; /* 5 */
	size_t payload_len = 4;

	ret = coap_decode_missing_blocks_cbor_seq(payload, payload_len,
						  missing_out, ARRAY_SIZE(missing_out),
						  &decoded_count);
	zassert_equal(ret, 0, "Decode with duplicates should succeed");

	/* Should have 3 blocks (duplicate removed) */
	zassert_equal(decoded_count, 3, "Should have 3 blocks (duplicate removed)");
	zassert_equal(missing_out[0], 1, "First block should be 1");
	zassert_equal(missing_out[1], 3, "Second block should be 3");
	zassert_equal(missing_out[2], 5, "Third block should be 5");
}
#endif /* CONFIG_ZCBOR */

#endif /* CONFIG_COAP_Q_BLOCK */


