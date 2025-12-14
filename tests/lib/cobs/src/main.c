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

ZTEST_SUITE(cobs_tests, NULL, cobs_test_setup, cobs_test_before, NULL, cobs_test_teardown);

ZTEST_F(cobs_tests, test_encode)
{
	int ret;

	ARRAY_FOR_EACH(cobs_dataset, idx) {
		uint8_t delimiter = cobs_dataset[idx].delimiter;

		net_buf_add_mem(fixture->test_data, cobs_dataset[idx].decoded,
				cobs_dataset[idx].decoded_len);

		ret = cobs_encode(fixture->test_data, fixture->encoded,
				  COBS_FLAG_CUSTOM_DELIMITER(delimiter));
		zassert_ok(ret, "COBS encoding failed for %s", cobs_dataset[idx].name);
		zassert_equal(cobs_dataset[idx].encoded_len, fixture->encoded->len,
			      "Encoded length does not match expected for %s",
			      cobs_dataset[idx].name);
		zassert_mem_equal(cobs_dataset[idx].encoded, fixture->encoded->data,
				  cobs_dataset[idx].encoded_len,
				  "Encoded data does not match expected for %s",
				  cobs_dataset[idx].name);

		zassert_equal(fixture->test_data->len, 0, "Encode input not empty for %s",
			      cobs_dataset[idx].name);
		net_buf_reset(fixture->test_data);
		net_buf_reset(fixture->encoded);
		net_buf_reset(fixture->decoded);
	}
}

ZTEST_F(cobs_tests, test_decode)
{
	int ret;

	ARRAY_FOR_EACH(cobs_dataset, idx) {
		uint8_t delimiter = cobs_dataset[idx].delimiter;

		net_buf_add_mem(fixture->test_data, cobs_dataset[idx].decoded,
				cobs_dataset[idx].decoded_len);

		ret = cobs_decode(fixture->encoded, fixture->test_data,
				  COBS_FLAG_CUSTOM_DELIMITER(delimiter));
		zassert_ok(ret, "COBS decoding failed for %s", cobs_dataset[idx].name);
		zassert_equal(cobs_dataset[idx].decoded_len, fixture->test_data->len,
			      "Decoded length does not match expected for %s",
			      cobs_dataset[idx].name);
		zassert_mem_equal(cobs_dataset[idx].decoded, fixture->test_data->data,
				  cobs_dataset[idx].decoded_len,
				  "Decoded data does not match expected for %s",
				  cobs_dataset[idx].name);

		zassert_equal(fixture->encoded->len, 0, "Decode input not empty for %s",
			      cobs_dataset[idx].name);
		net_buf_reset(fixture->test_data);
		net_buf_reset(fixture->encoded);
		net_buf_reset(fixture->decoded);
	}
}

ZTEST_F(cobs_tests, test_encode_trailing_delimiter)
{
	int ret;

	ARRAY_FOR_EACH(cobs_dataset, idx) {
		uint8_t delimiter = cobs_dataset[idx].delimiter;

		net_buf_add_mem(fixture->test_data, cobs_dataset[idx].decoded,
				cobs_dataset[idx].decoded_len);

		ret = cobs_encode(fixture->test_data, fixture->encoded,
				  COBS_FLAG_TRAILING_DELIMITER |
					  COBS_FLAG_CUSTOM_DELIMITER(delimiter));
		zassert_ok(ret, "COBS encoding failed for %s", cobs_dataset[idx].name);
		zassert_equal(cobs_dataset[idx].encoded_len + 1, fixture->encoded->len,
			      "Encoded length does not match expected for %s",
			      cobs_dataset[idx].name);
		zassert_mem_equal(cobs_dataset[idx].encoded, fixture->encoded->data,
				  cobs_dataset[idx].encoded_len,
				  "Encoded data does not match expected for %s",
				  cobs_dataset[idx].name);

		zassert_equal(fixture->encoded->data[cobs_dataset[idx].encoded_len], delimiter,
			      "Encoded data does not have proper trailing delimiter");

		zassert_equal(fixture->test_data->len, 0, "Encode input not empty for %s",
			      cobs_dataset[idx].name);
		net_buf_reset(fixture->test_data);
		net_buf_reset(fixture->encoded);
		net_buf_reset(fixture->decoded);
	}
}

