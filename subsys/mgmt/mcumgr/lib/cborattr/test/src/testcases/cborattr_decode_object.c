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
	CborEncoder sub_obj;

	test_cbor_len = 0;
	cbor_encoder_init(&test_encoder, &test_writer, 0);
	cbor_encoder_create_map(&test_encoder, &test_data, CborIndefiniteLength);

	/*
	 * p: { bm : 7 }
	 */
	cbor_encode_text_stringz(&test_data, "p");

	cbor_encoder_create_map(&test_data, &sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&test_data, "bm");
	cbor_encode_int(&test_data, 7);
	cbor_encoder_close_container(&test_data, &sub_obj);

	cbor_encoder_close_container(&test_encoder, &test_data);
}

static void
test_encode_data_complex(void)
{
	CborEncoder test_data;
	CborEncoder sub_obj;
	CborEncoder sub_sub_obj;

	test_cbor_len = 0;

	cbor_encoder_init(&test_encoder, &test_writer, 0);
	cbor_encoder_create_map(&test_encoder, &test_data, CborIndefiniteLength);

	/*
	 * p: { bm : 7 }, c: { d : { i : 1 } }, a: 3
	 */
	cbor_encode_text_stringz(&test_data, "p");

	cbor_encoder_create_map(&test_data, &sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&test_data, "bm");
	cbor_encode_int(&test_data, 7);
	cbor_encoder_close_container(&test_data, &sub_obj);

	cbor_encode_text_stringz(&test_data, "c");
	cbor_encoder_create_map(&test_data, &sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&sub_obj, "d");

	cbor_encoder_create_map(&sub_obj, &sub_sub_obj, CborIndefiniteLength);
	cbor_encode_text_stringz(&sub_sub_obj, "i");
	cbor_encode_int(&sub_sub_obj, 1);
	cbor_encoder_close_container(&sub_obj, &sub_sub_obj);
	cbor_encoder_close_container(&test_data, &sub_obj);

	cbor_encode_text_stringz(&test_data, "a");
	cbor_encode_int(&test_data, 3);

	cbor_encoder_close_container(&test_encoder, &test_data);
}

/*
 * Simple decoding.
 */
TEST_CASE(test_cborattr_decode_object)
{
	int rc;
	int64_t bm_val = 0;
	struct cbor_attr_t test_sub_attr_bm[] = {
		[0] = {
			.attribute = "bm",
			.type = CborAttrIntegerType,
			.addr.integer = &bm_val,
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};
	struct cbor_attr_t test_attrs[] = {
		[0] = {
			.attribute = "p",
			.type = CborAttrObjectType,
			.addr.obj = test_sub_attr_bm,
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};
	int64_t a_val = 0;
	int64_t i_val = 0;
	struct cbor_attr_t test_sub_sub_attr[] = {
		[0] = {
			.attribute = "i",
			.type = CborAttrIntegerType,
			.addr.integer = &i_val,
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};
	struct cbor_attr_t test_sub_attr_d[] = {
		[0] = {
			.attribute = "d",
			.type = CborAttrObjectType,
			.addr.obj = test_sub_sub_attr,
			.nodefault = true
		},
		[1] = {
			.attribute = NULL
		}
	};
	struct cbor_attr_t test_attr_complex[] = {
		[0] = {
			.attribute = "c",
			.type = CborAttrObjectType,
			.addr.obj = test_sub_attr_d,
			.nodefault = true
		},
		[1] = {
			.attribute = "a",
			.type = CborAttrIntegerType,
			.addr.integer = &a_val,
			.nodefault = true
		},
		[2] = {
			.attribute = "p",
			.type = CborAttrObjectType,
			.addr.obj = test_sub_attr_bm,
			.nodefault = true
		},
		[3] = {
			.attribute = NULL
		}
	};

	test_encode_data();

	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attrs);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(bm_val == 7);

	test_encode_data_complex();

	bm_val = 0;
	i_val = 0;
	rc = cbor_read_flat_attrs(test_cbor_buf, test_cbor_len, test_attr_complex);
	TEST_ASSERT(rc == 0);
	TEST_ASSERT(bm_val == 7);
	TEST_ASSERT(i_val == 1);
}
