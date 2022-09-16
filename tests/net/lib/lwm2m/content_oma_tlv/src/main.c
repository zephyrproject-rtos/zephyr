/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include "lwm2m_util.h"
#include "lwm2m_rw_oma_tlv.h"

#define TEST_RESOURCE_ID_SHORT 0xAA
#define TEST_RESOURCE_ID_LONG 0xAABB

#define TEST_TLV_RES_TYPE_ID_0_LEN_0 0xC0
#define TEST_TLV_RES_TYPE_ID_1_LEN_0 0xE0
#define TEST_TLV_RES_TYPE_ID_0_LEN_1 0xC8
#define TEST_TLV_RES_TYPE_ID_1_LEN_1 0xE8

#define TEST_MAX_PAYLOAD_BUFFER_LENGTH 16

struct test_payload_buffer {
	uint8_t data[TEST_MAX_PAYLOAD_BUFFER_LENGTH];
	size_t len;
};

static struct lwm2m_output_context test_out;
static struct lwm2m_input_context test_in;
static struct lwm2m_obj_path test_path;
static struct coap_packet test_packet;
static uint8_t test_payload[128];
static struct tlv_out_formatter_data test_formatter_data;

static void context_reset(void)
{
	memset(&test_out, 0, sizeof(test_out));
	test_out.writer = &oma_tlv_writer;
	test_out.out_cpkt = &test_packet;
	test_out.user_data = &test_formatter_data;

	memset(&test_formatter_data, 0, sizeof(test_formatter_data));

	memset(&test_in, 0, sizeof(test_in));
	test_in.reader = &oma_tlv_reader;
	test_in.in_cpkt = &test_packet;

	memset(&test_payload, 0, sizeof(test_payload));
	memset(&test_packet, 0, sizeof(test_packet));
	test_packet.data = test_payload;
	test_packet.max_len = sizeof(test_payload);

	memset(&test_path, 0, sizeof(test_path));
	test_path.level = LWM2M_PATH_LEVEL_RESOURCE;
}

static void test_prepare(void)
{
	context_reset();
}

static void test_prepare_nomem(void)
{
	context_reset();

	test_packet.offset = sizeof(test_payload);
}

static void test_prepare_nodata(void)
{
	context_reset();

	test_packet.offset = sizeof(test_payload);
	test_in.offset = sizeof(test_payload);
}

static void test_payload_set(const uint8_t *payload, size_t len)
{
	memcpy(test_payload + 1, payload, len);
	test_packet.offset = len + 1;
	test_in.offset = 1; /* Payload marker */
}

