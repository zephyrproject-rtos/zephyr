/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/http/hpack.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/ztest.h>

struct huffman_codes {
	uint32_t code;
	uint8_t bitlen;
};

/* Copy-paste from RFC7541. */
static struct huffman_codes test_huffman_codes[] = {
	{     0x1ff8, 13, }, {   0x7fffd8, 23, }, {  0xfffffe2, 28, }, {  0xfffffe3, 28, },
	{  0xfffffe4, 28, }, {  0xfffffe5, 28, }, {  0xfffffe6, 28, }, {  0xfffffe7, 28, },
	{  0xfffffe8, 28, }, {   0xffffea, 24, }, { 0x3ffffffc, 30, }, {  0xfffffe9, 28, },
	{  0xfffffea, 28, }, { 0x3ffffffd, 30, }, {  0xfffffeb, 28, }, {  0xfffffec, 28, },
	{  0xfffffed, 28, }, {  0xfffffee, 28, }, {  0xfffffef, 28, }, {  0xffffff0, 28, },
	{  0xffffff1, 28, }, {  0xffffff2, 28, }, { 0x3ffffffe, 30, }, {  0xffffff3, 28, },
	{  0xffffff4, 28, }, {  0xffffff5, 28, }, {  0xffffff6, 28, }, {  0xffffff7, 28, },
	{  0xffffff8, 28, }, {  0xffffff9, 28, }, {  0xffffffa, 28, }, {  0xffffffb, 28, },
	{       0x14,  6, }, {      0x3f8, 10, }, {      0x3f9, 10, }, {      0xffa, 12, },
	{     0x1ff9, 13, }, {       0x15,  6, }, {       0xf8,  8, }, {      0x7fa, 11, },
	{      0x3fa, 10, }, {      0x3fb, 10, }, {       0xf9,  8, }, {      0x7fb, 11, },
	{       0xfa,  8, }, {       0x16,  6, }, {       0x17,  6, }, {       0x18,  6, },
	{        0x0,  5, }, {        0x1,  5, }, {        0x2,  5, }, {       0x19,  6, },
	{       0x1a,  6, }, {       0x1b,  6, }, {       0x1c,  6, }, {       0x1d,  6, },
	{       0x1e,  6, }, {       0x1f,  6, }, {       0x5c,  7, }, {       0xfb,  8, },
	{     0x7ffc, 15, }, {       0x20,  6, }, {      0xffb, 12, }, {      0x3fc, 10, },
	{     0x1ffa, 13, }, {       0x21,  6, }, {       0x5d,  7, }, {       0x5e,  7, },
	{       0x5f,  7, }, {       0x60,  7, }, {       0x61,  7, }, {       0x62,  7, },
	{       0x63,  7, }, {       0x64,  7, }, {       0x65,  7, }, {       0x66,  7, },
	{       0x67,  7, }, {       0x68,  7, }, {       0x69,  7, }, {       0x6a,  7, },
	{       0x6b,  7, }, {       0x6c,  7, }, {       0x6d,  7, }, {       0x6e,  7, },
	{       0x6f,  7, }, {       0x70,  7, }, {       0x71,  7, }, {       0x72,  7, },
	{       0xfc,  8, }, {       0x73,  7, }, {       0xfd,  8, }, {     0x1ffb, 13, },
	{    0x7fff0, 19, }, {     0x1ffc, 13, }, {     0x3ffc, 14, }, {       0x22,  6, },
	{     0x7ffd, 15, }, {        0x3,  5, }, {       0x23,  6, }, {        0x4,  5, },
	{       0x24,  6, }, {        0x5,  5, }, {       0x25,  6, }, {       0x26,  6, },
	{       0x27,  6, }, {        0x6,  5, }, {       0x74,  7, }, {       0x75,  7, },
	{       0x28,  6, }, {       0x29,  6, }, {       0x2a,  6, }, {        0x7,  5, },
	{       0x2b,  6, }, {       0x76,  7, }, {       0x2c,  6, }, {        0x8,  5, },
	{        0x9,  5, }, {       0x2d,  6, }, {       0x77,  7, }, {       0x78,  7, },
	{       0x79,  7, }, {       0x7a,  7, }, {       0x7b,  7, }, {     0x7ffe, 15, },
	{      0x7fc, 11, }, {     0x3ffd, 14, }, {     0x1ffd, 13, }, {  0xffffffc, 28, },
	{    0xfffe6, 20, }, {   0x3fffd2, 22, }, {    0xfffe7, 20, }, {    0xfffe8, 20, },
	{   0x3fffd3, 22, }, {   0x3fffd4, 22, }, {   0x3fffd5, 22, }, {   0x7fffd9, 23, },
	{   0x3fffd6, 22, }, {   0x7fffda, 23, }, {   0x7fffdb, 23, }, {   0x7fffdc, 23, },
	{   0x7fffdd, 23, }, {   0x7fffde, 23, }, {   0xffffeb, 24, }, {   0x7fffdf, 23, },
	{   0xffffec, 24, }, {   0xffffed, 24, }, {   0x3fffd7, 22, }, {   0x7fffe0, 23, },
	{   0xffffee, 24, }, {   0x7fffe1, 23, }, {   0x7fffe2, 23, }, {   0x7fffe3, 23, },
	{   0x7fffe4, 23, }, {   0x1fffdc, 21, }, {   0x3fffd8, 22, }, {   0x7fffe5, 23, },
	{   0x3fffd9, 22, }, {   0x7fffe6, 23, }, {   0x7fffe7, 23, }, {   0xffffef, 24, },
	{   0x3fffda, 22, }, {   0x1fffdd, 21, }, {    0xfffe9, 20, }, {   0x3fffdb, 22, },
	{   0x3fffdc, 22, }, {   0x7fffe8, 23, }, {   0x7fffe9, 23, }, {   0x1fffde, 21, },
	{   0x7fffea, 23, }, {   0x3fffdd, 22, }, {   0x3fffde, 22, }, {   0xfffff0, 24, },
	{   0x1fffdf, 21, }, {   0x3fffdf, 22, }, {   0x7fffeb, 23, }, {   0x7fffec, 23, },
	{   0x1fffe0, 21, }, {   0x1fffe1, 21, }, {   0x3fffe0, 22, }, {   0x1fffe2, 21, },
	{   0x7fffed, 23, }, {   0x3fffe1, 22, }, {   0x7fffee, 23, }, {   0x7fffef, 23, },
	{    0xfffea, 20, }, {   0x3fffe2, 22, }, {   0x3fffe3, 22, }, {   0x3fffe4, 22, },
	{   0x7ffff0, 23, }, {   0x3fffe5, 22, }, {   0x3fffe6, 22, }, {   0x7ffff1, 23, },
	{  0x3ffffe0, 26, }, {  0x3ffffe1, 26, }, {    0xfffeb, 20, }, {    0x7fff1, 19, },
	{   0x3fffe7, 22, }, {   0x7ffff2, 23, }, {   0x3fffe8, 22, }, {  0x1ffffec, 25, },
	{  0x3ffffe2, 26, }, {  0x3ffffe3, 26, }, {  0x3ffffe4, 26, }, {  0x7ffffde, 27, },
	{  0x7ffffdf, 27, }, {  0x3ffffe5, 26, }, {   0xfffff1, 24, }, {  0x1ffffed, 25, },
	{    0x7fff2, 19, }, {   0x1fffe3, 21, }, {  0x3ffffe6, 26, }, {  0x7ffffe0, 27, },
	{  0x7ffffe1, 27, }, {  0x3ffffe7, 26, }, {  0x7ffffe2, 27, }, {   0xfffff2, 24, },
	{   0x1fffe4, 21, }, {   0x1fffe5, 21, }, {  0x3ffffe8, 26, }, {  0x3ffffe9, 26, },
	{  0xffffffd, 28, }, {  0x7ffffe3, 27, }, {  0x7ffffe4, 27, }, {  0x7ffffe5, 27, },
	{    0xfffec, 20, }, {   0xfffff3, 24, }, {    0xfffed, 20, }, {   0x1fffe6, 21, },
	{   0x3fffe9, 22, }, {   0x1fffe7, 21, }, {   0x1fffe8, 21, }, {   0x7ffff3, 23, },
	{   0x3fffea, 22, }, {   0x3fffeb, 22, }, {  0x1ffffee, 25, }, {  0x1ffffef, 25, },
	{   0xfffff4, 24, }, {   0xfffff5, 24, }, {  0x3ffffea, 26, }, {   0x7ffff4, 23, },
	{  0x3ffffeb, 26, }, {  0x7ffffe6, 27, }, {  0x3ffffec, 26, }, {  0x3ffffed, 26, },
	{  0x7ffffe7, 27, }, {  0x7ffffe8, 27, }, {  0x7ffffe9, 27, }, {  0x7ffffea, 27, },
	{  0x7ffffeb, 27, }, {  0xffffffe, 28, }, {  0x7ffffec, 27, }, {  0x7ffffed, 27, },
	{  0x7ffffee, 27, }, {  0x7ffffef, 27, }, {  0x7fffff0, 27, }, {  0x3ffffee, 26, },
};

