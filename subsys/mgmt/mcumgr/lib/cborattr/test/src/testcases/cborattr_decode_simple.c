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
test_encode_data(void)
{
	CborEncoder test_data;
	uint8_t data[4] = { 0, 1, 2 };

	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &test_data, CborIndefiniteLength);
	/*
	 * a:22
	 */
	cbor_encode_text_stringz(&test_data, "a");
	cbor_encode_uint(&test_data, 22);

	/*
	 * b:-13
	 */
	cbor_encode_text_stringz(&test_data, "b");
	cbor_encode_int(&test_data, -13);

	/*
	 * c:0x000102
	 */
	cbor_encode_text_stringz(&test_data, "c");
	cbor_encode_byte_string(&test_data, data, 3);
	cbor_encoder_close_container(&test_encoder, &test_data);

	/*
	 * XXX add other data types to encode here.
	 */

}

/*
 * Simple decoding.
 */
TEST_CASE(test_cborattr_decode_simple)
{
	int rc;
	uint64_t a_val = 0;
	int64_t b_val = 0;
	uint8_t c_data[4];
	size_t c_len;
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrIntegerType,
			.addr.uinteger = &a_val,
			.nodefault = true
		},
		[1] = {
			.attribute = "b",
			.type = CborAttrIntegerType,
			.addr.integer = &b_val,
			.nodefault = true
		},
		[2] = {
			.attribute = "c",
			.type = CborAttrByteStringType,
			.addr.bytestring.data = c_data,
			.addr.bytestring.len = &c_len,
			.len = sizeof(c_data)
		},
		[3] = {
			.attribute = NULL
		}
	};

	test_encode_data();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(a_val == 22);
	TEST_ASSERT(b_val == -13);
	TEST_ASSERT(c_len == 3);
	TEST_ASSERT(c_data[0] == 0 && c_data[1] == 1 && c_data[2] == 2);
}
