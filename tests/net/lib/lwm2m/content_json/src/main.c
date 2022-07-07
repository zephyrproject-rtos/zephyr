/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include "lwm2m_engine.h"
#include "lwm2m_rw_json.h"

#define TEST_OBJ_ID 0xFFFF
#define TEST_OBJ_INST_ID 0

#define TEST_RES_S8 0
#define TEST_RES_S16 1
#define TEST_RES_S32 2
#define TEST_RES_S64 3
#define TEST_RES_STRING 4
#define TEST_RES_FLOAT 5
#define TEST_RES_BOOL 6
#define TEST_RES_OBJLNK 7

#define TEST_OBJ_RES_MAX_ID 8

static struct lwm2m_engine_obj test_obj;

static struct lwm2m_engine_obj_field test_fields[] = {
	OBJ_FIELD_DATA(TEST_RES_S8, RW, S8),
	OBJ_FIELD_DATA(TEST_RES_S16, RW, S16),
	OBJ_FIELD_DATA(TEST_RES_S32, RW, S32),
	OBJ_FIELD_DATA(TEST_RES_S64, RW, S64),
	OBJ_FIELD_DATA(TEST_RES_STRING, RW, STRING),
	OBJ_FIELD_DATA(TEST_RES_FLOAT, RW, FLOAT),
	OBJ_FIELD_DATA(TEST_RES_BOOL, RW, BOOL),
	OBJ_FIELD_DATA(TEST_RES_OBJLNK, RW, OBJLNK),
};

static struct lwm2m_engine_obj_inst test_inst;
static struct lwm2m_engine_res test_res[TEST_OBJ_RES_MAX_ID];
static struct lwm2m_engine_res_inst test_res_inst[TEST_OBJ_RES_MAX_ID];

#define TEST_STRING_MAX_SIZE 16

static int8_t test_s8;
static int16_t test_s16;
static int32_t test_s32;
static int64_t test_s64;
static char test_string[TEST_STRING_MAX_SIZE];
static double test_float;
static bool test_bool;
static struct lwm2m_objlnk test_objlnk;

static struct lwm2m_engine_obj_inst *test_obj_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(test_res_inst, ARRAY_SIZE(test_res_inst));

	INIT_OBJ_RES_DATA(TEST_RES_S8, test_res, i, test_res_inst, j,
			  &test_s8, sizeof(test_s8));
	INIT_OBJ_RES_DATA(TEST_RES_S16, test_res, i, test_res_inst, j,
			  &test_s16, sizeof(test_s16));
	INIT_OBJ_RES_DATA(TEST_RES_S32, test_res, i, test_res_inst, j,
			  &test_s32, sizeof(test_s32));
	INIT_OBJ_RES_DATA(TEST_RES_S64, test_res, i, test_res_inst, j,
			  &test_s64, sizeof(test_s64));
	INIT_OBJ_RES_DATA(TEST_RES_STRING, test_res, i, test_res_inst, j,
			  &test_string, sizeof(test_string));
	INIT_OBJ_RES_DATA(TEST_RES_FLOAT, test_res, i, test_res_inst, j,
			  &test_float, sizeof(test_float));
	INIT_OBJ_RES_DATA(TEST_RES_BOOL, test_res, i, test_res_inst, j,
			  &test_bool, sizeof(test_bool));
	INIT_OBJ_RES_DATA(TEST_RES_OBJLNK, test_res, i, test_res_inst, j,
			  &test_objlnk, sizeof(test_objlnk));

	test_inst.resources = test_res;
	test_inst.resource_count = i;

	return &test_inst;
}

static void test_obj_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;

	test_obj.obj_id = TEST_OBJ_ID;
	test_obj.version_major = 1;
	test_obj.version_minor = 0;
	test_obj.is_core = false;
	test_obj.fields = test_fields;
	test_obj.field_count = ARRAY_SIZE(test_fields);
	test_obj.max_instance_count = 1U;
	test_obj.create_cb = test_obj_create;

	(void)lwm2m_register_obj(&test_obj);
	(void)lwm2m_create_obj_inst(TEST_OBJ_ID, TEST_OBJ_INST_ID, &obj_inst);
}


/* 2 bytes for Content Format option + payload marker */
#define TEST_PAYLOAD_OFFSET 3
#define TEST_PAYLOAD(res_id, type, value) \
	"{\"bn\":\"/65535/0/\",\"e\":[{\"n\":\"" STRINGIFY(res_id) \
	"\",\"" type "\":" value "}]}"

static struct lwm2m_message test_msg;

