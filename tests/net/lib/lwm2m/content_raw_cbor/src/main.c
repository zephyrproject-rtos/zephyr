/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "lwm2m_util.h"
#include "lwm2m_rw_cbor.h"

#define TEST_MAX_PAYLOAD_BUFFER_LENGTH 40

struct test_payload_buffer {
	uint8_t data[TEST_MAX_PAYLOAD_BUFFER_LENGTH];
	size_t len;
};

static struct lwm2m_output_context test_out;
static struct lwm2m_input_context test_in;
static struct lwm2m_obj_path test_path;
static struct coap_packet test_packet;
static uint8_t test_payload[128];

static void context_reset(void)
{
	memset(&test_out, 0, sizeof(test_out));
	test_out.writer = &cbor_writer;
	test_out.out_cpkt = &test_packet;

	memset(&test_in, 0, sizeof(test_in));
	test_in.reader = &cbor_reader;
	test_in.in_cpkt = &test_packet;

	memset(&test_payload, 0, sizeof(test_payload));
	memset(&test_packet, 0, sizeof(test_packet));
	test_packet.data = test_payload;
	test_packet.max_len = sizeof(test_payload);
}

static void test_prepare(void *dummy)
{
	ARG_UNUSED(dummy);
	context_reset();
}

static void test_prepare_nomem(void *dummy)
{
	ARG_UNUSED(dummy);

	context_reset();

	test_packet.offset = sizeof(test_payload);
}

