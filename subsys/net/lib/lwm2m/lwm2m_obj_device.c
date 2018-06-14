/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TODO:
 * - Implement UTC_OFFSET & TIMEZONE
 * - Configurable CURRENT_TIME notification delay
 */

#define SYS_LOG_DOMAIN "lwm2m_obj_device"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <string.h>
#include <stdio.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* Device resource IDs */
#define DEVICE_MANUFACTURER_ID			0
#define DEVICE_MODEL_NUMBER_ID			1
#define DEVICE_SERIAL_NUMBER_ID			2
#define DEVICE_FIRMWARE_VERSION_ID		3
#define DEVICE_REBOOT_ID			4
#define DEVICE_FACTORY_DEFAULT_ID		5
#define DEVICE_AVAILABLE_POWER_SOURCES_ID	6
#define DEVICE_POWER_SOURCE_VOLTAGE_ID		7
#define DEVICE_POWER_SOURCE_CURRENT_ID		8
#define DEVICE_BATTERY_LEVEL_ID			9
#define DEVICE_MEMORY_FREE_ID			10
#define DEVICE_ERROR_CODE_ID			11
#define DEVICE_RESET_ERROR_CODE_ID		12
#define DEVICE_CURRENT_TIME_ID			13
#define DEVICE_UTC_OFFSET_ID			14
#define DEVICE_TIMEZONE_ID			15
#define DEVICE_SUPPORTED_BINDING_MODES_ID	16
#define DEVICE_TYPE_ID				17
#define DEVICE_HARDWARE_VERSION_ID		18
#define DEVICE_SOFTWARE_VERSION_ID		19
#define DEVICE_BATTERY_STATUS_ID		20
#define DEVICE_MEMORY_TOTAL_ID			21
#define DEVICE_EXT_DEV_INFO_ID			22

#define DEVICE_MAX_ID				23

#ifdef CONFIG_LWM2M_DEVICE_ERROR_CODE_MAX
#define DEVICE_ERROR_CODE_MAX	CONFIG_LWM2M_DEVICE_ERROR_CODE_MAX
#else
#define DEVICE_ERROR_CODE_MAX	10
#endif

#ifdef CONFIG_LWM2M_DEVICE_PWRSRC_MAX
#define DEVICE_PWRSRC_MAX	CONFIG_LWM2M_DEVICE_PWRSRC_MAX
#else
#define DEVICE_PWRSRC_MAX	5
#endif

#define DEVICE_STRING_SHORT	8

#define DEVICE_SERVICE_INTERVAL K_SECONDS(10)

/* resource state variables */
static s8_t  pwrsrc_available[DEVICE_PWRSRC_MAX];
static s32_t pwrsrc_voltage_mv[DEVICE_PWRSRC_MAX];
static s32_t pwrsrc_current_ma[DEVICE_PWRSRC_MAX];
static u8_t  battery_level;
static s32_t mem_free_kb;
static u8_t  error_code_list[DEVICE_ERROR_CODE_MAX];
static s32_t time_temp;
static u32_t time_offset;
static u8_t  binding_mode[DEVICE_STRING_SHORT];
static u8_t  battery_status;
static s32_t mem_total_kb;

static u8_t  pwrsrc_count;
static u8_t  error_code_count;

/* only 1 instance of device object exists */
static struct lwm2m_engine_obj device;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(DEVICE_MANUFACTURER_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_MODEL_NUMBER_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_SERIAL_NUMBER_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_FIRMWARE_VERSION_ID, R_OPT, STRING),
	OBJ_FIELD_EXECUTE_OPT(DEVICE_REBOOT_ID),
	OBJ_FIELD_EXECUTE_OPT(DEVICE_FACTORY_DEFAULT_ID),
	OBJ_FIELD(DEVICE_AVAILABLE_POWER_SOURCES_ID, R_OPT, U8,
		  DEVICE_PWRSRC_MAX),
	OBJ_FIELD(DEVICE_POWER_SOURCE_VOLTAGE_ID, R_OPT, S32,
		  DEVICE_PWRSRC_MAX),
	OBJ_FIELD(DEVICE_POWER_SOURCE_CURRENT_ID, R_OPT, S32,
		  DEVICE_PWRSRC_MAX),
	OBJ_FIELD_DATA(DEVICE_BATTERY_LEVEL_ID, R_OPT, U8),
	OBJ_FIELD_DATA(DEVICE_MEMORY_FREE_ID, R_OPT, S32),
	OBJ_FIELD(DEVICE_ERROR_CODE_ID, R, U8, DEVICE_ERROR_CODE_MAX),
	OBJ_FIELD_EXECUTE_OPT(DEVICE_RESET_ERROR_CODE_ID),
	OBJ_FIELD_DATA(DEVICE_CURRENT_TIME_ID, RW_OPT, TIME),
	OBJ_FIELD_DATA(DEVICE_UTC_OFFSET_ID, RW_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_TIMEZONE_ID, RW_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_SUPPORTED_BINDING_MODES_ID, R, STRING),
	OBJ_FIELD_DATA(DEVICE_TYPE_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_HARDWARE_VERSION_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_SOFTWARE_VERSION_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_BATTERY_STATUS_ID, R_OPT, U8),
	OBJ_FIELD_DATA(DEVICE_MEMORY_TOTAL_ID, R_OPT, S32),
	OBJ_FIELD(DEVICE_EXT_DEV_INFO_ID, R_OPT, S32, 0)
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res_inst res[DEVICE_MAX_ID];

