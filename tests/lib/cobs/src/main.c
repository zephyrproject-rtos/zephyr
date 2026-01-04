/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/ztest_assert.h"
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/data/cobs.h>

#define TEST_BUF_SIZE  1024
#define TEST_BUF_COUNT 3

NET_BUF_POOL_DEFINE(test_pool, TEST_BUF_COUNT, TEST_BUF_SIZE, 0, NULL);

struct cobs_tests_fixture {
	struct net_buf *test_data;
	struct net_buf *encoded;
	struct net_buf *decoded;
};

static void *cobs_test_setup(void)
{
	struct cobs_tests_fixture *fixture = malloc(sizeof(struct cobs_tests_fixture));

	zassume_not_null(fixture);

	fixture->test_data = net_buf_alloc(&test_pool, K_NO_WAIT);
	fixture->encoded = net_buf_alloc(&test_pool, K_NO_WAIT);
	fixture->decoded = net_buf_alloc(&test_pool, K_NO_WAIT);

	zassert_not_null(fixture->test_data, "Failed to allocate test_data buffer");
	zassert_not_null(fixture->encoded, "Failed to allocate encoded buffer");
	zassert_not_null(fixture->decoded, "Failed to allocate decoded buffer");

	net_buf_reset(fixture->test_data);
	net_buf_reset(fixture->encoded);
	net_buf_reset(fixture->decoded);

	return fixture;
}

static void cobs_test_before(void *f)
{
	struct cobs_tests_fixture *fixture = (struct cobs_tests_fixture *)f;

	net_buf_reset(fixture->test_data);
	net_buf_reset(fixture->encoded);
	net_buf_reset(fixture->decoded);
}

static void cobs_test_teardown(void *f)
{
	struct cobs_tests_fixture *fixture = (struct cobs_tests_fixture *)f;

	if (fixture->test_data) {
		net_buf_unref(fixture->test_data);
	}
	if (fixture->encoded) {
		net_buf_unref(fixture->encoded);
	}
	if (fixture->decoded) {
		net_buf_unref(fixture->decoded);
	}

	free(fixture);
}

struct cobs_test_item {
	const char *name;
	const uint8_t *decoded;
	size_t decoded_len;
	const uint8_t *encoded;
	size_t encoded_len;
	uint8_t delimiter;
};

#define U8(...) (uint8_t[]) __VA_ARGS__

#define COBS_ITEM(d, e, del, n)                                                                    \
	{.name = n,                                                                                \
	 .decoded = d,                                                                             \
	 .decoded_len = sizeof(d),                                                                 \
	 .encoded = e,                                                                             \
	 .encoded_len = sizeof(e),                                                                 \
	 .delimiter = del}

/* Generate 253-byte sequence programmatically */
static void generate_sequence(uint8_t *buf, size_t len)
{
	static const char pattern[] = "0123456789ABCDEFGHIJKLMNOPQRSTabcdefghijklmnopqrst";

	for (size_t i = 0; i < len; i++) {
		buf[i] = pattern[i % strlen(pattern)];
	}
}

static const struct cobs_test_item cobs_dataset[] = {
	COBS_ITEM(U8({}), U8({0x01}), COBS_DEFAULT_DELIMITER, "Empty"),
	COBS_ITEM(U8({'1'}), U8({0x02, '1'}), COBS_DEFAULT_DELIMITER, "One char"),
	COBS_ITEM(U8({0x00}), U8({0x01, 0x01}), COBS_DEFAULT_DELIMITER, "One zero"),
	COBS_ITEM(U8({0x00, 0x00}), U8({0x01, 0x01, 0x01}), COBS_DEFAULT_DELIMITER, "Two zeroes"),
	COBS_ITEM(U8({0x00, 0x00, 0x00}), U8({0x01, 0x01, 0x01, 0x01}), COBS_DEFAULT_DELIMITER,
		  "Three zeroes"),
	COBS_ITEM(U8({'1', '2', '3', '4', '5'}), U8({0x06, '1', '2', '3', '4', '5'}),
		  COBS_DEFAULT_DELIMITER, "Five chars"),
	COBS_ITEM(U8({'1', '2', '3', '4', '5', 0x00, '6', '7', '8', '9'}),
		  U8({0x06, '1', '2', '3', '4', '5', 0x05, '6', '7', '8', '9'}),
		  COBS_DEFAULT_DELIMITER, "Embedded zero"),
	COBS_ITEM(U8({0x00, '1', '2', '3', '4', '5', 0x00, '6', '7', '8', '9'}),
		  U8({0x01, 0x06, '1', '2', '3', '4', '5', 0x05, '6', '7', '8', '9'}),
		  COBS_DEFAULT_DELIMITER, "Starting zero"),
	COBS_ITEM(U8({'1', '2', '3', '4', '5', 0x00, '6', '7', '8', '9', 0x00}),
		  U8({0x06, '1', '2', '3', '4', '5', 0x05, '6', '7', '8', '9', 0x01}),
		  COBS_DEFAULT_DELIMITER, "Trailing zero"),
	COBS_ITEM(U8({}), U8({0x01}), 0x7F, "Empty with custom delimiter 0x7F"),
	COBS_ITEM(U8({'1'}), U8({0x02, '1'}), 0x7F, "One char with custom delimiter 0x7F"),
	COBS_ITEM(U8({0x7F}), U8({0x01, 0x01}), 0x7F, "One 0x7F delimiter"),
	COBS_ITEM(U8({0x7F, 0x7F}), U8({0x01, 0x01, 0x01}), 0x7F, "Two 0x7F delimiters"),
	COBS_ITEM(U8({0x7F, 0x7F, 0x7F}), U8({0x01, 0x01, 0x01, 0x01}), 0x7F,
		  "Three 0x7F delimiters"),
};

