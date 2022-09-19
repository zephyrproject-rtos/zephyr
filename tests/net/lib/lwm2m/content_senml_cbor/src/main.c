/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "lwm2m_util.h"
#include "lwm2m_rw_senml_cbor.h"
#include "lwm2m_engine.h"

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
#define TEST_RES_OPAQUE 8
#define TEST_RES_TIME 9

#define TEST_OBJ_RES_MAX_ID 10

#define TEST_MAX_PAYLOAD_BUFFER_LENGTH 40

static struct lwm2m_engine_obj test_obj;

struct test_payload_buffer {
	uint8_t data[TEST_MAX_PAYLOAD_BUFFER_LENGTH];
	size_t len;
};

static struct lwm2m_engine_obj_field test_fields[] = {
	OBJ_FIELD_DATA(TEST_RES_S8, RW, S8),
	OBJ_FIELD_DATA(TEST_RES_S16, RW, S16),
	OBJ_FIELD_DATA(TEST_RES_S32, RW, S32),
	OBJ_FIELD_DATA(TEST_RES_S64, RW, S64),
	OBJ_FIELD_DATA(TEST_RES_STRING, RW, STRING),
	OBJ_FIELD_DATA(TEST_RES_FLOAT, RW, FLOAT),
	OBJ_FIELD_DATA(TEST_RES_BOOL, RW, BOOL),
	OBJ_FIELD_DATA(TEST_RES_OBJLNK, RW, OBJLNK),
	OBJ_FIELD_DATA(TEST_RES_OPAQUE, RW, OPAQUE),
	OBJ_FIELD_DATA(TEST_RES_TIME, RW, TIME)
};

static struct lwm2m_engine_obj_inst test_inst;
static struct lwm2m_engine_res test_res[TEST_OBJ_RES_MAX_ID];
static struct lwm2m_engine_res_inst test_res_inst[TEST_OBJ_RES_MAX_ID];

#define TEST_STRING_MAX_SIZE 16
#define TEST_OPAQUE_MAX_SIZE 11

static int8_t test_s8;
static int16_t test_s16;
static int32_t test_s32;
static int64_t test_s64;
static char test_string[TEST_STRING_MAX_SIZE];
static double test_float;
static bool test_bool;
static struct lwm2m_objlnk test_objlnk;
static uint8_t test_opaque[TEST_OPAQUE_MAX_SIZE];
static int64_t test_time;

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
	INIT_OBJ_RES_DATA(TEST_RES_OPAQUE, test_res, i, test_res_inst, j,
			  &test_opaque, sizeof(test_opaque));
	INIT_OBJ_RES_DATA(TEST_RES_TIME, test_res, i, test_res_inst, j,
			  &test_time, sizeof(test_time));

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

static struct lwm2m_message test_msg;

static void context_reset(void)
{
	memset(&test_msg, 0, sizeof(test_msg));

	test_msg.out.writer = &senml_cbor_writer;
	test_msg.out.out_cpkt = &test_msg.cpkt;

	test_msg.in.reader = &senml_cbor_reader;
	test_msg.in.in_cpkt = &test_msg.cpkt;

	test_msg.path.level = LWM2M_PATH_LEVEL_RESOURCE;
	test_msg.path.obj_id = TEST_OBJ_ID;
	test_msg.path.obj_inst_id = TEST_OBJ_INST_ID;

	test_msg.cpkt.data = test_msg.msg_data;
	test_msg.cpkt.max_len = sizeof(test_msg.msg_data);
}

static void test_payload_set(struct test_payload_buffer payload)
{
	memcpy(test_msg.msg_data + 1, payload.data, payload.len);
	test_msg.cpkt.offset = payload.len + 1;
	test_msg.in.offset = 1;
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

	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'0',
				(0x00 << 5) | 2,
				(0x00 << 5) | 0
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'0',
				(0x00 << 5) | 2,
				(0x00 << 5) | 24,
				127
			},
			.len = 19
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'0',
				(0x00 << 5) | 2,
				(0x01 << 5) | 24,
				127
			},
			.len = 19
		},
	};

	test_msg.path.res_id = TEST_RES_S8;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s8 = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;

		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

