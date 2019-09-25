/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TODO:
 * - Implement UTC_OFFSET & TIMEZONE
 * - Configurable CURRENT_TIME notification delay
 */

#define LOG_MODULE_NAME net_lwm2m_obj_device
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

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

/*
 * Calculate resource instances as follows:
 * start with DEVICE_MAX_ID
 * subtract EXEC resources (3)
 * subtract MULTI resources because their counts include 0 resource (4)
 * add 3x DEVICE_PWRSRC_MAX for POWER SOURCES resource instances
 * add DEVICE_ERROR_CODE_MAX for ERROR CODE resource instances
 */
#define RESOURCE_INSTANCE_COUNT	(DEVICE_MAX_ID - 3 - 4 + \
				DEVICE_PWRSRC_MAX*3 + DEVICE_ERROR_CODE_MAX)

/* resource state variables */
static u8_t  error_code_list[DEVICE_ERROR_CODE_MAX];
static s32_t time_temp;
static u32_t time_offset;
static u8_t  binding_mode[DEVICE_STRING_SHORT];

/* only 1 instance of device object exists */
static struct lwm2m_engine_obj device;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(DEVICE_MANUFACTURER_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_MODEL_NUMBER_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_SERIAL_NUMBER_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(DEVICE_FIRMWARE_VERSION_ID, R_OPT, STRING),
	OBJ_FIELD_EXECUTE_OPT(DEVICE_REBOOT_ID),
	OBJ_FIELD_EXECUTE_OPT(DEVICE_FACTORY_DEFAULT_ID),
	OBJ_FIELD_DATA(DEVICE_AVAILABLE_POWER_SOURCES_ID, R_OPT, U8),
	OBJ_FIELD_DATA(DEVICE_POWER_SOURCE_VOLTAGE_ID, R_OPT, S32),
	OBJ_FIELD_DATA(DEVICE_POWER_SOURCE_CURRENT_ID, R_OPT, S32),
	OBJ_FIELD_DATA(DEVICE_BATTERY_LEVEL_ID, R_OPT, U8),
	OBJ_FIELD_DATA(DEVICE_MEMORY_FREE_ID, R_OPT, S32),
	OBJ_FIELD_DATA(DEVICE_ERROR_CODE_ID, R, U8),
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
	OBJ_FIELD_DATA(DEVICE_EXT_DEV_INFO_ID, R_OPT, S32)
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[DEVICE_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

/* save error code resource instance point so we can easily clear later */
static struct lwm2m_engine_res_inst *error_code_ri;

/* callbacks */

static int reset_error_list_cb(u16_t obj_inst_id)
{
	int i;

	/* "delete" error codes */
	for (i = 0; i < DEVICE_ERROR_CODE_MAX; i++) {
		error_code_list[i] = 0;
		error_code_ri[i].res_inst_id = RES_INSTANCE_NOT_CREATED;
	}

	return 0;
}

static void *current_time_read_cb(u16_t obj_inst_id, u16_t res_id,
				  u16_t res_inst_id, size_t *data_len)
{
	time_temp = time_offset + (k_uptime_get() / 1000);
	*data_len = sizeof(time_temp);

	return &time_temp;
}

static void *current_time_pre_write_cb(u16_t obj_inst_id, u16_t res_id,
				       u16_t res_inst_id, size_t *data_len)
{
	*data_len = sizeof(time_temp);
	return &time_temp;
}

static int current_time_post_write_cb(u16_t obj_inst_id, u16_t res_id,
				      u16_t res_inst_id,
				      u8_t *data, u16_t data_len,
				      bool last_block, size_t total_size)
{
	if (data_len == 4U) {
		time_offset = *(s32_t *)data - (s32_t)(k_uptime_get() / 1000);
		return 0;
	}

	LOG_ERR("unknown size %u", data_len);
	return -EINVAL;
}

/* special setter functions */

__deprecated int lwm2m_device_add_pwrsrc(u8_t pwrsrc_type)
{
	int i;
	struct lwm2m_engine_res *res;

	if (pwrsrc_type >= LWM2M_DEVICE_PWR_SRC_TYPE_MAX) {
		LOG_ERR("power source id %d is invalid", pwrsrc_type);
		return -EINVAL;
	}

	i = lwm2m_engine_get_resource("3/0/6", &res);
	if (i < 0 || !res || !res->res_instances) {
		return -ENOENT;
	}

	for (i = 0; i < res->res_inst_count; i++) {
		if (res->res_instances[i].res_inst_id ==
				RES_INSTANCE_NOT_CREATED &&
		    res->res_instances[i].data_ptr != NULL) {
			break;
		}
	}

	if (i >= res->res_inst_count) {
		return -ENOMEM;
	}

	*(u8_t *)res->res_instances[i].data_ptr = pwrsrc_type;
	res->res_instances[i].res_inst_id = i;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0,
			DEVICE_AVAILABLE_POWER_SOURCES_ID);

	return 0;
}

/*
 * TODO: this will disable the index, but current printing function expects
 * all indexes to be in order up to pwrsrc_count
 */
__deprecated int lwm2m_device_remove_pwrsrc(int index)
{
	int ret;
	struct lwm2m_engine_res *res;

	ret = lwm2m_engine_get_resource("3/0/6", &res);
	if (ret < 0 || !res || !res->res_instances) {
		return -ENOENT;
	}

	if (index >= res->res_inst_count) {
		LOG_ERR("index %d is invalid", index);
		return -EINVAL;
	}

	if (res->res_instances[index].res_inst_id ==
			RES_INSTANCE_NOT_CREATED ||
	    !res->res_instances[index].data_ptr) {
		return -ENOMEM;
	}

	*(u8_t *)res->res_instances[index].data_ptr = 0;
	res->res_instances[index].res_inst_id = RES_INSTANCE_NOT_CREATED;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0,
			DEVICE_AVAILABLE_POWER_SOURCES_ID);

	return 0;
}