ZTEST_SUITE(cobs_tests, NULL, cobs_test_setup, cobs_test_before, NULL, cobs_test_teardown);

/* Helper: test encode/decode roundtrip with flags */
static void test_roundtrip(struct cobs_tests_fixture *fixture, const struct cobs_test_item *item,
			   uint32_t flags)
{
	uint8_t delimiter = item->delimiter;
	size_t expected_len = item->encoded_len + ((flags & COBS_FLAG_TRAILING_DELIMITER) ? 1 : 0);

	/* Arrange & Act: Encode */
	net_buf_add_mem(fixture->test_data, item->decoded, item->decoded_len);
	int ret = cobs_encode(fixture->test_data, fixture->encoded, flags);

	/* Assert: Encoding results */
	zassert_ok(ret, "Encode failed: %s", item->name);
	zassert_equal(fixture->encoded->len, expected_len, "Encoded len: %s", item->name);
	zassert_mem_equal(fixture->encoded->data, item->encoded, item->encoded_len,
			  "Encoded data: %s", item->name);
	zassert_equal(fixture->test_data->len, 0, "Encode consumed input: %s", item->name);
	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		zassert_equal(fixture->encoded->data[item->encoded_len], delimiter,
			      "Trailing delim: %s", item->name);
	}

	/* Act: Decode */
	ret = cobs_decode(fixture->encoded, fixture->decoded, flags);

	/* Assert: Decoding results */
	zassert_ok(ret, "Decode failed: %s", item->name);
	zassert_equal(fixture->decoded->len, item->decoded_len, "Decoded len: %s", item->name);
	zassert_mem_equal(fixture->decoded->data, item->decoded, item->decoded_len,
			  "Decoded data: %s", item->name);
	zassert_equal(fixture->encoded->len, 0, "Decode consumed input: %s", item->name);
}

/* Helper: test stream encode/decode roundtrip */
static void test_stream_roundtrip_helper(struct cobs_tests_fixture *fixture,
					 const uint8_t *input, size_t input_len,
					 uint8_t delimiter, const char *name)
{
	struct cobs_encode_state enc;
	struct cobs_decode_state dec;
	uint8_t encoded[256];

	/* Arrange & Act: Encode */
	net_buf_add_mem(fixture->test_data, input, input_len);
	cobs_encode_init(&enc);
	size_t enc_len = sizeof(encoded) - 1;

	int ret = cobs_encode_stream(&enc, fixture->test_data, encoded, &enc_len, delimiter);
	zassert_ok(ret, "Stream encode: %s", name);
	encoded[enc_len++] = delimiter;

	/* Act: Decode */
	cobs_decode_init(&dec);
	ret = cobs_decode_stream(&dec, encoded, enc_len, fixture->decoded, delimiter);

	/* Assert: Roundtrip results */
	zassert_true(ret > 0, "Stream decode: %s", name);
	zassert_equal(fixture->decoded->len, input_len, "Stream len: %s", name);
	zassert_mem_equal(fixture->decoded->data, input, input_len, "Stream data: %s", name);
	zassert_true(dec.frame_complete, "Frame complete: %s", name);
}

