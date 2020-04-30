/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Buzzer object (3338):
 * http://www.openmobilealliance.org/tech/profiles/lwm2m/3338.xml
 */

#define LOG_MODULE_NAME net_ipso_buzzer
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* resource IDs */
#define BUZZER_ON_OFF_ID		5850
#define BUZZER_LEVEL_ID			5548
#define BUZZER_DELAY_DURATION_ID	5521
#define BUZZER_MINIMUM_OFF_TIME_ID	5525
#define BUZZER_APPLICATION_TYPE_ID	5750
/* This field is actually not in the spec, so nothing sets it except here
 * users can listen for post_write events to it for buzzer on/off events
 */
#define BUZZER_DIGITAL_STATE_ID		5500

#define BUZZER_MAX_ID			6

#define MAX_INSTANCE_COUNT		CONFIG_LWM2M_IPSO_BUZZER_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with BUZZER_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (BUZZER_MAX_ID)

/* resource state */
struct ipso_buzzer_data {
	float32_value_t level;
	float64_value_t delay_duration;
	float64_value_t min_off_time;

	uint64_t trigger_offset;

	struct k_delayed_work buzzer_work;

	uint16_t obj_inst_id;
	bool onoff; /* toggle from resource */
	bool active; /* digital state */
};

static struct ipso_buzzer_data buzzer_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj buzzer;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(BUZZER_ON_OFF_ID, RW, BOOL),
	OBJ_FIELD_DATA(BUZZER_LEVEL_ID, RW_OPT, FLOAT32),
	OBJ_FIELD_DATA(BUZZER_DELAY_DURATION_ID, RW_OPT, FLOAT64),
	OBJ_FIELD_DATA(BUZZER_MINIMUM_OFF_TIME_ID, RW, FLOAT64),
	OBJ_FIELD_DATA(BUZZER_APPLICATION_TYPE_ID, RW_OPT, STRING),
	OBJ_FIELD_DATA(BUZZER_DIGITAL_STATE_ID, R, BOOL),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BUZZER_MAX_ID];
static struct lwm2m_engine_res_inst
		res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int float2ms(float64_value_t *f, uint32_t *ms)
{
	*ms = f->val1 * MSEC_PER_SEC;
	*ms += f->val2 / (LWM2M_FLOAT64_DEC_MAX / MSEC_PER_SEC);

	return 0;
}

static int get_buzzer_index(uint16_t obj_inst_id)
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

static int start_buzzer(struct ipso_buzzer_data *buzzer)
{
	uint32_t temp = 0U;
	char path[MAX_RESOURCE_LEN];

	/* make sure buzzer is currently not active */
	if (buzzer->active) {
		return -EINVAL;
	}

	/* check min off time from last trigger_offset */
	float2ms(&buzzer->min_off_time, &temp);
	if (k_uptime_get() < buzzer->trigger_offset + temp) {
		return -EINVAL;
	}

	/* TODO: check delay_duration > 0 */

	buzzer->trigger_offset = k_uptime_get();
	snprintk(path, MAX_RESOURCE_LEN, "%d/%u/%d", IPSO_OBJECT_BUZZER_ID,
		 buzzer->obj_inst_id, BUZZER_DIGITAL_STATE_ID);
	lwm2m_engine_set_bool(path, true);

	float2ms(&buzzer->delay_duration, &temp);
	k_delayed_work_submit(&buzzer->buzzer_work, K_MSEC(temp));

	return 0;
}

static int stop_buzzer(struct ipso_buzzer_data *buzzer, bool cancel)
{
	char path[MAX_RESOURCE_LEN];

	/* make sure buzzer is currently active */
	if (!buzzer->active) {
		return -EINVAL;
	}

	snprintk(path, MAX_RESOURCE_LEN, "%d/%u/%d", IPSO_OBJECT_BUZZER_ID,
		 buzzer->obj_inst_id, BUZZER_DIGITAL_STATE_ID);
	lwm2m_engine_set_bool(path, false);

	if (cancel) {
		k_delayed_work_cancel(&buzzer->buzzer_work);
	}

	return 0;
}

static int onoff_post_write_cb(uint16_t obj_inst_id,
			       uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len,
			       bool last_block, size_t total_size)
{
	int i;

	i = get_buzzer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	if (!buzzer_data[i].onoff && buzzer_data[i].active) {
		return stop_buzzer(&buzzer_data[i], true);
	} else if (buzzer_data[i].onoff && !buzzer_data[i].active) {
		return start_buzzer(&buzzer_data[i]);
	}

	return 0;
}

static void buzzer_work_cb(struct k_work *work)
{
	struct ipso_buzzer_data *buzzer = CONTAINER_OF(work,
						      struct ipso_buzzer_data,
						      buzzer_work);
	stop_buzzer(buzzer, false);
}

static struct lwm2m_engine_obj_inst *buzzer_create(uint16_t obj_inst_id)
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
	(void)memset(&buzzer_data[avail], 0, sizeof(buzzer_data[avail]));
	k_delayed_work_init(&buzzer_data[avail].buzzer_work, buzzer_work_cb);
	buzzer_data[avail].level.val1 = 50; /* 50% */
	buzzer_data[avail].delay_duration.val1 = 1; /* 1 seconds */
	buzzer_data[avail].obj_inst_id = obj_inst_id;

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES(BUZZER_ON_OFF_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &buzzer_data[avail].onoff,
		     sizeof(buzzer_data[avail].onoff),
		     NULL, NULL, onoff_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(BUZZER_LEVEL_ID, res[avail], i, res_inst[avail], j,
			  &buzzer_data[avail].level,
			  sizeof(buzzer_data[avail].level));
	INIT_OBJ_RES_DATA(BUZZER_DELAY_DURATION_ID, res[avail], i,
			  res_inst[avail], j,
			  &buzzer_data[avail].delay_duration,
			  sizeof(buzzer_data[avail].delay_duration));
	INIT_OBJ_RES_DATA(BUZZER_MINIMUM_OFF_TIME_ID, res[avail], i,
			  res_inst[avail], j,
			  &buzzer_data[avail].min_off_time,
			  sizeof(buzzer_data[avail].min_off_time));
	INIT_OBJ_RES_OPTDATA(BUZZER_APPLICATION_TYPE_ID, res[avail], i,
			     res_inst[avail], j);
	INIT_OBJ_RES_DATA(BUZZER_DIGITAL_STATE_ID, res[avail], i,
			  res_inst[avail], j,
			  &buzzer_data[avail].active,
			  sizeof(buzzer_data[avail].active));

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Buzzer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_buzzer_init(const struct device *dev)
{
	buzzer.obj_id = IPSO_OBJECT_BUZZER_ID;
	buzzer.fields = fields;
	buzzer.field_count = ARRAY_SIZE(fields);
	buzzer.max_instance_count = ARRAY_SIZE(inst);
	buzzer.create_cb = buzzer_create;
	lwm2m_register_obj(&buzzer);

	return 0;
}

SYS_INIT(ipso_buzzer_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