static void test_put_s8(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	int8_t value[] = { 0, INT8_MAX, INT8_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				0
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				INT8_MAX
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x01,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				INT8_MIN
			},
			.len = 4
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_s8(&test_out, &test_path, value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s8_nomem(void)
{
	int ret;

	ret = oma_tlv_writer.put_s8(&test_out, &test_path, INT8_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s16(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	int16_t value[] = { 0, INT16_MAX, INT16_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				0
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x02,
				TEST_RESOURCE_ID_SHORT,
				(uint8_t)(INT16_MAX >> 8),
				(uint8_t)INT16_MAX
			},
			.len = 4
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x02,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				(uint8_t)(INT16_MIN >> 8),
				(uint8_t)INT16_MIN
			},
			.len = 5
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_s16(&test_out, &test_path, value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s16_nomem(void)
{
	int ret;

	ret = oma_tlv_writer.put_s16(&test_out, &test_path, INT16_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s32(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	int32_t value[] = { 0, INT32_MAX, INT32_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				0
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				(uint8_t)(INT32_MAX >> 24),
				(uint8_t)(INT32_MAX >> 16),
				(uint8_t)(INT32_MAX >> 8),
				(uint8_t)INT32_MAX
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x04,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				(uint8_t)(INT32_MIN >> 24),
				(uint8_t)(INT32_MIN >> 16),
				(uint8_t)(INT32_MIN >> 8),
				(uint8_t)INT32_MIN
			},
			.len = 7
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_s32(&test_out, &test_path, value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s32_nomem(void)
{
	int ret;

	ret = oma_tlv_writer.put_s32(&test_out, &test_path, INT32_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s64(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	int64_t value[] = { 0, INT64_MAX, INT64_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				0
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof(int64_t),
				(uint8_t)(INT64_MAX >> 56),
				(uint8_t)(INT64_MAX >> 48),
				(uint8_t)(INT64_MAX >> 40),
				(uint8_t)(INT64_MAX >> 32),
				(uint8_t)(INT64_MAX >> 24),
				(uint8_t)(INT64_MAX >> 16),
				(uint8_t)(INT64_MAX >> 8),
				(uint8_t)INT64_MAX
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_1,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				sizeof(int64_t),
				(uint8_t)(INT64_MIN >> 56),
				(uint8_t)(INT64_MIN >> 48),
				(uint8_t)(INT64_MIN >> 40),
				(uint8_t)(INT64_MIN >> 32),
				(uint8_t)(INT64_MIN >> 24),
				(uint8_t)(INT64_MIN >> 16),
				(uint8_t)(INT64_MIN >> 8),
				(uint8_t)INT64_MIN
			},
			.len = 12
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_s64(&test_out, &test_path, value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s64_nomem(void)
{
	int ret;

	ret = oma_tlv_writer.put_s64(&test_out, &test_path, INT64_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_string(void)
{
	int ret;
	const char *test_string = "test_string";
	struct test_payload_buffer expected_payload = {
		.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof("test_string") - 1,
				't', 'e', 's', 't', '_', 's',
				't', 'r', 'i', 'n', 'g'
		},
		.len = sizeof("test_string") - 1 + 3
	};

	test_path.res_id = TEST_RESOURCE_ID_SHORT;

	ret = oma_tlv_writer.put_string(&test_out, &test_path, (char *)test_string,
					strlen(test_string));
	zassert_equal(ret, expected_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload.data,
			  expected_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, expected_payload.len,
		      "Invalid packet offset");
}

static void test_put_string_nomem(void)
{
	int ret;
	const char *test_string = "test_string";

	ret = oma_tlv_writer.put_string(&test_out, &test_path,
					   (char *)test_string,
					   strlen(test_string));
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

#define DOUBLE_CMP_EPSILON 0.000000001

static void test_put_float(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	double readback_value;
	double value[] = { 0., 0.123, -0.987, 3., -10., 2.333, -123.125 };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0, 0, 0, 0, 0, 0, 0, 0
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0x3F, 0xBF, 0x7C, 0xED, 0x91, 0x68, 0x72, 0xB0
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0xBF, 0xEF, 0x95, 0x81, 0x06, 0x24, 0xDD, 0x2F
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0xC0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0x40, 0x02, 0xA9, 0xFB, 0xE7, 0x6C, 0x8B, 0x44
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_1,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				8,
				0xC0, 0x5E, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 12
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_float(&test_out, &test_path, &value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		/* Ignore encoded least significant byte - it may differ slightly
		 * on various platform due to float rounding.
		 */
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len - 1,
				  "Invalid payload format");
		offset += expected_payload[i].len;

		/* Parse double back and compare it with the original one with
		 * a little error margin
		 */
		(void)lwm2m_b64_to_float(test_out.out_cpkt->data + offset - 8, 8,
					 &readback_value);
		zassert_true((readback_value > value[i] - DOUBLE_CMP_EPSILON) &&
			     (readback_value < value[i] + DOUBLE_CMP_EPSILON),
			     "Invalid value encoded");

		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_float_nomem(void)
{
	int ret;
	double value = 1.2;

	ret = oma_tlv_writer.put_float(&test_out, &test_path, &value);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_bool(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	bool value[] = { true, false };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				1
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x01,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				0
			},
			.len = 4
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_bool(&test_out, &test_path, value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_bool_nomem(void)
{
	int ret;

	ret = oma_tlv_writer.put_bool(&test_out, &test_path, true);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_opaque(void)
{
	int ret;
	const char *test_opaque = "test_opaque";
	struct test_payload_buffer expected_payload = {
		.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof("test_opaque") - 1,
				't', 'e', 's', 't', '_', 'o',
				'p', 'a', 'q', 'u', 'e',
		},
		.len = sizeof("test_opaque") - 1 + 3
	};

	test_path.res_id = TEST_RESOURCE_ID_SHORT;

	ret = oma_tlv_writer.put_opaque(&test_out, &test_path, (char *)test_opaque,
					strlen(test_opaque));
	zassert_equal(ret, expected_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload.data,
			  expected_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, expected_payload.len,
		      "Invalid packet offset");
}

static void test_put_opaque_nomem(void)
{
	int ret;
	const char *test_opaque = "test_opaque";

	ret = oma_tlv_writer.put_string(&test_out, &test_path,
					   (char *)test_opaque,
					   strlen(test_opaque));
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_objlnk(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	uint16_t res_id[] = {
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_SHORT,
		TEST_RESOURCE_ID_LONG
	};
	struct lwm2m_objlnk value[] = {
		{ 0, 0 }, { 1, 2 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				0, 0, 0, 0
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				0, 1, 0, 2
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x04,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				0xFF, 0xFF, 0xFF, 0xFF
			},
			.len = 7
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_path.res_id = res_id[i];

		ret = oma_tlv_writer.put_objlnk(&test_out, &test_path, &value[i]);
		zassert_equal(ret, expected_payload[i].len,
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_objlnk_nomem(void)
{
	int ret;
	struct lwm2m_objlnk value = { 0, 0 };

	ret = oma_tlv_writer.put_objlnk(&test_out, &test_path, &value);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_get_s32(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				0, 0, 0, 0
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				(uint8_t)(INT32_MAX >> 24),
				(uint8_t)(INT32_MAX >> 16),
				(uint8_t)(INT32_MAX >> 8),
				(uint8_t)INT32_MAX
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x04,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				(uint8_t)(INT32_MIN >> 24),
				(uint8_t)(INT32_MIN >> 16),
				(uint8_t)(INT32_MIN >> 8),
				(uint8_t)INT32_MIN
			},
			.len = 7
		}
	};
	int32_t expected_value[] = { 0, INT32_MAX, INT32_MIN };
	int32_t value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = oma_tlv_reader.get_s32(&test_in, &value);
		zassert_equal(ret, payload[i].len, "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_s32_nodata(void)
{
	int ret;
	int32_t value;

	ret = oma_tlv_reader.get_s32(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

static void test_get_s64(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof(int64_t),
				0, 0, 0, 0, 0, 0, 0, 0
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof(int64_t),
				(uint8_t)(INT64_MAX >> 56),
				(uint8_t)(INT64_MAX >> 48),
				(uint8_t)(INT64_MAX >> 40),
				(uint8_t)(INT64_MAX >> 32),
				(uint8_t)(INT64_MAX >> 24),
				(uint8_t)(INT64_MAX >> 16),
				(uint8_t)(INT64_MAX >> 8),
				(uint8_t)INT64_MAX
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_1,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				sizeof(int64_t),
				(uint8_t)(INT64_MIN >> 56),
				(uint8_t)(INT64_MIN >> 48),
				(uint8_t)(INT64_MIN >> 40),
				(uint8_t)(INT64_MIN >> 32),
				(uint8_t)(INT64_MIN >> 24),
				(uint8_t)(INT64_MIN >> 16),
				(uint8_t)(INT64_MIN >> 8),
				(uint8_t)INT64_MIN
			},
			.len = 12
		}
	};
	int64_t expected_value[] = { 0, INT64_MAX, INT64_MIN };
	int64_t value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = oma_tlv_reader.get_s64(&test_in, &value);
		zassert_equal(ret, payload[i].len, "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_s64_nodata(void)
{
	int ret;
	int64_t value;

	ret = oma_tlv_reader.get_s64(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

static void test_get_string(void)
{
	int ret;
	const char *test_string = "test_string";
	uint8_t buf[16];
	struct test_payload_buffer payload = {
		.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof("test_string") - 1,
				't', 'e', 's', 't', '_', 's',
				't', 'r', 'i', 'n', 'g'
		},
		.len = sizeof("test_string") - 1 + 3
	};


	test_payload_set(payload.data, payload.len);

	ret = oma_tlv_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, payload.len, "Invalid length returned");
	zassert_mem_equal(buf, test_string, strlen(test_string),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, payload.len + 1,
		      "Invalid packet offset");
}

static void test_get_string_nodata(void)
{
	int ret;
	uint8_t buf[16];

	ret = oma_tlv_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

#define DOUBLE_CMP_EPSILON 0.000000001

static void test_get_float(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0, 0, 0, 0, 0, 0, 0, 0
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0x3F, 0xBF, 0x7C, 0xED, 0x91, 0x68, 0x72, 0xB0
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0xBF, 0xEF, 0x95, 0x81, 0x06, 0x24, 0xDD, 0x2F
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0xC0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				8,
				0x40, 0x02, 0xA9, 0xFB, 0xE7, 0x6C, 0x8B, 0x44
			},
			.len = 11
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_1,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				8,
				0xC0, 0x5E, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 12
		}
	};
	double expected_value[] = {
		0., 0.123, -0.987, 3., -10., 2.333, -123.125
	};
	double value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = oma_tlv_reader.get_float(&test_in, &value);
		zassert_equal(ret, payload[i].len,
			      "Invalid length returned");
		zassert_true((value > expected_value[i] - DOUBLE_CMP_EPSILON) &&
			     (value < expected_value[i] + DOUBLE_CMP_EPSILON),
			     "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_float_nodata(void)
{
	int ret;
	double value;

	ret = oma_tlv_reader.get_float(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

static void test_get_bool(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				1
			},
			.len = 3
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x01,
				TEST_RESOURCE_ID_SHORT,
				0
			},
			.len = 3
		}
	};
	bool expected_value[] = { true, false };
	bool value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = oma_tlv_reader.get_bool(&test_in, &value);
		zassert_equal(ret, payload[i].len,
			      "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_bool_nodata(void)
{
	int ret;
	bool value;

	ret = oma_tlv_reader.get_bool(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

static void test_get_opaque(void)
{
	int ret;
	const char *test_opaque = "test_opaque";
	struct test_payload_buffer payload = {
		.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_1,
				TEST_RESOURCE_ID_SHORT,
				sizeof("test_opaque") - 1,
				't', 'e', 's', 't', '_', 'o',
				'p', 'a', 'q', 'u', 'e',
		},
		.len = sizeof("test_opaque") - 1 + 3
	};
	uint8_t buf[16];
	bool last_block;
	struct lwm2m_opaque_context ctx = { 0 };

	test_payload_set(payload.data, payload.len);

	ret = oma_tlv_reader.get_opaque(&test_in, buf, sizeof(buf),
					&ctx, &last_block);
	zassert_equal(ret, strlen(test_opaque), "Invalid length returned");
	zassert_mem_equal(buf, test_opaque, strlen(test_opaque),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, payload.len + 1,
		      "Invalid packet offset");
}

static void test_get_opaque_nodata(void)
{
	int ret;
	uint8_t value[4];
	bool last_block;
	struct lwm2m_opaque_context ctx = { 0 };

	ret = oma_tlv_reader.get_opaque(&test_in, value, sizeof(value),
					&ctx, &last_block);
	zassert_equal(ret, -ENODATA, "Invalid error code returned %d");
}

static void test_get_objlnk(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				0, 0, 0, 0
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_0_LEN_0 | 0x04,
				TEST_RESOURCE_ID_SHORT,
				0, 1, 0, 2
			},
			.len = 6
		},
		{
			.data = {
				TEST_TLV_RES_TYPE_ID_1_LEN_0 | 0x04,
				(uint8_t)(TEST_RESOURCE_ID_LONG >> 8),
				(uint8_t)TEST_RESOURCE_ID_LONG,
				0xFF, 0xFF, 0xFF, 0xFF
			},
			.len = 7
		}
	};
	struct lwm2m_objlnk expected_value[] = {
		{ 0, 0 }, { 1, 2 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	struct lwm2m_objlnk value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = oma_tlv_reader.get_objlnk(&test_in, &value);
		zassert_equal(ret, payload[i].len, "Invalid length returned");
		zassert_mem_equal(&value, &expected_value[i],
				  sizeof(struct lwm2m_objlnk),
				  "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_objlnk_nodata(void)
{
	int ret;
	struct lwm2m_objlnk value;

	ret = oma_tlv_reader.get_objlnk(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

void test_main(void)
{
	ztest_test_suite(
		lwm2m_content_plain_text,
		ztest_unit_test_setup_teardown(
			test_put_s8, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s8_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s16, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s16_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s32, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s32_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s64, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_s64_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_string, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_string_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_float, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_float_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_bool, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_bool_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_opaque, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_opaque_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_objlnk, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_objlnk_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_s32, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_s32_nodata, test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_s64, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_s64_nodata, test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_string, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_string_nodata, test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_float, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_float_nodata, test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_bool, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_bool_nodata, test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_opaque, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_opaque_nodata, test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_objlnk, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_objlnk_nodata, test_prepare_nodata, unit_test_noop)
	);

	ztest_run_test_suite(lwm2m_content_plain_text);
}
