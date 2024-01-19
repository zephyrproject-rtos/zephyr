/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "lwm2m_engine.h"
#include "lwm2m_util.h"

#define TEST_OBJ_ID 32768

static uint32_t callback_checker;
static char pre_write_cb_buf[10];

static void *pre_write_cb(uint16_t obj_inst_id,
			  uint16_t res_id,
			  uint16_t res_inst_id,
			  size_t *data_len)
{
	callback_checker |= 0x01;
	return pre_write_cb_buf;
}

static int post_write_cb(uint16_t obj_inst_id,
			 uint16_t res_id, uint16_t res_inst_id,
			 uint8_t *data, uint16_t data_len,
			 bool last_block, size_t total_size)
{
	callback_checker |= 0x02;
	return 0;
}

static void *read_cb(uint16_t obj_inst_id,
		     uint16_t res_id,
		     uint16_t res_inst_id,
		     size_t *data_len)
{
	callback_checker |= 0x04;
	return 0;
}

static int validate_cb(uint16_t obj_inst_id,
		       uint16_t res_id, uint16_t res_inst_id,
		       uint8_t *data, uint16_t data_len,
		       bool last_block, size_t total_size)
{
	callback_checker |= 0x08;
	return 0;
}

static int obj_create_cb(uint16_t obj_inst_id)
{
	callback_checker |= 0x10;
	return 0;
}

static int obj_delete_cb(uint16_t obj_inst_id)
{
	callback_checker |= 0x20;
	return 0;
}

static int exec_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	callback_checker |= 0x40;
	return 0;
}

ZTEST_SUITE(lwm2m_registry, NULL, NULL, NULL, NULL, NULL);

ZTEST(lwm2m_registry, test_object_creation_and_deletion)
{
	int ret;

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);

	ret = lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_registry, test_create_unknown_object)
{
	int ret;

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(49999, 0));
	zassert_equal(ret, -ENOENT);
}

ZTEST(lwm2m_registry, test_resource_buf)
{
	int ret;
	uint8_t resource_buf = 0;

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);

	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3303, 0, 6042), &resource_buf, sizeof(resource_buf),
				sizeof(resource_buf), 0);
	zassert_equal(ret, 0);

	ret = lwm2m_set_u8(&LWM2M_OBJ(3303, 0, 6042), 0x5A);
	zassert_equal(ret, 0);

	zassert_equal(resource_buf, 0x5A);

	ret = lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_registry, test_unknown_res)
{
	int ret;
	uint8_t resource_buf = 0;

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);

	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3303, 0, 49999), &resource_buf, sizeof(resource_buf),
				sizeof(resource_buf), 0);
	zassert_equal(ret, -ENOENT);

	ret = lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_registry, test_get_res_inst)
{
	zassert_is_null(lwm2m_engine_get_res_inst(&LWM2M_OBJ(3)));
	zassert_is_null(lwm2m_engine_get_res_inst(&LWM2M_OBJ(3, 0)));
	zassert_is_null(lwm2m_engine_get_res_inst(&LWM2M_OBJ(3, 0, 11)));
	zassert_not_null(lwm2m_engine_get_res_inst(&LWM2M_OBJ(3, 0, 11, 0)));
}