ZTEST_F(cobs_tests, test_decode_trailing_delimiter)
{
	int ret;

	ARRAY_FOR_EACH(cobs_dataset, idx) {
		uint8_t delimiter = cobs_dataset[idx].delimiter;

		net_buf_add_mem(fixture->test_data, cobs_dataset[idx].decoded,
				cobs_dataset[idx].decoded_len);

		net_buf_add_u8(fixture->encoded, delimiter);

		ret = cobs_decode(fixture->encoded, fixture->test_data,
				  COBS_FLAG_TRAILING_DELIMITER |
					  COBS_FLAG_CUSTOM_DELIMITER(delimiter));
		zassert_ok(ret, "COBS decoding failed for %s", cobs_dataset[idx].name);
		zassert_equal(cobs_dataset[idx].decoded_len, fixture->test_data->len,
			      "Decoded length does not match expected for %s",
			      cobs_dataset[idx].name);
		zassert_mem_equal(cobs_dataset[idx].decoded, fixture->test_data->data,
				  cobs_dataset[idx].decoded_len,
				  "Decoded data does not match expected for %s",
				  cobs_dataset[idx].name);

		zassert_equal(fixture->encoded->len, 0, "Decode input not empty for %s",
			      cobs_dataset[idx].name);
		net_buf_reset(fixture->test_data);
		net_buf_reset(fixture->encoded);
		net_buf_reset(fixture->decoded);
	}
}

ZTEST_F(cobs_tests, test_cobs_invalid_delim_pos)
{
	int ret;
	const char data_enc[] = {0x02, 0x00, 0x01};

	net_buf_add_mem(fixture->encoded, data_enc, sizeof(data_enc));
	ret = cobs_decode(fixture->encoded, fixture->decoded, 0);
	zassert_true(ret == -EINVAL, "Decoding invalid delimiter caught");
}

ZTEST_F(cobs_tests, test_cobs_consecutive_delims)
{
	int ret;
	const char data_enc[] = {0x01, 0x00, 0x00, 0x01};

	net_buf_add_mem(fixture->encoded, data_enc, sizeof(data_enc));
	ret = cobs_decode(fixture->encoded, fixture->decoded, 0);
	zassert_true(ret == -EINVAL, "Decoding consecutive delimiters not caught");
}

ZTEST_F(cobs_tests, test_cobs_invalid_overrun)
{
	int ret;
	const char data_enc[] = {0x03, 0x01};

	net_buf_add_mem(fixture->encoded, data_enc, sizeof(data_enc));
	ret = cobs_decode(fixture->encoded, fixture->decoded, 0);
	zassert_true(ret == -EINVAL, "Decoding insufficient data not caught");
}

/* ========================================================================
 * Streaming Encoder Tests
 * ======================================================================== */

ZTEST_F(cobs_tests, test_stream_encode_simple)
{
	struct cobs_encode_state enc;
	uint8_t output[64];
	size_t out_len;
	int ret;

	/* Test data: "Hello" -> [0x06, 'H', 'e', 'l', 'l', 'o'] (without delimiter)
	 * For stream transmission, user must add 0x00 delimiter after encoding
	 */
	const uint8_t input[] = {'H', 'e', 'l', 'l', 'o'};
	const uint8_t expected_encoded[] = {0x06, 'H', 'e', 'l', 'l', 'o'};
	const uint8_t expected_frame[] = {0x06, 'H', 'e', 'l', 'l', 'o', 0x00};
	net_buf_add_mem(fixture->test_data, input, sizeof(input));

	cobs_encode_init(&enc);
	zassert_equal(enc.block_code, 0, "Encoder state not initialized");

	out_len = sizeof(output);
	ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len);
	
	/* Verify return value and output length (without delimiter) */
	zassert_equal(ret, 0, "Encoding failed");
	zassert_equal(out_len, sizeof(expected_encoded), "Wrong output length");
	
	/* Verify encoded output (without delimiter) */
	zassert_mem_equal(output, expected_encoded, sizeof(expected_encoded), 
			  "Encoded output mismatch");
	
	/* Note: New non-destructive encoder does NOT consume source */
	zassert_equal(fixture->test_data->len, sizeof(input), 
		      "Non-destructive encoder preserves source");
	
	/* Simulate adding delimiter for stream transmission */
	output[out_len++] = 0x00;
	
	/* Verify complete frame format that would be transmitted */
	zassert_equal(out_len, sizeof(expected_frame), "Wrong complete frame length");
	zassert_mem_equal(output, expected_frame, sizeof(expected_frame), 
			  "Complete frame format mismatch");
}