static void context_reset(void)
{
	memset(&test_msg, 0, sizeof(test_msg));

	test_msg.out.writer = &json_writer;
	test_msg.out.out_cpkt = &test_msg.cpkt;

	test_msg.in.reader = &json_reader;
	test_msg.in.in_cpkt = &test_msg.cpkt;

	test_msg.path.level = LWM2M_PATH_LEVEL_RESOURCE;
	test_msg.path.obj_id = TEST_OBJ_ID;
	test_msg.path.obj_inst_id = TEST_OBJ_INST_ID;

	test_msg.cpkt.data = test_msg.msg_data;
	test_msg.cpkt.max_len = sizeof(test_msg.msg_data);
}

static void test_payload_set(const char *payload)
{
	memcpy(test_msg.msg_data + 1, payload, strlen(payload));
	test_msg.cpkt.offset = strlen(payload) + 1;
	test_msg.in.offset = 1; /* Payload marker */
}

static void test_prepare(void)
{
	context_reset();
}

static void test_prepare_nomem(void)
{
	context_reset();

	/* Leave some space for Content-format option */
	test_msg.cpkt.offset = sizeof(test_msg.msg_data) - TEST_PAYLOAD_OFFSET;
}

static void test_prepare_nodata(void)
{
	context_reset();
}



static void test_put_s8(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int8_t value[] = { 0, INT8_MAX, INT8_MIN };
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_S8, "v", "0"),
		TEST_PAYLOAD(TEST_RES_S8, "v", "127"),
		TEST_PAYLOAD(TEST_RES_S8, "v", "-128"),
	};

	test_msg.path.res_id = TEST_RES_S8;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s8 = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s8_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S8;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s16(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int16_t value[] = { 0, INT16_MAX, INT16_MIN };
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_S16, "v", "0"),
		TEST_PAYLOAD(TEST_RES_S16, "v", "32767"),
		TEST_PAYLOAD(TEST_RES_S16, "v", "-32768"),
	};

	test_msg.path.res_id = TEST_RES_S16;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s16 = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s16_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S16;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s32(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int32_t value[] = { 0, INT32_MAX, INT32_MIN };
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_S32, "v", "0"),
		TEST_PAYLOAD(TEST_RES_S32, "v", "2147483647"),
		TEST_PAYLOAD(TEST_RES_S32, "v", "-2147483648"),
	};

	test_msg.path.res_id = TEST_RES_S32;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s32 = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s32_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S32;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s64(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int64_t value[] = { 0, INT64_MAX, INT64_MIN };
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_S64, "v", "0"),
		TEST_PAYLOAD(TEST_RES_S64, "v", "9223372036854775807"),
		TEST_PAYLOAD(TEST_RES_S64, "v", "-9223372036854775808"),
	};

	test_msg.path.res_id = TEST_RES_S64;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s64 = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_s64_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S64;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_string(void)
{
	int ret;
	const char *expected_payload =
		TEST_PAYLOAD(TEST_RES_STRING, "sv", "\"test_string\"");

	strcpy(test_string, "test_string");
	test_msg.path.res_id = TEST_RES_STRING;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_true(ret >= 0, "Error reported");

	zassert_mem_equal(test_msg.msg_data + TEST_PAYLOAD_OFFSET,
			 expected_payload, strlen(expected_payload),
			  "Invalid payload format");
	zassert_equal(test_msg.cpkt.offset,
		      strlen(expected_payload) + TEST_PAYLOAD_OFFSET,
		      "Invalid packet offset");
}

