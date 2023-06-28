/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <zephyr/sensing/sensing_sensor.h>
#include "sensor_mgmt.h"

LOG_MODULE_DECLARE(sensing, CONFIG_SENSING_LOG_LEVEL);


/* check whether sensor need to poll data or not, if polling data is needed, update execute time
 * when time arriving
 */
static bool sensor_need_poll(struct sensing_sensor *sensor, uint64_t cur_us)
{
	bool poll = false;

	/* sensor is not in polling mode or sensor interval still not set yet,
	 * no need to poll, return directly
	 */
	if (sensor->mode != SENSOR_TRIGGER_MODE_POLLING || sensor->interval == 0) {
		LOG_INF("sensor %s not in polling mode:%d or sensor interval:%d not opened yet",
			sensor->dev->name, sensor->mode, sensor->interval);
		sensor->next_exec_time = EXEC_TIME_OFF;
		return false;
	}

	/* sensor is in polling mode, first time execute, will poll data at next interval */
	if (sensor->next_exec_time == EXEC_TIME_INIT) {
		LOG_INF("sensor:%s first time exe, cur time:%lld, interval:%d(us)",
				sensor->dev->name, cur_us, sensor->interval);
		sensor->next_exec_time = cur_us + sensor->interval;
		return false;
	}

	/* execute time arrived, execute this polling data, meanwhile calculate next execute time */
	if (sensor->next_exec_time <= cur_us) {
		poll = true;
		sensor->next_exec_time += sensor->interval;
	}

	LOG_INF("sensor:%s, need_poll:%u, cur:%llu, next_exec_time:%llu, mode:%d",
		sensor->dev->name, poll, cur_us, sensor->next_exec_time, sensor->mode);

	return poll;
}

/* check whether sensor needs to be executed/processed */
static bool sensor_need_exec(struct sensing_sensor *sensor, uint64_t cur_us)
{
	LOG_DBG("sensor:%s need to execute, next_exec_time:%lld, sensor_mode:%d, interval:%d",
		sensor->dev->name, sensor->next_exec_time, sensor->mode, sensor->interval);

	if (!is_sensor_opened(sensor)) {
		return false;
	}
	if (sensor_need_poll(sensor, cur_us) ||
	    is_sensor_data_ready(sensor) ||
	    sensor_has_new_data(sensor)) {
		return true;
	}

	return false;
}

static int virtual_sensor_process_data(struct sensing_sensor *sensor)
{
	const struct sensing_sensor_api *sensor_api;
	struct sensing_connection *conn;
	int ret = 0, i;

	__ASSERT(sensor && sensor->dev, "virtual sensor proc, sensor or sensor device is NULL");
	sensor_api = sensor->dev->api;
	__ASSERT(sensor_api, "virtual sensor proc, sensor device sensor_api is NULL");

	/* enumerate each connection, and call process data for each connection,
	 * after data processing, clear new_data_arrive flag
	 */
	for (i = 0; i < sensor->reporter_num; i++) {
		conn = &sensor->conns[i];
		if (!conn->new_data_arrive) {
			continue;
		}
		LOG_INF("virtual sensor proc data, index:%d, sensor:%s, sample_size:%d",
			i, sensor->dev->name, sensor->sample_size);

		ret |= sensor_api->process(sensor->dev, conn, conn->data, sensor->sample_size);
		conn->new_data_arrive = false;
	}

	return ret;
}

static int process_streaming_data(struct sensing_sensor *sensor, uint64_t cur_us)
{
	const struct sensing_sensor_api *sensor_api;
	uint64_t next_time;
	uint64_t *sample_time;
	int ret = 0;

	__ASSERT(sensor && sensor->dev, "process streaming data, sensor or sensor device is NULL");

	sample_time = &((struct sensing_sensor_value_header *)sensor->data_buf)->base_timestamp;
	/* sample time 0 is for first sample,
	 * update sample time according to current time
	 */
	next_time = (*sample_time == 0 ? cur_us : MIN(cur_us, *sample_time + sensor->interval));

	LOG_INF("proc stream data, sensor:%s, cur:%lld, sample:%lld, ri:%d(us), next:%lld",
		sensor->dev->name, cur_us, *sample_time, sensor->interval, next_time);

	sensor_api = sensor->dev->api;
	__ASSERT(sensor_api, "proc stream data, sensor device sensor_api is NULL");
	ret = sensor_api->read_sample(sensor->dev, sensor->data_buf, sensor->sample_size);
	if (ret) {
		return ret;
	}
	/* update data sample time */
	*sample_time = next_time;

	return 0;
}

