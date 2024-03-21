/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO On/Off Switch object (3347):
 * http://www.openmobilealliance.org/tech/profiles/lwm2m/3347.xml
 */

#define LOG_MODULE_NAME net_ipso_button
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define BUTTON_VERSION_MAJOR 1

#if defined(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)
#define BUTTON_VERSION_MINOR 1
#define BUTTON_MAX_ID 5
#else
#define BUTTON_VERSION_MINOR 0
#define BUTTON_MAX_ID 3
#endif /* defined(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1) */

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with BUTTON_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (BUTTON_MAX_ID)

/* resource state */
struct ipso_button_data {
	int64_t counter;
	uint16_t obj_inst_id;
	bool last_state;
	bool state;
};

static struct ipso_button_data button_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj onoff_switch;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(DIGITAL_INPUT_STATE_RID, R, BOOL),
	OBJ_FIELD_DATA(DIGITAL_INPUT_COUNTER_RID, R_OPT, S64),
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
#if defined(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)
	OBJ_FIELD_DATA(TIMESTAMP_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
#endif
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BUTTON_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int get_button_index(uint16_t obj_inst_id)
{
	int i, ret = -ENOENT;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (!inst[i].obj || inst[i].obj_inst_id != obj_inst_id) {
			continue;
		}

		ret = i;
		break;
	}

	return ret;
}

static int state_post_write_cb(uint16_t obj_inst_id,
			       uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len,
			       bool last_block, size_t total_size)
{
	int i;

	i = get_button_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	if (button_data[i].state && !button_data[i].last_state) {
		/* off to on transition, increment the counter */
		int64_t counter = button_data[i].counter + 1;
		struct lwm2m_obj_path path = LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, obj_inst_id,
						       DIGITAL_INPUT_COUNTER_RID);

		if (counter < 0) {
			counter = 0;
		}

		if (lwm2m_set_s64(&path, counter) < 0) {
			LOG_ERR("Failed to increment counter resource %d/%d/%d", path.obj_id,
				path.obj_inst_id, path.res_id);
		}
	}

	button_data[i].last_state = button_data[i].state;
	return 0;
}

static struct lwm2m_engine_obj_inst *button_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < ARRAY_SIZE(inst); index++) {
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
	(void)memset(&button_data[avail], 0, sizeof(button_data[avail]));
	button_data[avail].obj_inst_id = obj_inst_id;

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES(DIGITAL_INPUT_STATE_RID, res[avail], i, res_inst[avail],
		     j, 1, false, true, &button_data[avail].state,
		     sizeof(button_data[avail].state),
		     NULL, NULL, NULL, state_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(DIGITAL_INPUT_COUNTER_RID, res[avail], i,
			  res_inst[avail], j,
			  &button_data[avail].counter,
			  sizeof(button_data[avail].counter));
	INIT_OBJ_RES_OPTDATA(APPLICATION_TYPE_RID, res[avail], i,
			     res_inst[avail], j);
#if defined(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)
	INIT_OBJ_RES_OPTDATA(TIMESTAMP_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(FRACTIONAL_TIMESTAMP_RID, res[avail], i,
			     res_inst[avail], j);
#endif

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Button instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_button_init(void)
{
	onoff_switch.obj_id = IPSO_OBJECT_PUSH_BUTTON_ID;
	onoff_switch.version_major = BUTTON_VERSION_MAJOR;
	onoff_switch.version_minor = BUTTON_VERSION_MINOR;
	onoff_switch.is_core = false;
	onoff_switch.fields = fields;
	onoff_switch.field_count = ARRAY_SIZE(fields);
	onoff_switch.max_instance_count = ARRAY_SIZE(inst);
	onoff_switch.create_cb = button_create;
	lwm2m_register_obj(&onoff_switch);

	return 0;
}

LWM2M_OBJ_INIT(ipso_button_init);
