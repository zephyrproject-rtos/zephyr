/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Light Control object (3311):
 * https://github.com/IPSO-Alliance/pub/blob/master/docs/IPSO-Smart-Objects.pdf
 * Section: "16. IPSO Object: Light Control"
 */

#define SYS_LOG_DOMAIN "ipso_light_control"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <stdint.h>
#include <init.h>
#include <net/lwm2m.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* Server resource IDs */
#define LIGHT_ON_OFF_ID				5850
#define LIGHT_DIMMER_ID				5851
#define LIGHT_ON_TIME_ID			5852
#define LIGHT_CUMULATIVE_ACTIVE_POWER_ID	5805
#define LIGHT_POWER_FACTOR_ID			5820
#define LIGHT_COLOUR_ID				5706
#define LIGHT_SENSOR_UNITS_ID			5701

#define LIGHT_MAX_ID		7

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_LIGHT_CONTROL_INSTANCE_COUNT

#define LIGHT_STRING_SHORT	8

/* resource state variables */
static bool on_off_value[MAX_INSTANCE_COUNT];
static u8_t dimmer_value[MAX_INSTANCE_COUNT];
static s32_t on_time_value[MAX_INSTANCE_COUNT];
static u32_t on_time_offset[MAX_INSTANCE_COUNT];
static float32_value_t cumulative_active_value[MAX_INSTANCE_COUNT];
static float32_value_t power_factor_value[MAX_INSTANCE_COUNT];
static char colour[MAX_INSTANCE_COUNT][LIGHT_STRING_SHORT];
static char units[MAX_INSTANCE_COUNT][LIGHT_STRING_SHORT];

static struct lwm2m_engine_obj light_control;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LIGHT_ON_OFF_ID, RW, BOOL),
	OBJ_FIELD_DATA(LIGHT_DIMMER_ID, RW, U8),
	OBJ_FIELD_DATA(LIGHT_ON_TIME_ID, RW, S32),
	OBJ_FIELD_DATA(LIGHT_CUMULATIVE_ACTIVE_POWER_ID, R, FLOAT32),
	OBJ_FIELD_DATA(LIGHT_POWER_FACTOR_ID, R, FLOAT32),
	OBJ_FIELD_DATA(LIGHT_COLOUR_ID, RW, STRING),
	OBJ_FIELD_DATA(LIGHT_SENSOR_UNITS_ID, R, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res_inst res[MAX_INSTANCE_COUNT][LIGHT_MAX_ID];

static void *on_time_read_cb(u16_t obj_inst_id, size_t *data_len)
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

static int on_time_post_write_cb(u16_t obj_inst_id,
				 u8_t *data, u16_t data_len,
				 bool last_block, size_t total_size)
{
	int i;

	if (data_len != 4) {
		SYS_LOG_ERR("unknown size %u", data_len);
		return -EINVAL;
	}

	s32_t counter = *(s32_t *) data;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (!inst[i].obj || inst[i].obj_inst_id != obj_inst_id) {
			continue;
		}

		if (counter == 0) {
			on_time_offset[i] =
				(s32_t)(k_uptime_get() / MSEC_PER_SEC);
		}

		return 0;
	}

	return -ENOENT;
}

static struct lwm2m_engine_obj_inst *light_control_create(u16_t obj_inst_id)
{
	int index, avail = -1, i = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			SYS_LOG_ERR("Can not create instance - "
				    "already existing: %u", obj_inst_id);
			return NULL;
		}

		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		SYS_LOG_ERR("Can not create instance - "
			    "no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	on_off_value[avail] = false;
	dimmer_value[avail] = 0;
	on_time_value[avail] = 0;
	on_time_offset[avail] = 0;
	cumulative_active_value[avail].val1 = 0;
	cumulative_active_value[avail].val2 = 0;
	power_factor_value[avail].val1 = 0;
	power_factor_value[avail].val2 = 0;
	colour[avail][0] = '\0';
	units[avail][0] = '\0';

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(res[avail], i, LIGHT_ON_OFF_ID,
		&on_off_value[avail], sizeof(*on_off_value));
	INIT_OBJ_RES_DATA(res[avail], i, LIGHT_DIMMER_ID,
		&dimmer_value[avail], sizeof(*dimmer_value));
	INIT_OBJ_RES(res[avail], i, LIGHT_ON_TIME_ID, 0, &on_time_value[avail],
		sizeof(*on_time_value), on_time_read_cb,
		NULL, on_time_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(res[avail], i, LIGHT_CUMULATIVE_ACTIVE_POWER_ID,
		&cumulative_active_value[avail],
		sizeof(*cumulative_active_value));
	INIT_OBJ_RES_DATA(res[avail], i, LIGHT_POWER_FACTOR_ID,
		&power_factor_value[avail], sizeof(*power_factor_value));
	INIT_OBJ_RES_DATA(res[avail], i, LIGHT_COLOUR_ID,
		colour[avail], LIGHT_STRING_SHORT);
	INIT_OBJ_RES_DATA(res[avail], i, LIGHT_SENSOR_UNITS_ID,
		units[avail], LIGHT_STRING_SHORT);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	SYS_LOG_DBG("Create IPSO Light Control instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_light_control_init(struct device *dev)
{
	int ret = 0;

	/* Set default values */
	memset(inst, 0, sizeof(*inst) * MAX_INSTANCE_COUNT);
	memset(res, 0, sizeof(struct lwm2m_engine_res_inst) *
		       MAX_INSTANCE_COUNT * LIGHT_MAX_ID);

	light_control.obj_id = IPSO_OBJECT_LIGHT_CONTROL_ID;
	light_control.fields = fields;
	light_control.field_count = sizeof(fields) / sizeof(*fields);
	light_control.max_instance_count = MAX_INSTANCE_COUNT;
	light_control.create_cb = light_control_create;
	lwm2m_register_obj(&light_control);

	return ret;
}

SYS_INIT(ipso_light_control_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