static void test_prepare_nodata(void *dummy)
{
	ARG_UNUSED(dummy);

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

ZTEST(net_content_raw_cbor, test_put_s8)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int8_t value[] = { 1, -1, INT8_MAX, INT8_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x00 << 5) | 0x01
			},
			.len = 1
		},
		{
			.data = {
				(0x01 << 5) | 0x00
			},
			.len = 1
		},
		{
			.data = {
				(0x00 << 5) | 0x18,
				0x7f
			},
			.len = 2
		},
		{
			.data = {
				(0x01 << 5) | 0x18,
				0x7f
			},
			.len = 2
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = cbor_writer.put_s8(&test_out, &test_path, value[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_s8_nomem)
{
	int ret;

	ret = cbor_writer.put_s8(&test_out, &test_path, INT8_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_s16)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int16_t value[] = { 1, -1, INT16_MAX, INT16_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x00 << 5) | 0x01
			},
			.len = 1
		},
		{
			.data = {
				(0x01 << 5) | 0x00
			},
			.len = 1
		},
		{
			.data = {
				(0x00 << 5) | 0x19,
				0x7f, 0xff
			},
			.len = 3
		},
		{
			.data = {
				(0x01 << 5) | 0x19,
				0x7f, 0xff
			},
			.len = 3
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = cbor_writer.put_s16(&test_out, &test_path, value[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_s16_nomem)
{
	int ret;

	ret = cbor_writer.put_s16(&test_out, &test_path, INT16_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_s32)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int32_t value[] = { 1, -1, INT32_MAX, INT32_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x00 << 5) | 0x01
			},
			.len = 1
		},
		{
			.data = {
				(0x01 << 5) | 0x00
			},
			.len = 1
		},
		{
			.data = {
				(0x00 << 5) | 0x1a,
				0x7f, 0xff, 0xff, 0xff
			},
			.len = 5
		},
		{
			.data = {
				(0x01 << 5) | 0x1a,
				0x7f, 0xff, 0xff, 0xff
			},
			.len = 5
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = cbor_writer.put_s32(&test_out, &test_path, value[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_s32_nomem)
{
	int ret;

	ret = cbor_writer.put_s32(&test_out, &test_path, INT32_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_s64)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int64_t value[] = { 1, -1, INT64_MAX, INT64_MIN };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x00 << 5) | 0x01
			},
			.len = 1
		},
		{
			.data = {
				(0x01 << 5) | 0x00
			},
			.len = 1
		},
		{
			.data = {
				(0x00 << 5) | 0x1b,
				0x7f, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff
			},
			.len = 9
		},
		{
			.data = {
				(0x01 << 5) | 0x1b,
				0x7f, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff
			},
			.len = 9
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = cbor_writer.put_s64(&test_out, &test_path, value[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_s64_nomem)
{
	int ret;

	ret = cbor_writer.put_s64(&test_out, &test_path, INT64_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

#define DOUBLE_CMP_EPSILON 0.000000001

ZTEST(net_content_raw_cbor, test_put_float)
{
	int ret;
	int i;
	uint16_t offset = 0;

	double readback_value;
	double value[] = { 0.123, -0.987, 3., -10., 2.333, -123.125 };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0x3F, 0xBF, 0x7C, 0xED, 0x91, 0x68, 0x72, 0xB0
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0xBF, 0xEF, 0x95, 0x81, 0x06, 0x24, 0xDD, 0x2F
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0xC0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0x40, 0x02, 0xA9, 0xFB, 0xE7, 0x6C, 0x8B, 0x44
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0xC0, 0x5E, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 9
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {

		ret = cbor_writer.put_float(&test_out, &test_path, &value[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_float_nomem)
{
	int ret;
	double value = 1.2;

	ret = cbor_writer.put_float(&test_out, &test_path, &value);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_string)
{
	int ret;
	const char *test_string = "test_string";
	const char *long_string = "test_string_that_is_a_bit_longer";
	struct test_payload_buffer expected_payload = {
		.data = {
			(0x03 << 5) | (sizeof("test_string") - 1),
			't', 'e', 's', 't', '_', 's',
			't', 'r', 'i', 'n', 'g'
		},
		.len = sizeof("test_string") - 1 + 1
	};
	struct test_payload_buffer long_payload = {
		.data = {
			(0x03 << 5) | 0x18,
			(sizeof("test_string_that_is_a_bit_longer") - 1),
			't', 'e', 's', 't', '_', 's',
			't', 'r', 'i', 'n', 'g', '_',
			't', 'h', 'a', 't', '_', 'i',
			's', '_', 'a', '_', 'b', 'i',
			't', '_', 'l', 'o', 'n', 'g',
			'e', 'r'
		},
		.len = sizeof("test_string_that_is_a_bit_longer") - 1 + 2
	};

	ret = cbor_writer.put_string(&test_out, &test_path,
			  (char *)test_string, strlen(test_string));
	zassert_equal(ret, expected_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload.data,
			  expected_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, expected_payload.len,
			  "Invalid packet offset");

	uint16_t offset = expected_payload.len;

	ret = cbor_writer.put_string(&test_out, &test_path,
			  (char *)long_string, strlen(long_string));
	zassert_equal(ret, long_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data + offset, long_payload.data,
			  long_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, long_payload.len + offset,
			  "Invalid packet offset");
}

ZTEST(net_content_raw_cbor_nomem, test_put_string_nomem)
{
	int ret;
	const char *test_string = "test_string";

	ret = cbor_writer.put_string(&test_out, &test_path,
					   (char *)test_string,
					   strlen(test_string));
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_time)
{
	int ret;
	int64_t timestamp = 1;
	struct test_payload_buffer expected_payload = {
		.data = {
			(0x06 << 5) | 0x00,
			(0x03 << 5) | 0x18,
			sizeof("1970-01-01T00:00:01-00:00") - 1,
			'1', '9', '7', '0', '-', '0', '1', '-',
			'0', '1', 'T', '0', '0', ':', '0', '0',
			':', '0', '1', '-', '0', '0', ':', '0', '0', '\0'
		},
		.len = sizeof("1970-01-01T00:00:01-00:00") + 2
	};

	ret = cbor_writer.put_time(&test_out, &test_path, timestamp);
	zassert_equal(ret, expected_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload.data,
			  expected_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, expected_payload.len,
			  "Invalid packet offset");
}

ZTEST(net_content_raw_cbor_nomem, test_put_time_nomem)
{
	int ret;

	ret = cbor_writer.put_time(&test_out, &test_path, UINT64_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_bool)
{
	int ret;
	int i;
	uint16_t offset = 0;
	bool value[] = { true, false };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x07 << 5) | 0x15
			},
			.len = 1
		},
		{
			.data = {
				(0x07 << 5) | 0x14
			},
			.len = 1
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = cbor_writer.put_bool(&test_out, &test_path, value[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_bool_nomem)
{
	int ret;

	ret = cbor_writer.put_bool(&test_out, &test_path, true);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_opaque)
{
	int ret;
	const char *test_opaque = "test_opaque";
	const char *long_opaque = "test_opaque_that_is_a_bit_longer";
	struct test_payload_buffer expected_payload = {
		.data = {
			(0x02 << 5) | (sizeof("test_opaque") - 1),
			't', 'e', 's', 't', '_', 'o',
			'p', 'a', 'q', 'u', 'e'
		},
		.len = sizeof("test_opaque") - 1 + 1
	};
	struct test_payload_buffer long_payload = {
		.data = {
			(0x02 << 5) | 0x18,
			(sizeof("test_opaque_that_is_a_bit_longer") - 1),
			't', 'e', 's', 't', '_', 'o',
			'p', 'a', 'q', 'u', 'e', '_',
			't', 'h', 'a', 't', '_', 'i',
			's', '_', 'a', '_', 'b', 'i',
			't', '_', 'l', 'o', 'n', 'g',
			'e', 'r'
		},
		.len = sizeof("test_opaque_that_is_a_bit_longer") - 1 + 2
	};

	ret = cbor_writer.put_opaque(&test_out, &test_path,
			  (char *)test_opaque, strlen(test_opaque));
	zassert_equal(ret, expected_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload.data,
			  expected_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, expected_payload.len,
			  "Invalid packet offset");

	uint16_t offset = expected_payload.len;

	ret = cbor_writer.put_opaque(&test_out, &test_path,
			  (char *)long_opaque, strlen(long_opaque));
	zassert_equal(ret, long_payload.len, "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data + offset, long_payload.data,
			  long_payload.len, "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, long_payload.len + offset,
			  "Invalid packet offset");
}

ZTEST(net_content_raw_cbor_nomem, test_put_opaque_nomem)
{
	int ret;
	const char *test_opaque = "test_opaque";

	ret = cbor_writer.put_opaque(&test_out, &test_path,
					   (char *)test_opaque,
					   strlen(test_opaque));
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_put_objlnk)
{
	int ret;
	int i;
	uint16_t offset = 0;
	struct lwm2m_objlnk values[] = {
		{ 0, 0 }, { 1, 2 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID}
	};
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
			(0x03 << 5) | (sizeof("0:0")),
			'0', ':', '0', '\0'
			},
			.len = sizeof("0:0") + 1
		},
		{
			.data = {
			(0x03 << 5) | (sizeof("1:2")),
			'1', ':', '2', '\0'
			},
			.len = sizeof("1:2") + 1
		},
		{
			.data = {
			(0x03 << 5) | (sizeof("65535:65535")),
			'6', '5', '5', '3', '5', ':',
			'6', '5', '5', '3', '5', '\0'
			},
			.len = sizeof("65535:65535") + 1
		}
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = cbor_writer.put_objlnk(&test_out, &test_path, &values[i]);
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

ZTEST(net_content_raw_cbor_nomem, test_put_objlnk_nomem)
{
	int ret;
	struct lwm2m_objlnk value = { 0, 0 };

	ret = cbor_writer.put_objlnk(&test_out, &test_path, &value);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_get_s32)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x00 << 5) | 0x01
			},
			.len = 1
		},
		{
			.data = {
				(0x01 << 5) | 0x00
			},
			.len = 1
		},
		{
			.data = {
				(0x00 << 5) | 0x1a,
				0x7f, 0xff, 0xff, 0xff
			},
			.len = 5
		},
		{
			.data = {
				(0x01 << 5) | 0x1a,
				0x7f, 0xff, 0xff, 0xff
			},
			.len = 5
		}
	};
	int32_t expected_value[] = { 1, -1, INT32_MAX, INT32_MIN };
	int32_t value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = cbor_reader.get_s32(&test_in, &value);
		zassert_equal(ret, payload[i].len, "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
				  "Invalid packet offset");
	}
}

ZTEST(net_content_raw_cbor_nodata, test_get_s32_nodata)
{
	int ret;
	int32_t value;

	ret = cbor_reader.get_s32(&test_in, &value);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_get_s64)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x00 << 5) | 0x01
			},
			.len = 1
		},
		{
			.data = {
				(0x01 << 5) | 0x00
			},
			.len = 1
		},
		{
			.data = {
				(0x00 << 5) | 0x1b,
				0x7f, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff
			},
			.len = 9
		},
		{
			.data = {
				(0x01 << 5) | 0x1b,
				0x7f, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff
			},
			.len = 9
		}
	};
	int64_t expected_value[] = { 1, -1, INT64_MAX, INT64_MIN };
	int64_t value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = cbor_reader.get_s64(&test_in, &value);
		zassert_equal(ret, payload[i].len, "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
				  "Invalid packet offset");
	}
}