static void test_put_string_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_STRING;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_float(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	double value[] = { 0., 0.123, -0.987, 3., -10., 2.333, -123.125 };
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "0.0"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "0.123"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "-0.987"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "3.0"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "-10.0"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "2.333"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "-123.125"),
	};

	test_msg.path.res_id = TEST_RES_FLOAT;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_float = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_float_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_FLOAT;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_bool(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	bool value[] = { true, false };
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_BOOL, "bv", "true"),
		TEST_PAYLOAD(TEST_RES_BOOL, "bv", "false"),
	};

	test_msg.path.res_id = TEST_RES_BOOL;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_bool = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_bool_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_BOOL;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_objlnk(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	struct lwm2m_objlnk value[] = {
		{ 0, 0 }, { 1, 1 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	char * const expected_payload[] = {
		TEST_PAYLOAD(TEST_RES_OBJLNK, "ov", "\"0:0\""),
		TEST_PAYLOAD(TEST_RES_OBJLNK, "ov", "\"1:1\""),
		TEST_PAYLOAD(TEST_RES_OBJLNK, "ov", "\"65535:65535\""),
	};

	test_msg.path.res_id = TEST_RES_OBJLNK;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_objlnk = value[i];

		ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");

		offset += strlen(expected_payload[i]);
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_objlnk_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_OBJLNK;

	ret = do_read_op_json(&test_msg, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_get_s32(void)
{
	int ret;
	int i;
	char * const payload[] = {
		TEST_PAYLOAD(TEST_RES_S32, "v", "0"),
		TEST_PAYLOAD(TEST_RES_S32, "v", "2147483647"),
		TEST_PAYLOAD(TEST_RES_S32, "v", "-2147483648"),
	};
	int32_t expected_value[] = { 0, INT32_MAX, INT32_MIN };

	test_msg.path.res_id = TEST_RES_S32;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_json(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_equal(test_s32, expected_value[i], "Invalid value parsed");
	}
}

static void test_get_s32_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S32;

	ret = do_write_op_json(&test_msg);
	zassert_equal(ret, -EINVAL, "Invalid error code returned");
}

static void test_get_s64(void)
{
	int ret;
	int i;
	char * const payload[] = {
		TEST_PAYLOAD(TEST_RES_S64, "v", "0"),
		TEST_PAYLOAD(TEST_RES_S64, "v", "9223372036854775807"),
		TEST_PAYLOAD(TEST_RES_S64, "v", "-9223372036854775808"),
	};
	int64_t expected_value[] = { 0, INT64_MAX, INT64_MIN };

	test_msg.path.res_id = TEST_RES_S64;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_json(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_equal(test_s64, expected_value[i], "Invalid value parsed");
	}
}

static void test_get_s64_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S64;

	ret = do_write_op_json(&test_msg);
	zassert_equal(ret, -EINVAL, "Invalid error code returned");
}

static void test_get_string(void)
{
	int ret;
	const char *payload =
		TEST_PAYLOAD(TEST_RES_STRING, "sv", "\"test_string\"");
	const char *expected_value = "test_string";

	test_msg.path.res_id = TEST_RES_STRING;

	test_payload_set(payload);

	ret = do_write_op_json(&test_msg);
	zassert_true(ret >= 0, "Error reported");
	zassert_mem_equal(test_string, expected_value, strlen(expected_value),
			  "Invalid value parsed");
}

static void test_get_string_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_STRING;

	ret = do_write_op_json(&test_msg);
	zassert_equal(ret, -EINVAL, "Invalid error code returned");
}

#define DOUBLE_CMP_EPSILON 0.000000001

static void test_get_float(void)
{
	int ret;
	int i;
	char * const payload[] = {
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "0.0"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "0.123"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "-0.987"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "3.0"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "-10.0"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "2.333"),
		TEST_PAYLOAD(TEST_RES_FLOAT, "v", "-123.125"),
	};
	double expected_value[] = {
		0., 0.123, -0.987, 3., -10., 2.333, -123.125
	};

	test_msg.path.res_id = TEST_RES_FLOAT;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_json(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_true((test_float > expected_value[i] - DOUBLE_CMP_EPSILON) &&
			     (test_float < expected_value[i] + DOUBLE_CMP_EPSILON),
			     "Invalid value parsed");
	}
}

static void test_get_float_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_FLOAT;

	ret = do_write_op_json(&test_msg);
	zassert_equal(ret, -EINVAL, "Invalid error code returned");
}

static void test_get_bool(void)
{
	int ret;
	int i;
	char * const payload[] = {
		TEST_PAYLOAD(TEST_RES_BOOL, "bv", "true"),
		TEST_PAYLOAD(TEST_RES_BOOL, "bv", "false"),
	};
	bool expected_value[] = { true, false };

	test_msg.path.res_id = TEST_RES_BOOL;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_json(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_equal(test_bool, expected_value[i], "Invalid value parsed");
	}
}

static void test_get_bool_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_BOOL;

	ret = do_write_op_json(&test_msg);
	zassert_equal(ret, -EINVAL, "Invalid error code returned");
}

static void test_get_objlnk(void)
{
	int ret;
	int i;
	char * const payload[] = {
		TEST_PAYLOAD(TEST_RES_OBJLNK, "ov", "\"0:0\""),
		TEST_PAYLOAD(TEST_RES_OBJLNK, "ov", "\"1:1\""),
		TEST_PAYLOAD(TEST_RES_OBJLNK, "ov", "\"65535:65535\""),
	};
	struct lwm2m_objlnk expected_value[] = {
		{ 0, 0 }, { 1, 1 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};

	test_msg.path.res_id = TEST_RES_OBJLNK;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_json(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_mem_equal(&test_objlnk, &expected_value[i],
				  sizeof(test_objlnk), "Invalid value parsed");
	}
}

static void test_get_objlnk_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_OBJLNK;

	ret = do_write_op_json(&test_msg);
	zassert_equal(ret, -EINVAL, "Invalid error code returned");
}

void test_main(void)
{
	test_obj_init();

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
			test_get_objlnk, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_get_objlnk_nodata, test_prepare_nodata, unit_test_noop)
	);

	ztest_run_test_suite(lwm2m_content_plain_text);
}
