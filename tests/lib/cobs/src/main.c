/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */
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
	static struct cobs_tests_fixture fixture;

	fixture.test_data = net_buf_alloc(&test_pool, K_NO_WAIT);
	fixture.encoded = net_buf_alloc(&test_pool, K_NO_WAIT);
	fixture.decoded = net_buf_alloc(&test_pool, K_NO_WAIT);

	zassert_not_null(fixture.test_data, "Failed to allocate test_data buffer");
	zassert_not_null(fixture.encoded, "Failed to allocate encoded buffer");
	zassert_not_null(fixture.decoded, "Failed to allocate decoded buffer");

	net_buf_reset(fixture.test_data);
	net_buf_reset(fixture.encoded);
	net_buf_reset(fixture.decoded);

	return &fixture;
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

	net_buf_unref(fixture->test_data);
	net_buf_unref(fixture->encoded);
	net_buf_unref(fixture->decoded);
}

struct cobs_test_item {
	const char *name;
	const uint8_t *decoded;
	size_t decoded_len;
	const uint8_t *encoded;
	size_t encoded_len;
	uint32_t flags;
};

#define U8(...) (uint8_t[]) __VA_ARGS__

#define COBS_ITEM(d, e, f, n)                                                                      \
	{                                                                                          \
		.name = n,                                                                         \
		.decoded = d,                                                                      \
		.decoded_len = sizeof(d),                                                          \
		.encoded = e,                                                                      \
		.encoded_len = sizeof(e),                                                          \
		.flags = f,                                                                        \
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
	COBS_ITEM(U8({}), U8({0x01 ^ 0x7F}), COBS_FLAG_CUSTOM_DELIMITER(0x7F),
		  "Empty with custom delimiter 0x7F"),
	COBS_ITEM(U8({'1'}), U8({0x7D, '1' ^ 0x7F}), COBS_FLAG_CUSTOM_DELIMITER(0x7F),
		  "One char with custom delimiter 0x7F"),
	COBS_ITEM(U8({0x7F}), U8({0x7D, 0x00}), COBS_FLAG_CUSTOM_DELIMITER(0x7F),
		  "One 0x7F delimiter"),
	COBS_ITEM(U8({0x7F, 0x7F}), U8({0x7C, 0x00, 0x00}), COBS_FLAG_CUSTOM_DELIMITER(0x7F),
		  "Two 0x7F delimiters"),
	COBS_ITEM(U8({0x7F, 0x7F, 0x7F}), U8({0x7B, 0x00, 0x00, 0x00}),
		  COBS_FLAG_CUSTOM_DELIMITER(0x7F), "Three 0x7F delimiters"),
	COBS_ITEM(
		U8({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		    's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
		    'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		    'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		    'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		    'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3'}),
		U8({0xfe, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
		    'F',  'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a',
		    'b',  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
		    'r',  's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
		    'D',  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		    'p',  'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
		    'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		    'R',  'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		    'n',  'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		    'P',  'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
		    'l',  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6',
		    '7',  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		    'N',  'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3'}),
		COBS_DEFAULT_DELIMITER, "253 Chars"),
	COBS_ITEM(
		U8({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		    's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
		    'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		    'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		    'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		    'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4'}),
		U8({0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
		    'F',  'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a',
		    'b',  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
		    'r',  's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
		    'D',  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		    'p',  'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
		    'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		    'R',  'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		    'n',  'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		    'P',  'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
		    'l',  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6',
		    '7',  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		    'N',  'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4'}),
		COBS_DEFAULT_DELIMITER, "254 Chars"),
	COBS_ITEM(
		U8({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		    's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
		    'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		    'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		    'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		    'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', '5'}),
		U8({0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		    '4',  '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		    'J',  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		    'e',  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		    't',  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4',
		    0x02, '5'}),
		COBS_DEFAULT_DELIMITER, "255 Chars"),
	COBS_ITEM(U8({0x00, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		      'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		      'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		      'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		      '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		      'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		      'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		      '4',  '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		      'J',  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		      'e',  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		      't',  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		      'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		      'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		      'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		      '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		      'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		      'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4',
		      '5'}),
		  U8({0x01, 0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
		      'D',  'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		      'S',  'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		      'n',  'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		      '8',  '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		      'N',  'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		      'i',  'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2',
		      '3',  '4',  '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		      'I',  'J',  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c',
		      'd',  'e',  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		      's',  't',  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
		      'D',  'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		      'S',  'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		      'n',  'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		      '8',  '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		      'N',  'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		      'i',  'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3',
		      '4',  0x02, '5'}),
		  COBS_DEFAULT_DELIMITER, "Leading zero 255 chars"),
	COBS_ITEM(
		U8({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		    's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
		    'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		    'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		    'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		    'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', 0x00}),
		U8({0xfe, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
		    'F',  'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a',
		    'b',  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
		    'r',  's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
		    'D',  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		    'p',  'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
		    'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		    'R',  'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		    'n',  'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		    'P',  'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
		    'l',  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6',
		    '7',  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		    'N',  'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', 0x01}),
		COBS_DEFAULT_DELIMITER, "Ending zero 253 chars"),
	COBS_ITEM(
		U8({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
		    's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
		    'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		    'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		    'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		    'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', 0x00}),
		U8({0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		    '4',  '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		    'J',  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		    'e',  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		    't',  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		    'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		    'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		    'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		    '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		    'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		    'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4',
		    0x01, 0x01}),
		COBS_DEFAULT_DELIMITER, "Ending zero 254 chars"),
	COBS_ITEM(U8({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
		      'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		      'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		      'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		      'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4',
		      '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		      'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e',
		      'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
		      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
		      'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		      'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		      'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		      'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', '5',
		      0x00}),
		  U8({0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		      'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		      'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		      'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		      '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		      'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		      'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		      '4',  '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		      'J',  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		      'e',  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		      't',  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		      'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		      'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		      'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		      '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		      'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		      'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4',
		      0x02, '5', 0x01}),
		  COBS_DEFAULT_DELIMITER, "Ending zero 255 chars"),
};

/*
 * Derive the per-case label from each dataset item's descriptive name.
 * Spaces are replaced with underscores so the label is a single whitespace-free
 * token, which the twister ztest harness parser requires.
 */
static const char *cobs_item_name(size_t index, const void *value)
{
	const struct cobs_test_item *item = value;
	static char buf[64];
	size_t i;

	ARG_UNUSED(index);

	for (i = 0; i < sizeof(buf) - 1 && item->name[i] != '\0'; i++) {
		buf[i] = (item->name[i] == ' ') ? '_' : item->name[i];
	}
	buf[i] = '\0';

	return buf;
}

static const struct ztest_param_values cobs_vals = {
	.values = cobs_dataset,
	.count = ARRAY_SIZE(cobs_dataset),
	.elem_size = sizeof(cobs_dataset[0]),
	.name_cb = cobs_item_name,
};

ZTEST_SUITE(cobs_tests, NULL, cobs_test_setup, cobs_test_before, NULL, cobs_test_teardown);

/*
 * The dataset-driven tests below are instantiated once per cobs_dataset entry
 * via ZTEST_INSTANTIATE_TEST_SUITE_P(). The suite before() hook resets all
 * buffers ahead of every invocation, so no per-item reset is needed.
 */
ZTEST_P(cobs_tests, test_encode)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *item = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	int ret;

	net_buf_add_mem(fixture->test_data, item->decoded, item->decoded_len);

	ret = cobs_encode(fixture->test_data, fixture->encoded, item->flags);
	zassert_ok(ret, "COBS encoding failed for %s", item->name);
	zassert_equal(item->encoded_len, fixture->encoded->len,
		      "Encoded length does not match expected for %s", item->name);
	zassert_mem_equal(item->encoded, fixture->encoded->data, item->encoded_len,
			  "Encoded data does not match expected for %s", item->name);

	zassert_equal(fixture->test_data->len, 0, "Encode input not empty for %s", item->name);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_encode, cobs_vals);

static int cobs_net_buf_cb(const uint8_t *buf, size_t len, void *user_data)
{
	struct net_buf *dst = user_data;

	if (net_buf_tailroom(dst) < len) {
		return -ENOMEM;
	}

	(void)net_buf_add_mem(dst, buf, len);

	return 0;
}

ZTEST_P(cobs_tests, test_encode_stream)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *test = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	struct cobs_encoder enc;
	int ret;

	ret = cobs_encoder_init(&enc, cobs_net_buf_cb, fixture->encoded, test->flags);
	zassert_ok(ret, "encoder init failed for %s (%d)", test->name, ret);

	/* Simulate chunks by sending each byte */
	for (size_t i = 0; i < test->decoded_len; ++i) {
		ret = cobs_encoder_write(&enc, &test->decoded[i], 1);
		zassert_equal(ret, 1, "encoder write failed (%d) for %s", ret, test->name);
	}

	ret = cobs_encoder_close(&enc);
	zassert_ok(ret, "encoder close failed for %s (%d)", test->name, ret);

	zassert_equal(test->encoded_len, fixture->encoded->len);
	zassert_mem_equal(test->encoded, fixture->encoded->data, test->encoded_len);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_encode_stream, cobs_vals);

ZTEST_P(cobs_tests, test_decode)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *item = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	int ret;

	net_buf_add_mem(fixture->test_data, item->decoded, item->decoded_len);

	ret = cobs_decode(fixture->encoded, fixture->test_data, item->flags);
	zassert_ok(ret, "COBS decoding failed for %s", item->name);
	zassert_equal(item->decoded_len, fixture->test_data->len,
		      "Decoded length does not match expected for %s", item->name);
	zassert_mem_equal(item->decoded, fixture->test_data->data, item->decoded_len,
			  "Decoded data does not match expected for %s", item->name);

	zassert_equal(fixture->encoded->len, 0, "Decode input not empty for %s", item->name);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_decode, cobs_vals);

ZTEST_P(cobs_tests, test_decode_stream)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *test = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	struct cobs_decoder dec;
	int ret;

	ret = cobs_decoder_init(&dec, cobs_net_buf_cb, fixture->decoded, test->flags);
	zassert_ok(ret, "decoder init failed for %s (%d)", test->name, ret);

	/* Simulate chunks by sending each byte */
	for (size_t i = 0; i < test->encoded_len; ++i) {
		ret = cobs_decoder_write(&dec, &test->encoded[i], 1);
		zassert_equal(ret, 1, "decoder write failed (%d) for %s", ret, test->name);
	}

	ret = cobs_decoder_close(&dec);
	zassert_ok(ret, "decoder close failed for %s (%d)", test->name, ret);

	zassert_equal(test->decoded_len, fixture->decoded->len);
	zassert_mem_equal(test->decoded, fixture->decoded->data, test->decoded_len);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_decode_stream, cobs_vals);

static int cobs_forward_to_decoder(const uint8_t *buf, size_t len, void *user_data)
{
	struct cobs_decoder *dec = user_data;

	return cobs_decoder_write(dec, buf, len);
}

ZTEST_P(cobs_tests, test_encode_decode_stream)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *test = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	struct cobs_encoder enc;
	struct cobs_decoder dec;
	int ret;

	ret = cobs_encoder_init(&enc, cobs_forward_to_decoder, &dec, test->flags);
	zassert_ok(ret, "encoder init failed for %s (%d)", test->name, ret);

	ret = cobs_decoder_init(&dec, cobs_net_buf_cb, fixture->decoded, test->flags);
	zassert_ok(ret, "decoder init failed for %s (%d)", test->name, ret);

	ret = cobs_encoder_write(&enc, test->decoded, test->decoded_len);
	zassert_equal(ret, test->decoded_len, "encoder write failed for %s (%d)", test->name, ret);

	ret = cobs_encoder_close(&enc);
	zassert_ok(ret, "encoder close failed for %s (%d)", test->name, ret);

	ret = cobs_decoder_close(&dec);
	zassert_ok(ret, "decoder close failed for %s (%d)", test->name, ret);

	zassert_equal(test->decoded_len, fixture->decoded->len);
	zassert_mem_equal(test->decoded, fixture->decoded->data, test->decoded_len);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_encode_decode_stream, cobs_vals);

static size_t frame_test_idx;

static int cobs_frame_tester(const uint8_t *buf, size_t len, void *user_data)
{
	struct net_buf *dst = user_data;

	if (buf == NULL) {
		const struct cobs_test_item *test = &cobs_dataset[frame_test_idx];

		zassert_equal(test->decoded_len, dst->len);
		zassert_mem_equal(test->decoded, dst->data, test->decoded_len);

		net_buf_reset(dst);
		frame_test_idx++;
		return 0;
	}

	if (net_buf_tailroom(dst) < len) {
		return -ENOMEM;
	}

	(void)net_buf_add_mem(dst, buf, len);

	return 0;
}

ZTEST_F(cobs_tests, test_decode_stream_frame_complete)
{
	struct cobs_encoder enc;
	struct cobs_decoder dec;
	int ret;

	/* This test re-uses the same encoder/decoder pair and streams multiple frames */

	ret = cobs_encoder_init(&enc, cobs_forward_to_decoder, &dec, COBS_FLAG_TRAILING_DELIMITER);
	zassert_ok(ret, "encoder init failed (%d)", ret);

	ret = cobs_decoder_init(&dec, cobs_frame_tester, fixture->decoded,
				COBS_FLAG_TRAILING_DELIMITER);
	zassert_ok(ret, "decoder init failed (%d)", ret);

	ARRAY_FOR_EACH(cobs_dataset, idx) {
		ret = cobs_encoder_write(&enc, cobs_dataset[idx].decoded,
					 cobs_dataset[idx].decoded_len);
		zassert_equal(ret, cobs_dataset[idx].decoded_len);

		/* Closing will write a delimiter and reset the state */
		ret = cobs_encoder_close(&enc);
		zassert_ok(ret);
	}

	zassert_equal(frame_test_idx, ARRAY_SIZE(cobs_dataset));
}

ZTEST_P(cobs_tests, test_encode_trailing_delimiter)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *item = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(item->flags);
	int ret;

	net_buf_add_mem(fixture->test_data, item->decoded, item->decoded_len);

	ret = cobs_encode(fixture->test_data, fixture->encoded,
			  COBS_FLAG_TRAILING_DELIMITER | item->flags);
	zassert_ok(ret, "COBS encoding failed for %s", item->name);
	zassert_equal(item->encoded_len + 1, fixture->encoded->len,
		      "Encoded length does not match expected for %s", item->name);
	zassert_mem_equal(item->encoded, fixture->encoded->data, item->encoded_len,
			  "Encoded data does not match expected for %s", item->name);

	zassert_equal(fixture->encoded->data[item->encoded_len], delimiter,
		      "Encoded data does not have proper trailing delimiter");

	zassert_equal(fixture->test_data->len, 0, "Encode input not empty for %s", item->name);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_encode_trailing_delimiter, cobs_vals);

ZTEST_P(cobs_tests, test_decode_trailing_delimiter)
{
	struct cobs_tests_fixture *fixture = data;
	const struct cobs_test_item *item = ZTEST_GET_PARAM_PTR(struct cobs_test_item);
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(item->flags);
	int ret;

	net_buf_add_mem(fixture->test_data, item->decoded, item->decoded_len);

	net_buf_add_u8(fixture->encoded, delimiter);

	ret = cobs_decode(fixture->encoded, fixture->test_data,
			  COBS_FLAG_TRAILING_DELIMITER | item->flags);
	zassert_ok(ret, "COBS decoding failed for %s", item->name);
	zassert_equal(item->decoded_len, fixture->test_data->len,
		      "Decoded length does not match expected for %s", item->name);
	zassert_mem_equal(item->decoded, fixture->test_data->data, item->decoded_len,
			  "Decoded data does not match expected for %s", item->name);

	zassert_equal(fixture->encoded->len, 0, "Decode input not empty for %s", item->name);
}
ZTEST_INSTANTIATE_TEST_SUITE_P(dataset, cobs_tests, test_decode_trailing_delimiter, cobs_vals);

ZTEST_F(cobs_tests, test_cobs_invalid_delim_pos)
{
	int ret;
	const char data_enc[] = {0x02, 0x00, 0x01};

	net_buf_add_mem(fixture->encoded, data_enc, sizeof(data_enc));
	ret = cobs_decode(fixture->encoded, fixture->decoded, 0);
	zassert_equal(ret, -EINVAL, "Decoding invalid delimiter caught");
}

ZTEST_F(cobs_tests, test_cobs_consecutive_delims)
{
	int ret;
	const char data_enc[] = {0x01, 0x00, 0x00, 0x01};

	net_buf_add_mem(fixture->encoded, data_enc, sizeof(data_enc));
	ret = cobs_decode(fixture->encoded, fixture->decoded, 0);
	zassert_equal(ret, -EINVAL, "Decoding consecutive delimiters not caught");
}

ZTEST_F(cobs_tests, test_cobs_invalid_overrun)
{
	int ret;
	const char data_enc[] = {0x03, 0x01};

	net_buf_add_mem(fixture->encoded, data_enc, sizeof(data_enc));
	ret = cobs_decode(fixture->encoded, fixture->decoded, 0);
	zassert_equal(ret, -EINVAL, "Decoding insufficient data not caught");
}
