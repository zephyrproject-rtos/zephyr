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

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* Server resource IDs */
#define TIMER_DELAY_DURATION_ID		5521
#define TIMER_REMAINING_TIME_ID		5538
#define TIMER_MINIMUM_OFF_TIME_ID	5525
#define TIMER_TRIGGER_ID		5523
#define TIMER_ON_OFF_ID			5850
#define TIMER_DIGITAL_INPUT_COUNTER_ID	5501
#define TIMER_CUMULATIVE_TIME_ID	5544
#define TIMER_DIGITAL_STATE_ID		5543
#define TIMER_COUNTER_ID		5534
#define TIMER_MODE_ID			5526
#define TIMER_APPLICATION_TYPE_ID	5750

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
	float64_value_t delay_duration;
	float64_value_t remaining_time;
	float64_value_t min_off_time;
	float64_value_t cumulative_time;

	u64_t trigger_offset;
	u32_t trigger_counter;
	u32_t cumulative_time_ms;

	struct k_delayed_work timer_work;

	u16_t obj_inst_id;
	u8_t timer_mode;
	bool enabled;
	bool active;
};

static struct ipso_timer_data timer_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj timer;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(TIMER_DELAY_DURATION_ID, RW, FLOAT64),
	OBJ_FIELD_DATA(TIMER_REMAINING_TIME_ID, R_OPT, FLOAT64),
	OBJ_FIELD_DATA(TIMER_MINIMUM_OFF_TIME_ID, RW_OPT, FLOAT64),
	OBJ_FIELD_EXECUTE_OPT(TIMER_TRIGGER_ID),
	OBJ_FIELD_DATA(TIMER_ON_OFF_ID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(TIMER_DIGITAL_INPUT_COUNTER_ID, RW_OPT, U32), /* TODO */
	OBJ_FIELD_DATA(TIMER_CUMULATIVE_TIME_ID, RW_OPT, FLOAT64),
	OBJ_FIELD_DATA(TIMER_DIGITAL_STATE_ID, R_OPT, BOOL),
	OBJ_FIELD_DATA(TIMER_COUNTER_ID, R_OPT, U32),
	OBJ_FIELD_DATA(TIMER_MODE_ID, RW_OPT, U8),
	OBJ_FIELD_DATA(TIMER_APPLICATION_TYPE_ID, RW_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][TIMER_MAX_ID];
static struct lwm2m_engine_res_inst
		res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int ms2float(u32_t ms, float64_value_t *f)
{
	f->val1 = ms / MSEC_PER_SEC;
	f->val2 = (ms % MSEC_PER_SEC) * (LWM2M_FLOAT64_DEC_MAX / MSEC_PER_SEC);

	return 0;
}

static int float2ms(float64_value_t *f, u32_t *ms)
{
	*ms = f->val1 * MSEC_PER_SEC;
	*ms += f->val2 / (LWM2M_FLOAT64_DEC_MAX / MSEC_PER_SEC);

	return 0;
}

static int get_timer_index(u16_t obj_inst_id)
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
	u32_t temp = 0U;
	char path[MAX_RESOURCE_LEN];

	/* make sure timer is enabled and not already active */
	if (timer->timer_mode == TIMER_MODE_OFF || timer->active ||
	    !timer->enabled) {
		return -EINVAL;
	}

	/* check min off time from last trigger_offset */
	float2ms(&timer->min_off_time, &temp);
	if (k_uptime_get() < timer->trigger_offset + temp) {
		return -EINVAL;
	}

	/* TODO: check delay_duration > 0 ? other modes can it be 0? */

	timer->trigger_offset = k_uptime_get();
	timer->trigger_counter += 1U;

	snprintk(path, MAX_RESOURCE_LEN, "%d/%u/%d", IPSO_OBJECT_TIMER_ID,
		 timer->obj_inst_id, TIMER_DIGITAL_STATE_ID);
	lwm2m_engine_set_bool(path, true);

	float2ms(&timer->delay_duration, &temp);
	k_delayed_work_submit(&timer->timer_work, K_MSEC(temp));

	return 0;
}

static int stop_timer(struct ipso_timer_data *timer, bool cancel)
{
	char path[MAX_RESOURCE_LEN];

	/* make sure timer is active */
	if (!timer->active) {
		return -EINVAL;
	}

	timer->cumulative_time_ms += k_uptime_get() - timer->trigger_offset;
	snprintk(path, MAX_RESOURCE_LEN, "%d/%u/%d", IPSO_OBJECT_TIMER_ID,
		 timer->obj_inst_id, TIMER_DIGITAL_STATE_ID);
	lwm2m_engine_set_bool(path, false);

	if (cancel) {
		k_delayed_work_cancel(&timer->timer_work);
	}

	return 0;
}

