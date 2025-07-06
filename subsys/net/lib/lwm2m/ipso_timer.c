/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Timer object (3340):
 * http://www.openmobilealliance.org/tech/profiles/lwm2m/3340.xml
 */

#define LOG_MODULE_NAME net_ipso_timer
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define TIMER_VERSION_MAJOR 1
#define TIMER_VERSION_MINOR 0

#define TIMER_MAX_ID		11

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_TIMER_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with TIMER_MAX_ID
 * subtract EXEC resources (1)
 */
#define RESOURCE_INSTANCE_COUNT	(TIMER_MAX_ID - 1)

enum ipso_timer_mode {
	TIMER_MODE_OFF = 0,
	TIMER_MODE_ONE_SHOT,
	TIMER_MODE_INTERVAL,		/* TODO */
	TIMER_MODE_DELAY_ON_PICKUP,	/* TODO */
	TIMER_MODE_DELAY_ON_DROPOUT,	/* TODO */
};

/* resource state */
struct ipso_timer_data {
	double delay_duration;
	double remaining_time;
	double min_off_time;
	double cumulative_time;

	uint64_t trigger_offset;
	uint32_t trigger_counter;
	uint32_t cumulative_time_ms;

	struct k_work_delayable timer_work;

	uint16_t obj_inst_id;
	uint8_t timer_mode;
	bool enabled;
	bool active;
};

static struct ipso_timer_data timer_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj ipso_timer;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(DELAY_DURATION_RID, RW, FLOAT),
	OBJ_FIELD_DATA(REMAINING_TIME_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(MINIMUM_OFF_TIME_RID, RW_OPT, FLOAT),
	OBJ_FIELD_EXECUTE_OPT(TRIGGER_RID),
	OBJ_FIELD_DATA(ON_OFF_RID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(DIGITAL_INPUT_COUNTER_RID, RW_OPT, U32), /* TODO */
	OBJ_FIELD_DATA(CUMULATIVE_TIME_RID, RW_OPT, FLOAT),
	OBJ_FIELD_DATA(DIGITAL_STATE_RID, R_OPT, BOOL),
	OBJ_FIELD_DATA(COUNTER_RID, R_OPT, U32),
	OBJ_FIELD_DATA(TIMER_MODE_RID, RW_OPT, U8),
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][TIMER_MAX_ID];
static struct lwm2m_engine_res_inst
		res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int get_timer_index(uint16_t obj_inst_id)
{
	int i, ret = -ENOENT;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (!inst[i].obj || inst[i].obj_inst_id != obj_inst_id) {
			continue;
		}

		ret = i;
		break;
	}

	return ret;
}

static int start_timer(struct ipso_timer_data *timer)
{
	uint32_t temp = 0U;
	struct lwm2m_obj_path path = LWM2M_OBJ(IPSO_OBJECT_TIMER_ID, timer->obj_inst_id,
					       DIGITAL_STATE_RID);

	/* make sure timer is enabled and not already active */
	if (timer->timer_mode == TIMER_MODE_OFF || timer->active ||
	    !timer->enabled) {
		return -EINVAL;
	}

	/* check min off time from last trigger_offset */
	temp = timer->min_off_time * MSEC_PER_SEC;
	if (k_uptime_get() < timer->trigger_offset + temp) {
		return -EINVAL;
	}

	/* TODO: check delay_duration > 0 ? other modes can it be 0? */

	timer->trigger_offset = k_uptime_get();
	timer->trigger_counter += 1U;

	lwm2m_set_bool(&path, true);

	temp = timer->delay_duration * MSEC_PER_SEC;
	k_work_reschedule(&timer->timer_work, K_MSEC(temp));

	return 0;
}

static int stop_timer(struct ipso_timer_data *timer, bool cancel)
{
	struct lwm2m_obj_path path = LWM2M_OBJ(IPSO_OBJECT_TIMER_ID, timer->obj_inst_id,
					       DIGITAL_STATE_RID);

	/* make sure timer is active */
	if (!timer->active) {
		return -EINVAL;
	}

	timer->cumulative_time_ms += k_uptime_get() - timer->trigger_offset;
	lwm2m_set_bool(&path, false);

	if (cancel) {
		k_work_cancel_delayable(&timer->timer_work);
	}

	return 0;
}

static void *remaining_time_read_cb(uint16_t obj_inst_id,
				    uint16_t res_id, uint16_t res_inst_id,
				    size_t *data_len)
{
	uint32_t temp = 0U;
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return NULL;
	}

	if (timer_data[i].active) {
		temp = timer_data[i].delay_duration * MSEC_PER_SEC;
		temp -= (k_uptime_get() - timer_data[i].trigger_offset);
		timer_data[i].remaining_time = (double)temp / MSEC_PER_SEC;
	} else {
		timer_data[i].remaining_time = 0;
	}

	*data_len = sizeof(timer_data[i].remaining_time);
	return &timer_data[i].remaining_time;
}