ZTEST(net_content_raw_cbor_nodata, test_get_s64_nodata)
{
	int ret;
	int64_t value;

	ret = cbor_reader.get_s64(&test_in, &value);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

#define DOUBLE_CMP_EPSILON 0.000000001

ZTEST(net_content_raw_cbor, test_get_float)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0, 0, 0, 0, 0, 0, 0, 0
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0x3F, 0xBF, 0x7C, 0xED, 0x91, 0x68, 0x72, 0xB0
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0xBF, 0xEF, 0x95, 0x81, 0x06, 0x24, 0xDD, 0x2F
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0xC0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0x40, 0x02, 0xA9, 0xFB, 0xE7, 0x6C, 0x8B, 0x44
			},
			.len = 9
		},
		{
			.data = {
				(0x07 << 5) | 0x1b,
				0xC0, 0x5E, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 9
		}
	};
	double expected_value[] = {
		0., 0.123, -0.987, 3., -10., 2.333, -123.125
	};
	double value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = cbor_reader.get_float(&test_in, &value);
		zassert_equal(ret, payload[i].len,
				  "Invalid length returned");
		zassert_true((value > expected_value[i] - DOUBLE_CMP_EPSILON) &&
				 (value < expected_value[i] + DOUBLE_CMP_EPSILON),
				 "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
				  "Invalid packet offset");
	}
}

