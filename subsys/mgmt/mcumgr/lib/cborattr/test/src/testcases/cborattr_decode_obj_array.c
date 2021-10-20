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
test_encode_obj_array(void)
{
	CborEncoder data;
	CborEncoder array;
	CborEncoder obj;

	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	/*
	 * a: [{ n:"a", v:1}, {n:"b", v:2} ]
	 */
	cbor_encode_text_stringz(&data, "a");
	cbor_encoder_create_array(&data, &array, CborIndefiniteLength);

	cbor_encoder_create_map(&array, &obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&obj, "n");
	cbor_encode_text_stringz(&obj, "a");
	cbor_encode_text_stringz(&obj, "v");
	cbor_encode_int(&obj, 1);
	cbor_encoder_close_container(&array, &obj);

	cbor_encoder_create_map(&array, &obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&obj, "n");
	cbor_encode_text_stringz(&obj, "b");
	cbor_encode_text_stringz(&obj, "v");
	cbor_encode_int(&obj, 2);
	cbor_encoder_close_container(&array, &obj);

	cbor_encoder_close_container(&data, &array);
	cbor_encoder_close_container(&test_encoder, &data);
}

/*
 * object array
 */
TEST_CASE(test_cborattr_decode_obj_array)
{
	int rc;
	char arr_data[4];
	int arr_cnt;
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrArrayType,
			.addr.array.element_type = CborAttrNullType,
			.addr.array.arr.objects.base = arr_data,
			.addr.array.count = &arr_cnt,
			.addr.array.maxlen = 4,
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};

	test_encode_obj_array();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(arr_cnt == 2);
}
