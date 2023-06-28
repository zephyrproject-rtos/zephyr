/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include "sensor_mgmt.h"

#define DT_DRV_COMPAT zephyr_sensing

#define SENSING_SENSOR_NUM (sizeof((int []){ DT_FOREACH_CHILD_STATUS_OKAY_SEP(		\
			    DT_DRV_INST(0), DT_NODE_EXISTS, (,))}) / sizeof(int))

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one 'zephyr_sensing' compatible node may be present");

LOG_MODULE_REGISTER(sensing, CONFIG_SENSING_LOG_LEVEL);

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), SENSING_SENSOR_INFO_DEFINE)
DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), SENSING_SENSOR_DEFINE)

K_THREAD_STACK_DEFINE(runtime_stack, CONFIG_SENSING_RUNTIME_THREAD_STACK_SIZE);


struct sensing_sensor *sensors[SENSING_SENSOR_NUM];

static struct sensing_context sensing_ctx = {
	.sensor_num = SENSING_SENSOR_NUM,
};


/* sensor_later_config including arbitrate/set interval/sensitivity
 */
static uint32_t arbitrate_interval(struct sensing_sensor *sensor)
{
	struct sensing_connection *conn;
	uint32_t min_interval = UINT32_MAX;
	uint32_t interval;

	/* search from all clients, arbitrate the interval */
	for_each_client_conn(sensor, conn) {
		LOG_DBG("arbitrate interval, sensor:%s for each conn:%p, interval:%d(us)",
			sensor->dev->name, conn, conn->interval);
		if (!is_client_request_data(conn)) {
			continue;
		}
		if (conn->interval < min_interval) {
			min_interval = conn->interval;
		}
	}
	/* min_interval == UINT32_MAX means sensor is not opened by any clients,
	 * then interval should be 0
	 */
	interval = (min_interval == UINT32_MAX ? 0 : min_interval);

	if (interval == 0) {
		/* sensor is closed by all clients, reset next_exec_time as EXEC_TIME_OFF
		 * open -> close: next_exec_time = EXEC_TIME_OFF
		 */
		sensor->next_exec_time = EXEC_TIME_OFF;
	} else {
		/* sensor is still closed last time, set next_exec_time as EXEC_TIME_INIT
		 * close -> open: next_exec_time = EXEC_TIME_INIT
		 */
		if (sensor->next_exec_time == EXEC_TIME_OFF) {
			sensor->next_exec_time = EXEC_TIME_INIT;
		}
	}
	LOG_DBG("arbitrate interval, sensor:%s, interval:%d(us), next_exec_time:%lld",
		sensor->dev->name, interval, sensor->next_exec_time);

	return interval;
}

static int set_arbitrate_interval(struct sensing_sensor *sensor, uint32_t interval)
{
	const struct sensing_sensor_api *sensor_api;

	__ASSERT(sensor && sensor->dev, "set arbitrate interval, sensor or sensor device is NULL");
	sensor_api = sensor->dev->api;
	__ASSERT(sensor_api, "set arbitrate interval, sensor device sensor_api is NULL");

	sensor->interval = interval;
	/* reset sensor next_exec_time and sample timestamp as soon as sensor interval is changed */
	sensor->next_exec_time = interval > 0 ? EXEC_TIME_INIT : EXEC_TIME_OFF;

	LOG_DBG("set arbitrate interval:%d, sensor:%s, next_exec_time:%lld",
		interval, sensor->dev->name, sensor->next_exec_time);

	((struct sensing_sensor_value_header *)sensor->data_buf)->base_timestamp = 0;

	if (!sensor_api->set_interval) {
		LOG_ERR("sensor:%s set_interval callback is not set yet", sensor->dev->name);
		return -ENODEV;
	}

	return sensor_api->set_interval(sensor->dev, interval);
}

static int config_interval(struct sensing_sensor *sensor)
{
	uint32_t interval = arbitrate_interval(sensor);

	LOG_INF("config interval, sensor:%s, interval:%d", sensor->dev->name, interval);

	return set_arbitrate_interval(sensor, interval);
}

