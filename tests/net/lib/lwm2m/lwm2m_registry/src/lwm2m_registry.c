/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "lwm2m_engine.h"
#include "lwm2m_util.h"

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

/* Different objects employ different resources, test some of those*/
ZTEST(lwm2m_registry, test_connmon)
{
	int ret;
	uint16_t u16_buf = 0;
	uint32_t u32_buf = 0;
	int8_t s8_buf = 0;
	int32_t s32_buf = 0;

	uint16_t u16_getbuf = 0;
	uint32_t u32_getbuf = 0;
	int8_t s8_getbuf = 0;
	int32_t s32_getbuf = 0;

	ret = lwm2m_set_res_buf(&LWM2M_OBJ(4, 0, 9), &u16_buf, sizeof(u16_buf),
				sizeof(u16_buf), 0);
	zassert_equal(ret, 0);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(4, 0, 8), &u32_buf, sizeof(u32_buf),
				sizeof(u32_buf), 0);
	zassert_equal(ret, 0);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(4, 0, 2), &s8_buf, sizeof(s8_buf),
				sizeof(s8_buf), 0);
	zassert_equal(ret, 0);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(4, 0, 11), &s32_buf, sizeof(s32_buf),
				sizeof(s32_buf), 0);
	zassert_equal(ret, 0);

	ret = lwm2m_set_u16(&LWM2M_OBJ(4, 0, 9), 0x5A5A);
	zassert_equal(ret, 0);
	ret = lwm2m_set_u32(&LWM2M_OBJ(4, 0, 8), 0xDEADBEEF);
	zassert_equal(ret, 0);
	ret = lwm2m_set_s8(&LWM2M_OBJ(4, 0, 2), -5);
	zassert_equal(ret, 0);
	ret = lwm2m_set_s32(&LWM2M_OBJ(4, 0, 11), 0xCC00CC00);
	zassert_equal(ret, 0);

	zassert_equal(u16_buf, 0x5A5A);
	zassert_equal(u32_buf, 0xDEADBEEF);
	zassert_equal(s8_buf, -5);
	zassert_equal(s32_buf, 0xCC00CC00);

	ret = lwm2m_get_u16(&LWM2M_OBJ(4, 0, 9), &u16_getbuf);
	zassert_equal(ret, 0);
	ret = lwm2m_get_u32(&LWM2M_OBJ(4, 0, 8), &u32_getbuf);
	zassert_equal(ret, 0);
	ret = lwm2m_get_s8(&LWM2M_OBJ(4, 0, 2), &s8_getbuf);
	zassert_equal(ret, 0);
	ret = lwm2m_get_s32(&LWM2M_OBJ(4, 0, 11), &s32_getbuf);
	zassert_equal(ret, 0);

	zassert_equal(u16_buf, u16_getbuf);
	zassert_equal(u32_buf, u32_getbuf);
	zassert_equal(s8_buf, s8_getbuf);
	zassert_equal(s32_buf, s32_getbuf);
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