void test_put_s8_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S8;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s16(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int16_t value[] = { 0, INT16_MAX, INT16_MIN };

	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'1',
				(0x00 << 5) | 2,
				(0x00 << 5) | 0
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'1',
				(0x00 << 5) | 2,
				(0x00 << 5) | 25,
				127, 255
			},
			.len = 20
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'1',
				(0x00 << 5) | 2,
				(0x01 << 5) | 25,
				127, 255
			},
			.len = 20
		},
	};

	test_msg.path.res_id = TEST_RES_S16;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s16 = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;

		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

void test_put_s16_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S16;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s32(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int32_t value[] = { 0, INT32_MAX, INT32_MIN };

	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'2',
				(0x00 << 5) | 2,
				(0x00 << 5) | 0
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'2',
				(0x00 << 5) | 2,
				(0x00 << 5) | 26,
				127, 255, 255, 255
			},
			.len = 22
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'2',
				(0x00 << 5) | 2,
				(0x01 << 5) | 26,
				127, 255, 255, 255
			},
			.len = 22
		},
	};

	test_msg.path.res_id = TEST_RES_S32;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s32 = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;

		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

void test_put_s32_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S32;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_s64(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	int64_t value[] = { 1, INT64_MIN, INT64_MAX };

	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'3',
				(0x00 << 5) | 2,
				(0x00 << 5) | 1
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'3',
				(0x00 << 5) | 2,
				(0x01 << 5) | 27,
				127, 255, 255, 255, 255, 255, 255, 255
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'3',
				(0x00 << 5) | 2,
				(0x00 << 5) | 27,
				127, 255, 255, 255, 255, 255, 255, 255
			},
			.len = 26
		},
	};

	test_msg.path.res_id = TEST_RES_S64;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_s64 = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;

		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

void test_put_s64_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S64;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_string(void)
{
	int ret;
	struct test_payload_buffer expected_payload = {
		.data = {
			(0x04 << 5) | 1,
			(0x05 << 5) | 3,
			(0x01 << 5) | 1,
			(0x03 << 5) | 9,
			'/', '6', '5', '5', '3', '5', '/', '0', '/',
			(0x00 << 5) | 0,
			(0x03 << 5) | 1,
			'4',
			(0x00 << 5) | 3,
			(0x03 << 5) | 11,
			't', 'e', 's', 't', '_', 's', 't', 'r', 'i', 'n', 'g'
		},
		.len = 29
	};

	strcpy(test_string, "test_string");
	test_msg.path.res_id = TEST_RES_STRING;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_true(ret >= 0, "Error reported");

	zassert_mem_equal(test_msg.msg_data + TEST_PAYLOAD_OFFSET,
			 expected_payload.data, expected_payload.len,
			  "Invalid payload format");
	zassert_equal(test_msg.cpkt.offset,
		      expected_payload.len + TEST_PAYLOAD_OFFSET,
		      "Invalid packet offset");
}

static void test_put_string_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_STRING;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_float(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	double value[] = { 0.123, -0.987, 3., -10., 2.333, -123.125 };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0x3F, 0xBF, 0x7C, 0xED, 0x91, 0x68, 0x72, 0xB0
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0xBF, 0xEF, 0x95, 0x81, 0x06, 0x24, 0xDD, 0x2F
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0xC0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0x40, 0x02, 0xA9, 0xFB, 0xE7, 0x6C, 0x8B, 0x44
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0xC0, 0x5E, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 26
		}
	};

	test_msg.path.res_id = TEST_RES_FLOAT;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_float = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;
		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_float_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_FLOAT;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_bool(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	bool value[] = { true, false };
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'6',
				(0x00 << 5) | 4,
				(0x07 << 5) | 21
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'6',
				(0x00 << 5) | 4,
				(0x07 << 5) | 20
			},
			.len = 18
		}
	};

	test_msg.path.res_id = TEST_RES_BOOL;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_bool = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;

		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_bool_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_BOOL;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_objlnk(void)
{
	int ret;
	int i;
	uint16_t offset = 0;
	struct lwm2m_objlnk value[] = {
		{ 0, 0 }, { 1, 2 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};
	struct test_payload_buffer expected_payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'7',
				(0x00 << 5) | 2,
				(0x00 << 5) | 0
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'7',
				(0x00 << 5) | 2,
				(0x00 << 5) | 26,
				0, 1, 0, 2
			},
			.len = 22
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'7',
				(0x00 << 5) | 2,
				(0x01 << 5) | 0,
			},
			.len = 18
		},
	};

	test_msg.path.res_id = TEST_RES_OBJLNK;

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		test_objlnk = value[i];

		ret = do_read_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");

		offset += TEST_PAYLOAD_OFFSET;

		zassert_mem_equal(test_msg.msg_data + offset,
				  expected_payload[i].data,
				  expected_payload[i].len,
				  "Invalid payload format");

		offset += expected_payload[i].len;
		zassert_equal(test_msg.cpkt.offset, offset,
			      "Invalid packet offset");
	}
}