static int physical_sensor_process_data(struct sensing_sensor *sensor, uint64_t cur_us)
{
	int ret = 0;

	ret = process_streaming_data(sensor, cur_us);
	if (ret)
		return ret;
	/* TODO: process fifo data*/

	return 0;
}

static int sensor_process_data(struct sensing_sensor *sensor, uint64_t cur_us)
{
	__ASSERT(sensor, "sensor process data, sensing_sensor is NULL");

	if (is_phy_sensor(sensor)) {
		return physical_sensor_process_data(sensor, cur_us);
	} else {
		return virtual_sensor_process_data(sensor);
	}

	return 0;
}

/* check whether it is right time for client to consume this sample */
static bool sensor_test_consume_time(struct sensing_sensor *sensor,
				     struct sensing_connection *conn,
				     uint64_t cur_time)
{
	uint64_t ts = ((struct sensing_sensor_value_header *)sensor->data_buf)->base_timestamp;

	LOG_INF("sensor:%s next_consume_time:%lld sample_time:%lld, cur_time:%lld",
			sensor->dev->name, conn->next_consume_time, ts, cur_time);

	if (conn->next_consume_time <= ts)
		return true;

	LOG_INF("sensor:%s data not ready, next_consume_time:%lld sample_time:%lld, cur_time:%lld",
			sensor->dev->name, conn->next_consume_time, ts, cur_time);

	return false;
}

static void update_client_consume_time(struct sensing_sensor *sensor,
				       struct sensing_connection *conn)
{
	uint32_t interval = conn->interval;
	uint64_t ts = ((struct sensing_sensor_value_header *)sensor->data_buf)->base_timestamp;

	LOG_DBG("update time, sensor:%s, next_consume:%lld, interval:%d, sample_time:%lld",
		sensor->dev->name, conn->next_consume_time, interval, ts);

	if (conn->next_consume_time == 0 || conn->next_consume_time + interval < ts) {
		/* three cases next consume time start counting from last sample time:
		 * 1) first sample arrived, next_consume_time is set to 0
		 * 2) samples dropped
		 * 3) data ready mode is also processed this way to avoid error accumulation
		 */
		conn->next_consume_time = ts + interval;
	} else {
		/* regular flow */
		conn->next_consume_time += interval;
	}
}

static int sensor_sensitivity_test(struct sensing_sensor *sensor,
				   struct sensing_connection *conn)
{
	const struct sensing_sensor_api *sensor_api;
	void *last_sample = conn->data;
	void *cur_sample = sensor->data_buf;
	int ret = 0, i;

	__ASSERT(sensor && sensor->dev, "sensor sensitivity test, sensor or sensor device is NULL");
	sensor_api = sensor->dev->api;
	__ASSERT(sensor_api, "sensor sensitivity test, sensor device sensor_api is NULL");

	if (!sensor_api->sensitivity_test) {
		LOG_ERR("sensor:%s not register sensitivity callback", sensor->dev->name);
		return -ENODEV;
	}
	for (i = 0; i < sensor->sensitivity_count; i++) {
		ret |= sensor_api->sensitivity_test(sensor->dev, i, sensor->sensitivity[i],
				last_sample, sensor->sample_size, cur_sample, sensor->sample_size);
	}

	return ret;
}

/* check whether new sample could pass sensitivity test, sample will be sent to client if passed */
static bool sensor_test_sensitivity(struct sensing_sensor *sensor, struct sensing_connection *conn)
{
	int ret = 0;

	/* always send the first sample to client */
	if (conn->next_consume_time == EXEC_TIME_INIT) {
		return true;
	}

	/* skip checking if sensitivity equals to 0 */
	if (!is_filtering_sensitivity(&sensor->sensitivity[0])) {
		return true;
	}

	/* call sensor sensitivity_test, ret:
	 * < 0: means call sensor_sensitivity_test() failed
	 * 0: means sample delta less than sensitivity threshold
	 * 1: means sample data over sensitivity threshold
	 */
	ret = sensor_sensitivity_test(sensor, conn);
	if (ret) {
		return false;
	}

	return true;
}