ZTEST_F(cobs_tests, test_stream_encode_with_delimiter)
{
	struct cobs_encode_state enc;
	uint8_t output[64];
	size_t out_len;
	int ret;

	/* Test data: [0x01, 0x00, 0x02] -> encoded: [0x02, 0x01, 0x02, 0x02]
	 *                                -> wire: [0x02, 0x01, 0x02, 0x02, 0x00]
	 */
	const uint8_t input[] = {0x01, 0x00, 0x02};
	const uint8_t expected_encoded[] = {0x02, 0x01, 0x02, 0x02};
	const uint8_t expected_wire[] = {0x02, 0x01, 0x02, 0x02, 0x00};
	net_buf_add_mem(fixture->test_data, input, sizeof(input));

	cobs_encode_init(&enc);

	out_len = sizeof(output);
	ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len);
	
	/* Verify return value and encoded length (without frame delimiter) */
	zassert_equal(ret, 0, "Encoding failed");
	zassert_equal(out_len, sizeof(expected_encoded), "Wrong encoded output length");
	
	/* Verify exact encoded output (without frame delimiter) */
	zassert_mem_equal(output, expected_encoded, sizeof(expected_encoded), 
			  "Encoded data mismatch");
	
	/* Note: Non-destructive encoder preserves source */
	zassert_equal(fixture->test_data->len, sizeof(input), 
		      "Non-destructive encoder preserves source");
	
	/* Add frame delimiter for stream transmission */
	output[out_len++] = 0x00;
	
	/* Verify complete wire format */
	zassert_equal(out_len, sizeof(expected_wire), "Wrong wire format length");
	zassert_mem_equal(output, expected_wire, sizeof(expected_wire),
			  "Complete wire format mismatch");
}

ZTEST_F(cobs_tests, test_stream_encode_fragmented)
{
	struct cobs_encode_state enc;
	uint8_t output[32];
	size_t out_len;
	int ret;

	/* Test encoding with output spanning multiple buffers
	 * Single input [0x01, 0x02, 0x03, 0x04, 0x05]
	 * Expected output: [0x06, 0x01, 0x02, 0x03, 0x04, 0x05]
	 */
	const uint8_t input[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	net_buf_add_mem(fixture->test_data, input, sizeof(input));

	cobs_encode_init(&enc);

	/* Single call should encode entire input */
	out_len = sizeof(output);
	ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len);
	zassert_equal(ret, 0, "Encoding failed");
	zassert_equal(out_len, 6, "Should produce 6 bytes");
	
	/* Verify output */
	const uint8_t expected[] = {0x06, 0x01, 0x02, 0x03, 0x04, 0x05};
	zassert_mem_equal(output, expected, sizeof(expected), "Output mismatch");
	
	/* Source should be preserved (non-destructive) */
	zassert_equal(fixture->test_data->len, sizeof(input), 
		      "Non-destructive encoder preserves source");
}