/* Prepare a MSB aligned Huffman code, with padding. */
static void test_huffman_code_prepare(uint32_t *buf, int index)
{
	uint8_t pad_len = 32 - test_huffman_codes[index].bitlen;

	*buf = test_huffman_codes[index].code;
	/* Prepare buffer - align to MSB, add padding and covert to
	 * network byte order.
	 */
	*buf <<= pad_len;
	*buf |= (1UL << pad_len) - 1UL;
	*buf = htonl(*buf);
}

ZTEST(http2_hpack, test_huffman_encode_single)
{
	ARRAY_FOR_EACH(test_huffman_codes, i) {
		uint8_t expected_len = DIV_ROUND_UP(test_huffman_codes[i].bitlen, 8);
		uint32_t expected;
		uint32_t buf = 0;
		uint8_t symbol = i;
		int ret;

		test_huffman_code_prepare(&expected, i);

		ret = http_hpack_huffman_encode(&symbol, 1, (uint8_t *)&buf, sizeof(buf));
		zassert_equal(ret, expected_len, "Wrong encoding length");
		zassert_mem_equal(&buf, &expected, expected_len,
				  "Symbol wrongly encoded");
	}
}

ZTEST(http2_hpack, test_huffman_decode_single)
{
	ARRAY_FOR_EACH(test_huffman_codes, i) {
		uint8_t buflen = DIV_ROUND_UP(test_huffman_codes[i].bitlen, 8);
		uint32_t buf;
		uint8_t symbol;
		int ret;

		test_huffman_code_prepare(&buf, i);

		ret = http_hpack_huffman_decode((uint8_t *)&buf, buflen, &symbol, 1);
		zassert_equal(ret, 1, "Expected to decode 1 symbol");
		zassert_equal(symbol, i, "Wrong symbol decoded");
	}
}