ZTEST_F(cobs_tests, test_block_encode_decode)
{
	ARRAY_FOR_EACH(cobs_dataset, idx) {
		test_roundtrip(fixture, &cobs_dataset[idx],
			       COBS_FLAG_CUSTOM_DELIMITER(cobs_dataset[idx].delimiter));
		cobs_test_before(fixture);
	}
}

ZTEST_F(cobs_tests, test_block_trailing_delimiter)
{
	ARRAY_FOR_EACH(cobs_dataset, idx) {
		test_roundtrip(fixture, &cobs_dataset[idx],
			       COBS_FLAG_TRAILING_DELIMITER |
			       COBS_FLAG_CUSTOM_DELIMITER(cobs_dataset[idx].delimiter));
		cobs_test_before(fixture);
	}
}

ZTEST_F(cobs_tests, test_block_boundary_conditions)
{
	uint8_t large_data[255];
	struct {
		size_t len;
		size_t encoded_len;
		uint8_t first_code;
		bool test_decode;
	} cases[] = {
		{253, 254, 0xFE, false}, /* Max without split */
		{254, 255, 0xFF, false}, /* Max block */
		{255, 256, 0xFF, true},  /* Requires split + verify roundtrip */
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		/* Arrange */
		generate_sequence(large_data, cases[i].len);
		net_buf_add_mem(fixture->test_data, large_data, cases[i].len);

		/* Act: Encode */
		int ret = cobs_encode(fixture->test_data, fixture->encoded, 0);

		/* Assert: Encoding */
		zassert_ok(ret, "%zu-byte encoding failed", cases[i].len);
		zassert_equal(fixture->encoded->data[0], cases[i].first_code,
			      "%zu-byte code wrong", cases[i].len);

		if (cases[i].test_decode) {
			/* Act: Decode */
			ret = cobs_decode(fixture->encoded, fixture->decoded, 0);

			/* Assert: Roundtrip */
			zassert_ok(ret, "%zu-byte decoding failed", cases[i].len);
			zassert_equal(fixture->decoded->len, cases[i].len,
				      "%zu-byte decoded length wrong", cases[i].len);
			zassert_mem_equal(fixture->decoded->data, large_data, cases[i].len,
					  "%zu-byte roundtrip failed", cases[i].len);
		}
		cobs_test_before(fixture);
	}
}

ZTEST_F(cobs_tests, test_block_decode_errors)
{
	const struct {
		const uint8_t *data;
		size_t len;
		const char *name;
	} error_cases[] = {
		{U8({0x02, 0x00, 0x01}), 3, "Invalid delimiter position"},
		{U8({0x01, 0x00, 0x00, 0x01}), 4, "Consecutive delimiters"},
		{U8({0x03, 0x01}), 2, "Overrun"},
	};

	for (size_t i = 0; i < ARRAY_SIZE(error_cases); i++) {
		/* Arrange */
		net_buf_add_mem(fixture->encoded, error_cases[i].data, error_cases[i].len);

		/* Act */
		int ret = cobs_decode(fixture->encoded, fixture->decoded, 0);

		/* Assert */
		zassert_equal(ret, -EINVAL, "%s not caught", error_cases[i].name);
		cobs_test_before(fixture);
	}
}

/* ========================================================================
 * Streaming Encoder Tests
 * ========================================================================
 */

ZTEST_F(cobs_tests, test_stream_encode_basic)
{
	struct cobs_encode_state enc;
	uint8_t output[64];
	const struct {
		const uint8_t *input;
		size_t input_len;
		const uint8_t *expected;
		size_t expected_len;
		const char *name;
	} cases[] = {
		{U8({'H', 'e', 'l', 'l', 'o'}), 5,
		 U8({0x06, 'H', 'e', 'l', 'l', 'o', 0x00}), 7, "Simple string"},
		{U8({0x01, 0x00, 0x02, 0x00, 0x00}), 5,
		 U8({0x02, 0x01, 0x02, 0x02, 0x01, 0x00}), 6, "Embedded zeros"},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		/* Arrange */
		net_buf_add_mem(fixture->test_data, cases[i].input, cases[i].input_len);
		cobs_encode_init(&enc);
		size_t out_len = sizeof(output);

		/* Act */
		int ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len,
					     COBS_DEFAULT_DELIMITER);
		output[out_len++] = 0x00;

		/* Assert */
		zassert_ok(ret, "Encode failed: %s", cases[i].name);
		zassert_equal(out_len, cases[i].expected_len, "Length: %s", cases[i].name);
		zassert_mem_equal(output, cases[i].expected, cases[i].expected_len,
				  "Data: %s", cases[i].name);
		cobs_test_before(fixture);
	}
}

