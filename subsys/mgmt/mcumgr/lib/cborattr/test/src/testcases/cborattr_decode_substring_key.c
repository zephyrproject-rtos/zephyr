/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_cborattr.h"

/*
 * Where we collect cbor data.
 */
static uint8_t test_cbor_buf[1024];
static int test_cbor_len;

/*
 * CBOR encoder data structures.
 */
static int test_cbor_wr(struct cbor_encoder_writer *, const char *, int);
static CborEncoder test_encoder;
static struct cbor_encoder_writer test_writer = {
	.write = test_cbor_wr
};

static int
test_cbor_wr(struct cbor_encoder_writer *cew, const char *data, int len)
{
	memcpy(test_cbor_buf + test_cbor_len, data, len);
	test_cbor_len += len;

	assert(test_cbor_len < sizeof(test_cbor_buf));
	return 0;
}

static void
test_encode_substring_key(void)
{
	CborEncoder data;

	cbor_encoder_init(&test_encoder, &test_writer, 0);

	/*
	 * { "a": "A", "aa": "AA", "aaa" : "AAA" }
	 */
	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	cbor_encode_text_stringz(&data, "a");
	cbor_encode_text_stringz(&data, "A");
	cbor_encode_text_stringz(&data, "aa");
	cbor_encode_text_stringz(&data, "AA");
	cbor_encode_text_stringz(&data, "aaa");
	cbor_encode_text_stringz(&data, "AAA");

	cbor_encoder_close_container(&test_encoder, &data);
}

/*
 * substring key
 */
TEST_CASE(test_cborattr_decode_substring_key)
{
	int rc;
	char test_str_1a[4] = { '\0' };
	char test_str_2a[4] = { '\0' };
	char test_str_3a[4] = { '\0' };
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "aaa",
			.type = CborAttrTextStringType,
			.addr.string = test_str_3a,
			.len = sizeof(test_str_3a),
			.nodefault = true
		},
		[1] = {
			.attribute = "aa",
			.type = CborAttrTextStringType,
			.addr.string = test_str_2a,
			.len = sizeof(test_str_2a),
			.nodefault = true
			},
		[2] = {
			.attribute = "a",
			.type = CborAttrTextStringType,
			.addr.string = test_str_1a,
			.len = sizeof(test_str_1a),
			.nodefault = true
			},
		[3] = {
			.attribute = NULL
		}
	};

	test_encode_substring_key();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(!strcmp(test_str_1a, "A"));
	TEST_ASSERT(!strcmp(test_str_2a, "AA"));
	TEST_ASSERT(!strcmp(test_str_3a, "AAA"));
}