static uint8_t test_buf[600];

ZTEST(http2_hpack, test_huffman_encode_decode_all)
{
	uint8_t str[ARRAY_SIZE(test_huffman_codes)];
	int expected_len = 0;
	int ret;

	ARRAY_FOR_EACH(str, i) {
		expected_len += test_huffman_codes[i].bitlen;
		str[i] = i;
	}

	expected_len = DIV_ROUND_UP(expected_len, 8);

	ret = http_hpack_huffman_encode(str, sizeof(str), test_buf, sizeof(test_buf));
	zassert_equal(ret, expected_len, "Wrong encoded length");
	memset(str, 0, sizeof(str));
	ret = http_hpack_huffman_decode(test_buf, expected_len, str, sizeof(str));
	zassert_equal(ret, sizeof(str), "Wrong decoded length");

	ARRAY_FOR_EACH(str, i) {
		expected_len += test_huffman_codes[i].bitlen;
		zassert_equal(str[i], i, "Wrong symbol decoded");
	}
}

struct example_huffman {
	const char *str;
	uint8_t encoded[46];
	uint8_t encoded_len;
};

/* Encoding examples from RFC7541 */
static const struct example_huffman test_huffman[] = {
	{ "www.example.com",
	  { 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0,
	    0xab, 0x90, 0xf4, 0xff },
	  12 },
	{ "no-cache", { 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf }, 6 },
	{ "custom-key", { 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9, 0x7d, 0x7f }, 8 },
	{ "custom-value",
	  { 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4,
	    0xbf },
	  9 },
	{ "302", { 0x64, 0x02 }, 2 },
	{ "private", { 0xae, 0xc3, 0x77, 0x1a, 0x4b }, 5 },
	{ "Mon, 21 Oct 2013 20:13:21 GMT",
	  { 0xd0, 0x7a, 0xbe, 0x94, 0x10, 0x54, 0xd4, 0x44,
	    0xa8, 0x20, 0x05, 0x95, 0x04, 0x0b, 0x81, 0x66,
	    0xe0, 0x82, 0xa6, 0x2d, 0x1b, 0xff},
	  22 },
	{ "https://www.example.com",
	  { 0x9d, 0x29, 0xad, 0x17, 0x18, 0x63, 0xc7, 0x8f,
	    0x0b, 0x97, 0xc8, 0xe9, 0xae, 0x82, 0xae, 0x43,
	    0xd3},
	  17 },
	{ "307", { 0x64, 0x0e, 0xff }, 3 },
	{ "gzip", { 0x9b, 0xd9, 0xab }, 3 },
	{ "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1",
	  { 0x94, 0xe7, 0x82, 0x1d, 0xd7, 0xf2, 0xe6, 0xc7,
	    0xb3, 0x35, 0xdf, 0xdf, 0xcd, 0x5b, 0x39, 0x60,
	    0xd5, 0xaf, 0x27, 0x08, 0x7f, 0x36, 0x72, 0xc1,
	    0xab, 0x27, 0x0f, 0xb5, 0x29, 0x1f, 0x95, 0x87,
	    0x31, 0x60, 0x65, 0xc0, 0x03, 0xed, 0x4e, 0xe5,
	    0xb1, 0x06, 0x3d, 0x50, 0x07 },
	  45 },
};