static void test_put_objlnk_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_OBJLNK;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_opaque(void)
{
	int ret;
	struct test_payload_buffer expected_payload = {
		.data = {
			(0x04 << 5) | 1,
			(0x05 << 5) | 3,
			(0x01 << 5) | 1,
			(0x03 << 5) | 9,
			'/', '6', '5', '5', '3', '5', '/', '0', '/',
			(0x00 << 5) | 0,
			(0x03 << 5) | 1,
			'8',
			(0x00 << 5) | 8,
			(0x02 << 5) | 11,
			't', 'e', 's', 't', '_', 'o', 'p', 'a', 'q', 'u', 'e',
		},
		.len = 29
	};

	memcpy(test_opaque, "test_opaque", 11 * sizeof(uint8_t));
	test_msg.path.res_id = TEST_RES_OPAQUE;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_true(ret >= 0, "Error reported");

	zassert_mem_equal(test_msg.msg_data + TEST_PAYLOAD_OFFSET,
			 expected_payload.data, expected_payload.len,
			  "Invalid payload format");
	zassert_equal(test_msg.cpkt.offset,
		      expected_payload.len + TEST_PAYLOAD_OFFSET,
		      "Invalid packet offset");
}

static void test_put_opaque_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_OPAQUE;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_put_time(void)
{
	int ret;
	int64_t value = 1170111600;
	struct test_payload_buffer expected_payload = {
		.data = {
			(0x04 << 5) | 1,
			(0x05 << 5) | 3,
			(0x01 << 5) | 1,
			(0x03 << 5) | 9,
			'/', '6', '5', '5', '3', '5', '/', '0', '/',
			(0x00 << 5) | 0,
			(0x03 << 5) | 1,
			'9',
			(0x00 << 5) | 2,
			(0x00 << 5) | 26,
			0x45, 0xbe, 0x7c, 0x70
		},
		.len = 22
	};

	test_msg.path.res_id = TEST_RES_TIME;
	test_time = value;

	ret = do_read_op_senml_cbor(&test_msg);

	zassert_true(ret >= 0, "Error reported");
	zassert_mem_equal(test_msg.msg_data + TEST_PAYLOAD_OFFSET,
				expected_payload.data,
				expected_payload.len,
				"Invalid payload format");
	zassert_equal(test_msg.cpkt.offset, expected_payload.len + TEST_PAYLOAD_OFFSET,
				"Invalid packet offset");

}