ZTEST_F(cobs_tests, test_stream_encode_fragmented)
{
	struct cobs_encode_state enc;
	uint8_t output[3];
	const uint8_t input[] = {0x01, 0x02, 0x03, 0x04, 0x05};

	/* Arrange */
	net_buf_add_mem(fixture->test_data, input, sizeof(input));
	cobs_encode_init(&enc);
	size_t out_len = sizeof(output);

	/* Act: Encode with limited output buffer */
	int ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len,
				     COBS_DEFAULT_DELIMITER);

	/* Assert: Partial encoding state */
	zassert_ok(ret, "Fragmented encode");
	zassert_equal(out_len, 3, "Partial output length");
	zassert_equal(output[0], 0x06, "Block code");
	zassert_equal(enc.block_pos, 2, "Encoder state preserved");
}

ZTEST_F(cobs_tests, test_stream_encode_max_block)
{
	struct cobs_encode_state enc;
	uint8_t output[256];

	/* Arrange: 254-byte non-zero sequence */
	for (int i = 0; i < 254; i++) {
		net_buf_add_u8(fixture->test_data, (uint8_t)(i + 1));
	}
	cobs_encode_init(&enc);
	size_t out_len = sizeof(output);

	/* Act */
	int ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len,
				     COBS_DEFAULT_DELIMITER);

	/* Assert: Max block encoding */
	zassert_ok(ret, "Max block encoding");
	zassert_equal(out_len, 255, "Max block length");
	zassert_equal(output[0], 0xFF, "Max block code");
}

/* ========================================================================
 * Streaming Decoder Tests
 * ========================================================================
 */

ZTEST_F(cobs_tests, test_stream_decode_basic)
{
	struct cobs_decode_state dec;

	const struct {
		const uint8_t *input;
		size_t input_len;
		const uint8_t *expected;
		size_t expected_len;
	} cases[] = {
		{U8({0x06, 'H', 'e', 'l', 'l', 'o', 0x00}), 7,
		 U8({'H', 'e', 'l', 'l', 'o'}), 5},
		{U8({0x02, 0x01, 0x02, 0x02, 0x00}), 5, U8({0x01, 0x00, 0x02}), 3},
		{U8({0x05, 0x01, 0x02, 0x03, 0x04, 0x00}), 6, U8({0x01, 0x02, 0x03, 0x04}), 4},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		/* Arrange */
		cobs_decode_init(&dec);

		/* Act */
		int ret = cobs_decode_stream(&dec, cases[i].input, cases[i].input_len,
					     fixture->decoded, COBS_DEFAULT_DELIMITER);

		/* Assert */
		zassert_true(ret > 0, "Decode failed: %zu", i);
		zassert_equal(fixture->decoded->len, cases[i].expected_len, "Length: %zu", i);
		zassert_mem_equal(fixture->decoded->data, cases[i].expected,
				  cases[i].expected_len, "Data: %zu", i);
		zassert_true(dec.frame_complete, "Frame complete: %zu", i);
		cobs_test_before(fixture);
	}
}

ZTEST_F(cobs_tests, test_stream_decode_fragmented)
{
	struct cobs_decode_state dec;
	const uint8_t fragments[][3] = {{0x05, 0x01, 0x02}, {0x03, 0x04}, {0x00}};
	const size_t frag_lens[] = {3, 2, 1};
	const uint8_t expected[] = {0x01, 0x02, 0x03, 0x04};

	/* Arrange */
	cobs_decode_init(&dec);

	/* Act: Process fragments */
	for (size_t i = 0; i < ARRAY_SIZE(fragments); i++) {
		int ret = cobs_decode_stream(&dec, fragments[i], frag_lens[i],
					     fixture->decoded, COBS_DEFAULT_DELIMITER);
		zassert_true(ret > 0, "Fragment %zu failed", i);
	}

	/* Assert */
	zassert_equal(fixture->decoded->len, sizeof(expected), "Length mismatch");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
	zassert_true(dec.frame_complete, "Frame not complete");
}

ZTEST_F(cobs_tests, test_stream_decode_errors)
{
	struct cobs_decode_state dec;

	/* Arrange: Invalid delimiter in data stream */
	const uint8_t bad_delim[] = {0x03, 0x01, 0x00};

	cobs_decode_init(&dec);

	/* Act */
	int ret = cobs_decode_stream(&dec, bad_delim, sizeof(bad_delim),
				     fixture->decoded, COBS_DEFAULT_DELIMITER);

	/* Assert */
	zassert_equal(ret, -EINVAL, "Unexpected delimiter not caught");
}