ZTEST(http2_hpack, test_huffman_encode_examples)
{
	ARRAY_FOR_EACH(test_huffman, i) {
		int ret;

		ret = http_hpack_huffman_encode(test_huffman[i].str,
						strlen(test_huffman[i].str),
						test_buf, sizeof(test_buf));
		zassert_equal(ret, test_huffman[i].encoded_len,
			      "Wrong encoding length");
		zassert_mem_equal(test_buf, test_huffman[i].encoded, ret,
				  "Symbol wrongly encoded");
	}
}

ZTEST(http2_hpack, test_huffman_decode_examples)
{
	ARRAY_FOR_EACH(test_huffman, i) {
		int ret;

		ret = http_hpack_huffman_decode(test_huffman[i].encoded,
						test_huffman[i].encoded_len,
						test_buf, sizeof(test_buf));
		zassert_equal(ret, strlen(test_huffman[i].str),
			      "Wrong decoded length");
		zassert_mem_equal(test_buf, test_huffman[i].str, ret,
				  "Symbol wrongly encoded");
	}
}

struct example_headers {
	const char *name;
	const char *value;
	uint8_t encoded[60];
	uint8_t encoded_len;
};

/* Examples from RFC7541 */
static const struct example_headers test_static_headers[] = {
	{ ":method", "GET", { 0x82 }, 1 },
	{ ":scheme", "http", { 0x86 }, 1 },
	{ ":path", "/", { 0x84 }, 1 },
	{ ":scheme", "https", { 0x87 }, 1 },
	{ ":path", "/index.html", { 0x85 }, 1 },
	{ ":status", "200", { 0x88 }, 1 },
};

static void test_hpack_verify_encode(const struct example_headers *example,
				     size_t num_examples)
{
	for (int i = 0; i < num_examples; i++) {
		struct http_hpack_header_buf hdr = {
			.name = example[i].name,
			.value = example[i].value,
			.name_len = strlen(example[i].name),
			.value_len = strlen(example[i].value)
		};
		int ret;

		ret = http_hpack_encode_header(test_buf, sizeof(test_buf), &hdr);
		zassert_equal(ret, example[i].encoded_len, "Wrong encoding length");
		zassert_mem_equal(test_buf, example[i].encoded, ret,
				  "Header wrongly decoded");
	}
}