static void test_put_time_nomem(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_TIME;

	ret = do_read_op_senml_cbor(&test_msg);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

static void test_get_s32(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'2',
				(0x00 << 5) | 2,
				(0x00 << 5) | 0
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'2',
				(0x00 << 5) | 2,
				(0x00 << 5) | 26,
				127, 255, 255, 255
			},
			.len = 22
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'2',
				(0x00 << 5) | 2,
				(0x01 << 5) | 26,
				127, 255, 255, 255
			},
			.len = 22
		},
	};
	int32_t expected_value[] = { 0, INT32_MAX, INT32_MIN };

	test_msg.path.res_id = TEST_RES_S32;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_equal(test_s32, expected_value[i], "Invalid value parsed");
		zassert_equal(test_msg.in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_s32_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S32;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

static void test_get_s64(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'3',
				(0x00 << 5) | 2,
				(0x00 << 5) | 0
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'3',
				(0x00 << 5) | 2,
				(0x01 << 5) | 27,
				127, 255, 255, 255, 255, 255, 255, 255
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'3',
				(0x00 << 5) | 2,
				(0x00 << 5) | 27,
				127, 255, 255, 255, 255, 255, 255, 255
			},
			.len = 26
		},
	};
	int64_t expected_value[] = { 0, INT64_MIN, INT64_MAX };

	test_msg.path.res_id = TEST_RES_S64;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_equal(test_s64, expected_value[i], "Invalid value parsed");
		zassert_equal(test_msg.in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_s64_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_S64;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

static void test_get_string(void)
{
	int ret;
	struct test_payload_buffer payload = {
		.data = {
			(0x04 << 5) | 1,
			(0x05 << 5) | 3,
			(0x01 << 5) | 1,
			(0x03 << 5) | 9,
			'/', '6', '5', '5', '3', '5', '/', '0', '/',
			(0x00 << 5) | 0,
			(0x03 << 5) | 1,
			'4',
			(0x00 << 5) | 3,
			(0x03 << 5) | 11,
			't', 'e', 's', 't', '_', 's', 't', 'r', 'i', 'n', 'g'
		},
		.len = 29
	};
	const char *expected_value = "test_string";

	test_msg.path.res_id = TEST_RES_STRING;

	test_payload_set(payload);

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_true(ret >= 0, "Error reported");
	zassert_mem_equal(test_string, expected_value, strlen(expected_value),
			  "Invalid value parsed");
	zassert_equal(test_msg.in.offset, payload.len + 1,
		      "Invalid packet offset");
}

static void test_get_string_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_STRING;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

#define DOUBLE_CMP_EPSILON 0.000000001

static void test_get_float(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0x3F, 0xBF, 0x7C, 0xED, 0x91, 0x68, 0x72, 0xB0
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0xBF, 0xEF, 0x95, 0x81, 0x06, 0x24, 0xDD, 0x2F
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0xC0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0x40, 0x02, 0xA9, 0xFB, 0xE7, 0x6C, 0x8B, 0x44
			},
			.len = 26
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'5',
				(0x00 << 5) | 2,
				(0x07 << 5) | 0x1b,
				0xC0, 0x5E, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00
			},
			.len = 26
		}
	};
	double expected_value[] = {
		0.123, -0.987, 3., -10., 2.333, -123.125
	};

	test_msg.path.res_id = TEST_RES_FLOAT;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_true((test_float > expected_value[i] - DOUBLE_CMP_EPSILON) &&
			     (test_float < expected_value[i] + DOUBLE_CMP_EPSILON),
			     "Invalid value parsed");
		zassert_equal(test_msg.in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_float_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_FLOAT;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

static void test_get_bool(void)
{
	int ret;
	int i;
	struct test_payload_buffer const payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'6',
				(0x00 << 5) | 4,
				(0x07 << 5) | 21
			},
			.len = 18
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'6',
				(0x00 << 5) | 4,
				(0x07 << 5) | 20
			},
			.len = 18
		}
	};
	bool expected_value[] = { true, false };

	test_msg.path.res_id = TEST_RES_BOOL;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_equal(test_bool, expected_value[i], "Invalid value parsed");
		zassert_equal(test_msg.in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_bool_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_BOOL;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

static void test_get_objlnk(void)
{
	int ret;
	int i;
	struct test_payload_buffer payload[] = {
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'7',
				(0x00 << 5) | 2,
				(0x03 << 5) | (sizeof("0:0")),
				'0', ':', '0', '\0'
			},
			.len = 22
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'7',
				(0x00 << 5) | 3,
				(0x03 << 5) | (sizeof("1:2")),
				'1', ':', '2', '\0'
			},
			.len = 22
		},
		{
			.data = {
				(0x04 << 5) | 1,
				(0x05 << 5) | 3,
				(0x01 << 5) | 1,
				(0x03 << 5) | 9,
				'/', '6', '5', '5', '3', '5', '/', '0', '/',
				(0x00 << 5) | 0,
				(0x03 << 5) | 1,
				'7',
				(0x00 << 5) | 3,
				(0x03 << 5) | (sizeof("65535:65535")),
				'6', '5', '5', '3', '5', ':',
				'6', '5', '5', '3', '5', '\0'
			},
			.len = 30
		},
	};
	struct lwm2m_objlnk expected_value[] = {
		{ 0, 0 }, { 1, 2 }, { LWM2M_OBJLNK_MAX_ID, LWM2M_OBJLNK_MAX_ID }
	};

	test_msg.path.res_id = TEST_RES_OBJLNK;

	for (i = 0; i < ARRAY_SIZE(expected_value); i++) {
		test_payload_set(payload[i]);

		ret = do_write_op_senml_cbor(&test_msg);
		zassert_true(ret >= 0, "Error reported");
		zassert_mem_equal(&test_objlnk, &expected_value[i],
				  sizeof(test_objlnk), "Invalid value parsed");
		zassert_equal(test_msg.in.offset, payload[i].len + 1,
			      "Invalid packet offset");
	}
}