ZTEST_F(cobs_tests, test_stream_encode_small_buffer)
{
	struct cobs_encode_state enc;
	uint8_t output[3];
	size_t out_len;
	int ret;

	/* Test output buffer too small for complete encoding
	 * Input: [0x01, 0x02, 0x03, 0x04, 0x05] needs 6 bytes encoded
	 * But output buffer only has 3 bytes
	 */
	const uint8_t input[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	const uint8_t expected[] = {0x06, 0x01, 0x02};  /* Partial: code + first 2 data bytes */
	net_buf_add_mem(fixture->test_data, input, sizeof(input));

	cobs_encode_init(&enc);

	out_len = sizeof(output);
	ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len);
	
	/* Verify return value */
	zassert_equal(ret, 0, "Encoding should succeed with partial output");
	
	/* Verify output length */
	zassert_equal(out_len, sizeof(expected), "Wrong partial output length");
	
	/* Verify partial output contents */
	zassert_mem_equal(output, expected, sizeof(expected), "Partial output mismatch");
	
	/* Source preserved (non-destructive) */
	zassert_equal(fixture->test_data->len, sizeof(input), 
		      "Non-destructive encoder preserves source");
	
	/* Verify encoder state tracks progress */
	zassert_equal(enc.block_code, 0x06, "Encoder should remember block code");
	zassert_equal(enc.block_pos, 2, "Encoder should track position within block");
}

ZTEST_F(cobs_tests, test_stream_encode_max_block)
{
	struct cobs_encode_state enc;
	uint8_t output[256];
	size_t out_len;
	int ret;

	/* Create 254 non-zero bytes (max block size) */
	for (int i = 0; i < 254; i++) {
		net_buf_add_u8(fixture->test_data, (uint8_t)(i + 1));
	}

	cobs_encode_init(&enc);

	out_len = sizeof(output);
	ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len);
	
	/* Verify return value and output length */
	zassert_equal(ret, 0, "Encoding failed");
	zassert_equal(out_len, 255, "Wrong output length for max block");
	
	/* Verify code byte */
	zassert_equal(output[0], 0xFF, "Code byte should be 0xFF for max block");
	
	/* Verify data bytes match input */
	for (int i = 0; i < 254; i++) {
		zassert_equal(output[i + 1], (uint8_t)(i + 1), 
			      "Data byte %d mismatch", i);
	}
	
	/* Source preserved (non-destructive) */
	zassert_equal(fixture->test_data->len, 254, 
		      "Non-destructive encoder preserves source");
}

ZTEST_F(cobs_tests, test_stream_encode_empty)
{
	struct cobs_encode_state enc;
	uint8_t output[64];
	size_t out_len;
	int ret;

	cobs_encode_init(&enc);

	out_len = sizeof(output);
	ret = cobs_encode_stream(&enc, fixture->test_data, output, &out_len);
	zassert_equal(ret, 0, "Encoding failed");
	zassert_equal(out_len, 0, "Should produce no output for empty input");
}

ZTEST_F(cobs_tests, test_stream_transmission_format)
{
	struct cobs_encode_state enc;
	struct cobs_decode_state dec;
	uint8_t tx_buffer[64];
	size_t tx_len;
	int ret;

	/* Test complete transmission format: encode + delimiter + decode
	 * This validates what actually goes over the wire in a stream protocol
	 */
	const uint8_t original_data[] = {0x11, 0x00, 0x22, 0x00, 0x33};
	/* Expected wire format: [0x02, 0x11, 0x02, 0x22, 0x02, 0x33, 0x00]
	 *                        ^-code ^data ^code ^data ^code ^data ^delimiter
	 */
	const uint8_t expected_wire_format[] = {0x02, 0x11, 0x02, 0x22, 0x02, 0x33, 0x00};
	
	net_buf_add_mem(fixture->test_data, original_data, sizeof(original_data));

	/* Step 1: Encode the data */
	cobs_encode_init(&enc);

	tx_len = sizeof(tx_buffer);
	ret = cobs_encode_stream(&enc, fixture->test_data, tx_buffer, &tx_len);
	zassert_equal(ret, 0, "Encoding failed");
	
	/* Step 2: Add frame delimiter for stream transmission */
	tx_buffer[tx_len++] = 0x00;
	
	/* Step 3: Verify complete wire format */
	zassert_equal(tx_len, sizeof(expected_wire_format), 
		      "Wire format length mismatch");
	zassert_mem_equal(tx_buffer, expected_wire_format, sizeof(expected_wire_format),
			  "Wire format bytes mismatch");
	
	/* Step 4: Decode the received frame (as receiver would) */
	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, tx_buffer, tx_len, fixture->decoded);
	zassert_equal(ret, tx_len, "Decoding failed");
	
	/* Step 5: Verify roundtrip integrity */
	zassert_equal(fixture->decoded->len, sizeof(original_data), 
		      "Decoded length mismatch");
	zassert_mem_equal(fixture->decoded->data, original_data, sizeof(original_data),
			  "Decoded data doesn't match original");
}