ZTEST_F(cobs_tests, test_stream_decode_frame_complete_flag)
{
	struct cobs_decode_state dec;
	const struct {
		const uint8_t *data;
		size_t len;
		bool should_be_complete;
		const char *stage;
	} stages[] = {
		{U8({0x05, 0x01, 0x02, 0x03}), 4, false, "Incomplete frame"},
		{U8({0x04, 0x00}), 2, true, "Complete frame"},
		{U8({0x03, 0xAA, 0xBB}), 3, false, "New frame resets"},
	};

	cobs_decode_init(&dec);

	for (size_t i = 0; i < ARRAY_SIZE(stages); i++) {
		if (i == 2) {
			cobs_test_before(fixture); /* Reset for new frame */
		}

		/* Act */
		int ret = cobs_decode_stream(&dec, stages[i].data, stages[i].len,
					     fixture->decoded, COBS_DEFAULT_DELIMITER);

		/* Assert */
		zassert_true(ret > 0, "%s failed", stages[i].stage);
		zassert_equal(dec.frame_complete, stages[i].should_be_complete,
			      "Flag wrong: %s", stages[i].stage);
	}
}

ZTEST_F(cobs_tests, test_stream_roundtrip)
{
	const uint8_t input[] = {0x01, 0x00, 0x02, 0x00, 0x00, 0x03};
	test_stream_roundtrip_helper(fixture, input, sizeof(input),
				      COBS_DEFAULT_DELIMITER, "Basic roundtrip");
}

/* ========================================================================
 * Custom Delimiter Tests
 * ========================================================================
 */

ZTEST_F(cobs_tests, test_custom_delimiter_support)
{
	const uint8_t input[] = {'T', 'e', 's', 't'};
	const uint8_t test_delimiters[] = {0x01, 0x7F, 0xFF};

	for (size_t i = 0; i < ARRAY_SIZE(test_delimiters); i++) {
		uint8_t delim = test_delimiters[i];

		/* Test block roundtrip */
		net_buf_add_mem(fixture->test_data, input, sizeof(input));
		int ret = cobs_encode(fixture->test_data, fixture->encoded,
				      COBS_FLAG_CUSTOM_DELIMITER(delim));
		zassert_ok(ret, "Block encode: 0x%02X", delim);

		ret = cobs_decode(fixture->encoded, fixture->decoded,
				  COBS_FLAG_CUSTOM_DELIMITER(delim));
		zassert_ok(ret, "Block decode: 0x%02X", delim);
		zassert_mem_equal(fixture->decoded->data, input, sizeof(input),
				  "Block roundtrip: 0x%02X", delim);

		cobs_test_before(fixture);

		/* Test stream roundtrip */
		test_stream_roundtrip_helper(fixture, input, sizeof(input), delim,
					     "Stream roundtrip");
		cobs_test_before(fixture);
	}
}

/* ========================================================================
 * Multiple Frame Tests
 * ========================================================================
 */

/* Helper: decode multiple frames from stream and verify */
static void decode_frames_helper(struct cobs_tests_fixture *fixture,
				 const uint8_t *stream, size_t stream_len,
				 const uint8_t expected[][16], const size_t exp_lens[],
				 size_t num_frames, uint8_t delimiter)
{
	struct cobs_decode_state dec;
	size_t offset = 0;

	for (size_t i = 0; i < num_frames; i++) {
		cobs_decode_init(&dec);

		/* Act */
		int ret = cobs_decode_stream(&dec, stream + offset, stream_len - offset,
					     fixture->decoded, delimiter);

		/* Assert */
		zassert_true(ret > 0, "Frame %zu decode failed", i);
		zassert_true(dec.frame_complete, "Frame %zu not complete", i);
		zassert_equal(fixture->decoded->len, exp_lens[i], "Frame %zu length", i);
		zassert_mem_equal(fixture->decoded->data, expected[i], exp_lens[i],
				  "Frame %zu data", i);

		offset += ret;
		cobs_test_before(fixture);
	}

	zassert_equal(offset, stream_len, "Not all stream data consumed");
}

