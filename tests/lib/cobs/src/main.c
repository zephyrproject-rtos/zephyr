/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/data/cobs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cobs_test, LOG_LEVEL_DBG);

#define TEST_BUF_SIZE  1024
#define TEST_BUF_COUNT 4

NET_BUF_POOL_DEFINE(test_pool, TEST_BUF_COUNT, TEST_BUF_SIZE, 0, NULL);

static struct net_buf *test_data;
static struct net_buf *encoded;
static struct net_buf *decoded;

static void setup(void *data)
{
	ARG_UNUSED(data);

	if (test_data) {
		net_buf_unref(test_data);
	}
	if (encoded) {
		net_buf_unref(encoded);
	}
	if (decoded) {
		net_buf_unref(decoded);
	}

	test_data = net_buf_alloc(&test_pool, K_NO_WAIT);
	encoded = net_buf_alloc(&test_pool, K_NO_WAIT);
	decoded = net_buf_alloc(&test_pool, K_NO_WAIT);

	zassert_not_null(test_data, "Failed to allocate test_data buffer");
	zassert_not_null(encoded, "Failed to allocate encoded buffer");
	zassert_not_null(decoded, "Failed to allocate decoded buffer");

	net_buf_reset(test_data);
	net_buf_reset(encoded);
	net_buf_reset(decoded);
}

