/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TODO:
 * - Configurable CURRENT_TIME notification delay
 */

#define LOG_MODULE_NAME net_lwm2m_obj_device
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define DEVICE_VERSION_MAJOR 1
#define DEVICE_VERSION_MINOR 0

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

#ifdef CONFIG_LWM2M_DEVICE_EXT_DEV_INFO_MAX
#define DEVICE_EXT_DEV_INFO_MAX CONFIG_LWM2M_DEVICE_EXT_DEV_INFO_MAX
#else
#define DEVICE_EXT_DEV_INFO_MAX	1
#endif

#define DEVICE_STRING_SHORT	8

#define DEVICE_SERVICE_INTERVAL_MS (MSEC_PER_SEC * 10)

/*
 * Calculate resource instances as follows:
 * start with DEVICE_MAX_ID
 * subtract EXEC resources (3)
 * subtract MULTI resources because their counts include 0 resource (5)
 * add 3x DEVICE_PWRSRC_MAX for POWER SOURCES resource instances
 * add DEVICE_ERROR_CODE_MAX for ERROR CODE resource instances
 * add DEVICE_EXT_DEV_INFO_MAX for EXT DEV INFO  resource instances
 */
#define RESOURCE_INSTANCE_COUNT	(DEVICE_MAX_ID - 3 - 5 + \
				 DEVICE_PWRSRC_MAX*3 + DEVICE_ERROR_CODE_MAX + \
				 DEVICE_EXT_DEV_INFO_MAX)