ZTEST(lwm2m_registry, test_get_set)
{
	bool b = true;
	uint8_t opaque[] = {0xde, 0xad, 0xbe, 0xff, 0, 0};
	char string[] = "Hello";
	uint8_t u8 = 8;
	int8_t s8 = -8;
	uint16_t u16 = 16;
	int16_t s16 = -16;
	uint32_t u32 = 32;
	int32_t s32 = -32;
	int64_t s64 = -64;
	time_t t = 1687949519;
	double d = 3.1415;
	double d2;
	struct lwm2m_objlnk objl = {.obj_id = 1, .obj_inst = 2};
	uint16_t l = sizeof(opaque);

	zassert_equal(lwm2m_set_bool(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_BOOL), b), 0);
	zassert_equal(lwm2m_set_opaque(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_OPAQUE), opaque, l), 0);
	zassert_equal(lwm2m_set_string(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_STRING), string), 0);
	zassert_equal(lwm2m_set_u8(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_U8), u8), 0);
	zassert_equal(lwm2m_set_s8(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S8), s8), 0);
	zassert_equal(lwm2m_set_u16(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_U16), u16), 0);
	zassert_equal(lwm2m_set_s16(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S16), s16), 0);
	zassert_equal(lwm2m_set_u32(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_U32), u32), 0);
	zassert_equal(lwm2m_set_s32(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S32), s32), 0);
	zassert_equal(lwm2m_set_s64(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S64), s64), 0);
	zassert_equal(lwm2m_set_time(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_TIME), t), 0);
	zassert_equal(lwm2m_set_f64(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_FLOAT), d), 0);
	zassert_equal(lwm2m_set_objlnk(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_OBJLNK), &objl), 0);

	zassert_equal(lwm2m_get_bool(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_BOOL), &b), 0);
	zassert_equal(lwm2m_get_opaque(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_OPAQUE), &opaque, l), 0);
	zassert_equal(lwm2m_get_string(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_STRING), &string, l), 0);
	zassert_equal(lwm2m_get_u8(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_U8), &u8), 0);
	zassert_equal(lwm2m_get_s8(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S8), &s8), 0);
	zassert_equal(lwm2m_get_u16(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_U16), &u16), 0);
	zassert_equal(lwm2m_get_s16(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S16), &s16), 0);
	zassert_equal(lwm2m_get_u32(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_U32), &u32), 0);
	zassert_equal(lwm2m_get_s32(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S32), &s32), 0);
	zassert_equal(lwm2m_get_s64(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_S64), &s64), 0);
	zassert_equal(lwm2m_get_time(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_TIME), &t), 0);
	zassert_equal(lwm2m_get_f64(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_FLOAT), &d2), 0);
	zassert_equal(lwm2m_get_objlnk(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_OBJLNK), &objl), 0);

	zassert_equal(b, true);
	zassert_equal(memcmp(opaque, &(uint8_t[6]) {0xde, 0xad, 0xbe, 0xff, 0, 0}, l), 0);
	zassert_equal(strcmp(string, "Hello"), 0);
	zassert_equal(u8, 8);
	zassert_equal(s8, -8);
	zassert_equal(u16, 16);
	zassert_equal(s16, -16);
	zassert_equal(u32, 32);
	zassert_equal(s32, -32);
	zassert_equal(s64, -64);
	zassert_equal(t, 1687949519);
	zassert_equal(d, d2);
	zassert_equal(
		memcmp(&objl, &(struct lwm2m_objlnk){.obj_id = 1, .obj_inst = 2}, sizeof(objl)), 0);

	zassert_equal(lwm2m_set_res_data_len(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_STRING), 0), 0);
	char buf[10];

	zassert_equal(
		lwm2m_get_string(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_STRING), buf, sizeof(buf)), 0);
	zassert_equal(strlen(buf), 0);
}