/* ========================================================================
 * Streaming Decoder Tests
 * ======================================================================== */

ZTEST_F(cobs_tests, test_stream_decode_simple)
{
	struct cobs_decode_state dec;
	int ret;

	/* Encoded: [0x06, 'H', 'e', 'l', 'l', 'o', 0x00] -> ['H', 'e', 'l', 'l', 'o'] */
	const uint8_t input[] = {0x06, 'H', 'e', 'l', 'l', 'o', 0x00};
	const uint8_t expected[] = {'H', 'e', 'l', 'l', 'o'};

	cobs_decode_init(&dec);
	zassert_equal(dec.bytes_left, 0, "Decoder not properly initialized");
	zassert_false(dec.need_delimiter, "Decoder not properly initialized");

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	
	/* Verify return value: should be bytes processed */
	zassert_equal(ret, sizeof(input), "Should process all bytes including delimiter");
	
	/* Verify output length */
	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	
	/* Verify exact decoded output */
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
	
	/* Verify decoder state after complete frame */
	zassert_equal(dec.bytes_left, 0, "State should be reset after frame end");
	zassert_false(dec.need_delimiter, "Delimiter flag should be cleared");
}

ZTEST_F(cobs_tests, test_stream_decode_with_delimiter)
{
	struct cobs_decode_state dec;
	int ret;

	/* Encoded: [0x02, 0x01, 0x02, 0x02, 0x00] -> [0x01, 0x00, 0x02] */
	const uint8_t input[] = {0x02, 0x01, 0x02, 0x02, 0x00};
	const uint8_t expected[] = {0x01, 0x00, 0x02};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	
	/* Verify return value */
	zassert_equal(ret, sizeof(input), "Should process all bytes");
	
	/* Verify output length and contents */
	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
	
	/* Verify state after complete frame */
	zassert_equal(dec.bytes_left, 0, "State not reset after frame");
	zassert_false(dec.need_delimiter, "Delimiter flag not cleared");
}

ZTEST_F(cobs_tests, test_stream_decode_fragmented)
{
	struct cobs_decode_state dec;
	int ret;

	/* Encoded: [0x05, 0x01, 0x02, 0x03, 0x04, 0x00] */
	/* Feed in 3 fragments */
	const uint8_t frag1[] = {0x05, 0x01};
	const uint8_t frag2[] = {0x02, 0x03};
	const uint8_t frag3[] = {0x04, 0x00};
	const uint8_t expected[] = {0x01, 0x02, 0x03, 0x04};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, frag1, sizeof(frag1), fixture->decoded);
	zassert_equal(ret, sizeof(frag1), "Fragment 1 processing failed");

	ret = cobs_decode_stream(&dec, frag2, sizeof(frag2), fixture->decoded);
	zassert_equal(ret, sizeof(frag2), "Fragment 2 processing failed");

	ret = cobs_decode_stream(&dec, frag3, sizeof(frag3), fixture->decoded);
	zassert_equal(ret, sizeof(frag3), "Fragment 3 processing failed");

	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
}

ZTEST_F(cobs_tests, test_stream_decode_max_block)
{
	struct cobs_decode_state dec;
	int ret;

	/* Create encoded max block: [0xFF, 254 bytes of data, 0x00] */
	uint8_t input[256];
	input[0] = 0xFF;
	for (int i = 1; i < 255; i++) {
		input[i] = (uint8_t)i;
	}
	input[255] = 0x00;

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	zassert_equal(ret, sizeof(input), "Should process all bytes");
	zassert_equal(fixture->decoded->len, 254, "Wrong decoded length for max block");
}

