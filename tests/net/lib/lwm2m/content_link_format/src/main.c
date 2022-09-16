/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include "lwm2m_engine.h"
#include "lwm2m_rw_link_format.h"

#define TEST_SSID 101
#define TEST_PMIN 5
#define TEST_PMAX 200

#define TEST_OBJ_ID 0xFFFF
#define TEST_OBJ_INST_ID 0

#define TEST_RES_S8 0

#define TEST_OBJ_RES_MAX_ID 2

static struct lwm2m_engine_obj test_obj;

static struct lwm2m_engine_obj_field test_fields[] = {
	OBJ_FIELD_DATA(TEST_RES_S8, RW, S8),
};

static struct lwm2m_engine_obj_inst test_inst;
static struct lwm2m_engine_res test_res[TEST_OBJ_RES_MAX_ID];
static struct lwm2m_engine_res_inst test_res_inst[TEST_OBJ_RES_MAX_ID];

static int8_t test_s8;

static struct lwm2m_engine_obj_inst *test_obj_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(test_res_inst, ARRAY_SIZE(test_res_inst));

	INIT_OBJ_RES_DATA(TEST_RES_S8, test_res, i, test_res_inst, j,
			  &test_s8, sizeof(test_s8));

	test_inst.resources = test_res;
	test_inst.resource_count = i;

	return &test_inst;
}

static void test_obj_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;

	test_obj.obj_id = TEST_OBJ_ID;
	test_obj.version_major = 1;
	test_obj.version_minor = 1;
	test_obj.is_core = false;
	test_obj.fields = test_fields;
	test_obj.field_count = ARRAY_SIZE(test_fields);
	test_obj.max_instance_count = 1U;
	test_obj.create_cb = test_obj_create;

	(void)lwm2m_register_obj(&test_obj);
	(void)lwm2m_create_obj_inst(TEST_OBJ_ID, TEST_OBJ_INST_ID, &obj_inst);
}

static void test_attr_init(void)
{
	struct lwm2m_attr *attr;

	attr = lwm2m_engine_get_next_attr(NULL, NULL);
	attr->type = LWM2M_ATTR_PMIN;
	attr->int_val = TEST_PMIN;
	attr->ref = &test_inst;

	attr = lwm2m_engine_get_next_attr(NULL, NULL);
	attr->type = LWM2M_ATTR_PMAX;
	attr->int_val = TEST_PMAX;
	attr->ref = &test_res;
}

static struct lwm2m_output_context test_out;
static struct lwm2m_obj_path test_path;
static struct coap_packet test_packet;
static uint8_t test_payload[128];
static struct link_format_out_formatter_data test_formatter_data;