/* callbacks */

static int reboot_cb(u16_t obj_inst_id)
{
	SYS_LOG_DBG("REBOOT");
	return -EPERM;
}

static int factory_default_cb(u16_t obj_inst_id)
{
	SYS_LOG_DBG("FACTORY_DEFAULT");
	return -EPERM;
}

static int reset_error_list_cb(u16_t obj_inst_id)
{
	error_code_count = 0;
	return 0;
}

static void *current_time_read_cb(u16_t obj_inst_id, size_t *data_len)
{
	time_temp = time_offset + (k_uptime_get() / 1000);
	*data_len = sizeof(time_temp);

	return &time_temp;
}

static void *current_time_pre_write_cb(u16_t obj_inst_id, size_t *data_len)
{
	*data_len = sizeof(time_temp);
	return &time_temp;
}

static int current_time_post_write_cb(u16_t obj_inst_id,
				 u8_t *data, u16_t data_len,
				 bool last_block, size_t total_size)
{
	if (data_len == 4) {
		time_offset = *(s32_t *)data - (s32_t)(k_uptime_get() / 1000);
		return 0;
	}

	SYS_LOG_ERR("unknown size %u", data_len);
	return -EINVAL;
}

/* special setter functions */

int lwm2m_device_add_pwrsrc(u8_t pwrsrc_type)
{
	int index;

	if (pwrsrc_type < 0 || pwrsrc_type >= LWM2M_DEVICE_PWR_SRC_TYPE_MAX) {
		SYS_LOG_ERR("power source id %d is invalid",
			    pwrsrc_type);
		return -EINVAL;
	}

	for (index = 0; index < DEVICE_PWRSRC_MAX; index++) {
		if (pwrsrc_available[index] < 0) {
			break;
		}
	}

	if (index >= DEVICE_PWRSRC_MAX) {
		return -ENOMEM;
	}

	pwrsrc_available[index] = pwrsrc_type;
	pwrsrc_voltage_mv[index] = 0;
	pwrsrc_current_ma[index] = 0;
	pwrsrc_count++;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0,
			DEVICE_AVAILABLE_POWER_SOURCES_ID);
	return index;
}

/*
 * TODO: this will disable the index, but current printing function expects
 * all indexes to be in order up to pwrsrc_count
 */
int lwm2m_device_remove_pwrsrc(int index)
{
	if (index < 0 || index >= DEVICE_PWRSRC_MAX) {
		SYS_LOG_ERR("index is out of range: %d", index);
		return -EINVAL;
	}

	if (pwrsrc_available[index] < 0) {
		SYS_LOG_ERR("Power source index %d isn't registered", index);
		return -EINVAL;
	}

	pwrsrc_available[index] = -1;
	pwrsrc_voltage_mv[index] = 0;
	pwrsrc_current_ma[index] = 0;
	pwrsrc_count--;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0,
			DEVICE_AVAILABLE_POWER_SOURCES_ID);
	return 0;
}

int lwm2m_device_set_pwrsrc_voltage_mv(int index, int voltage_mv)
{
	if (index < 0 || index >= DEVICE_PWRSRC_MAX) {
		SYS_LOG_ERR("index is out of range: %d", index);
		return -EINVAL;
	}

	if (pwrsrc_available[index] < 0) {
		SYS_LOG_ERR("Power source index %d isn't registered.", index);
		return -EINVAL;
	}

	pwrsrc_voltage_mv[index] = voltage_mv;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0,
			DEVICE_POWER_SOURCE_VOLTAGE_ID);
	return 0;
}

int lwm2m_device_set_pwrsrc_current_ma(int index, int current_ma)
{
	if (index < 0 || index >= DEVICE_PWRSRC_MAX) {
		SYS_LOG_ERR("index is out of range: %d", index);
		return -EINVAL;
	}

	if (pwrsrc_available[index] < 0) {
		SYS_LOG_ERR("Power source index %d isn't registered.", index);
		return -EINVAL;
	}

	pwrsrc_current_ma[index] = current_ma;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0,
			DEVICE_POWER_SOURCE_CURRENT_ID);
	return 0;
}