static int device_set_pwrsrc_value(char *pathstr, int index, s32_t value)
{
	int ret;
	struct lwm2m_engine_res *res;

	ret = lwm2m_engine_get_resource(pathstr, &res);
	if (ret < 0 || !res || !res->res_instances) {
		return -ENOENT;
	}

	if (index >= res->res_inst_count) {
		LOG_ERR("index %d is invalid", index);
		return -EINVAL;
	}

	if (res->res_instances[index].res_inst_id ==
			RES_INSTANCE_NOT_CREATED ||
	    !res->res_instances[index].data_ptr) {
		return -ENOMEM;
	}

	*(s32_t *)res->res_instances[index].data_ptr = value;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0, res->res_id);

	return 0;
}

__deprecated int lwm2m_device_set_pwrsrc_voltage_mv(int index, int voltage_mv)
{
	return device_set_pwrsrc_value("3/0/7", index, voltage_mv);
}

__deprecated int lwm2m_device_set_pwrsrc_current_ma(int index, int current_ma)
{
	return device_set_pwrsrc_value("3/0/8", index, current_ma);
}

/* error code function */

int lwm2m_device_add_err(u8_t error_code)
{
	int i;

	for (i = 0; i < DEVICE_ERROR_CODE_MAX; i++) {
		if (error_code_ri[i].res_inst_id == RES_INSTANCE_NOT_CREATED) {
			break;
		}
	}

	if (i >= DEVICE_ERROR_CODE_MAX) {
		return -ENOMEM;
	}

	error_code_list[i] = error_code;
	error_code_ri[i].res_inst_id = i;
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_ERROR_CODE_ID);

	return 0;
}

static void device_periodic_service(struct k_work *work)
{
	NOTIFY_OBSERVER(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_CURRENT_TIME_ID);
}

static struct lwm2m_engine_obj_inst *device_create(u16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPTDATA(DEVICE_MANUFACTURER_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_MODEL_NUMBER_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_SERIAL_NUMBER_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_FIRMWARE_VERSION_ID, res, i, res_inst, j);
	INIT_OBJ_RES_EXECUTE(DEVICE_REBOOT_ID, res, i, NULL);
	INIT_OBJ_RES_EXECUTE(DEVICE_FACTORY_DEFAULT_ID, res, i, NULL);
	INIT_OBJ_RES_MULTI_OPTDATA(DEVICE_AVAILABLE_POWER_SOURCES_ID, res, i,
				   res_inst, j, DEVICE_PWRSRC_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(DEVICE_POWER_SOURCE_VOLTAGE_ID, res, i,
				   res_inst, j, DEVICE_PWRSRC_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(DEVICE_POWER_SOURCE_CURRENT_ID, res, i,
				   res_inst, j, DEVICE_PWRSRC_MAX, false);
	INIT_OBJ_RES_OPTDATA(DEVICE_BATTERY_LEVEL_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_MEMORY_FREE_ID, res, i, res_inst, j);
	error_code_ri = &res_inst[j];
	INIT_OBJ_RES_MULTI_DATA(DEVICE_ERROR_CODE_ID, res, i,
				res_inst, j, DEVICE_ERROR_CODE_MAX, false,
				error_code_list, sizeof(*error_code_list));
	INIT_OBJ_RES_EXECUTE(DEVICE_RESET_ERROR_CODE_ID, res, i,
			     reset_error_list_cb);
	INIT_OBJ_RES_OPT(DEVICE_CURRENT_TIME_ID, res, i, res_inst, j, 1, true,
			 current_time_read_cb,
			 current_time_pre_write_cb,
			 current_time_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(DEVICE_SUPPORTED_BINDING_MODES_ID, res, i,
			  res_inst, j, binding_mode, DEVICE_STRING_SHORT);
	INIT_OBJ_RES_OPTDATA(DEVICE_TYPE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_HARDWARE_VERSION_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_SOFTWARE_VERSION_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_BATTERY_STATUS_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_MEMORY_TOTAL_ID, res, i, res_inst, j);

	inst.resources = res;
	inst.resource_count = i;

	LOG_DBG("Create LWM2M device instance: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_device_init(struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	/* Set default values */
	time_offset = 0U;
	/* currently only support UDP binding mode (no SMS or Queue mode) */
	strcpy(binding_mode, "U");

	/* initialize the device field data */
	device.obj_id = LWM2M_OBJECT_DEVICE_ID;
	device.fields = fields;
	device.field_count = ARRAY_SIZE(fields);
	device.max_instance_count = 1U;
	device.create_cb = device_create;
	lwm2m_register_obj(&device);

	/* auto create the only instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_DEVICE_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_DBG("Create LWM2M instance 0 error: %d", ret);
	}

	/* call device_periodic_service() every 10 seconds */
	ret = lwm2m_engine_add_service(device_periodic_service,
				       DEVICE_SERVICE_INTERVAL);
	return ret;
}

SYS_INIT(lwm2m_device_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