static void context_reset(void)
{
	memset(&test_out, 0, sizeof(test_out));
	test_out.writer = &link_format_writer;
	test_out.out_cpkt = &test_packet;
	test_out.user_data = &test_formatter_data;

	memset(&test_payload, 0, sizeof(test_payload));
	memset(&test_packet, 0, sizeof(test_packet));
	test_packet.data = test_payload;
	test_packet.max_len = sizeof(test_payload);

	memset(&test_path, 0, sizeof(test_path));
	test_path.level = LWM2M_PATH_LEVEL_OBJECT;
	test_path.obj_id = TEST_OBJ_ID;
	test_path.obj_inst_id = TEST_OBJ_INST_ID;
	test_path.res_id = TEST_RES_S8;

	memset(&test_formatter_data, 0, sizeof(test_formatter_data));
	test_formatter_data.is_first = true;
	test_formatter_data.request_level = LWM2M_PATH_LEVEL_OBJECT;
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

static void test_put_begin_discovery(void)
{
	int ret;
	const char *expected_payload = "";

	test_formatter_data.mode = LINK_FORMAT_MODE_DISCOVERY;

	ret = link_format_writer.put_begin(&test_out, &test_path);
	zassert_equal(ret, strlen(expected_payload),
		      "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload,
			  strlen(expected_payload), "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, strlen(expected_payload),
			"Invalid packet offset");
}

static void test_put_begin_bs_discovery(void)
{
	int ret;
	const char *expected_payload = "lwm2m=\"1.0\"";

	test_formatter_data.mode = LINK_FORMAT_MODE_BOOTSTRAP_DISCOVERY;

	ret = link_format_writer.put_begin(&test_out, &test_path);
	zassert_equal(ret, strlen(expected_payload),
		      "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload,
			  strlen(expected_payload), "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, strlen(expected_payload),
			"Invalid packet offset");
}

static void test_put_begin_register(void)
{
	int ret;
	const char *expected_payload = "</>;rt=\"oma.lwm2m\";ct=11543";

	test_formatter_data.mode = LINK_FORMAT_MODE_REGISTER;

	ret = link_format_writer.put_begin(&test_out, &test_path);
	zassert_equal(ret, strlen(expected_payload),
		      "Invalid length returned");
	zassert_mem_equal(test_out.out_cpkt->data, expected_payload,
			  strlen(expected_payload), "Invalid payload format");
	zassert_equal(test_out.out_cpkt->offset, strlen(expected_payload),
			"Invalid packet offset");
}

static void test_put_begin_nomem(void)
{
	int ret;

	test_formatter_data.mode = LINK_FORMAT_MODE_REGISTER;

	ret = link_format_writer.put_begin(&test_out, &test_path);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

struct test_case_corelink {
	uint8_t request_level;
	uint8_t path_level;
	const char *expected_payload;
};

static void test_put_corelink_discovery(void)
{
	int ret;
	int i;
	struct test_case_corelink test_case[] = {
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT,
			.path_level = LWM2M_PATH_LEVEL_OBJECT,
			.expected_payload = "</65535>;ver=1.1"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT,
			.path_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.expected_payload = "</65535/0>"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT,
			.path_level = LWM2M_PATH_LEVEL_RESOURCE,
			.expected_payload = "</65535/0/0>"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.path_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.expected_payload = "</65535/0>;pmin=" STRINGIFY(TEST_PMIN)
		},
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.path_level = LWM2M_PATH_LEVEL_RESOURCE,
			.expected_payload = "</65535/0/0>;pmax=" STRINGIFY(TEST_PMAX)
		},
		{
			.request_level = LWM2M_PATH_LEVEL_RESOURCE,
			.path_level = LWM2M_PATH_LEVEL_RESOURCE,
			.expected_payload = "</65535/0/0>;pmin=" STRINGIFY(TEST_PMIN)
					    ";pmax=" STRINGIFY(TEST_PMAX)
		},
	};

	for (i = 0; i < ARRAY_SIZE(test_case); i++) {
		context_reset();

		test_formatter_data.mode = LINK_FORMAT_MODE_DISCOVERY;
		test_formatter_data.request_level = test_case[i].request_level;
		test_path.level = test_case[i].path_level;

		ret = link_format_writer.put_corelink(&test_out, &test_path);
		zassert_equal(ret, strlen(test_case[i].expected_payload),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data,
				  test_case[i].expected_payload,
				  strlen(test_case[i].expected_payload),
				  "Invalid payload format");
		zassert_equal(test_out.out_cpkt->offset,
			      strlen(test_case[i].expected_payload),
			      "Invalid packet offset");
	}
}

static void test_put_corelink_bs_discovery(void)
{
	int ret;
	int i;
	struct test_case_corelink test_case[] = {
		{
			.request_level = LWM2M_PATH_LEVEL_NONE,
			.path_level = LWM2M_PATH_LEVEL_OBJECT,
			.expected_payload = "</65535>;ver=1.1"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_NONE,
			.path_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.expected_payload = "</65535/0>"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT,
			.path_level = LWM2M_PATH_LEVEL_OBJECT,
			.expected_payload = "</65535>;ver=1.1"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_OBJECT,
			.path_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.expected_payload = "</65535/0>"
		},
	};


	for (i = 0; i < ARRAY_SIZE(test_case); i++) {
		context_reset();

		test_formatter_data.mode = LINK_FORMAT_MODE_BOOTSTRAP_DISCOVERY;
		test_formatter_data.request_level = test_case[i].request_level;
		test_path.level = test_case[i].path_level;

		ret = link_format_writer.put_corelink(&test_out, &test_path);
		zassert_equal(ret, strlen(test_case[i].expected_payload),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data,
				  test_case[i].expected_payload,
				  strlen(test_case[i].expected_payload),
				  "Invalid payload format");
		zassert_equal(test_out.out_cpkt->offset,
			      strlen(test_case[i].expected_payload),
			      "Invalid packet offset");
	}
}