ZTEST(net_content_raw_cbor_nodata, test_get_float_nodata)
{
	int ret;
	double value;

	ret = cbor_reader.get_float(&test_in, &value);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_get_string)
{
	int ret;
	const char *short_string = "test_string";
	const char *long_string = "test_string_that_is_a_bit_longer";
	uint8_t buf[40];
	struct test_payload_buffer short_payload = {
		.data = {
			(0x03 << 5) | (sizeof("test_string") - 1),
			't', 'e', 's', 't', '_', 's',
			't', 'r', 'i', 'n', 'g'
		},
		.len = sizeof("test_string") - 1 + 1
	};
	struct test_payload_buffer long_payload = {
		.data = {
			(0x03 << 5) | 0x18,
			(sizeof("test_string_that_is_a_bit_longer") - 1),
			't', 'e', 's', 't', '_', 's',
			't', 'r', 'i', 'n', 'g', '_',
			't', 'h', 'a', 't', '_', 'i',
			's', '_', 'a', '_', 'b', 'i',
			't', '_', 'l', 'o', 'n', 'g',
			'e', 'r'
		},
		.len = sizeof("test_string_that_is_a_bit_longer") - 1 + 2
	};

	test_payload_set(short_payload.data, short_payload.len);

	ret = cbor_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, short_payload.len, "Invalid length returned");
	zassert_mem_equal(buf, short_string, strlen(short_string),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, short_payload.len + 1,
			  "Invalid packet offset");

	test_payload_set(long_payload.data, long_payload.len);

	ret = cbor_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, long_payload.len, "Invalid length returned");
	zassert_mem_equal(buf, long_string, strlen(long_string),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, long_payload.len + 1,
			  "Invalid packet offset");
}

ZTEST(net_content_raw_cbor_nodata, test_get_string_nodata)
{
	int ret;
	uint8_t buf[16];

	ret = cbor_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_get_bool)
{
	int ret;
	int i;
	bool expected_value[] = { true, false };
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x07 << 5) | 0x15
			},
			.len = 1
		},
		{
			.data = {
				(0x07 << 5) | 0x14
			},
			.len = 1
		}
	};
	bool value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = cbor_reader.get_bool(&test_in, &value);
		zassert_equal(ret, payload[i].len,
				  "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
				  "Invalid packet offset");
	}
}