static void test_hpack_verify_decode(const struct example_headers *example,
				     size_t num_examples)
{
	for (int i = 0; i < num_examples; i++) {
		struct http_hpack_header_buf hdr;
		int ret;

		ret = http_hpack_decode_header(example[i].encoded, example[i].encoded_len, &hdr);
		zassert_equal(ret, example[i].encoded_len, "Wrong decoding length");
		zassert_equal(hdr.name_len, strlen(example[i].name),
			      "Wrong decoded header name length");
		zassert_equal(hdr.value_len, strlen(example[i].value),
			      "Wrong decoded header value length");
		zassert_mem_equal(hdr.name, example[i].name, hdr.name_len,
				  "Header name wrongly decoded");
		zassert_mem_equal(hdr.value, example[i].value, hdr.value_len,
				  "Header value wrongly decoded");
	}
}

ZTEST(http2_hpack, test_http2_hpack_static_encode)
{
	test_hpack_verify_encode(test_static_headers,
				 ARRAY_SIZE(test_static_headers));
}

ZTEST(http2_hpack, test_http2_hpack_static_decode)
{
	test_hpack_verify_decode(test_static_headers,
				 ARRAY_SIZE(test_static_headers));
}

static const struct example_headers test_dec_literal_indexed_headers[] = {
	{ ":path", "/sample/path",
	  { 0x04, 0x0c, 0x2f, 0x73, 0x61, 0x6d, 0x70, 0x6c,
	    0x65, 0x2f, 0x70, 0x61, 0x74, 0x68 },
	  14 },
	{ ":authority", "www.example.com",
	  { 0x41, 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78,
	    0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f,
	    0x6d },
	  17 },
	{ "cache-control", "no-cache",
	  { 0x58, 0x08, 0x6e, 0x6f, 0x2d, 0x63, 0x61, 0x63,
	    0x68, 0x65 },
	  10 },
	{ ":authority", "www.example.com", /* Huffman encoded */
	  { 0x41, 0x8c, 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a,
	    0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff },
	  14 },
	{ "cache-control", "no-cache", /* Huffman encoded */
	  { 0x58, 0x86, 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf },
	  8 },
	{ ":status", "302",
	  { 0x48, 0x03, 0x33, 0x30, 0x32 },
	  5 },
	{ "cache-control", "private",
	  { 0x58, 0x07, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74,
	    0x65 },
	  9 },
	{ "date", "Mon, 21 Oct 2013 20:13:21 GMT",
	  { 0x61, 0x1d, 0x4d, 0x6f, 0x6e, 0x2c, 0x20, 0x32,
	    0x31, 0x20, 0x4f, 0x63, 0x74, 0x20, 0x32, 0x30,
	    0x31, 0x33, 0x20, 0x32, 0x30, 0x3a, 0x31, 0x33,
	    0x3a, 0x32, 0x31, 0x20, 0x47, 0x4d, 0x54 },
	  31 },
	{ "location", "https://www.example.com",
	  { 0x6e, 0x17, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3a,
	    0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78,
	    0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f,
	    0x6d },
	  25 },
	{ ":status", "307",
	  { 0x48, 0x03, 0x33, 0x30, 0x37 },
	  5 },
	{ "content-encoding", "gzip",
	  { 0x5a, 0x04, 0x67, 0x7a, 0x69, 0x70 },
	  6 },
	{ "set-cookie", "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1",
	  { 0x77, 0x38, 0x66, 0x6f, 0x6f, 0x3d, 0x41, 0x53,
	    0x44, 0x4a, 0x4b, 0x48, 0x51, 0x4b, 0x42, 0x5a,
	    0x58, 0x4f, 0x51, 0x57, 0x45, 0x4f, 0x50, 0x49,
	    0x55, 0x41, 0x58, 0x51, 0x57, 0x45, 0x4f, 0x49,
	    0x55, 0x3b, 0x20, 0x6d, 0x61, 0x78, 0x2d, 0x61,
	    0x67, 0x65, 0x3d, 0x33, 0x36, 0x30, 0x30, 0x3b,
	    0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
	    0x3d, 0x31 },
	  58 },
	{ ":status", "302", /* Huffman encoded */
	  { 0x48, 0x82, 0x64, 0x02 },
	  4 },
	{ "cache-control", "private", /* Huffman encoded */
	  { 0x58, 0x85, 0xae, 0xc3, 0x77, 0x1a, 0x4b },
	  7 },
	{ "date", "Mon, 21 Oct 2013 20:13:21 GMT", /* Huffman encoded */
	  { 0x61, 0x96, 0xd0, 0x7a, 0xbe, 0x94, 0x10, 0x54,
	    0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95, 0x04, 0x0b,
	    0x81, 0x66, 0xe0, 0x82, 0xa6, 0x2d, 0x1b, 0xff },
	  24 },
	{ "location", "https://www.example.com", /* Huffman encoded */
	  { 0x6e, 0x91, 0x9d, 0x29, 0xad, 0x17, 0x18, 0x63,
	    0xc7, 0x8f, 0x0b, 0x97, 0xc8, 0xe9, 0xae, 0x82,
	    0xae, 0x43, 0xd3 },
	  19 },
	{ ":status", "307", /* Huffman encoded */
	  { 0x48, 0x83, 0x64, 0x0e, 0xff },
	  5 },
	{ "content-encoding", "gzip", /* Huffman encoded */
	  { 0x5a, 0x83, 0x9b, 0xd9, 0xab },
	  5 },
	{ "set-cookie", "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1",
	  /* Huffman encoded */
	  { 0x77, 0xad, 0x94, 0xe7, 0x82, 0x1d, 0xd7, 0xf2,
	    0xe6, 0xc7, 0xb3, 0x35, 0xdf, 0xdf, 0xcd, 0x5b,
	    0x39, 0x60, 0xd5, 0xaf, 0x27, 0x08, 0x7f, 0x36,
	    0x72, 0xc1, 0xab, 0x27, 0x0f, 0xb5, 0x29, 0x1f,
	    0x95, 0x87, 0x31, 0x60, 0x65, 0xc0, 0x03, 0xed,
	    0x4e, 0xe5, 0xb1, 0x06, 0x3d, 0x50, 0x07 },
	  47 },
};