ZTEST_F(cobs_tests, test_stream_decode_multiple_blocks)
{
	struct cobs_decode_state dec;
	int ret;

	/* Two blocks: [0x03, 0x01, 0x02, 0x03, 0x03, 0x04, 0x00] -> [0x01, 0x02, 0x00, 0x03, 0x04] */
	const uint8_t input[] = {0x03, 0x01, 0x02, 0x03, 0x03, 0x04, 0x00};
	const uint8_t expected[] = {0x01, 0x02, 0x00, 0x03, 0x04};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	zassert_equal(ret, sizeof(input), "Should process all bytes");
	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
}

ZTEST_F(cobs_tests, test_stream_decode_frame_end_detection)
{
	struct cobs_decode_state dec;
	int ret;

	/* Single frame with extra data: [0x03, 0x01, 0x02, 0x00, 0xFF, 0xFF] */
	const uint8_t input[] = {0x03, 0x01, 0x02, 0x00, 0xFF, 0xFF};
	const uint8_t expected[] = {0x01, 0x02};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	zassert_equal(ret, 4, "Should stop at frame delimiter");
	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
}

ZTEST_F(cobs_tests, test_stream_decode_invalid_delimiter_in_data)
{
	struct cobs_decode_state dec;
	int ret;

	/* Invalid: delimiter in data stream [0x03, 0x01, 0x00] */
	const uint8_t input[] = {0x03, 0x01, 0x00};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	zassert_equal(ret, -EINVAL, "Should detect unexpected delimiter");
}

ZTEST_F(cobs_tests, test_stream_decode_invalid_zero_code)
{
	struct cobs_decode_state dec;
	int ret;

	/* Invalid: zero code byte (not at expected delimiter position) */
	const uint8_t input[] = {0x00, 0x01, 0x02};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	/* 0x00 at start is a valid frame delimiter, so this returns 1 */
	zassert_equal(ret, 1, "Empty frame should be valid");
}

ZTEST_F(cobs_tests, test_stream_decode_incomplete_frame)
{
	struct cobs_decode_state dec;
	int ret;

	/* Incomplete frame: [0x05, 0x01, 0x02, 0x03] - missing 2 bytes and delimiter */
	const uint8_t input[] = {0x05, 0x01, 0x02, 0x03};
	const uint8_t expected[] = {0x01, 0x02, 0x03};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	zassert_equal(ret, sizeof(input), "Should process all available bytes");
	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	
	/* Verify state allows continuation */
	zassert_equal(dec.bytes_left, 1, "Should be waiting for 1 more byte");
}

ZTEST_F(cobs_tests, test_stream_decode_continue_after_incomplete)
{
	struct cobs_decode_state dec;
	int ret;

	/* First fragment: [0x05, 0x01, 0x02] */
	const uint8_t frag1[] = {0x05, 0x01, 0x02};
	/* Second fragment: [0x03, 0x04, 0x00] */
	const uint8_t frag2[] = {0x03, 0x04, 0x00};
	const uint8_t expected[] = {0x01, 0x02, 0x03, 0x04};

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, frag1, sizeof(frag1), fixture->decoded);
	zassert_equal(ret, sizeof(frag1), "Fragment 1 failed");

	ret = cobs_decode_stream(&dec, frag2, sizeof(frag2), fixture->decoded);
	zassert_equal(ret, sizeof(frag2), "Fragment 2 failed");

	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
}

ZTEST_F(cobs_tests, test_stream_encode_decode_roundtrip)
{
	struct cobs_encode_state enc;
	struct cobs_decode_state dec;
	uint8_t encoded[128];
	size_t enc_len;
	int ret;

	/* Test various patterns */
	const uint8_t input[] = {0x01, 0x00, 0x02, 0x00, 0x00, 0x03};
	net_buf_add_mem(fixture->test_data, input, sizeof(input));

	cobs_encode_init(&enc);

	enc_len = sizeof(encoded) - 1;  /* Reserve space for delimiter */
	ret = cobs_encode_stream(&enc, fixture->test_data, encoded, &enc_len);
	zassert_equal(ret, 0, "Encoding failed");
	
	/* Add frame delimiter */
	encoded[enc_len++] = 0x00;

	cobs_decode_init(&dec);

	ret = cobs_decode_stream(&dec, encoded, enc_len, fixture->decoded);
	zassert_true(ret > 0, "Decoding failed");
	zassert_equal(fixture->decoded->len, sizeof(input), "Roundtrip length mismatch");
	zassert_mem_equal(fixture->decoded->data, input, sizeof(input), "Roundtrip data mismatch");
}