static uint32_t arbitrate_sensitivity(struct sensing_sensor *sensor, int index)
{
	struct sensing_connection *conn;
	uint32_t min_sensitivity = UINT32_MAX;

	/* search from all clients, arbitrate the sensitivity */
	for_each_client_conn(sensor, conn) {
		LOG_DBG("arbitrate sensitivity, sensor:%s for each conn:%p, idx:%d, sens:%d",
				sensor->dev->name, conn, index,
				conn->sensitivity[index]);
		if (!is_client_request_data(conn)) {
			continue;
		}
		if (conn->sensitivity[index] < min_sensitivity) {
			min_sensitivity = conn->sensitivity[index];
		}
	}
	LOG_DBG("arbitrate sensitivity, sensor:%s, min_sensitivity:%d",
		sensor->dev->name, min_sensitivity);

	/* min_sensitivity == UINT32_MAX means no client is requesting to open sensor,
	 * by any client, in this case, return sensitivity 0
	 */
	return (min_sensitivity == UINT32_MAX ? 0 : min_sensitivity);
}

static int set_arbitrate_sensitivity(struct sensing_sensor *sensor, int index, uint32_t sensitivity)
{
	const struct sensing_sensor_api *sensor_api;

	__ASSERT(sensor && sensor->dev, "arbitrate sensitivity, sensor or sensor device is NULL");
	sensor_api = sensor->dev->api;
	__ASSERT(sensor_api, "arbitrate sensitivity, sensor device sensor_api is NULL");

	/* update sensor sensitivity */
	sensor->sensitivity[index] = sensitivity;

	if (!sensor_api->set_sensitivity) {
		LOG_WRN("sensor:%s set_sensitivity callback is not set", sensor->dev->name);
		/* sensor driver may not set sensitivity callback, no need to return error here */
		return 0;
	}

	return sensor_api->set_sensitivity(sensor->dev, index, sensitivity);
}

static int config_sensitivity(struct sensing_sensor *sensor, int index)
{
	uint32_t sensitivity = arbitrate_sensitivity(sensor, index);

	LOG_INF("config sensitivity, sensor:%s, index:%d, sensitivity:%d",
		sensor->dev->name, index, sensitivity);

	return set_arbitrate_sensitivity(sensor, index, sensitivity);
}

static int config_sensor(struct sensing_sensor *sensor)
{
	int ret;
	int i = 0;

	ret = config_interval(sensor);
	if (ret) {
		LOG_WRN("sensor:%s config interval error", sensor->dev->name);
	}

	for (i = 0; i < sensor->sensitivity_count; i++) {
		ret = config_sensitivity(sensor, i);
		if (ret) {
			LOG_WRN("sensor:%s config sensitivity index:%d error",
					sensor->dev->name, i);
		}
	}

	return ret;
}

static void sensor_later_config(struct sensing_context *ctx)
{
	struct sensing_sensor *sensor;
	int i = 0;

	LOG_INF("sensor later config begin...");

	for_each_sensor_reverse(ctx, i, sensor) {
		if (atomic_test_and_clear_bit(&sensor->flag, SENSOR_LATER_CFG_BIT)) {
			LOG_INF("sensor later config, index:%d, sensor:%s",
				i, sensor->dev->name);
			config_sensor(sensor);
		}
	}
}

static void sensing_runtime_thread(void *p1, void *p2, void *p3)
{
	struct sensing_context *ctx = p1;
	int sleep_time = 100;
	k_timeout_t timeout;
	int ret;

	LOG_INF("sensing runtime thread start...");

	do {
		loop_sensors(ctx);

		timeout = (sleep_time == UINT32_MAX ? K_FOREVER : K_MSEC(sleep_time));

		ret = k_sem_take(&ctx->runtime_event_sem, timeout);
		if (!ret) {
			if (atomic_test_and_clear_bit(&ctx->runtime_event_flag, EVENT_CONFIG_READY)) {
				LOG_INF("runtime thread triggered by event_config ready");
				sensor_later_config(ctx);
			}
		}
	} while (1);

}

static void save_config_and_notify(struct sensing_sensor *sensor)
{
	struct sensing_context *ctx = &sensing_ctx;

	__ASSERT(sensor, "save config and notify, sensing_sensor not be NULL");

	LOG_INF("save config and notify, sensor:%s", sensor->dev->name);

	/* remember sensor_later_config bit to sensor */
	atomic_set_bit(&sensor->flag, SENSOR_LATER_CFG_BIT);

	/*remember event config ready and notify sensing_runtime_thread */
	atomic_set_bit(&ctx->runtime_event_flag, EVENT_CONFIG_READY);

	k_sem_give(&ctx->runtime_event_sem);
}