/* error code function */

int lwm2m_device_add_err(u8_t error_code)
{
	if (error_code_count < DEVICE_ERROR_CODE_MAX) {
		error_code_list[error_code_count] = error_code;
		error_code_count++;
		return 0;
	}

	return -ENOMEM;
}

static void device_periodic_service(void)
{
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_CURRENT_TIME_ID);
}

static struct lwm2m_engine_obj_inst *device_create(u16_t obj_inst_id)
{
	int i = 0;

	/* initialize instance resource data */
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_MANUFACTURER_ID);
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_MODEL_NUMBER_ID);
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_SERIAL_NUMBER_ID);
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_FIRMWARE_VERSION_ID);
	INIT_OBJ_RES_EXECUTE(res, i, DEVICE_REBOOT_ID, reboot_cb);
	INIT_OBJ_RES_EXECUTE(res, i, DEVICE_FACTORY_DEFAULT_ID,
			     factory_default_cb);
	INIT_OBJ_RES_MULTI_DATA(res, i, DEVICE_AVAILABLE_POWER_SOURCES_ID,
				&pwrsrc_count, pwrsrc_available,
				sizeof(*pwrsrc_available));
	INIT_OBJ_RES_MULTI_DATA(res, i, DEVICE_POWER_SOURCE_VOLTAGE_ID,
				&pwrsrc_count, pwrsrc_voltage_mv,
				sizeof(*pwrsrc_voltage_mv));
	INIT_OBJ_RES_MULTI_DATA(res, i, DEVICE_POWER_SOURCE_CURRENT_ID,
				&pwrsrc_count, pwrsrc_current_ma,
				sizeof(*pwrsrc_current_ma));
	INIT_OBJ_RES_DATA(res, i, DEVICE_BATTERY_LEVEL_ID,
			  &battery_level, sizeof(battery_level));
	INIT_OBJ_RES_DATA(res, i, DEVICE_MEMORY_FREE_ID,
			  &mem_free_kb, sizeof(mem_free_kb));
	INIT_OBJ_RES_MULTI_DATA(res, i, DEVICE_ERROR_CODE_ID,
				&error_code_count, error_code_list,
				sizeof(*error_code_list));
	INIT_OBJ_RES_EXECUTE(res, i, DEVICE_RESET_ERROR_CODE_ID,
			     reset_error_list_cb);
	INIT_OBJ_RES(res, i, DEVICE_CURRENT_TIME_ID, 0, NULL, 0,
		     current_time_read_cb, current_time_pre_write_cb,
		     current_time_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(res, i, DEVICE_SUPPORTED_BINDING_MODES_ID,
			  binding_mode, DEVICE_STRING_SHORT);
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_TYPE_ID);
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_HARDWARE_VERSION_ID);
	INIT_OBJ_RES_DUMMY(res, i, DEVICE_SOFTWARE_VERSION_ID);
	INIT_OBJ_RES_DATA(res, i, DEVICE_BATTERY_STATUS_ID,
			  &battery_status, sizeof(battery_status));
	INIT_OBJ_RES_DATA(res, i, DEVICE_MEMORY_TOTAL_ID,
			  &mem_total_kb, sizeof(mem_total_kb));

	inst.resources = res;
	inst.resource_count = i;
	SYS_LOG_DBG("Create LWM2M device instance: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_device_init(struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0, i;

	/* Set default values */
	time_offset = 0;
	mem_total_kb = 0;
	mem_free_kb = -1;
	pwrsrc_count = 0;
	error_code_count = 0;
	/* currently only support UDP binding mode (no SMS or Queue mode) */
	strcpy(binding_mode, "U");

	for (i = 0; i < DEVICE_PWRSRC_MAX; i++) {
		pwrsrc_available[i] = -1;
	}

	/* initialize the device field data */
	device.obj_id = LWM2M_OBJECT_DEVICE_ID;
	device.fields = fields;
	device.field_count = ARRAY_SIZE(fields);
	device.max_instance_count = 1;
	device.create_cb = device_create;
	lwm2m_register_obj(&device);

	/* auto create the only instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_DEVICE_ID, 0, &obj_inst);
	if (ret < 0) {
		SYS_LOG_DBG("Create LWM2M instance 0 error: %d", ret);
	}

	/* call device_periodic_service() every 10 seconds */
	ret = lwm2m_engine_add_service(device_periodic_service,
				       DEVICE_SERVICE_INTERVAL);
	return ret;
}

SYS_INIT(lwm2m_device_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
