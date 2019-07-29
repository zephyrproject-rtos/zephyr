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

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* resource IDs */
#define BUTTON_DIGITAL_STATE_ID		5500
#define BUTTON_DIGITAL_INPUT_COUNTER_ID	5501
#define BUTTON_APPLICATION_TYPE_ID	5750

#define BUTTON_MAX_ID			3

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with BUTTON_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (BUTTON_MAX_ID)

/* resource state */
struct ipso_button_data {
	u64_t counter;
	u16_t obj_inst_id;
	bool last_state;
	bool state;
};

static struct ipso_button_data button_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj onoff_switch;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(BUTTON_DIGITAL_STATE_ID, R, BOOL),
	OBJ_FIELD_DATA(BUTTON_DIGITAL_INPUT_COUNTER_ID, R_OPT, U64),
	OBJ_FIELD_DATA(BUTTON_APPLICATION_TYPE_ID, RW_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BUTTON_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int get_button_index(u16_t obj_inst_id)
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

static int state_post_write_cb(u16_t obj_inst_id,
			       u16_t res_id, u16_t res_inst_id,
			       u8_t *data, u16_t data_len,
			       bool last_block, size_t total_size)
{
	int i;

	i = get_button_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	if (button_data[i].state && !button_data[i].last_state) {
		/* off to on transition */
		button_data[i].counter++;
	}

	button_data[i].last_state = button_data[i].state;
	return 0;
}

static struct lwm2m_engine_obj_inst *button_create(u16_t obj_inst_id)
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
	INIT_OBJ_RES(BUTTON_DIGITAL_STATE_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &button_data[avail].state,
		     sizeof(button_data[avail].state),
		     NULL, NULL, state_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(BUTTON_DIGITAL_INPUT_COUNTER_ID, res[avail], i,
			  res_inst[avail], j,
			  &button_data[avail].counter,
			  sizeof(button_data[avail].counter));
	INIT_OBJ_RES_OPTDATA(BUTTON_APPLICATION_TYPE_ID, res[avail], i,
			     res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Button instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_button_init(struct device *dev)
{
	onoff_switch.obj_id = IPSO_OBJECT_PUSH_BUTTON_ID;
	onoff_switch.fields = fields;
	onoff_switch.field_count = ARRAY_SIZE(fields);
	onoff_switch.max_instance_count = ARRAY_SIZE(inst);
	onoff_switch.create_cb = button_create;
	lwm2m_register_obj(&onoff_switch);

	return 0;
}

SYS_INIT(ipso_button_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