static int set_sensor_state(struct sensing_sensor *sensor, enum sensing_sensor_state state)
{
	__ASSERT(sensor, "set sensor state, sensing_sensor is NULL");

	sensor->state = state;

	return 0;
}

static void init_connection(struct sensing_connection *conn,
			    struct sensing_sensor *source,
			    struct sensing_sensor *sink)
{
	__ASSERT(conn, "init each connection, invalid connection");

	conn->source = source;
	conn->sink = sink;
	conn->interval = 0;
	memset(conn->sensitivity, 0x00, sizeof(conn->sensitivity));
	/* link connection to its reporter's client_list */
	sys_slist_append(&source->client_list, &conn->snode);
}

static int init_sensor(struct sensing_sensor *sensor, int conns_num)
{
	const struct sensing_sensor_api *sensor_api;
	struct sensing_sensor *reporter;
	struct sensing_connection *conn;
	void *tmp_conns[conns_num];
	int i;

	__ASSERT(sensor && sensor->dev, "init sensor, sensor or sensor device is NULL");
	sensor_api = sensor->dev->api;
	__ASSERT(sensor_api, "init sensor, sensor device sensor_api is NULL");

	if (sensor->data_buf == NULL) {
		LOG_ERR("sensor:%s memory alloc failed", sensor->dev->name);
		return -ENOMEM;
	}
	/* physical sensor has no reporters, conns_num is 0 */
	if (conns_num == 0) {
		sensor->conns = NULL;
	}

	for (i = 0; i < conns_num; i++) {
		conn = &sensor->conns[i];
		reporter = get_reporter_sensor(sensor, i);
		__ASSERT(reporter, "sensor's reporter should not be NULL");

		init_connection(conn, reporter, sensor);

		LOG_DBG("init sensor, reporter:%s, client:%s, connection:%d",
			reporter->dev->name, sensor->dev->name, i);

		tmp_conns[i] = conn;
	}

	/* physical sensor is working at polling mode by default,
	 * virtual sensor working mode is inherited from its reporter
	 */
	if (is_phy_sensor(sensor)) {
		sensor->mode = SENSOR_TRIGGER_MODE_POLLING;
	}

	return sensor_api->init(sensor->dev, sensor->info, tmp_conns, conns_num);
}

/* create struct sensing_sensor *sensor according to sensor device tree */
static int pre_init_sensor(struct sensing_sensor *sensor)
{
	struct sensing_sensor_ctx *sensor_ctx;
	uint16_t sample_size, total_size;
	uint16_t conn_sample_size = 0;
	int i = 0;
	void *tmp_data;

	__ASSERT(sensor && sensor->dev, "sensor or sensor dev is invalid");
	sensor_ctx = sensor->dev->data;
	__ASSERT(sensor_ctx, "sensing sensor context is invalid");

	sample_size = sensor_ctx->register_info->sample_size;
	for (i = 0; i < sensor->reporter_num; i++) {
		conn_sample_size += get_reporter_sample_size(sensor, i);
	}

	/* total memory to be allocated for a sensor according to sensor device tree:
	 * 1) sample data point to struct sensing_sensor->data_buf
	 * 2) size of struct sensing_connection* for sensor connection to its reporter
	 * 3) reporter sample size to be stored in connection data
	 */
	total_size = sample_size + sensor->reporter_num * sizeof(*sensor->conns) +
		     conn_sample_size;

	/* total size for different sensor maybe different, for example:
	 * there's no reporter for physical sensor, so no connection memory is needed
	 * reporter num of each virtual sensor may also different, so connection memory is also
	 * varied, so here malloc is a must for different sensor.
	 */
	tmp_data = malloc(total_size);
	if (!tmp_data) {
		LOG_ERR("malloc memory for sensing_sensor error");
		return -ENOMEM;
	}
	sensor->sample_size = sample_size;
	sensor->data_buf = tmp_data;
	sensor->conns = (struct sensing_connection *)((uint8_t *)sensor->data_buf + sample_size);

	tmp_data = sensor->conns + sensor->reporter_num;
	for (i = 0; i < sensor->reporter_num; i++) {
		sensor->conns[i].data = tmp_data;
		tmp_data = (uint8_t *)tmp_data + get_reporter_sample_size(sensor, i);
	}

	if (tmp_data != ((uint8_t *)sensor->data_buf + total_size)) {
		LOG_ERR("sensor memory assign error, data_buf:%p, tmp_data:%p, size:%d",
			sensor->data_buf, tmp_data, total_size);
		free(sensor->data_buf);
		sensor->data_buf = NULL;
		return -EINVAL;
	}

	LOG_INF("pre init sensor, sensor:%s, min_ri:%d(us)",
		sensor->dev->name, sensor->info->minimal_interval);

	sensor->interval = 0;
	sensor->sensitivity_count = sensor_ctx->register_info->sensitivity_count;
	__ASSERT(sensor->sensitivity_count <= CONFIG_SENSING_MAX_SENSITIVITY_COUNT,
		 "sensitivity count:%d should not exceed MAX_SENSITIVITY_COUNT",
		 sensor->sensitivity_count);
	memset(sensor->sensitivity, 0x00, sizeof(sensor->sensitivity));

	sys_slist_init(&sensor->client_list);

	sensor_ctx->priv_ptr = sensor;

	return 0;
}

