/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Light Control object (3311):
 * https://github.com/IPSO-Alliance/pub/blob/master/docs/IPSO-Smart-Objects.pdf
 * Section: "16. IPSO Object: Light Control"
 */

#define LOG_MODULE_NAME net_ipso_light_control
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define LIGHT_VERSION_MAJOR 1
#define LIGHT_VERSION_MINOR 0

#define LIGHT_MAX_ID		8

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_LIGHT_CONTROL_INSTANCE_COUNT

#define LIGHT_STRING_SHORT	8
#define LIGHT_STRING_LONG       64

/*
 * Calculate resource instances as follows:
 * start with LIGHT_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(LIGHT_MAX_ID)

/* resource state variables */
static bool on_off_value[MAX_INSTANCE_COUNT];
static uint8_t dimmer_value[MAX_INSTANCE_COUNT];
static int32_t on_time_value[MAX_INSTANCE_COUNT];
static uint32_t on_time_offset[MAX_INSTANCE_COUNT];
static double cumulative_active_value[MAX_INSTANCE_COUNT];
static double power_factor_value[MAX_INSTANCE_COUNT];
static char colour[MAX_INSTANCE_COUNT][LIGHT_STRING_LONG];
static char units[MAX_INSTANCE_COUNT][LIGHT_STRING_SHORT];

static struct lwm2m_engine_obj light_control;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(ON_OFF_RID, RW, BOOL),
	OBJ_FIELD_DATA(DIMMER_RID, RW_OPT, U8),
	OBJ_FIELD_DATA(ON_TIME_RID, RW_OPT, S32),
	OBJ_FIELD_DATA(CUMULATIVE_ACTIVE_POWER_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(POWER_FACTOR_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(COLOUR_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(SENSOR_UNITS_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][LIGHT_MAX_ID];
static struct lwm2m_engine_res_inst
		res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static void *on_time_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			     size_t *data_len)
{
	int i;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (!inst[i].obj || inst[i].obj_inst_id != obj_inst_id) {
			continue;
		}

		if (on_off_value[i]) {
			on_time_value[i] = (k_uptime_get() / MSEC_PER_SEC) -
				on_time_offset[i];
		}

		*data_len = sizeof(on_time_value[i]);
		return &on_time_value[i];
	}

	return NULL;
}

static int on_time_post_write_cb(uint16_t obj_inst_id,
				 uint16_t res_id, uint16_t res_inst_id,
				 uint8_t *data, uint16_t data_len,
				 bool last_block, size_t total_size)
{
	int i;

	if (data_len != 4U) {
		LOG_ERR("unknown size %u", data_len);
		return -EINVAL;
	}

	int32_t counter = *(int32_t *) data;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (!inst[i].obj || inst[i].obj_inst_id != obj_inst_id) {
			continue;
		}

		if (counter == 0) {
			on_time_offset[i] =
				(int32_t)(k_uptime_get() / MSEC_PER_SEC);
		}

		return 0;
	}

	return -ENOENT;
}

static struct lwm2m_engine_obj_inst *light_control_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u", obj_inst_id);
			return NULL;
		}

		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		LOG_ERR("Can not create instance - no more room: %u",
			obj_inst_id);
		return NULL;
	}

	/* Set default values */
	on_off_value[avail] = false;
	dimmer_value[avail] = 0U;
	on_time_value[avail] = 0;
	on_time_offset[avail] = 0U;
	cumulative_active_value[avail] = 0;
	power_factor_value[avail] = 0;
	colour[avail][0] = '\0';
	units[avail][0] = '\0';

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(ON_OFF_RID, res[avail], i, res_inst[avail], j,
			  &on_off_value[avail], sizeof(*on_off_value));
	INIT_OBJ_RES_DATA(DIMMER_RID, res[avail], i, res_inst[avail], j,
			  &dimmer_value[avail], sizeof(*dimmer_value));
	INIT_OBJ_RES(ON_TIME_RID, res[avail], i, res_inst[avail], j, 1, false,
		     true, &on_time_value[avail], sizeof(*on_time_value),
		     on_time_read_cb, NULL, NULL, on_time_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(CUMULATIVE_ACTIVE_POWER_RID, res[avail], i,
			  res_inst[avail], j, &cumulative_active_value[avail],
			  sizeof(*cumulative_active_value));
	INIT_OBJ_RES_DATA(POWER_FACTOR_RID, res[avail], i, res_inst[avail], j,
			  &power_factor_value[avail],
			  sizeof(*power_factor_value));
	INIT_OBJ_RES_DATA(COLOUR_RID, res[avail], i, res_inst[avail], j,
			  colour[avail], LIGHT_STRING_LONG);
	INIT_OBJ_RES_DATA(SENSOR_UNITS_RID, res[avail], i, res_inst[avail], j,
			  units[avail], LIGHT_STRING_SHORT);
	INIT_OBJ_RES_OPTDATA(APPLICATION_TYPE_RID, res[avail], i,
			     res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Light Control instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_light_control_init(const struct device *dev)
{
	light_control.obj_id = IPSO_OBJECT_LIGHT_CONTROL_ID;
	light_control.version_major = LIGHT_VERSION_MAJOR;
	light_control.version_minor = LIGHT_VERSION_MINOR;
	light_control.is_core = false;
	light_control.fields = fields;
	light_control.field_count = ARRAY_SIZE(fields);
	light_control.max_instance_count = MAX_INSTANCE_COUNT;
	light_control.create_cb = light_control_create;
	lwm2m_register_obj(&light_control);

	return 0;
}

SYS_INIT(ipso_light_control_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
