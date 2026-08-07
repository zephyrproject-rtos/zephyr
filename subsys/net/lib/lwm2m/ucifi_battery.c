/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI battery object (3411):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3411.xml
 */

#define LOG_MODULE_NAME net_ucifi_battery
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"
#include "ucifi_battery.h"

#define BATTERY_VERSION_MAJOR 1
#define BATTERY_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_BATTERY_INSTANCE_COUNT

#define BATTERY_MAX_ID 12
#define RESOURCE_INSTANCE_COUNT (BATTERY_MAX_ID)

/* resource state variables */
static uint8_t battery_level[MAX_INSTANCE_COUNT];
static uint32_t supply_loss_counter[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj battery;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(UCIFI_BATTERY_LEVEL_RID, R, U8),
	OBJ_FIELD_DATA(UCIFI_BATTERY_CAPACITY_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(UCIFI_BATTERY_VOLTAGE_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(UCIFI_BATTERY_TYPE_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_BATTERY_LOW_THESHOLD_RID, RW_OPT, U8),
	OBJ_FIELD_DATA(UCIFI_BATTERY_LEVEL_TOO_LOW_RID, R_OPT, BOOL),
	OBJ_FIELD_DATA(UCIFI_BATTERY_SHUTDOWN_RID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(UCIFI_BATTERY_NUM_CYCLES_RID, R_OPT, U32),
	OBJ_FIELD_DATA(UCIFI_BATTERY_SUPPLY_LOSS_RID, R_OPT, BOOL),
	OBJ_FIELD_DATA(UCIFI_BATTERY_SUPPLY_LOSS_COUNTER_RID, R_OPT, U32),
	OBJ_FIELD_EXECUTE_OPT(UCIFI_BATTERY_SUPPLY_LOSS_COUNTER_RESET_RID),
	OBJ_FIELD_DATA(UCIFI_BATTERY_SUPPLY_LOSS_REASON_RID, R_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BATTERY_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static void clear_supply_loss_counter(uint16_t obj_inst_id, int index)
{
	supply_loss_counter[index] = 0;
	lwm2m_notify_observer(UCIFI_OBJECT_BATTERY_ID, obj_inst_id,
			UCIFI_BATTERY_SUPPLY_LOSS_COUNTER_RID);
}

static int supply_loss_counter_reset_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	int i;

	LOG_DBG("RESET supply loss counter %d", obj_inst_id);
	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			clear_supply_loss_counter(obj_inst_id, i);
			return 0;
		}
	}

	return -ENOENT;
}

static struct lwm2m_engine_obj_inst *battery_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u",
				obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	battery_level[index] = 0;
	supply_loss_counter[index] = 0;

	(void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(UCIFI_BATTERY_LEVEL_RID, res[index], i, res_inst[index], j,
			  &battery_level[index], sizeof(*battery_level));
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_CAPACITY_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_VOLTAGE_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_TYPE_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_LOW_THESHOLD_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_LEVEL_TOO_LOW_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_SHUTDOWN_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_NUM_CYCLES_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_SUPPLY_LOSS_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_DATA(UCIFI_BATTERY_SUPPLY_LOSS_COUNTER_RID, res[index], i, res_inst[index], j,
			  &supply_loss_counter[index], sizeof(*supply_loss_counter));
	INIT_OBJ_RES_EXECUTE(UCIFI_BATTERY_SUPPLY_LOSS_COUNTER_RESET_RID, res[index], i,
			     supply_loss_counter_reset_cb);
	INIT_OBJ_RES_OPTDATA(UCIFI_BATTERY_SUPPLY_LOSS_REASON_RID, res[index], i, res_inst[index],
			     j);

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Create uCIFI Battery instance: %d", obj_inst_id);
	return &inst[index];
}

static int ucifi_battery_init(void)
{
	battery.obj_id = UCIFI_OBJECT_BATTERY_ID;
	battery.version_major = BATTERY_VERSION_MAJOR;
	battery.version_minor = BATTERY_VERSION_MINOR;
	battery.is_core = false;
	battery.fields = fields;
	battery.field_count = ARRAY_SIZE(fields);
	battery.max_instance_count = MAX_INSTANCE_COUNT;
	battery.create_cb = battery_create;
	lwm2m_register_obj(&battery);

	return 0;
}

LWM2M_OBJ_INIT(ucifi_battery_init);