/* For encoding, we always use never indexed and Huffman when applicable. */
static const struct example_headers test_enc_literal_indexed_headers[] = {
	{ ":authority", "www.example.com", /* Huffman encoded */
	  { 0x11, 0x8c, 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a,
	    0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff },
	  14 },
	{ "cache-control", "no-cache", /* Huffman encoded */
	  { 0x1f, 0x09, 0x86, 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf },
	  9 },
	{ ":status", "302", /* Huffman encoded */
	  { 0x18, 0x82, 0x64, 0x02 },
	  4 },
	{ "cache-control", "private", /* Huffman encoded */
	  { 0x1f, 0x09, 0x85, 0xae, 0xc3, 0x77, 0x1a, 0x4b },
	  8 },
	{ "date", "Mon, 21 Oct 2013 20:13:21 GMT", /* Huffman encoded */
	  { 0x1f, 0x12, 0x96, 0xd0, 0x7a, 0xbe, 0x94, 0x10,
	    0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95, 0x04,
	    0x0b, 0x81, 0x66, 0xe0, 0x82, 0xa6, 0x2d, 0x1b,
	    0xff },
	  25 },
	{ "location", "https://www.example.com", /* Huffman encoded */
	  { 0x1f, 0x1f, 0x91, 0x9d, 0x29, 0xad, 0x17, 0x18,
	    0x63, 0xc7, 0x8f, 0x0b, 0x97, 0xc8, 0xe9, 0xae,
	    0x82, 0xae, 0x43, 0xd3 },
	  20 },
	{ ":status", "307",
	  /* In this case Huffman is not used, as it does not give any size savings. */
	  { 0x18, 0x03, 0x33, 0x30, 0x37 },
	  5 },
	{ "content-encoding", "gzip", /* Huffman encoded */
	  { 0x1f, 0x0b, 0x83, 0x9b, 0xd9, 0xab },
	  6 },
	{ "set-cookie", "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1",
	  /* Huffman encoded */
	  { 0x1f, 0x28, 0xad, 0x94, 0xe7, 0x82, 0x1d, 0xd7,
	    0xf2, 0xe6, 0xc7, 0xb3, 0x35, 0xdf, 0xdf, 0xcd,
	    0x5b, 0x39, 0x60, 0xd5, 0xaf, 0x27, 0x08, 0x7f,
	    0x36, 0x72, 0xc1, 0xab, 0x27, 0x0f, 0xb5, 0x29,
	    0x1f, 0x95, 0x87, 0x31, 0x60, 0x65, 0xc0, 0x03,
	    0xed, 0x4e, 0xe5, 0xb1, 0x06, 0x3d, 0x50, 0x07 },
	  48 },
};

