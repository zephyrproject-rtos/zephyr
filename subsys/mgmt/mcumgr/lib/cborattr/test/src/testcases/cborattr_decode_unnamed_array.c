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
test_encode_unnamed_array(void)
{
	CborEncoder data;
	CborEncoder array;

	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	/*
	 * [1,2,33]
	 */
	cbor_encoder_create_array(&data, &array, CborIndefiniteLength);
	cbor_encode_int(&array, 1);
	cbor_encode_int(&array, 2);
	cbor_encode_int(&array, 33);
	cbor_encoder_close_container(&data, &array);

	cbor_encoder_close_container(&test_encoder, &data);
}

/*
 * integer array
 */
TEST_CASE(test_cborattr_decode_unnamed_array)
{
	int rc;
	int64_t arr_data[5];
	int arr_cnt = 0;
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = CBORATTR_ATTR_UNNAMED,
			.type = CborAttrArrayType,
			.addr.array.element_type = CborAttrIntegerType,
			.addr.array.arr.integers.store = arr_data,
			.addr.array.count = &arr_cnt,
			.addr.array.maxlen = ARRAY_SIZ(arr_data),
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};

	test_encode_unnamed_array();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(arr_cnt == 3);
	TEST_ASSERT(arr_data[0] == 1);
	TEST_ASSERT(arr_data[1] == 2);
	TEST_ASSERT(arr_data[2] == 33);
}