ZTEST(lwm2m_registry, test_temp_sensor)
{
	int ret;
	uint8_t u8_buf = 0;
	time_t time_buf = 0;
	double dbl_buf = 0;
	char char_buf[10];

	uint8_t u8_getbuf = 0;
	time_t time_getbuf = 0;
	double dbl_getbuf = 0;
	char char_getbuf[10];

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);

	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3303, 0, 6042), &u8_buf, sizeof(u8_buf),
				sizeof(u8_buf), 0);
	zassert_equal(ret, 0);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3303, 0, 5518), &time_buf, sizeof(time_buf),
				sizeof(time_buf), 0);
	zassert_equal(ret, 0);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3303, 0, 5601), &dbl_buf, sizeof(dbl_buf),
				sizeof(dbl_buf), 0);
	zassert_equal(ret, 0);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3303, 0, 5701), &char_buf, sizeof(char_buf),
				sizeof(char_buf), 0);
	zassert_equal(ret, 0);

	ret = lwm2m_set_u8(&LWM2M_OBJ(3303, 0, 6042), 0x5A);
	zassert_equal(ret, 0);
	ret = lwm2m_set_time(&LWM2M_OBJ(3303, 0, 5518), 1674118825);
	zassert_equal(ret, 0);
	ret = lwm2m_set_f64(&LWM2M_OBJ(3303, 0, 5601), 5.89);
	zassert_equal(ret, 0);
	ret = lwm2m_set_string(&LWM2M_OBJ(3303, 0, 5701), "test");
	zassert_equal(ret, 0);

	zassert_equal(u8_buf, 0x5A);
	zassert_equal(time_buf, 1674118825);
	zassert_within(dbl_buf, 5.89, 0.01);
	zassert_equal(strncmp(char_buf, "test", 10), 0);

	ret = lwm2m_get_u8(&LWM2M_OBJ(3303, 0, 6042), &u8_getbuf);
	zassert_equal(ret, 0);
	ret = lwm2m_get_time(&LWM2M_OBJ(3303, 0, 5518), &time_getbuf);
	zassert_equal(ret, 0);
	ret = lwm2m_get_f64(&LWM2M_OBJ(3303, 0, 5601), &dbl_getbuf);
	zassert_equal(ret, 0);
	ret = lwm2m_get_string(&LWM2M_OBJ(3303, 0, 5701), &char_getbuf, 10);
	zassert_equal(ret, 0);

	zassert_equal(u8_buf, u8_getbuf);
	zassert_equal(time_buf, time_getbuf);
	zassert_within(dbl_buf, dbl_getbuf, 0.01);
	zassert_equal(strncmp(char_buf, char_getbuf, 10), 0);

	ret = lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_registry, test_resource_instance_creation_and_deletion)
{
	int ret;

	ret = lwm2m_create_res_inst(&LWM2M_OBJ(4, 0, 1, 0));
	zassert_equal(ret, 0);

	ret = lwm2m_delete_res_inst(&LWM2M_OBJ(4, 0, 1, 0));
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_registry, test_resource_instance_strings)
{
	int ret;
	char buf[256] = {0};
	static const char string_a[] = "Hello";
	static const char string_b[] = "World";
	struct lwm2m_obj_path path_a = LWM2M_OBJ(16, 0, 0, 0);
	struct lwm2m_obj_path path_b = LWM2M_OBJ(16, 0, 0, 1);

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(16, 0));
	zassert_equal(ret, 0);

	ret = lwm2m_create_res_inst(&path_a);
	zassert_equal(ret, 0);

	ret = lwm2m_create_res_inst(&path_b);
	zassert_equal(ret, 0);

	ret = lwm2m_set_string(&path_a, string_a);
	zassert_equal(ret, 0);

	ret = lwm2m_set_string(&path_b, string_b);
	zassert_equal(ret, 0);

	ret = lwm2m_get_string(&path_a, buf, sizeof(buf));
	zassert_equal(ret, 0);
	zassert_equal(0, memcmp(buf, string_a, sizeof(string_a)));

	ret = lwm2m_get_string(&path_b, buf, sizeof(buf));
	zassert_equal(ret, 0);
	zassert_equal(0, memcmp(buf, string_b, sizeof(string_b)));

	ret = lwm2m_delete_object_inst(&LWM2M_OBJ(16, 0));
	zassert_equal(ret, 0);
}

ZTEST(lwm2m_registry, test_callbacks)
{
	int ret;
	double sensor_val;
	struct lwm2m_engine_res *exec_res;

	callback_checker = 0;
	ret = lwm2m_register_create_callback(3303, obj_create_cb);
	zassert_equal(ret, 0);
	lwm2m_register_delete_callback(3303, obj_delete_cb);
	zassert_equal(ret, 0);

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x10);

	ret = lwm2m_register_exec_callback(&LWM2M_OBJ(3303, 0, 5605), exec_cb);
	zassert_equal(ret, 0);
	ret = lwm2m_register_read_callback(&LWM2M_OBJ(3303, 0, 5700), read_cb);
	zassert_equal(ret, 0);
	ret = lwm2m_register_validate_callback(&LWM2M_OBJ(3303, 0, 5701), validate_cb);
	zassert_equal(ret, 0);
	ret = lwm2m_register_pre_write_callback(&LWM2M_OBJ(3303, 0, 5701), pre_write_cb);
	zassert_equal(ret, 0);
	ret = lwm2m_register_post_write_callback(&LWM2M_OBJ(3303, 0, 5701), post_write_cb);
	zassert_equal(ret, 0);

	exec_res = lwm2m_engine_get_res(&LWM2M_OBJ(3303, 0, 5605));
	exec_res->execute_cb(0, 0, 0);

	ret = lwm2m_set_string(&LWM2M_OBJ(3303, 0, 5701), "test");
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x5B);

	ret = lwm2m_get_f64(&LWM2M_OBJ(3303, 0, 5700), &sensor_val);
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x5F);

	ret = lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 0));
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x7F);
}