ZTEST(cobs_tests, test_cobs_empty)
{
	int ret;
	uint8_t empty_enc[] = {0x01};

	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding empty data failed");
	zassert_mem_equal(encoded->data, empty_enc, sizeof(empty_enc));
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding empty data failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}
ZTEST(cobs_tests, test_cobs_single_char)
{
	int ret;
	uint8_t one_char[] = {'1'};
	uint8_t one_char_enc[] = {0x02, '1', 0x00};

	net_buf_add_mem(test_data, one_char, sizeof(one_char));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding single character failed");
	zassert_mem_equal(encoded->data, one_char_enc, sizeof(one_char_enc));
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding single character failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_single_zero)
{
	int ret;
	uint8_t zero[] = {0x00};
	uint8_t zero_enc[] = {0x01, 0x01, 0x00};

	net_buf_add_mem(test_data, zero, sizeof(zero));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding single zero failed");
	zassert_mem_equal(encoded->data, zero_enc, sizeof(zero_enc));
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding single zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_two_zeros)
{
	int ret;
	uint8_t zeros[] = {0x00, 0x00};
	uint8_t zeros_enc[] = {0x01, 0x01, 0x01, 0x00};

	net_buf_add_mem(test_data, zeros, sizeof(zeros));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding two zeros failed");
	zassert_mem_equal(encoded->data, zeros_enc, sizeof(zeros_enc));
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding two zeros failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_three_zeros)
{
	int ret;
	uint8_t zeros[] = {0x00, 0x00, 0x00};
	uint8_t zeros_enc[] = {0x01, 0x01, 0x01, 0x01, 0x00};

	net_buf_add_mem(test_data, zeros, sizeof(zeros));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding three zeros failed");
	zassert_mem_equal(encoded->data, zeros_enc, sizeof(zeros_enc));
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding three zeros failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_five_chars)
{
	int ret;
	uint8_t chars[] = {'1', '2', '3', '4', '5'};
	uint8_t chars_enc[] = {0x06, '1', '2', '3', '4', '5', 0x00};

	net_buf_add_mem(test_data, chars, sizeof(chars));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding five characters failed");
	zassert_mem_equal(encoded->data, chars_enc, 6);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding five characters failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_embedded_zero)
{
	int ret;
	uint8_t data[] = {'1', '2', '3', '4', '5', 0x00, '6', '7', '8', '9'};

	uint8_t data_enc[] = {0x06, '1', '2', '3', '4', '5', 0x05, '6', '7', '8', '9', 0x00};

	net_buf_add_mem(test_data, data, sizeof(data));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding embedded zero failed");
	zassert_mem_equal(encoded->data, data_enc, 11);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding embedded zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_starting_embedded_zero)
{
	int ret;
	uint8_t data[] = {0x00, '1', '2', '3', '4', '5', 0x00, '6', '7', '8', '9'};

	uint8_t data_enc[] = {0x01, 0x06, '1', '2', '3', '4', '5', 0x05, '6', '7', '8', '9', 0x00};

	net_buf_add_mem(test_data, data, sizeof(data));
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding starting and embedded zero failed");
	zassert_mem_equal(encoded->data, data_enc, 12);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding starting and embedded zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_embedded_trailing_zero)
{
	int ret;
	uint8_t data[] = {'1', '2', '3', '4', '5', 0x00, '6', '7', '8', '9', 0x00};

	uint8_t data_enc[] = {0x06, '1', '2', '3', '4', '5', 0x05, '6', '7', '8', '9', 0x01, 0x00};

	net_buf_add_mem(test_data, data, 11);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding embedded and trailing zero failed");
	zassert_mem_equal(encoded->data, data_enc, 12);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding embedded and trailing zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_253_nonzero)
{
	int ret;
	const char data[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e',
		'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1',
		'2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2',
		'3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
		'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3'};
	const char data_enc[] = {
		0xfe, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
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
		'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '\0'};

	net_buf_add_mem(test_data, data, 253);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding 253 non-zero bytes failed");
	zassert_mem_equal(encoded->data, data_enc, 254);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding 253 non-zero bytes failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_254_nonzero)
{
	int ret;
	const char data[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e',
		'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1',
		'2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2',
		'3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
		'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4'};
	const char data_enc[] = {
		0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
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
		'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', '\0'};

	net_buf_add_mem(test_data, data, 254);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding 254 non-zero bytes failed");
	zassert_mem_equal(encoded->data, data_enc, 255);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding 254 non-zero bytes failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_255_nonzero)
{
	int ret;
	const unsigned char data[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
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
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', '5'};
	const unsigned char data_enc[] = {
		0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
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
		'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', 0x02,
		'5'};

	net_buf_add_mem(test_data, data, 255);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding 255 non-zero bytes failed");
	zassert_mem_equal(encoded->data, data_enc, 256);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding 255 non-zero bytes failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_zero_255_nonzero)
{
	int ret;
	const unsigned char data[] = {
		0x00, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
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
		'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', '5'};
	const unsigned char data_enc[] = {
		0x01, 0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
		'E',  'F',  'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'a',  'b',  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
		'q',  'r',  's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		'C',  'D',  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		'S',  'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o',  'p',  'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A',  'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q',  'R',  'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		'm',  'n',  'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',
		'8',  '9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O',  'P',  'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		'k',  'l',  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5',
		'6',  '7',  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
		'M',  'N',  'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		'i',  'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4',
		0x02, '5',  0x00};

	net_buf_add_mem(test_data, data, 256);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding zero followed by 255 non-zero bytes failed");
	zassert_mem_equal(encoded->data, data_enc, 257);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding zero followed by 255 non-zero bytes failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_253_nonzero_zero)
{
	int ret;
	const char data[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd',
		'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e',
		'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1',
		'2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2',
		'3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
		'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', 0x00};
	const char data_enc[] = {
		0xfe, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',  'E',
		'F',  'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',  'a',
		'b',  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',  'q',
		'r',  's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',  'C',
		'D',  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',  'S',
		'T',  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',  'o',
		'p',  'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A',
		'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',  'Q',
		'R',  'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',  'm',
		'n',  'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7',  '8',
		'9',  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',  'O',
		'P',  'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k',
		'l',  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5',  '6',
		'7',  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M',
		'N',  'O', 'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',  'i',
		'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', 0x01, 0x00};

	net_buf_add_mem(test_data, data, 254);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding 253 non-zero bytes followed by zero failed");
	zassert_mem_equal(encoded->data, data_enc, 255);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding 253 non-zero bytes followed by zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_254_nonzero_zero)
{
	int ret;
	const char data[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
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
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', 0x00};
	const char data_enc[] = {
		0xff, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
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
		'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', 0x01,
		0x01, 0x00};

	net_buf_add_mem(test_data, data, 255);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding 254 non-zero bytes followed by zero failed");
	zassert_mem_equal(encoded->data, data_enc, 256);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding 254 non-zero bytes followed by zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST(cobs_tests, test_cobs_255_nonzero_zero)
{
	int ret;
	const char data[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
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
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', '5', 0x00};
	const char data_enc[] = {
		0xff, '0',  '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
		'F',  'G',  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'a',
		'b',  'c',  'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
		'r',  's',  't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
		'D',  'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		'T',  'a',  'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		'p',  'q',  'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
		'B',  'C',  'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		'R',  'S',  'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n',  'o',  'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		'9',  'A',  'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		'P',  'Q',  'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
		'l',  'm',  'n', 'o', 'p', 'q', 'r', 's', 't', '0', '1', '2', '3', '4', '5', '6',
		'7',  '8',  '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N',  'O',  'P', 'Q', 'R', 'S', 'T', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j',  'k',  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', '1', '2', '3', '4', 0x02,
		'5',  0x01, 0x00};

	net_buf_add_mem(test_data, data, 256);
	ret = cobs_encode(test_data, encoded);
	zassert_equal(ret, 0, "Encoding 255 non-zero bytes followed by zero failed");
	zassert_mem_equal(encoded->data, data_enc, 257);
	ret = cobs_decode(encoded, decoded);
	zassert_equal(ret, 0, "Decoding 255 non-zero bytes followed by zero failed");
	zassert_mem_equal(decoded->data, test_data->data, test_data->len);
}

ZTEST_SUITE(cobs_tests, NULL, NULL, setup, NULL, NULL);