static void test_get_objlnk_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_OBJLNK;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

static void test_get_opaque(void)
{
	int ret;
	struct test_payload_buffer payload = {
		.data = {
			(0x04 << 5) | 1,
			(0x05 << 5) | 3,
			(0x01 << 5) | 1,
			(0x03 << 5) | 9,
			'/', '6', '5', '5', '3', '5', '/', '0', '/',
			(0x00 << 5) | 0,
			(0x03 << 5) | 1,
			'4',
			(0x00 << 5) | 3,
			(0x03 << 5) | 11,
			't', 'e', 's', 't', '_', 'o', 'p', 'a', 'q', 'u', 'e'
		},
		.len = 29
	};
	uint8_t expected_value[11];

	memcpy(expected_value, "test_opaque", 11 * sizeof(uint8_t));

	test_msg.path.res_id = TEST_RES_OPAQUE;

	test_payload_set(payload);

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_true(ret >= 0, "Error reported");
	zassert_mem_equal(test_opaque, expected_value, sizeof(expected_value),
			  "Invalid value parsed");
	zassert_equal(test_msg.in.offset, payload.len + 1,
		      "Invalid packet offset");
}

static void test_get_opaque_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_OPAQUE;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

static void test_get_time(void)
{
	int ret;
	int64_t expected_value = 1170111600;
	struct test_payload_buffer payload = {
		.data = {
			(0x04 << 5) | 1,
			(0x05 << 5) | 3,
			(0x01 << 5) | 1,
			(0x03 << 5) | 9,
			'/', '6', '5', '5', '3', '5', '/', '0', '/',
			(0x00 << 5) | 0,
			(0x03 << 5) | 1,
			'9',
			(0x00 << 5) | 2,
			(0x00 << 5) | 26,
			0x45, 0xbe, 0x7c, 0x70
		},
		.len = 22
	};

	test_msg.path.res_id = TEST_RES_TIME;

	test_payload_set(payload);

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_true(ret >= 0, "Error reported");
	zassert_equal(test_time, expected_value, "Invalid value parsed");
	zassert_equal(test_msg.in.offset, payload.len + 1,
				"Invalid packet offset");
}

static void test_get_time_nodata(void)
{
	int ret;

	test_msg.path.res_id = TEST_RES_TIME;

	ret = do_write_op_senml_cbor(&test_msg);
	zassert_equal(ret, -EBADMSG, "Invalid error code returned");
}

void test_main(void)
{
	test_obj_init();

	ztest_test_suite(
		lwm2m_content_senml_cbor,
		ztest_unit_test_setup_teardown(test_put_s8, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s8_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s16, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s16_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s32, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s32_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s64, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_s64_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_string, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_string_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_float, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_float_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_bool, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_bool_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_objlnk, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_objlnk_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_opaque, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_opaque_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_time, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_put_time_nomem,
				  test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_s32, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_s32_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_s64, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_s64_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_string, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_string_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_float, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_float_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_bool, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_bool_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_objlnk, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_objlnk_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_opaque, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_opaque_nodata,
				  test_prepare_nodata, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_time, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(test_get_time_nodata,
				  test_prepare_nodata, unit_test_noop)
	);

	ztest_run_test_suite(lwm2m_content_senml_cbor);
}