ZTEST(http2_hpack, test_http2_hpack_literal_indexed_encode)
{
	test_hpack_verify_encode(test_enc_literal_indexed_headers,
				 ARRAY_SIZE(test_enc_literal_indexed_headers));
}

ZTEST(http2_hpack, test_http2_hpack_literal_indexed_decode)
{
	test_hpack_verify_decode(test_dec_literal_indexed_headers,
				 ARRAY_SIZE(test_dec_literal_indexed_headers));
	/* We should be able to decode encoding test cases as well. */
	test_hpack_verify_decode(test_enc_literal_indexed_headers,
				 ARRAY_SIZE(test_enc_literal_indexed_headers));
}

static const struct example_headers test_dec_literal_not_indexed_headers[] = {
	{ "custom-key", "custom-header",
	  { 0x40, 0x0a, 0x63, 0x75, 0x73, 0x74, 0x6f, 0x6d,
	    0x2d, 0x6b, 0x65, 0x79, 0x0d, 0x63, 0x75, 0x73,
	    0x74, 0x6f, 0x6d, 0x2d, 0x68, 0x65, 0x61, 0x64,
	    0x65, 0x72 },
	  26 },
	{ "password", "secret",
	  { 0x10, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f,
	    0x72, 0x64, 0x06, 0x73, 0x65, 0x63, 0x72, 0x65,
	    0x74 },
	  17 },
	{ "custom-key", "custom-value",
	  { 0x40, 0x0a, 0x63, 0x75, 0x73, 0x74, 0x6f, 0x6d,
	    0x2d, 0x6b, 0x65, 0x79, 0x0c, 0x63, 0x75, 0x73,
	    0x74, 0x6f, 0x6d, 0x2d, 0x76, 0x61, 0x6c, 0x75,
	    0x65 },
	  25 },
	{ "custom-key", "custom-value", /* Huffman encoded */
	  { 0x40, 0x88, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9,
	    0x7d, 0x7f, 0x89, 0x25, 0xa8, 0x49, 0xe9, 0x5b,
	    0xb8, 0xe8, 0xb4, 0xbf },
	  20 },
};

/* For encoding, we always use never indexed and Huffman. */
static const struct example_headers test_enc_literal_not_indexed_headers[] = {
	{ "custom-key", "custom-value", /* Huffman encoded */
	  { 0x10, 0x88, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9,
	    0x7d, 0x7f, 0x89, 0x25, 0xa8, 0x49, 0xe9, 0x5b,
	    0xb8, 0xe8, 0xb4, 0xbf },
	  20 },
};

/* For encoding, we always use never indexed and Huffman codes. */
ZTEST(http2_hpack, test_http2_hpack_literal_not_indexed_encode)
{
	test_hpack_verify_encode(test_enc_literal_not_indexed_headers,
				 ARRAY_SIZE(test_enc_literal_not_indexed_headers));
}

ZTEST(http2_hpack, test_http2_hpack_literal_not_indexed_decode)
{
	test_hpack_verify_decode(test_dec_literal_not_indexed_headers,
				 ARRAY_SIZE(test_dec_literal_not_indexed_headers));
	/* We should be able to decode encoding test cases as well. */
	test_hpack_verify_decode(test_enc_literal_not_indexed_headers,
				 ARRAY_SIZE(test_enc_literal_not_indexed_headers));
}

ZTEST_SUITE(http2_hpack, NULL, NULL, NULL, NULL, NULL);
