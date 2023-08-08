/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "lwm2m_rw_plain_text.h"

static struct lwm2m_output_context test_out;
static struct lwm2m_input_context test_in;
static struct lwm2m_obj_path test_path;
static struct coap_packet test_packet;
static uint8_t test_payload[128];

static void context_reset(void)
{
	memset(&test_out, 0, sizeof(test_out));
	test_out.writer = &plain_text_writer;
	test_out.out_cpkt = &test_packet;

	memset(&test_in, 0, sizeof(test_in));
	test_in.reader = &plain_text_reader;
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

static void test_payload_set(const char *payload)
{
	memcpy(test_payload + 1, payload, strlen(payload));
	test_packet.offset = strlen(payload) + 1;
	test_in.offset = 1; /* Payload marker */
}

ZTEST(net_content_plain_text, test_put_s8)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int8_t value[] = { 0, INT8_MAX, INT8_MIN };
	char * const expected_payload[] = { "0", "127", "-128" };

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_s8(&test_out, &test_path, value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_s8_nomem)
{
	int ret;

	ret = plain_text_writer.put_s8(&test_out, &test_path, INT8_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_s16)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int16_t value[] = { 0, INT16_MAX, INT16_MIN };
	char * const expected_payload[] = { "0", "32767", "-32768" };

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_s16(&test_out, &test_path, value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_s16_nomem)
{
	int ret;

	ret = plain_text_writer.put_s16(&test_out, &test_path, INT16_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_s32)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int32_t value[] = { 0, INT32_MAX, INT32_MIN };
	char * const expected_payload[] = { "0", "2147483647", "-2147483648" };

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_s32(&test_out, &test_path, value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_s32_nomem)
{
	int ret;

	ret = plain_text_writer.put_s32(&test_out, &test_path, INT32_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_s64)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int64_t value[] = { 0, INT64_MAX, INT64_MIN };
	char * const expected_payload[] = {
		"0", "9223372036854775807", "-9223372036854775808"
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_s64(&test_out, &test_path, value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_s64_nomem)
{
	int ret;

	ret = plain_text_writer.put_s64(&test_out, &test_path, INT64_MAX);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_string)
{
	int ret;
	const char *test_string = "test_string";

	ret = plain_text_writer.put_string(&test_out, &test_path,
					   (char *)test_string,
					   strlen(test_string));
	zassert_equal(ret, strlen(test_string), "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, test_string,
			  strlen(test_string),
			  "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, strlen(test_string),
		      "Invalid packet offset");
}

ZTEST(net_content_plain_text_nomem, test_put_string_nomem)
{
	int ret;
	const char *test_string = "test_string";

	ret = plain_text_writer.put_string(&test_out, &test_path,
					   (char *)test_string,
					   strlen(test_string));
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_float)
{
	int ret;
	int i;
	uint16_t offset = 0;
	double value[] = { 0., 0.123, -0.987, 3., -10., 2.333, -123.125 };
	char * const expected_payload[] = {
		"0.0", "0.123", "-0.987", "3.0", "-10.0", "2.333", "-123.125"
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_float(&test_out, &test_path, &value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_float_nomem)
{
	int ret;
	double value = 1.2;

	ret = plain_text_writer.put_float(&test_out, &test_path, &value);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_bool)
{
	int ret;
	int i;
	uint16_t offset = 0;
	bool value[] = { true, false };
	char * const expected_payload[] = { "1", "0" };

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_bool(&test_out, &test_path, value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_bool_nomem)
{
	int ret;

	ret = plain_text_writer.put_bool(&test_out, &test_path, true);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_put_objlnk)
{
	int ret;
	int i;
	uint16_t offset = 0;
	struct lwm2m_objlnk value[] = {
		{ 0, 0 }, { 1, 1 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	char * const expected_payload[] = {
		"0:0", "1:1", "65535:65535"
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		ret = plain_text_writer.put_objlnk(&test_out, &test_path, &value[i]);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_out.out_cpkt->offset, offset,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nomem, test_put_objlnk_nomem)
{
	int ret;
	struct lwm2m_objlnk value = { 0, 0 };

	ret = plain_text_writer.put_objlnk(&test_out, &test_path, &value);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_get_s32)
{
	int ret;
	int i;
	char * const payload[] = { "0", "2147483647", "-2147483648" };
	int32_t expected_value[] = { 0, INT32_MAX, INT32_MIN };
	int32_t value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = plain_text_reader.get_s32(&test_in, &value);
		zassert_equal(ret, strlen(payload[i]),
			      "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, strlen(payload[i]) + 1,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nodata, test_get_s32_nodata)
{
	int ret;
	int32_t value;

	ret = plain_text_reader.get_s32(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_get_s64)
{
	int ret;
	int i;
	char * const payload[] = {
		"0", "9223372036854775807", "-9223372036854775808"
	};
	int64_t expected_value[] = { 0, INT64_MAX, INT64_MIN };
	int64_t value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = plain_text_reader.get_s64(&test_in, &value);
		zassert_equal(ret, strlen(payload[i]),
			      "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, strlen(payload[i]) + 1,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nodata, test_get_s64_nodata)
{
	int ret;
	int64_t value;

	ret = plain_text_reader.get_s64(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_get_string)
{
	int ret;
	const char *test_string = "test_string";
	uint8_t buf[16];

	test_payload_set(test_string);

	ret = plain_text_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, strlen(test_string), "Invalid length returned");
	zassert_mem_equal(buf, test_string, strlen(test_string),
			  "Invalid value parsed");
	zassert_equal(test_in.offset, strlen(test_string) + 1,
		      "Invalid packet offset");
}

ZTEST(net_content_plain_text_nodata, test_get_string_nodata)
{
	int ret;
	uint8_t buf[16];

	/* 0 is fine in this case, there's no other way to indicate empty string */
	ret = plain_text_reader.get_string(&test_in, buf, sizeof(buf));
	zassert_equal(ret, 0, "Invalid error code returned");
	zassert_equal(strlen(buf), 0, "Invalid value parsed");
}

#define DOUBLE_CMP_EPSILON 0.000000001

ZTEST(net_content_plain_text, test_get_float)
{
	int ret;
	int i;
	char * const payload[] = {
		"0", "0.123", "-0.987", "3", "-10",  "2.333", "-123.125"
	};
	double expected_value[] = {
		0., 0.123, -0.987, 3., -10., 2.333, -123.125
	};
	double value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = plain_text_reader.get_float(&test_in, &value);
		zassert_equal(ret, strlen(payload[i]),
			      "Invalid length returned");
		zassert_true((value > expected_value[i] - DOUBLE_CMP_EPSILON) &&
			     (value < expected_value[i] + DOUBLE_CMP_EPSILON),
			     "Invalid value parsed");
		zassert_equal(test_in.offset, strlen(payload[i]) + 1,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nodata, test_get_float_nodata)
{
	int ret;
	double value;

	ret = plain_text_reader.get_float(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_get_bool)
{
	int ret;
	int i;
	char * const payload[] = { "1", "0" };
	bool expected_value[] = { true, false };
	bool value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = plain_text_reader.get_bool(&test_in, &value);
		zassert_equal(ret, strlen(payload[i]),
			      "Invalid length returned");
		zassert_equal(value, expected_value[i], "Invalid value parsed");
		zassert_equal(test_in.offset, strlen(payload[i]) + 1,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nodata, test_get_bool_nodata)
{
	int ret;
	bool value;

	ret = plain_text_reader.get_bool(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

ZTEST(net_content_plain_text, test_get_objlnk)
{
	int ret;
	int i;
	char * const payload[] = { "0:0", "1:1", "65535:65535" };
	struct lwm2m_objlnk expected_value[] = {
		{ 0, 0 }, { 1, 1 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	struct lwm2m_objlnk value;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = plain_text_reader.get_objlnk(&test_in, &value);
		zassert_equal(ret, strlen(payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(&value, &expected_value[i],
				  sizeof(struct lwm2m_objlnk),
				  "Invalid value parsed");
		zassert_equal(test_in.offset, strlen(payload[i]) + 1,
			      "Invalid packet offset");
	}
}

ZTEST(net_content_plain_text_nodata, test_get_objlnk_nodata)
{
	int ret;
	struct lwm2m_objlnk value;

	ret = plain_text_reader.get_objlnk(&test_in, &value);
	zassert_equal(ret, -ENODATA, "Invalid error code returned");
}

ZTEST_SUITE(net_content_plain_text, NULL, NULL, test_prepare, NULL, NULL);
ZTEST_SUITE(net_content_plain_text_nomem, NULL, NULL, test_prepare_nomem, NULL, NULL);
ZTEST_SUITE(net_content_plain_text_nodata, NULL, NULL, test_prepare_nodata, NULL, NULL);
