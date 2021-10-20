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
test_encode_object_array(void)
{
	CborEncoder data;
	CborEncoder array;
	CborEncoder sub_obj;

	test_cbor_len = 0;
	cbor_encoder_init(&test_encoder, &test_writer, 0);

	cbor_encoder_create_map(&test_encoder, &data, CborIndefiniteLength);

	/*
	 * a: [ { h:"str1"}, {h:"2str"}, {h:"str3"}]
	 */
	cbor_encode_text_stringz(&data, "a");

	cbor_encoder_create_array(&data, &array, CborIndefiniteLength);

	cbor_encoder_create_map(&array, &sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&sub_obj, "h");
	cbor_encode_text_stringz(&sub_obj, "str1");
	cbor_encoder_close_container(&array, &sub_obj);

	cbor_encoder_create_map(&array, &sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&sub_obj, "h");
	cbor_encode_text_stringz(&sub_obj, "2str");
	cbor_encoder_close_container(&array, &sub_obj);

	cbor_encoder_create_map(&array, &sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&sub_obj, "h");
	cbor_encode_text_stringz(&sub_obj, "str3");
	cbor_encoder_close_container(&array, &sub_obj);

	cbor_encoder_close_container(&data, &array);

	cbor_encoder_close_container(&test_encoder, &data);
}

/*
 * object array
 */
TEST_CASE(test_cborattr_decode_object_array)
{
	int rc;
	struct h_obj {
		char h_data[32];
	} arr_objs[5];
	int arr_cnt = 0;
	struct cbor_attr_t sub_attr[] = {
		[0] = {
			.attribute = "h",
			.type = CborAttrTextStringType,
			CBORATTR_STRUCT_OBJECT(struct h_obj, h_data),
			.len = sizeof(arr_objs[0].h_data)
		},
		[1] = {
			.attribute = NULL
		}
	};
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "a",
			.type = CborAttrArrayType,
			CBORATTR_STRUCT_ARRAY(arr_objs, sub_attr, &arr_cnt),
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};

	test_encode_object_array();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(arr_cnt == 3);
	TEST_ASSERT(!strcmp(arr_objs[0].h_data, "str1"));
	TEST_ASSERT(!strcmp(arr_objs[1].h_data, "2str"));
	TEST_ASSERT(!strcmp(arr_objs[2].h_data, "str3"));
}