ZTEST(lwm2m_registry, test_strings)
{
	int ret;
	char buf[256] = {0};
	struct lwm2m_obj_path path = LWM2M_OBJ(0, 0, 0);
	static const char uri[] = "coap://127.0.0.1";
	uint16_t len;
	uint8_t *p;

	ret = lwm2m_get_res_buf(&path, (void **)&p, &len, NULL, NULL);
	zassert_equal(ret, 0);
	memset(p, 0xff, len); /* Pre-fill buffer to check */

	/* Handle strings in string resources */
	ret = lwm2m_set_string(&path, uri);
	zassert_equal(ret, 0);
	ret = lwm2m_get_res_buf(&path, (void **)&p, NULL, &len, NULL);
	zassert_equal(ret, 0);
	zassert_equal(len, sizeof(uri));
	zassert_equal(p[len - 1], '\0'); /* string terminator in buffer */
	zassert_equal(p[len], 0xff);

	ret = lwm2m_get_string(&path, buf, sizeof(buf));
	zassert_equal(ret, 0);
	zassert_equal(memcmp(uri, buf, sizeof(uri)), 0);
	ret = lwm2m_get_string(&path, buf, sizeof(uri));
	zassert_equal(ret, 0);
	ret = lwm2m_get_string(&path, buf, strlen(uri));
	zassert_equal(ret, -ENOMEM);

	/* Handle strings in opaque resources (no terminator) */
	path = LWM2M_OBJ(0, 0, 3);
	ret = lwm2m_get_res_buf(&path, (void **)&p, &len, NULL, NULL);
	zassert_equal(ret, 0);
	memset(p, 0xff, len); /* Pre-fill buffer to check */

	ret = lwm2m_set_string(&path, uri);
	zassert_equal(ret, 0);
	ret = lwm2m_get_res_buf(&path, (void **)&p, NULL, &len, NULL);
	zassert_equal(ret, 0);
	zassert_equal(len, strlen(uri)); /* No terminator counted in data length */
	zassert_equal(p[len - 1], '1'); /* Last character in buffer is not terminator */
	zassert_equal(p[len], 0xff);
	memset(buf, 0xff, sizeof(buf));
	ret = lwm2m_get_string(&path, buf, sizeof(buf)); /* get_string ensures termination */
	zassert_equal(ret, 0);
	zassert_equal(memcmp(uri, buf, sizeof(uri)), 0);
	ret = lwm2m_get_string(&path, buf, sizeof(uri));
	zassert_equal(ret, 0);
	ret = lwm2m_get_string(&path, buf, strlen(uri));
	zassert_equal(ret, -ENOMEM);
	/* Corner case: we request exactly as much is stored in opaque resource, */
	/* but because we request as a string, it must have room for terminator. */
	ret = lwm2m_get_string(&path, buf, len);
	zassert_equal(ret, -ENOMEM);
}