static int sensing_init(void)
{
	struct sensing_context *ctx = &sensing_ctx;
	struct sensing_sensor *sensor;
	enum sensing_sensor_state state;
	int ret = 0;
	int i = 0;

	LOG_INF("sensing init begin...");

	if (ctx->sensing_initialized) {
		LOG_INF("sensing is already initialized");
		return 0;
	}

	if (ctx->sensor_num == 0) {
		LOG_WRN("no sensor created by device tree yet");
		return 0;
	}

	for (i = 0; i < ctx->sensor_num; i++) {
		STRUCT_SECTION_GET(sensing_sensor, i, &sensor);
		ret = pre_init_sensor(sensor);
		if (ret) {
			LOG_ERR("sensing init, pre init sensor error");
		}
		sensors[i] = sensor;
	}
	ctx->sensors = sensors;

	for_each_sensor(ctx, i, sensor) {
		ret = init_sensor(sensor, sensor->reporter_num);
		if (ret) {
			LOG_ERR("sensor:%s initial error", sensor->dev->name);
		}
		state = (ret ? SENSING_SENSOR_STATE_OFFLINE : SENSING_SENSOR_STATE_READY);
		ret = set_sensor_state(sensor, state);
		if (ret) {
			LOG_ERR("set sensor:%s state:%d error", sensor->dev->name, state);
		}
		LOG_INF("sensing init, sensor:%s, state:%d", sensor->dev->name, sensor->state);
	}

	k_sem_init(&ctx->runtime_event_sem, 0, 1);

	ctx->sensing_initialized = true;

	/* sensing subsystem runtime thread: sensor scheduling and sensor data processing */
	ctx->runtime_id = k_thread_create(&ctx->runtime_thread, runtime_stack,
			CONFIG_SENSING_RUNTIME_THREAD_STACK_SIZE,
			(k_thread_entry_t) sensing_runtime_thread, ctx, NULL, NULL,
			CONFIG_SENSING_RUNTIME_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!ctx->runtime_id) {
		LOG_ERR("create sensing runtime thread error");
		return -EAGAIN;
	}

	LOG_INF("%s(%d), runtime_id:%p", __func__, __LINE__, ctx->runtime_id);

	return ret;
}

int open_sensor(struct sensing_sensor *sensor, struct sensing_connection **conn)
{
	struct sensing_connection *tmp_conn;

	if (sensor->state != SENSING_SENSOR_STATE_READY)
		return -EINVAL;

	/* allocate struct sensing_connection *conn and conn data for application client */
	tmp_conn = malloc(sizeof(*tmp_conn) + sensor->sample_size);
	if (!tmp_conn) {
		LOG_ERR("malloc memory for struct sensing_connection error");
		return -ENOMEM;
	}
	tmp_conn->data = (uint8_t *)tmp_conn + sizeof(*tmp_conn);

	/* create connection from sensor to application(client = NULL) */
	init_connection(tmp_conn, sensor, NULL);

	*conn = tmp_conn;

	return 0;
}

int close_sensor(struct sensing_connection **conn)
{
	struct sensing_connection *tmp_conn = *conn;

	if (tmp_conn == NULL) {
		LOG_ERR("connection should not be NULL");
		return -EINVAL;
	}

	__ASSERT(!tmp_conn->sink, "sensor derived from device tree cannot be closed");

	__ASSERT(tmp_conn->source, "reporter should not be NULL");

	sys_slist_find_and_remove(&tmp_conn->source->client_list, &tmp_conn->snode);

	*conn = NULL;
	free(*conn);

	save_config_and_notify(tmp_conn->source);

	return 0;
}