static void *cumulative_time_read_cb(uint16_t obj_inst_id,
				     uint16_t res_id, uint16_t res_inst_id,
				     size_t *data_len)
{
	int i;
	uint32_t temp;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return NULL;
	}

	temp = timer_data[i].cumulative_time_ms;
	if (timer_data[i].active) {
		temp += k_uptime_get() - timer_data[i].trigger_offset;
	}

	timer_data[i].cumulative_time = (double)temp / MSEC_PER_SEC;

	*data_len = sizeof(timer_data[i].cumulative_time);
	return &timer_data[i].cumulative_time;
}

static int cumulative_time_post_write_cb(uint16_t obj_inst_id,
					 uint16_t res_id,
					 uint16_t res_inst_id, uint8_t *data,
					 uint16_t data_len, bool last_block,
					 size_t total_size, size_t offset)
{
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	timer_data[i].cumulative_time_ms = 0U;
	return 0;
}

static int enabled_post_write_cb(uint16_t obj_inst_id, uint16_t res_id,
				 uint16_t res_inst_id, uint8_t *data,
				 uint16_t data_len, bool last_block,
				 size_t total_size, size_t offset)
{
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	/* check if timer is active and move to disabled state */
	if (!timer_data[i].enabled && timer_data[i].active) {
		return stop_timer(&timer_data[i], true);
	}

	return 0;
}

static int trigger_counter_post_write_cb(uint16_t obj_inst_id,
					 uint16_t res_id,
					 uint16_t res_inst_id, uint8_t *data,
					 uint16_t data_len, bool last_block,
					 size_t total_size, size_t offset)
{
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	timer_data[i].trigger_counter = 0U;
	return 0;
}

static void timer_work_cb(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ipso_timer_data *timer = CONTAINER_OF(dwork,
						     struct ipso_timer_data,
						     timer_work);
	stop_timer(timer, false);
}

static int timer_trigger_cb(uint16_t obj_inst_id,
			    uint8_t *args, uint16_t args_len)
{
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	return start_timer(&timer_data[i]);
}

static struct lwm2m_engine_obj_inst *timer_inst_create(uint16_t obj_inst_id)
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
	(void)memset(&timer_data[avail], 0, sizeof(timer_data[avail]));
	k_work_init_delayable(&timer_data[avail].timer_work, timer_work_cb);
	timer_data[avail].delay_duration = 5; /* 5 seconds */
	timer_data[avail].enabled = true;
	timer_data[avail].timer_mode = TIMER_MODE_ONE_SHOT;
	timer_data[avail].obj_inst_id = obj_inst_id;

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(DELAY_DURATION_RID, res[avail], i,
			  res_inst[avail], j, &timer_data[avail].delay_duration,
			  sizeof(timer_data[avail].delay_duration));
	INIT_OBJ_RES(REMAINING_TIME_RID, res[avail], i, res_inst[avail], j, 1,
		     false, true, &timer_data[avail].remaining_time,
		     sizeof(timer_data[avail].remaining_time),
		     remaining_time_read_cb, NULL, NULL, NULL, NULL);
	INIT_OBJ_RES_DATA(MINIMUM_OFF_TIME_RID, res[avail], i,
			  res_inst[avail], j, &timer_data[avail].min_off_time,
			  sizeof(timer_data[avail].min_off_time));
	INIT_OBJ_RES_EXECUTE(TRIGGER_RID, res[avail], i, timer_trigger_cb);
	INIT_OBJ_RES(ON_OFF_RID, res[avail], i, res_inst[avail], j, 1, false,
		     true, &timer_data[avail].enabled,
		     sizeof(timer_data[avail].enabled),
		     NULL, NULL, NULL, enabled_post_write_cb, NULL);
	INIT_OBJ_RES(CUMULATIVE_TIME_RID, res[avail], i, res_inst[avail], j, 1,
		     false, true, &timer_data[avail].cumulative_time,
		     sizeof(timer_data[avail].cumulative_time),
		     cumulative_time_read_cb, NULL, NULL,
		     cumulative_time_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(DIGITAL_STATE_RID, res[avail], i,
			  res_inst[avail], j, &timer_data[avail].active,
			  sizeof(timer_data[avail].active));
	INIT_OBJ_RES(COUNTER_RID, res[avail], i, res_inst[avail], j, 1, false,
		     true, &timer_data[avail].trigger_counter,
		     sizeof(timer_data[avail].trigger_counter),
		     NULL, NULL, NULL, trigger_counter_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(TIMER_MODE_RID, res[avail], i, res_inst[avail], j,
			  &timer_data[avail].timer_mode,
			  sizeof(timer_data[avail].timer_mode));
	INIT_OBJ_RES_OPTDATA(APPLICATION_TYPE_RID, res[avail], i,
			     res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Timer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_timer_init(void)
{
	ipso_timer.obj_id = IPSO_OBJECT_TIMER_ID;
	ipso_timer.version_major = TIMER_VERSION_MAJOR;
	ipso_timer.version_minor = TIMER_VERSION_MINOR;
	ipso_timer.is_core = false;
	ipso_timer.fields = fields;
	ipso_timer.field_count = ARRAY_SIZE(fields);
	ipso_timer.max_instance_count = MAX_INSTANCE_COUNT;
	ipso_timer.create_cb = timer_inst_create;
	lwm2m_register_obj(&ipso_timer);

	return 0;
}

LWM2M_OBJ_INIT(ipso_timer_init);