ZTEST_F(cobs_tests, test_multiple_frames_stream_decode)
{
	/* Arrange: Three frames "Hi" + "OK" + "End" */
	const uint8_t stream[] = {
		0x03, 'H', 'i', 0x00, 0x03, 'O', 'K', 0x00, 0x04, 'E', 'n', 'd', 0x00
	};
	const uint8_t expected[][16] = {{'H', 'i'}, {'O', 'K'}, {'E', 'n', 'd'}};
	const size_t lens[] = {2, 2, 3};

	decode_frames_helper(fixture, stream, sizeof(stream), expected, lens, 3,
			     COBS_DEFAULT_DELIMITER);
}

ZTEST_F(cobs_tests, test_multiple_frames_with_custom_delimiter)
{
	struct cobs_encode_state enc;
	uint8_t stream[128];
	size_t stream_len = 0;
	const uint8_t frames[][16] = {{'A', 'B', 'C'}, {0x7F, 0x7F}, {'X', 'Y', 'Z', '!'}};
	const size_t frame_lens[] = {3, 2, 4};
	const uint8_t delim = 0x7F;

	/* Arrange: Encode multiple frames into stream */
	for (size_t i = 0; i < ARRAY_SIZE(frames); i++) {
		net_buf_add_mem(fixture->test_data, frames[i], frame_lens[i]);
		cobs_encode_init(&enc);

		size_t chunk_len = sizeof(stream) - stream_len;
		int ret = cobs_encode_stream(&enc, fixture->test_data, stream + stream_len,
					     &chunk_len, delim);
		zassert_ok(ret, "Encode frame %zu", i);
		stream_len += chunk_len;

		chunk_len = sizeof(stream) - stream_len;
		ret = cobs_encode_finalize(&enc, stream + stream_len, &chunk_len, delim);
		zassert_ok(ret, "Finalize frame %zu", i);
		stream_len += chunk_len;
		stream[stream_len++] = delim;

		cobs_test_before(fixture);
	}

	/* Act & Assert: Decode all frames */
	decode_frames_helper(fixture, stream, stream_len, frames, frame_lens,
			     ARRAY_SIZE(frames), delim);
}

ZTEST_F(cobs_tests, test_multiple_frames_with_empty_frames)
{
	/* Arrange: empty + "Hi" + empty + "OK" + empty */
	const uint8_t stream[] = {
		0x01, 0x00, 0x03, 'H', 'i', 0x00, 0x01, 0x00,
		0x03, 'O', 'K', 0x00, 0x01, 0x00
	};
	const uint8_t expected[][16] = {{}, {'H', 'i'}, {}, {'O', 'K'}, {}};
	const size_t lens[] = {0, 2, 0, 2, 0};

	decode_frames_helper(fixture, stream, sizeof(stream), expected, lens, 5,
			     COBS_DEFAULT_DELIMITER);
}

ZTEST_F(cobs_tests, test_multiple_frames_error_recovery)
{
	struct cobs_decode_state dec;
	const struct {
		const uint8_t *frame;
		size_t len;
		int expected_ret;
		size_t expected_len;
	} cases[] = {
		{U8({0x03, 'O', 'K', 0x00}), 4, 4, 2},           /* Good */
		{U8({0x04, 'B', 'A', 0x00}), 4, -EINVAL, 0},    /* Bad: embedded delim */
		{U8({0x05, 'G', 'o', 'o', 'd', 0x00}), 6, 6, 4} /* Good after error */
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		/* Arrange */
		cobs_decode_init(&dec);

		/* Act */
		int ret = cobs_decode_stream(&dec, cases[i].frame, cases[i].len,
					     fixture->decoded, COBS_DEFAULT_DELIMITER);

		/* Assert */
		zassert_equal(ret, cases[i].expected_ret, "Frame %zu ret code", i);
		if (ret > 0) {
			zassert_true(dec.frame_complete, "Frame %zu complete", i);
			zassert_equal(fixture->decoded->len, cases[i].expected_len,
				      "Frame %zu length", i);
		}
		cobs_test_before(fixture);
	}
}

ZTEST_F(cobs_tests, test_multiple_frames_continuous_processing)
{
	/* Arrange: Four single-char frames without decoder reset between them */
	const uint8_t stream[] = {0x02, 'A', 0x00, 0x02, 'B', 0x00,
				  0x02, 'C', 0x00, 0x02, 'D', 0x00};
	const uint8_t expected[][16] = {{'A'}, {'B'}, {'C'}, {'D'}};
	const size_t lens[] = {1, 1, 1, 1};

	decode_frames_helper(fixture, stream, sizeof(stream), expected, lens, 4,
			     COBS_DEFAULT_DELIMITER);
}