#if CONFIG_TEST_DEPRECATED
/* Don't test deprecated functions on Twister builds */
ZTEST(lwm2m_registry, test_deprecated_functions)
{
	bool b = true;
	uint8_t opaque[] = {0xde, 0xad, 0xbe, 0xff, 0, 0};
	char string[] = "Hello";
	uint8_t u8 = 8;
	int8_t s8 = -8;
	uint16_t u16 = 16;
	int16_t s16 = -16;
	uint32_t u32 = 32;
	int32_t s32 = -32;
	int64_t s64 = -64;
	time_t t = 1687949519;
	double d = 3.1415;
	double d2;
	struct lwm2m_objlnk objl = {.obj_id = 1, .obj_inst = 2};
	uint16_t l = sizeof(opaque);

	zassert_equal(lwm2m_engine_set_bool("32768/0/" STRINGIFY(LWM2M_RES_TYPE_BOOL), b), 0);
	zassert_equal(
		lwm2m_engine_set_opaque("32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), opaque, l), 0);
	zassert_equal(lwm2m_engine_set_string("32768/0/" STRINGIFY(LWM2M_RES_TYPE_STRING), string),
					      0);
	zassert_equal(lwm2m_engine_set_u8("32768/0/" STRINGIFY(LWM2M_RES_TYPE_U8), u8), 0);
	zassert_equal(lwm2m_engine_set_s8("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S8), s8), 0);
	zassert_equal(lwm2m_engine_set_u16("32768/0/" STRINGIFY(LWM2M_RES_TYPE_U16), u16), 0);
	zassert_equal(lwm2m_engine_set_s16("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S16), s16), 0);
	zassert_equal(lwm2m_engine_set_u32("32768/0/" STRINGIFY(LWM2M_RES_TYPE_U32), u32), 0);
	zassert_equal(lwm2m_engine_set_s32("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S32), s32), 0);
	zassert_equal(lwm2m_engine_set_s64("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S64), s64), 0);
	zassert_equal(lwm2m_engine_set_time("32768/0/" STRINGIFY(LWM2M_RES_TYPE_TIME), t), 0);
	zassert_equal(lwm2m_engine_set_float("32768/0/" STRINGIFY(LWM2M_RES_TYPE_FLOAT), &d), 0);
	zassert_equal(lwm2m_engine_set_objlnk("32768/0/" STRINGIFY(LWM2M_RES_TYPE_OBJLNK), &objl),
					      0);
	zassert_equal(lwm2m_engine_get_bool("32768/0/" STRINGIFY(LWM2M_RES_TYPE_BOOL), &b), 0);
	zassert_equal(lwm2m_engine_get_opaque(
		"32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), &opaque, l), 0);
	zassert_equal(lwm2m_engine_get_string(
		"32768/0/" STRINGIFY(LWM2M_RES_TYPE_STRING), &string, l), 0);
	zassert_equal(lwm2m_engine_get_u8("32768/0/" STRINGIFY(LWM2M_RES_TYPE_U8), &u8), 0);
	zassert_equal(lwm2m_engine_get_s8("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S8), &s8), 0);
	zassert_equal(lwm2m_engine_get_u16("32768/0/" STRINGIFY(LWM2M_RES_TYPE_U16), &u16), 0);
	zassert_equal(lwm2m_engine_get_s16("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S16), &s16), 0);
	zassert_equal(lwm2m_engine_get_u32("32768/0/" STRINGIFY(LWM2M_RES_TYPE_U32), &u32), 0);
	zassert_equal(lwm2m_engine_get_s32("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S32), &s32), 0);
	zassert_equal(lwm2m_engine_get_s64("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S64), &s64), 0);
	zassert_equal(lwm2m_engine_get_time("32768/0/" STRINGIFY(LWM2M_RES_TYPE_TIME), &t), 0);
	zassert_equal(lwm2m_engine_get_float("32768/0/" STRINGIFY(LWM2M_RES_TYPE_FLOAT), &d2), 0);
	zassert_equal(lwm2m_engine_get_objlnk("32768/0/" STRINGIFY(LWM2M_RES_TYPE_OBJLNK), &objl),
					      0);

	zassert_equal(b, true);
	zassert_equal(memcmp(opaque, &(uint8_t[6]) {0xde, 0xad, 0xbe, 0xff, 0, 0}, l), 0);
	zassert_equal(strcmp(string, "Hello"), 0);
	zassert_equal(u8, 8);
	zassert_equal(s8, -8);
	zassert_equal(u16, 16);
	zassert_equal(s16, -16);
	zassert_equal(u32, 32);
	zassert_equal(s32, -32);
	zassert_equal(s64, -64);
	zassert_equal(t, 1687949519);
	zassert_equal(d, d2);
	zassert_equal(
		memcmp(&objl, &(struct lwm2m_objlnk){.obj_id = 1, .obj_inst = 2}, sizeof(objl)), 0);

	uint64_t u64 = 0xc0ffee;

	zassert_equal(lwm2m_engine_set_u64("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S64), u64), 0);
	zassert_equal(lwm2m_engine_get_u64("32768/0/" STRINGIFY(LWM2M_RES_TYPE_S64), &u64), 0);
	zassert_equal(u64, 0xc0ffee);

	zassert_equal(lwm2m_engine_create_obj_inst("32768/1"), -ENOMEM);
	zassert_equal(lwm2m_engine_delete_obj_inst("32768/1"), -ENOENT);

	void *o_ptr;
	uint16_t o_len;

	lwm2m_get_res_buf(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_OPAQUE), &o_ptr, &o_len, NULL, NULL);

	zassert_equal(lwm2m_engine_set_res_buf(
		"32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), opaque, sizeof(opaque), 6, 0), 0);
	void *p;
	uint16_t len;

	zassert_equal(lwm2m_engine_get_res_buf(
		"32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), &p, NULL, &len, NULL), 0);
	zassert_true(p == opaque);
	zassert_equal(len, 6);

	zassert_equal(lwm2m_engine_set_res_data(
		"32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), string, sizeof(string), 0), 0);
	lwm2m_engine_get_res_data("32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), &p, &len, NULL);
	zassert_true(p == string);
	zassert_equal(len, sizeof(string));
	zassert_equal(lwm2m_engine_set_res_data_len("32768/0/" STRINGIFY(LWM2M_RES_TYPE_OPAQUE), 0),
						    0);

	lwm2m_set_res_buf(&LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_OPAQUE), o_ptr, o_len, 0, 0);
}
#endif

ZTEST(lwm2m_registry, test_lock_unlock)
{
	/* Should be recursive mutex and should not block */
	lwm2m_registry_lock();
	lwm2m_registry_lock();
	lwm2m_registry_unlock();
	lwm2m_registry_unlock();
}

ZTEST(lwm2m_registry, test_resource_wrappers)
{
	zassert_not_null(lwm2m_engine_obj_list());
	zassert_not_null(lwm2m_engine_obj_inst_list());
}

ZTEST(lwm2m_registry, test_unregister_obj)
{

	struct lwm2m_obj_path none = {0};
	struct lwm2m_engine_obj *obj;

	zassert_is_null(lwm2m_engine_get_obj(&none));

	obj = lwm2m_engine_get_obj(&LWM2M_OBJ(1));

	zassert_not_null(obj);
	lwm2m_unregister_obj(obj);
	zassert_is_null(lwm2m_engine_get_obj(&LWM2M_OBJ(1)));
}

ZTEST(lwm2m_registry, test_next_engine_obj_inst)
{
	zassert_equal(lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0)), 0);
	zassert_equal(lwm2m_create_object_inst(&LWM2M_OBJ(3303, 1)), 0);

	struct lwm2m_engine_obj_inst *oi = lwm2m_engine_get_obj_inst(&LWM2M_OBJ(3303, 1));

	zassert_not_null(oi);

	zassert_not_null(next_engine_obj_inst(3303, 0));
	zassert_equal(oi, next_engine_obj_inst(3303, 0));
	zassert_is_null(next_engine_obj_inst(3303, 1));

	zassert_equal(lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 0)), 0);
	zassert_equal(lwm2m_delete_object_inst(&LWM2M_OBJ(3303, 1)), 0);
	zassert_is_null(lwm2m_engine_get_obj_inst(&LWM2M_OBJ(3303, 1)));
}

ZTEST(lwm2m_registry, test_null_strings)
{
	int ret;
	char buf[256] = {0};
	struct lwm2m_obj_path path = LWM2M_OBJ(0, 0, 0);

	ret = lwm2m_register_post_write_callback(&path, post_write_cb);
	zassert_equal(ret, 0);

	callback_checker = 0;
	ret = lwm2m_set_string(&path, "string");
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x02);
	ret = lwm2m_get_string(&path, buf, sizeof(buf));
	zassert_equal(ret, 0);
	zassert_equal(strlen(buf), strlen("string"));

	callback_checker = 0;
	ret = lwm2m_set_string(&path, "");
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x02);
	ret = lwm2m_get_string(&path, buf, sizeof(buf));
	zassert_equal(ret, 0);
	zassert_equal(strlen(buf), 0);

	callback_checker = 0;
	ret = lwm2m_set_opaque(&path, NULL, 0);
	zassert_equal(ret, 0);
	zassert_equal(callback_checker, 0x02);
	ret = lwm2m_get_string(&path, buf, sizeof(buf));
	zassert_equal(ret, 0);
	zassert_equal(strlen(buf), 0);
}

ZTEST(lwm2m_registry, test_obj_version)
{

	zassert_false(lwm2m_engine_shall_report_obj_version(lwm2m_engine_get_obj(&LWM2M_OBJ(0))));
	zassert_false(
		lwm2m_engine_shall_report_obj_version(lwm2m_engine_get_obj(&LWM2M_OBJ(32768))));
	zassert_true(lwm2m_engine_shall_report_obj_version(lwm2m_engine_get_obj(&LWM2M_OBJ(3303))));
}

ZTEST(lwm2m_registry, test_resource_cache)
{
	struct lwm2m_obj_path path = LWM2M_OBJ(32768, 0, LWM2M_RES_TYPE_BOOL);
	struct lwm2m_time_series_elem e;

	/* Resource cache is turned off */
	zassert_is_null(lwm2m_cache_entry_get_by_object(&path));
	zassert_equal(lwm2m_enable_cache(&path, &e, 1), -ENOTSUP);
	/* deprecated */
	/* zassert_equal(lwm2m_engine_enable_cache("32768/0/1", &e, 1), -ENOTSUP); */
	zassert_false(lwm2m_cache_write(NULL, NULL));
	zassert_false(lwm2m_cache_read(NULL, NULL));
	zassert_equal(lwm2m_cache_size(NULL), 0);
}