/* send data to clients based on interval and sensitivity */
static int send_data_to_clients(struct sensing_context *ctx,
				struct sensing_sensor *sensor,
				uint64_t cur_us)
{
	struct sensing_sensor *client;
	struct sensing_connection *conn;
	bool sensitivity_pass = false;

	for_each_client_conn(sensor, conn) {
		client = conn->sink;
		LOG_INF("sensor:%s send data to client", conn->source->dev->name);
		if (!is_client_request_data(conn)) {
			continue;
		}
		/* sensor_test_consume_time(), check whether time is ready or not:
		 * true: it's time for client consuming the data
		 * false: client time not arrived yet, not consume the data
		 */
		if (!sensor_test_consume_time(sensor, conn, cur_us)) {
			continue;
		}

		/* sensor_test_sensitivity(), check sensitivity threshold passing or not:
		 * true: sensitivity checking pass, could post the data
		 * false: sensitivity checking not pass, return
		 */
		sensitivity_pass = sensor_test_sensitivity(sensor, conn);

		update_client_consume_time(sensor, conn);

		if (!sensitivity_pass) {
			continue;
		}

		conn->new_data_arrive = true;
		/* copy sensor data to connection data buf
		 * 1) connection data will be used as last sample in next cycle sensitivity test
		 * 2) connection data will be passed to client in its process() callback
		 */
		memcpy(conn->data, sensor->data_buf, sensor->sample_size);

		if (conn->sink) {
			/* pass the sensor mode to its client */
			client->mode = sensor->mode;
			/* if client switch to polling mode, reset next_execute_time */
			if (client->mode == SENSOR_TRIGGER_MODE_POLLING &&
			    client->next_exec_time == EXEC_TIME_OFF) {
				client->next_exec_time = EXEC_TIME_INIT;
			}
		} else {
			// add_data_to_sensor_ring_buf(ctx, sensor, conn);
			ctx->data_to_ring_buf = true;
		}
	}

	/* notify dispatch thread to dispatch data to application */
	if (ctx->data_to_ring_buf) {
		ctx->data_to_ring_buf = false;
	}

	return 0;
}

static uint64_t calc_next_poll_time(struct sensing_context *ctx)
{
	struct sensing_sensor *sensor;
	uint64_t next_poll = EXEC_TIME_OFF;
	int i;

	for_each_sensor(ctx, i, sensor) {
		if (!is_sensor_state_ready(sensor))
			continue;
		if (!is_sensor_opened(sensor))
			continue;
		if (sensor->next_exec_time == EXEC_TIME_OFF) {
			continue;
		}
		next_poll = MIN(next_poll, sensor->next_exec_time);
	}

	return next_poll;
}

static int calc_sleep_time(struct sensing_context *ctx, uint64_t cur_us)
{
	uint64_t next_poll_time;
	uint32_t sleep_time;

	next_poll_time = calc_next_poll_time(ctx);
	if (next_poll_time == EXEC_TIME_OFF) {
		/* no sampling request. sleep forever */
		sleep_time = UINT32_MAX;
	} else if (next_poll_time <= cur_us) {
		sleep_time = 0;
	} else {
		sleep_time = (uint32_t)((next_poll_time - cur_us) / USEC_PER_MSEC);
	}

	LOG_DBG("calc sleep time, next:%lld, cur:%lld, sleep_time:%d(ms)",
		next_poll_time, cur_us, sleep_time);

	return sleep_time;
}

int loop_sensors(struct sensing_context *ctx)
{
	struct sensing_sensor *sensor;
	uint64_t cur_us;
	int i = 0, ret = 0;

	cur_us = get_us();
	LOG_INF("loop sensors, cur_us:%lld(us)", cur_us);

	for_each_sensor(ctx, i, sensor) {
		if (!sensor_need_exec(sensor, cur_us)) {
			continue;
		}
		ret = sensor_process_data(sensor, cur_us);
		if (ret) {
			LOG_ERR("sensor:%s processed error:%d", sensor->dev->name, ret);
		}
		ret = send_data_to_clients(ctx, sensor, cur_us);
		if (ret) {
			LOG_ERR("sensor:%s send data to client error:%d", sensor->dev->name, ret);
		}
	}

	return calc_sleep_time(ctx, cur_us);
}
