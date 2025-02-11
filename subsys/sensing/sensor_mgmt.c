/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include "sensor_mgmt.h"

#define DT_DRV_COMPAT zephyr_sensing

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one 'zephyr_sensing' compatible node may be present");

LOG_MODULE_REGISTER(sensing, CONFIG_SENSING_LOG_LEVEL);

static struct sensing_context sensing_ctx = {
};
RTIO_DEFINE_WITH_MEMPOOL(sensing_rtio_ctx, CONFIG_SENSING_RTIO_SQE_NUM,
		CONFIG_SENSING_RTIO_CQE_NUM,
		CONFIG_SENSING_RTIO_BLOCK_COUNT,
		CONFIG_SENSING_RTIO_BLOCK_SIZE, 4);

static enum sensor_channel sensing_sensor_type_to_chan(const int32_t type)
{
	switch (type) {
	case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		return SENSOR_CHAN_ACCEL_XYZ;
	case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
		return SENSOR_CHAN_GYRO_XYZ;
	default:
		break;
	}

	return SENSOR_CHAN_PRIV_START;
}

/* sensor_later_config including arbitrate/set interval/sensitivity
 */
static uint32_t arbitrate_interval(struct sensing_sensor *sensor)
{
	struct sensing_connection *conn;
	uint32_t min_interval = UINT32_MAX;
	uint32_t interval;

	/* search from all clients, arbitrate the interval */
	for_each_client_conn(sensor, conn) {
		LOG_INF("arbitrate interval, sensor:%s for each conn:%p, interval:%d(us)",
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

	LOG_DBG("arbitrate interval, sensor:%s, interval:%d(us)",
			sensor->dev->name, interval);

	return interval;
}

static int set_arbitrate_interval(struct sensing_sensor *sensor, uint32_t interval)
{
	struct sensing_submit_config *config = sensor->iodev->data;
	struct sensor_value odr = {0};
	int ret;

	__ASSERT(sensor && sensor->dev, "set arbitrate interval, sensor or sensor device is NULL");

	LOG_INF("set arbitrate interval:%d, sensor:%s, is_streaming:%d",
			interval, sensor->dev->name, config->is_streaming);

	if (interval) {
		odr.val1 = USEC_PER_SEC / interval;
		odr.val2 = (uint64_t)USEC_PER_SEC * 1000000 / interval % 1000000;
	}

	ret = sensor_attr_set(sensor->dev, config->chan,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
	if (ret) {
		LOG_ERR("%s set attr freq failed:%d", sensor->dev->name, ret);
		return ret;
	}

	if (sensor->interval) {
		if (config->is_streaming) {
			rtio_sqe_cancel(sensor->stream_sqe);
		} else {
			k_timer_stop(&sensor->timer);
		}
	}

	if (interval) {
		if (config->is_streaming) {
			ret = sensor_stream(sensor->iodev, &sensing_rtio_ctx,
					sensor, &sensor->stream_sqe);
		} else {
			k_timer_start(&sensor->timer, K_USEC(interval),
					K_USEC(interval));
		}
	}

	sensor->interval = interval;

	return ret;
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
	struct sensing_submit_config *config = (struct sensing_submit_config *)sensor->iodev->data;
	struct sensor_value threshold = {.val1 = sensitivity};
	int i;

	/* update sensor sensitivity */
	sensor->sensitivity[index] = sensitivity;

	for (i = 0; i < sensor->sensitivity_count; i++) {
		threshold.val1 = MIN(sensor->sensitivity[i], threshold.val1);
	}

	return sensor_attr_set(sensor->dev, config->chan,
			SENSOR_ATTR_HYSTERESIS, &threshold);
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
			LOG_WRN("sensor:%s config sensitivity index:%d error:%d",
					sensor->dev->name, i, ret);
		}
	}

	return ret;
}

static void sensor_later_config(void)
{
	LOG_INF("sensor later config begin...");

	for_each_sensor_reverse(sensor) {
		if (atomic_test_and_clear_bit(&sensor->flag, SENSOR_LATER_CFG_BIT)) {
			LOG_INF("sensor later config, sensor:%s",
				sensor->dev->name);
			config_sensor(sensor);
		}
	}
}

static void sensing_runtime_thread(void *p1, void *p2, void *p3)
{
	struct sensing_context *ctx = p1;
	int ret;

	LOG_INF("sensing runtime thread start...");

	do {
		ret = k_sem_take(&ctx->event_sem, K_FOREVER);
		if (!ret) {
			if (atomic_test_and_clear_bit(&ctx->event_flag, EVENT_CONFIG_READY)) {
				LOG_INF("runtime thread triggered by EVENT_CONFIG_READY");
				sensor_later_config();
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
	atomic_set_bit(&ctx->event_flag, EVENT_CONFIG_READY);

	k_sem_give(&ctx->event_sem);
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

	if (source) {
		conn->source = source;
	}

	if (sink) {
		conn->sink = sink;
	}

	conn->interval = 0;
	memset(conn->sensitivity, 0x00, sizeof(conn->sensitivity));
	/* link connection to its reporter's client_list */
	sys_slist_append(&conn->source->client_list, &conn->snode);
}

static void sensing_sensor_polling_timer(struct k_timer *timer_id)
{
	struct sensing_sensor *sensor = CONTAINER_OF(timer_id,
			struct sensing_sensor, timer);

	/* TODO: move it into sensing_runtime_thread */
	sensor_read_async_mempool(sensor->iodev, &sensing_rtio_ctx, sensor);
}

static int init_sensor(struct sensing_sensor *sensor)
{
	struct sensing_submit_config *config;
	struct sensing_connection *conn;
	int i;

	__ASSERT(sensor && sensor->dev, "init sensor, sensor or sensor device is NULL");

	k_timer_init(&sensor->timer, sensing_sensor_polling_timer, NULL);
	sys_slist_init(&sensor->client_list);

	for (i = 0; i < sensor->reporter_num; i++) {
		conn = &sensor->conns[i];

		/* source sensor has been assigned in compile time */
		init_connection(conn, NULL, sensor);

		LOG_INF("init sensor, reporter:%s, client:%s, connection:%d(%p)",
			conn->source->dev->name, sensor->dev->name, i, conn);
	}

	config = sensor->iodev->data;
	config->chan = sensing_sensor_type_to_chan(sensor->info->type);

	return 0;
}

static int sensing_init(const struct device *dev)
{
	struct sensing_context *ctx = dev->data;
	enum sensing_sensor_state state;
	int ret = 0;

	LOG_INF("sensing init begin...");

	for_each_sensor(sensor) {
		ret = init_sensor(sensor);
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

	k_sem_init(&ctx->event_sem, 0, 1);

	LOG_INF("create sensing runtime thread ok");
	ctx->sensing_initialized = true;

	return ret;
}

int open_sensor(struct sensing_sensor *sensor, struct sensing_connection **conn)
{
	struct sensing_connection *tmp_conn;

	if (sensor->state != SENSING_SENSOR_STATE_READY) {
		return -EINVAL;
	}

	/* create connection from sensor to application(client = NULL) */
	tmp_conn = malloc(sizeof(*tmp_conn));
	if (!tmp_conn) {
		return -ENOMEM;
	}

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

	save_config_and_notify(tmp_conn->source);

	free(*conn);
	*conn = NULL;

	return 0;
}

int sensing_register_callback(struct sensing_connection *conn,
			      struct sensing_callback_list *cb_list)
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
	conn->callback_list = cb_list;

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
	conn->next_consume_time = EXEC_TIME_INIT;

	LOG_INF("set interval, sensor:%s, conn:%p, interval:%d",
		conn->source->dev->name, conn, interval);

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

K_THREAD_DEFINE(sensing_runtime, CONFIG_SENSING_RUNTIME_THREAD_STACK_SIZE, sensing_runtime_thread,
		&sensing_ctx, NULL, NULL, CONFIG_SENSING_RUNTIME_THREAD_PRIORITY, 0, 0);

DEVICE_DT_INST_DEFINE(0, sensing_init, NULL, &sensing_ctx, NULL, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, NULL);