static void *remaining_time_read_cb(u16_t obj_inst_id,
				    u16_t res_id, u16_t res_inst_id,
				    size_t *data_len)
{
	u32_t temp = 0U;
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return NULL;
	}

	if (timer_data[i].active) {
		float2ms(&timer_data[i].delay_duration, &temp);
		temp -= (k_uptime_get() - timer_data[i].trigger_offset);
		ms2float(temp, &timer_data[i].remaining_time);
	} else {
		timer_data[i].remaining_time.val1 = 0;
		timer_data[i].remaining_time.val2 = 0;
	}

	*data_len = sizeof(timer_data[i].remaining_time);
	return &timer_data[i].remaining_time;
}

static void *cumulative_time_read_cb(u16_t obj_inst_id,
				     u16_t res_id, u16_t res_inst_id,
				     size_t *data_len)
{
	int i;
	u32_t temp;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return NULL;
	}

	temp = timer_data[i].cumulative_time_ms;
	if (timer_data[i].active) {
		temp += k_uptime_get() - timer_data[i].trigger_offset;
	}

	ms2float(temp, &timer_data[i].cumulative_time);

	*data_len = sizeof(timer_data[i].cumulative_time);
	return &timer_data[i].cumulative_time;
}

static int cumulative_time_post_write_cb(u16_t obj_inst_id,
					 u16_t res_id, u16_t res_inst_id,
					 u8_t *data, u16_t data_len,
					 bool last_block, size_t total_size)
{
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	timer_data[i].cumulative_time_ms = 0U;
	return 0;
}

static int enabled_post_write_cb(u16_t obj_inst_id,
				 u16_t res_id, u16_t res_inst_id,
				 u8_t *data, u16_t data_len,
				 bool last_block, size_t total_size)
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

static int trigger_counter_post_write_cb(u16_t obj_inst_id,
					 u16_t res_id, u16_t res_inst_id,
					 u8_t *data, u16_t data_len,
					 bool last_block, size_t total_size)
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
	struct ipso_timer_data *timer = CONTAINER_OF(work,
						     struct ipso_timer_data,
						     timer_work);
	stop_timer(timer, false);
}

static int timer_trigger_cb(u16_t obj_inst_id)
{
	int i;

	i = get_timer_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	return start_timer(&timer_data[i]);
}

static struct lwm2m_engine_obj_inst *timer_create(u16_t obj_inst_id)
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
	k_delayed_work_init(&timer_data[avail].timer_work, timer_work_cb);
	timer_data[avail].delay_duration.val1 = 5; /* 5 seconds */
	timer_data[avail].enabled = true;
	timer_data[avail].timer_mode = TIMER_MODE_ONE_SHOT;
	timer_data[avail].obj_inst_id = obj_inst_id;

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(TIMER_DELAY_DURATION_ID, res[avail], i,
			  res_inst[avail], j, &timer_data[avail].delay_duration,
			  sizeof(timer_data[avail].delay_duration));
	INIT_OBJ_RES(TIMER_REMAINING_TIME_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &timer_data[avail].remaining_time,
		     sizeof(timer_data[avail].remaining_time),
		     remaining_time_read_cb, NULL, NULL, NULL);
	INIT_OBJ_RES_DATA(TIMER_MINIMUM_OFF_TIME_ID, res[avail], i,
			  res_inst[avail], j, &timer_data[avail].min_off_time,
			  sizeof(timer_data[avail].min_off_time));
	INIT_OBJ_RES_EXECUTE(TIMER_TRIGGER_ID, res[avail], i,
			     timer_trigger_cb);
	INIT_OBJ_RES(TIMER_ON_OFF_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &timer_data[avail].enabled,
		     sizeof(timer_data[avail].enabled),
		     NULL, NULL, enabled_post_write_cb, NULL);
	INIT_OBJ_RES(TIMER_CUMULATIVE_TIME_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &timer_data[avail].cumulative_time,
		     sizeof(timer_data[avail].cumulative_time),
		     cumulative_time_read_cb, NULL,
		     cumulative_time_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(TIMER_DIGITAL_STATE_ID, res[avail], i,
			  res_inst[avail], j, &timer_data[avail].active,
			  sizeof(timer_data[avail].active));
	INIT_OBJ_RES(TIMER_COUNTER_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &timer_data[avail].trigger_counter,
		     sizeof(timer_data[avail].trigger_counter),
		     NULL, NULL, trigger_counter_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(TIMER_MODE_ID, res[avail], i, res_inst[avail], j,
			  &timer_data[avail].timer_mode,
			  sizeof(timer_data[avail].timer_mode));
	INIT_OBJ_RES_OPTDATA(TIMER_APPLICATION_TYPE_ID, res[avail], i,
			     res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Timer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_timer_init(struct device *dev)
{
	timer.obj_id = IPSO_OBJECT_TIMER_ID;
	timer.fields = fields;
	timer.field_count = ARRAY_SIZE(fields);
	timer.max_instance_count = MAX_INSTANCE_COUNT;
	timer.create_cb = timer_create;
	lwm2m_register_obj(&timer);

	return 0;
}

SYS_INIT(ipso_timer_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