int sensing_register_callback(struct sensing_connection *conn,
			      const struct sensing_callback_list *cb_list)
{
	if (conn == NULL) {
		LOG_ERR("register sensing callback list, connection not be NULL");
		return -ENODEV;
	}

	__ASSERT(!conn->sink, "only connection to application could register sensing callback");

	if (cb_list == NULL) {
		LOG_ERR("callback should not be NULL");
		return -ENODEV;
	}
	conn->data_evt_cb = cb_list->on_data_event;

	return 0;
}

int set_interval(struct sensing_connection *conn, uint32_t interval)
{
	LOG_INF("set interval, sensor:%s, interval:%u(us)", conn->source->dev->name, interval);

	__ASSERT(conn && conn->source, "set interval, connection or reporter not be NULL");

	if (interval > 0 && interval < conn->source->info->minimal_interval) {
		LOG_ERR("interval:%d(us) should no less than min interval:%d(us)",
					interval, conn->source->info->minimal_interval);
		return -EINVAL;
	}

	conn->interval = interval;

	save_config_and_notify(conn->source);

	return 0;
}

int get_interval(struct sensing_connection *conn, uint32_t *interval)
{
	__ASSERT(conn, "get interval, connection not be NULL");
	*interval = conn->interval;

	LOG_INF("get interval, sensor:%s, interval:%u(us)", conn->source->dev->name, *interval);

	return 0;
}

int set_sensitivity(struct sensing_connection *conn, int8_t index, uint32_t sensitivity)
{
	int i;

	__ASSERT(conn && conn->source, "set sensitivity, connection or reporter not be NULL");

	LOG_INF("set sensitivity, sensor:%s, index:%d, sensitivity:%d, count:%d",
		conn->source->dev->name, index,
		sensitivity, conn->source->sensitivity_count);

	if (index < SENSING_SENSITIVITY_INDEX_ALL || index >= conn->source->sensitivity_count) {
		LOG_ERR("sensor:%s sensitivity index:%d invalid", conn->source->dev->name, index);
		return -EINVAL;
	}

	if (index == SENSING_SENSITIVITY_INDEX_ALL) {
		for (i = 0; i < conn->source->sensitivity_count; i++) {
			conn->sensitivity[i] = sensitivity;
		}
	} else {
		conn->sensitivity[index] = sensitivity;
	}

	return 0;
}

int get_sensitivity(struct sensing_connection *conn, int8_t index, uint32_t *sensitivity)
{
	int i = 0;

	__ASSERT(conn && conn->source, "get sensitivity, connection or reporter not be NULL");

	*sensitivity = UINT32_MAX;

	if (index < SENSING_SENSITIVITY_INDEX_ALL || index >= conn->source->sensitivity_count) {
		LOG_ERR("sensor:%s sensitivity index:%d invalid", conn->source->dev->name, index);
		return -EINVAL;
	}

	if (index == SENSING_SENSITIVITY_INDEX_ALL) {
		/* each sensitivity index value should be same for global sensitivity */
		for (i = 1; i < conn->source->sensitivity_count; i++) {
			if (conn->sensitivity[i] != conn->sensitivity[0]) {
				LOG_ERR("sensitivity[%d]:%d should be same as sensitivity:%d",
					i, conn->sensitivity[i], conn->sensitivity[0]);
				return -EINVAL;
			}
		}
		*sensitivity = conn->sensitivity[0];
	} else {
		*sensitivity = conn->sensitivity[index];
	}

	LOG_INF("get_sensitivity, sensor:%s, index:%d, sensitivity:%d, count:%d",
		conn->source->dev->name, index,
		*sensitivity, conn->source->sensitivity_count);

	return 0;
}

int sensing_get_sensors(int *sensor_nums, const struct sensing_sensor_info **info)
{
	if (info == NULL) {
		LOG_ERR("sensing_sensor_info should not be NULL");
		return -ENODEV;
	}

	STRUCT_SECTION_COUNT(sensing_sensor_info, sensor_nums);

	STRUCT_SECTION_GET(sensing_sensor_info, 0, info);

	return 0;
}


SYS_INIT(sensing_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