/* resource state variables */
static uint8_t  error_code_list[DEVICE_ERROR_CODE_MAX] = { LWM2M_DEVICE_ERROR_NONE };
static time_t time_temp;
static time_t time_offset;
static uint8_t  binding_mode[DEVICE_STRING_SHORT];

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
	OBJ_FIELD_DATA(DEVICE_EXT_DEV_INFO_ID, R_OPT, OBJLNK)
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[DEVICE_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

/* save error code resource instance point so we can easily clear later */
static struct lwm2m_engine_res_inst *error_code_ri;

#define SETTINGS_SUBTREE_LWM2M_OBJ_DEVICE "lwm2m_obj_dev"
#define ERROR_LIST_KEY "err"

/* callbacks */

static void reset_error_list(void)
{
	int i;

	/* "delete" error codes */
	for (i = 0; i < DEVICE_ERROR_CODE_MAX; i++) {
		error_code_list[i] = LWM2M_DEVICE_ERROR_NONE;
		error_code_ri[i].res_inst_id = RES_INSTANCE_NOT_CREATED;
	}

	/* Default error code indicating no error */
	error_code_ri[0].res_inst_id = 0;
}

static int reset_error_list_cb(uint16_t obj_inst_id,
			       uint8_t *args, uint16_t args_len)
{
	int ret = 0;

	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	reset_error_list();

	lwm2m_notify_observer(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_ERROR_CODE_ID);

	if (IS_ENABLED(CONFIG_LWM2M_DEVICE_ERROR_CODE_SETTINGS)) {
		ret = settings_delete(SETTINGS_SUBTREE_LWM2M_OBJ_DEVICE "/" ERROR_LIST_KEY);
		if (ret != 0) {
			LOG_ERR("Couldn't save error list: %d", ret);
		}
	}

	return ret;
}

static void *current_time_read_cb(uint16_t obj_inst_id, uint16_t res_id,
				  uint16_t res_inst_id, size_t *data_len)
{
	time_temp = time_offset + k_uptime_seconds();
	*data_len = sizeof(time_temp);

	return &time_temp;
}

static void *current_time_pre_write_cb(uint16_t obj_inst_id, uint16_t res_id,
				       uint16_t res_inst_id, size_t *data_len)
{
	*data_len = sizeof(time_temp);
	return &time_temp;
}

static int current_time_post_write_cb(uint16_t obj_inst_id, uint16_t res_id,
				      uint16_t res_inst_id, uint8_t *data,
				      uint16_t data_len, bool last_block,
				      size_t total_size, size_t offset)
{
	if (data_len == 4U) {
		time_offset = *(uint32_t *)data - k_uptime_seconds();
		return 0;
	} else if (data_len == 8U) {
		time_offset = *(time_t *)data - (time_t)k_uptime_seconds();
		return 0;
	}

	LOG_ERR("unknown size %u", data_len);
	return -EINVAL;
}

/* error code function */
int lwm2m_device_add_err(uint8_t error_code)
{
	int ret = 0;
	int i;

	for (i = 0; i < DEVICE_ERROR_CODE_MAX; i++) {
		if (error_code_list[i] == LWM2M_DEVICE_ERROR_NONE) {
			break;
		}

		/* No duplicate error codes allowed */
		if (error_code_list[i] == error_code) {
			return 0;
		}
	}

	if (i >= DEVICE_ERROR_CODE_MAX) {
		return -ENOMEM;
	}

	error_code_list[i] = error_code;
	error_code_ri[i].res_inst_id = i;
	lwm2m_notify_observer(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_ERROR_CODE_ID);

	if (IS_ENABLED(CONFIG_LWM2M_DEVICE_ERROR_CODE_SETTINGS)) {
		ret = settings_save_one(SETTINGS_SUBTREE_LWM2M_OBJ_DEVICE "/" ERROR_LIST_KEY,
					error_code_list, i + 1);
		if (ret != 0) {
			LOG_ERR("Couldn't save error list: %d", ret);
		}
	}

	return ret;
}

static void device_periodic_service(struct k_work *work)
{
	lwm2m_notify_observer(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_CURRENT_TIME_ID);
}

int lwm2m_update_device_service_period(uint32_t period_ms)
{
	return lwm2m_engine_update_service_period(device_periodic_service, period_ms);
}

static int lwm2m_obj_device_settings_set(const char *name, size_t len,
					 settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;
	int i;

	if (IS_ENABLED(CONFIG_LWM2M_DEVICE_ERROR_CODE_SETTINGS)) {
		if (settings_name_steq(name, ERROR_LIST_KEY, &next) && !next) {
			if (len > sizeof(error_code_list)) {
				LOG_ERR("Error code list too large: %zu", len);
				return -EINVAL;
			}

			rc = read_cb(cb_arg, error_code_list, sizeof(error_code_list));
			if (rc == 0) {
				reset_error_list();
				return 0;
			} else if (rc > 0) {
				for (i = 0; i < ARRAY_SIZE(error_code_list); i++) {
					if (i < rc) {
						error_code_ri[i].res_inst_id = i;
					} else {
						/* Reset remaining error code instances */
						error_code_list[i] = LWM2M_DEVICE_ERROR_NONE;
						error_code_ri[i].res_inst_id =
							RES_INSTANCE_NOT_CREATED;
					}
				}
				return 0;
			}

			LOG_ERR("Error code list read failure: %d", rc);

			return rc;
		}
	}

	return -ENOENT;
}

static struct settings_handler lwm2m_obj_device_settings_handler = {
	.name = SETTINGS_SUBTREE_LWM2M_OBJ_DEVICE,
	.h_set = lwm2m_obj_device_settings_set,
};

static struct lwm2m_engine_obj_inst *device_create(uint16_t obj_inst_id)
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
	INIT_OBJ_RES_OPT(DEVICE_CURRENT_TIME_ID, res, i, res_inst, j, 1, false,
			 true, current_time_read_cb, current_time_pre_write_cb,
			 NULL, current_time_post_write_cb, NULL);
	INIT_OBJ_RES_OPTDATA(DEVICE_UTC_OFFSET_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_TIMEZONE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_DATA_LEN(DEVICE_SUPPORTED_BINDING_MODES_ID, res, i,
			  res_inst, j, binding_mode, DEVICE_STRING_SHORT, strlen(binding_mode) + 1);
	INIT_OBJ_RES_OPTDATA(DEVICE_TYPE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_HARDWARE_VERSION_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_SOFTWARE_VERSION_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_BATTERY_STATUS_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(DEVICE_MEMORY_TOTAL_ID, res, i, res_inst, j);
	INIT_OBJ_RES_MULTI_OPTDATA(DEVICE_EXT_DEV_INFO_ID, res, i, res_inst, j,
				   DEVICE_EXT_DEV_INFO_MAX, false);

	inst.resources = res;
	inst.resource_count = i;

	LOG_DBG("Create LWM2M device instance: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_device_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret;

	/* Set default values */
	time_offset = 0U;
	lwm2m_engine_get_binding(binding_mode);

	/* initialize the device field data */
	device.obj_id = LWM2M_OBJECT_DEVICE_ID;
	device.version_major = DEVICE_VERSION_MAJOR;
	device.version_minor = DEVICE_VERSION_MINOR;
	device.is_core = true;
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

	/* Ensure error list is reset if not loaded from settings */
	reset_error_list();

	/* Load error code resource instances */
	if (IS_ENABLED(CONFIG_LWM2M_DEVICE_ERROR_CODE_SETTINGS)) {
		ret = settings_register(&lwm2m_obj_device_settings_handler);
		if (ret == 0) {
			ret = settings_load_subtree(SETTINGS_SUBTREE_LWM2M_OBJ_DEVICE);
			if (ret != 0) {
				LOG_ERR("Settings load failed: %d", ret);
			}
		} else {
			LOG_ERR("Settings register failed: %d", ret);
		}
	}

	/* call device_periodic_service() every 10 seconds */
	ret = lwm2m_engine_add_service(device_periodic_service,
				       DEVICE_SERVICE_INTERVAL_MS);

	return ret;
}

LWM2M_CORE_INIT(lwm2m_device_init);