ZTEST(net_content_raw_cbor_nodata, test_get_bool_nodata)
{
	int ret;
	bool value;

	ret = cbor_reader.get_bool(&test_in, &value);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

ZTEST(net_content_raw_cbor, test_get_opaque)
{
	int ret;
	const char *short_opaque = "test_opaque";
	const char *long_opaque = "test_opaque_that_is_a_bit_longer";
	struct test_payload_buffer short_payload = {
		.data = {
			(0x02 << 5) | (sizeof("test_opaque") - 1),
			't', 'e', 's', 't', '_', 'o',
			'p', 'a', 'q', 'u', 'e'
		},
		.len = sizeof("test_opaque") - 1 + 1
	};
	struct test_payload_buffer long_payload = {
		.data = {
			(0x02 << 5) | 0x18,
			(sizeof("test_opaque_that_is_a_bit_longer") - 1),
			't', 'e', 's', 't', '_', 'o',
			'p', 'a', 'q', 'u', 'e', '_',
			't', 'h', 'a', 't', '_', 'i',
			's', '_', 'a', '_', 'b', 'i',
			't', '_', 'l', 'o', 'n', 'g',
			'e', 'r'
		},
		.len = sizeof("test_opaque_that_is_a_bit_longer") - 1 + 2
	};
	uint8_t buf[40];
	bool last_block;
	struct lwm2m_opaque_context ctx = { 0 };

	test_payload_set(short_payload.data, short_payload.len);
	ret = cbor_reader.get_opaque(&test_in, buf, sizeof(buf),
					&ctx, &last_block);
	zassert_equal(ret, strlen(short_opaque), "Invalid length returned");
	zassert_mem_equal(buf, short_opaque, strlen(short_opaque),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, short_payload.len + 1,
			  "Invalid packet offset");

	test_payload_set(long_payload.data, long_payload.len);
	ret = cbor_reader.get_opaque(&test_in, buf, sizeof(buf), &ctx, &last_block);
	zassert_equal(ret, strlen(long_opaque), "Invalid length returned");
	zassert_mem_equal(buf, long_opaque, strlen(long_opaque),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, long_payload.len + 1,
			  "Invalid packet offset");
}

ZTEST(net_content_raw_cbor_nodata, test_get_opaque_nodata)
{
	int ret;
	uint8_t value[4];
	bool last_block;
	struct lwm2m_opaque_context ctx = { 0 };

	ret = cbor_reader.get_opaque(&test_in, value, sizeof(value), &ctx, &last_block);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned %d");
}

ZTEST(net_content_raw_cbor, test_get_objlnk)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
			(0x03 << 5) | (sizeof("0:0")),
			'0', ':', '0', '\0'
			},
			.len = sizeof("0:0") + 1
		},
		{
			.data = {
			(0x03 << 5) | (sizeof("1:2")),
			'1', ':', '2', '\0'
			},
			.len = sizeof("1:2") + 1
		},
		{
			.data = {
			(0x03 << 5) | (sizeof("65535:65535")),
			'6', '5', '5', '3', '5', ':',
			'6', '5', '5', '3', '5', '\0'
			},
			.len = sizeof("65535:65535") + 1
		}
	};
	struct lwm2m_objlnk expected_value[] = {
		{ 0, 0 }, { 1, 2 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	struct lwm2m_objlnk value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i].data, payload[i].len);

		ret = cbor_reader.get_objlnk(&test_in, &value);
		zassert_equal(ret, payload[i].len, "Invalid length returned");
		zassert_mem_equal(&value, &expected_value[i],
				  sizeof(struct lwm2m_objlnk),
				  "Invalid value parsed");
		zassert_equal(test_in.offset, payload[i].len + 1,
				  "Invalid packet offset");
	}
}

ZTEST(net_content_raw_cbor_nodata, test_get_objlnk_nodata)
{
	int ret;
	struct lwm2m_objlnk value;

	ret = cbor_reader.get_objlnk(&test_in, &value);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

ZTEST_SUITE(net_content_raw_cbor, NULL, NULL, test_prepare, NULL, NULL);
ZTEST_SUITE(net_content_raw_cbor_nomem, NULL, NULL, test_prepare_nomem, NULL, NULL);
ZTEST_SUITE(net_content_raw_cbor_nodata, NULL, NULL, test_prepare_nodata, NULL, NULL);