static void test_put_corelink_bs_discovery_ssid(void)
{
	int ret;
	int i;
	char * const expected_payload[] = {
		"</0/0>;ssid=" STRINGIFY(TEST_SSID),
		"</1/0>;ssid=" STRINGIFY(TEST_SSID),
	};
	uint16_t test_obj_id[] = {
		LWM2M_OBJECT_SECURITY_ID,
		LWM2M_OBJECT_SERVER_ID,
	};

	for (i = 0; i < ARRAY_SIZE(expected_payload); i++) {
		context_reset();

		test_formatter_data.mode = LINK_FORMAT_MODE_BOOTSTRAP_DISCOVERY;
		test_formatter_data.request_level = LWM2M_PATH_LEVEL_NONE;
		test_path.level = LWM2M_PATH_LEVEL_OBJECT_INST;
		test_path.obj_id = test_obj_id[i];
		test_path.obj_inst_id = 0;

		ret = link_format_writer.put_corelink(&test_out, &test_path);
		zassert_equal(ret, strlen(expected_payload[i]),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data,
				  expected_payload[i],
				  strlen(expected_payload[i]),
				  "Invalid payload format");
		zassert_equal(test_out.out_cpkt->offset,
			      strlen(expected_payload[i]),
			      "Invalid packet offset");
	}
}

static void test_put_corelink_register(void)
{
	int ret;
	int i;
	struct test_case_corelink test_case[] = {
		{
			.request_level = LWM2M_PATH_LEVEL_NONE,
			.path_level = LWM2M_PATH_LEVEL_OBJECT,
			.expected_payload = "</65535>;ver=1.1"
		},
		{
			.request_level = LWM2M_PATH_LEVEL_NONE,
			.path_level = LWM2M_PATH_LEVEL_OBJECT_INST,
			.expected_payload = "</65535/0>"
		},
	};

	for (i = 0; i < ARRAY_SIZE(test_case); i++) {
		context_reset();

		test_formatter_data.mode = LINK_FORMAT_MODE_REGISTER;
		test_formatter_data.request_level = test_case[i].request_level;
		test_path.level = test_case[i].path_level;

		ret = link_format_writer.put_corelink(&test_out, &test_path);
		zassert_equal(ret, strlen(test_case[i].expected_payload),
			      "Invalid length returned");
		zassert_mem_equal(test_out.out_cpkt->data,
				  test_case[i].expected_payload,
				  strlen(test_case[i].expected_payload),
				  "Invalid payload format");
		zassert_equal(test_out.out_cpkt->offset,
			      strlen(test_case[i].expected_payload),
			      "Invalid packet offset");
	}
}


static void test_put_corelink_nomem(void)
{
	int ret;

	test_formatter_data.mode = LINK_FORMAT_MODE_REGISTER;
	test_formatter_data.request_level = LWM2M_PATH_LEVEL_NONE;

	ret = link_format_writer.put_corelink(&test_out, &test_path);
	zassert_equal(ret, -ENOMEM, "Invalid error code returned");
}

void test_main(void)
{
	test_obj_init();
	test_attr_init();

	/* Initialize Security/Server objects with SSID. */
	lwm2m_engine_set_u16("0/0/10", TEST_SSID);
	lwm2m_engine_set_u16("1/0/0", TEST_SSID);

	ztest_test_suite(
		lwm2m_content_link_format,
		ztest_unit_test_setup_teardown(
			test_put_begin_discovery, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_begin_bs_discovery, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_begin_register, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_begin_nomem, test_prepare_nomem, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_corelink_discovery, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_corelink_bs_discovery, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_corelink_bs_discovery_ssid, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_corelink_register, test_prepare, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_put_corelink_nomem, test_prepare_nomem, unit_test_noop)
	);

	ztest_run_test_suite(lwm2m_content_link_format);
}