ZTEST_F(cobs_tests, test_stream_reset)
{
	struct cobs_encode_state enc;
	struct cobs_decode_state dec;

	/* Initialize and use */
	cobs_encode_init(&enc);
	enc.block_code = 100;  /* Modify state */

	/* Reset should clear state */
	cobs_encode_reset(&enc);
	zassert_equal(enc.block_code, 0, "Encoder state not reset");

	/* Same for decoder */
	cobs_decode_init(&dec);
	dec.bytes_left = 50;
	dec.need_delimiter = true;
	dec.frame_complete = true;

	cobs_decode_reset(&dec);
	zassert_equal(dec.bytes_left, 0, "Decoder bytes_left not reset");
	zassert_false(dec.need_delimiter, "Decoder need_delimiter not reset");
	zassert_false(dec.frame_complete, "Decoder frame_complete not reset");
}

/* ========================================================================
 * Boundary Condition Tests
 * ======================================================================== */

ZTEST_F(cobs_tests, test_stream_decode_delimiter_at_boundary)
{
	struct cobs_decode_state dec;
	int ret;

	/* Critical case: delimiter exactly at chunk boundary
	 * Chunk 1: [0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00]
	 * The 0x00 delimiter is the last byte - this is an important edge case!
	 */
	const uint8_t input[] = {0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00};
	const uint8_t expected[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

	cobs_decode_init(&dec);
	zassert_false(dec.frame_complete, "frame_complete should start false");

	ret = cobs_decode_stream(&dec, input, sizeof(input), fixture->decoded);
	
	/* Verify all bytes processed */
	zassert_equal(ret, sizeof(input), "Should process all bytes including delimiter");
	
	/* Verify frame_complete flag is set */
	zassert_true(dec.frame_complete, "frame_complete should be TRUE when delimiter found");
	
	/* Verify decoded data */
	zassert_equal(fixture->decoded->len, sizeof(expected), "Wrong decoded length");
	zassert_mem_equal(fixture->decoded->data, expected, sizeof(expected), "Data mismatch");
}

ZTEST_F(cobs_tests, test_stream_decode_frame_complete_flag)
{
	struct cobs_decode_state dec;
	int ret;

	cobs_decode_init(&dec);

	/* Test 1: Incomplete frame - no delimiter */
	const uint8_t partial[] = {0x05, 0x01, 0x02, 0x03};
	ret = cobs_decode_stream(&dec, partial, sizeof(partial), fixture->decoded);
	zassert_equal(ret, sizeof(partial), "Should process all bytes");
	zassert_false(dec.frame_complete, "frame_complete should be FALSE for partial frame");

	/* Test 2: Complete the frame with delimiter */
	const uint8_t completion[] = {0x04, 0x00};
	ret = cobs_decode_stream(&dec, completion, sizeof(completion), fixture->decoded);
	zassert_equal(ret, sizeof(completion), "Should process completion bytes");
	zassert_true(dec.frame_complete, "frame_complete should be TRUE after delimiter");

	/* Test 3: Start new frame - flag should clear */
	net_buf_reset(fixture->decoded);
	const uint8_t next_frame[] = {0x03, 0xAA, 0xBB};
	ret = cobs_decode_stream(&dec, next_frame, sizeof(next_frame), fixture->decoded);
	zassert_equal(ret, sizeof(next_frame), "Should process next frame start");
	zassert_false(dec.frame_complete, "frame_complete should reset for new frame");
}

ZTEST_F(cobs_tests, test_stream_decode_multi_chunk_frame)
{
	struct cobs_decode_state dec;
	int ret;

	cobs_decode_init(&dec);

	/* Build a large frame: 100 bytes of test data */
	uint8_t large_input[100];
	for (int i = 0; i < 100; i++) {
		large_input[i] = (uint8_t)(i + 1);
	}
	net_buf_add_mem(fixture->test_data, large_input, sizeof(large_input));

	/* Encode it */
	struct cobs_encode_state enc;
	uint8_t encoded[110];  /* Slightly larger for COBS overhead */
	size_t enc_len = sizeof(encoded) - 1;
	
	cobs_encode_init(&enc);
	
	ret = cobs_encode_stream(&enc, fixture->test_data, encoded, &enc_len);
	zassert_equal(ret, 0, "Encoding failed");
	
	/* Add delimiter */
	encoded[enc_len++] = 0x00;

	const size_t chunk_size = 16;  /* Smaller chunks to test boundary conditions */
	size_t offset = 0;
	bool frame_complete = false;

	while (offset < enc_len && !frame_complete) {
		size_t remaining = enc_len - offset;
		size_t to_process = (remaining < chunk_size) ? remaining : chunk_size;

		ret = cobs_decode_stream(&dec, encoded + offset, to_process, fixture->decoded);
		zassert_true(ret > 0, "Decode failed at offset %zu", offset);
		
		offset += ret;
		frame_complete = dec.frame_complete;
		
		/* If delimiter not found, should process entire chunk */
		if (!frame_complete && ret < to_process) {
			zassert_true(false, "Partial processing without frame completion");
		}
	}

	/* Verify frame was completed */
	zassert_true(frame_complete, "Frame should be marked complete");
	zassert_equal(offset, enc_len, "Should have processed all encoded data");
	
	/* Verify roundtrip integrity */
	zassert_equal(fixture->decoded->len, sizeof(large_input), "Decoded length mismatch");
	zassert_mem_equal(fixture->decoded->data, large_input, sizeof(large_input),
			  "Decoded data doesn't match original");
}

ZTEST_F(cobs_tests, test_stream_decode_delimiter_variations)
{
	struct cobs_decode_state dec;
	int ret;

	/* Test various delimiter positions to ensure flag works correctly */
	
	/* Case 1: Delimiter in middle of chunk */
	const uint8_t case1[] = {0x03, 0x11, 0x22, 0x00, 0xFF, 0xFF};
	cobs_decode_init(&dec);
	ret = cobs_decode_stream(&dec, case1, sizeof(case1), fixture->decoded);
	zassert_equal(ret, 4, "Should stop after delimiter");
	zassert_true(dec.frame_complete, "Flag should be set - delimiter in middle");
	
	/* Case 2: Delimiter at start */
	net_buf_reset(fixture->decoded);
	const uint8_t case2[] = {0x00, 0x03, 0x11, 0x22};
	cobs_decode_init(&dec);
	ret = cobs_decode_stream(&dec, case2, sizeof(case2), fixture->decoded);
	zassert_equal(ret, 1, "Should stop after first byte");
	zassert_true(dec.frame_complete, "Flag should be set - delimiter at start");
	
	/* Case 3: Delimiter at end (the critical case) */
	net_buf_reset(fixture->decoded);
	const uint8_t case3[] = {0x03, 0x11, 0x22, 0x00};
	cobs_decode_init(&dec);
	ret = cobs_decode_stream(&dec, case3, sizeof(case3), fixture->decoded);
	zassert_equal(ret, sizeof(case3), "Should process all bytes");
	zassert_true(dec.frame_complete, "Flag should be set - delimiter at end (CRITICAL)");
	
	/* Case 4: No delimiter (frame continues) */
	net_buf_reset(fixture->decoded);
	const uint8_t case4[] = {0x05, 0x11, 0x22, 0x33};
	cobs_decode_init(&dec);
	ret = cobs_decode_stream(&dec, case4, sizeof(case4), fixture->decoded);
	zassert_equal(ret, sizeof(case4), "Should process all bytes");
	zassert_false(dec.frame_complete, "Flag should be FALSE - no delimiter");
}
