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
test_encode_int_array(void)
{
	CborEncoder data;
	CborEncoder array;

	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	/*
	 * a: [1,2,33,15,-4]
	 */
	cbor_encode_text_stringz(&data, "a");

	cbor_encoder_create_array(&data, &array, CborIndefiniteLength);
	cbor_encode_int(&array, 1);
	cbor_encode_int(&array, 2);
	cbor_encode_int(&array, 33);
	cbor_encode_int(&array, 15);
	cbor_encode_int(&array, -4);
	cbor_encoder_close_container(&data, &array);

	cbor_encoder_close_container(&test_encoder, &data);
}

/*
 * integer array
 */
TEST_CASE(test_cborattr_decode_int_array)
{
	int rc;
	int64_t arr_data[5];
	int64_t b_int;
	int arr_cnt = 0;
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrArrayType,
			.addr.array.element_type = CborAttrIntegerType,
			.addr.array.arr.integers.store = arr_data,
			.addr.array.count = &arr_cnt,
			.addr.array.maxlen = ARRAY_SIZE(arr_data),
			.nodefault = true
		},
		[1] = {
			.attribute = "b",
			.type = CborAttrIntegerType,
			.addr.integer = &b_int,
			.dflt.integer = 1
		},
		[2] = {
			.attribute = NULL
		}
	};
	struct cbor_attr_t test_attrs_small[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrArrayType,
			.addr.array.element_type = CborAttrIntegerType,
			.addr.array.arr.integers.store = arr_data,
			.addr.array.count = &arr_cnt,
			.addr.array.maxlen = 1,
			.nodefault = true
		},
		[1] = {
			.attribute = "b",
			.type = CborAttrIntegerType,
			.addr.integer = &b_int,
			.dflt.integer = 1
		},
		[2] = {
			.attribute = NULL
		}
	};

	test_encode_int_array();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(arr_cnt == 5);
	TEST_ASSERT(arr_data[0] == 1);
	TEST_ASSERT(arr_data[1] == 2);
	TEST_ASSERT(arr_data[2] == 33);
	TEST_ASSERT(arr_data[3] == 15);
	TEST_ASSERT(arr_data[4] == -4);
	TEST_ASSERT(b_int == 1);

	memset(arr_data, 0, sizeof(arr_data));
	b_int = 0;

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs_small);
	TEST_ASSERT(rc == CborErrorDataTooLarge);
	TEST_ASSERT(arr_cnt == 1);
	TEST_ASSERT(arr_data[0] == 1);
	TEST_ASSERT(arr_data[1] == 0);
	TEST_ASSERT(b_int == 1);
}
