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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define BUZZER_VERSION_MAJOR 1


#if defined(CONFIG_LWM2M_IPSO_BUZZER_VERSION_1_1)
#define BUZZER_VERSION_MINOR 1
#define BUZZER_MAX_ID 8
#else
#define BUZZER_VERSION_MINOR 0
#define BUZZER_MAX_ID 6
#endif /* defined(CONFIG_LWM2M_IPSO_BUZZER_VERSION_1_1) */

#define MAX_INSTANCE_COUNT		CONFIG_LWM2M_IPSO_BUZZER_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with BUZZER_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (BUZZER_MAX_ID)

/* resource state */
struct ipso_buzzer_data {
	double level;
	double delay_duration;
	double min_off_time;

	uint64_t trigger_offset;

	struct k_work_delayable buzzer_work;

	uint16_t obj_inst_id;
	bool onoff; /* toggle from resource */
	bool active; /* digital state */
};

static struct ipso_buzzer_data buzzer_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj ipso_buzzer;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(ON_OFF_RID, RW, BOOL),
	OBJ_FIELD_DATA(LEVEL_RID, RW_OPT, FLOAT),
	OBJ_FIELD_DATA(DELAY_DURATION_RID, RW_OPT, FLOAT),
	OBJ_FIELD_DATA(MINIMUM_OFF_TIME_RID, RW, FLOAT),
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
	/* This field is actually not in the spec, so nothing sets it except
	 * here users can listen for post_write events to it for buzzer on/off
	 * events
	 */
	OBJ_FIELD_DATA(DIGITAL_INPUT_STATE_RID, R, BOOL),
#if defined(CONFIG_LWM2M_IPSO_BUZZER_VERSION_1_1)
	OBJ_FIELD_DATA(TIMESTAMP_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
#endif
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BUZZER_MAX_ID];
static struct lwm2m_engine_res_inst
		res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

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
	struct lwm2m_obj_path path = LWM2M_OBJ(IPSO_OBJECT_BUZZER_ID, buzzer->obj_inst_id,
					       DIGITAL_INPUT_STATE_RID);

	/* make sure buzzer is currently not active */
	if (buzzer->active) {
		return -EINVAL;
	}

	/* check min off time from last trigger_offset */
	temp = (uint32_t)(buzzer->min_off_time * MSEC_PER_SEC);
	if (k_uptime_get() < buzzer->trigger_offset + temp) {
		return -EINVAL;
	}

	/* TODO: check delay_duration > 0 */

	buzzer->trigger_offset = k_uptime_get();
	lwm2m_set_bool(&path, true);

	temp = (uint32_t)(buzzer->delay_duration * MSEC_PER_SEC);
	k_work_reschedule(&buzzer->buzzer_work, K_MSEC(temp));

	return 0;
}

static int stop_buzzer(struct ipso_buzzer_data *buzzer, bool cancel)
{
	struct lwm2m_obj_path path = LWM2M_OBJ(IPSO_OBJECT_BUZZER_ID, buzzer->obj_inst_id,
					       DIGITAL_INPUT_STATE_RID);

	/* make sure buzzer is currently active */
	if (!buzzer->active) {
		return -EINVAL;
	}

	lwm2m_set_bool(&path, false);

	if (cancel) {
		k_work_cancel_delayable(&buzzer->buzzer_work);
	}

	return 0;
}

static int onoff_post_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			       uint16_t res_inst_id, uint8_t *data,
			       uint16_t data_len, bool last_block,
			       size_t total_size, size_t offset)
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
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ipso_buzzer_data *buzzer = CONTAINER_OF(dwork,
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
	k_work_init_delayable(&buzzer_data[avail].buzzer_work, buzzer_work_cb);
	buzzer_data[avail].level = 50; /* 50% */
	buzzer_data[avail].delay_duration = 1; /* 1 seconds */
	buzzer_data[avail].obj_inst_id = obj_inst_id;

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES(ON_OFF_RID, res[avail], i, res_inst[avail], j, 1, false,
		     true, &buzzer_data[avail].onoff,
		     sizeof(buzzer_data[avail].onoff),
		     NULL, NULL, NULL, onoff_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(LEVEL_RID, res[avail], i, res_inst[avail], j,
			  &buzzer_data[avail].level,
			  sizeof(buzzer_data[avail].level));
	INIT_OBJ_RES_DATA(DELAY_DURATION_RID, res[avail], i, res_inst[avail], j,
			  &buzzer_data[avail].delay_duration,
			  sizeof(buzzer_data[avail].delay_duration));
	INIT_OBJ_RES_DATA(MINIMUM_OFF_TIME_RID, res[avail], i, res_inst[avail],
			  j, &buzzer_data[avail].min_off_time,
			  sizeof(buzzer_data[avail].min_off_time));
	INIT_OBJ_RES_OPTDATA(APPLICATION_TYPE_RID, res[avail], i,
			     res_inst[avail], j);
	INIT_OBJ_RES_DATA(DIGITAL_INPUT_STATE_RID, res[avail], i,
			  res_inst[avail], j, &buzzer_data[avail].active,
			  sizeof(buzzer_data[avail].active));
#if defined(CONFIG_LWM2M_IPSO_BUZZER_VERSION_1_1)
	INIT_OBJ_RES_OPTDATA(TIMESTAMP_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(FRACTIONAL_TIMESTAMP_RID, res[avail], i,
			     res_inst[avail], j);
#endif


	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Buzzer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_buzzer_init(void)
{
	ipso_buzzer.obj_id = IPSO_OBJECT_BUZZER_ID;
	ipso_buzzer.version_major = BUZZER_VERSION_MAJOR;
	ipso_buzzer.version_minor = BUZZER_VERSION_MINOR;
	ipso_buzzer.is_core = false;
	ipso_buzzer.fields = fields;
	ipso_buzzer.field_count = ARRAY_SIZE(fields);
	ipso_buzzer.max_instance_count = ARRAY_SIZE(inst);
	ipso_buzzer.create_cb = buzzer_create;
	lwm2m_register_obj(&ipso_buzzer);

	return 0;
}

LWM2M_OBJ_INIT(ipso_buzzer_init);
