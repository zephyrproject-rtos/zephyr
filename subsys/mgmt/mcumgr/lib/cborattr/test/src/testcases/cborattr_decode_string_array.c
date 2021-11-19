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
test_encode_string_array_one(void)
{
	CborEncoder data;
	CborEncoder array;

	test_cbor_len = 0;
	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	/*
	 * a: ["asdf"]
	 */
	cbor_encode_text_stringz(&data, "a");

	cbor_encoder_create_array(&data, &array, CborIndefiniteLength);
	cbor_encode_text_stringz(&array, "asdf");
	cbor_encoder_close_container(&data, &array);

	cbor_encoder_close_container(&test_encoder, &data);
}

static void
test_encode_string_array_three(void)
{
	CborEncoder data;
	CborEncoder array;

	test_cbor_len = 0;
	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	/*
	 * a: ["asdf", "k", "blurb"]
	 */
	cbor_encode_text_stringz(&data, "a");

	cbor_encoder_create_array(&data, &array, CborIndefiniteLength);
	cbor_encode_text_stringz(&array, "asdf");
	cbor_encode_text_stringz(&array, "k");
	cbor_encode_text_stringz(&array, "blurb");
	cbor_encoder_close_container(&data, &array);

	cbor_encoder_close_container(&test_encoder, &data);
}

/*
 * string array
 */
TEST_CASE(test_cborattr_decode_string_array)
{
	int rc;
	char *str_ptrs[5];
	char arr_data[256];
	int arr_cnt = 0;
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrArrayType,
			.addr.array.element_type = CborAttrTextStringType,
			.addr.array.arr.strings.ptrs = str_ptrs,
			.addr.array.arr.strings.store = arr_data,
			.addr.array.arr.strings.storelen = sizeof(arr_data),
			.addr.array.count = &arr_cnt,
			.addr.array.maxlen = ARRAY_SIZE(arr_data),
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};

	test_encode_string_array_one();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(arr_cnt == 1);
	TEST_ASSERT(!strcmp(str_ptrs[0], "asdf"));

	test_encode_string_array_three();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(arr_cnt == 3);
	TEST_ASSERT(!strcmp(str_ptrs[0], "asdf"));
	TEST_ASSERT(!strcmp(str_ptrs[1], "k"));
	TEST_ASSERT(!strcmp(str_ptrs[2], "blurb"));
}
