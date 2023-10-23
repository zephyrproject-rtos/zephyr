/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
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

static int init_sensor(struct sensing_sensor *sensor)
{
	struct sensing_connection *conn;
	int i;

	__ASSERT(sensor && sensor->dev, "init sensor, sensor or sensor device is NULL");

	sys_slist_init(&sensor->client_list);

	for (i = 0; i < sensor->reporter_num; i++) {
		conn = &sensor->conns[i];

		/* source sensor has been assigned in compile time */
		init_connection(conn, NULL, sensor);

		LOG_INF("init sensor, reporter:%s, client:%s, connection:%d(%p)",
			conn->source->dev->name, sensor->dev->name, i, conn);
	}

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

	if (sensor->state != SENSING_SENSOR_STATE_READY)
		return -EINVAL;

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

	free(*conn);
	*conn = NULL;

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
	conn->callback_list = *cb_list;

	return 0;
}

int set_interval(struct sensing_connection *conn, uint32_t interval)
{
	return -ENOTSUP;
}

int get_interval(struct sensing_connection *conn, uint32_t *interval)
{
	return -ENOTSUP;
}

int set_sensitivity(struct sensing_connection *conn, int8_t index, uint32_t sensitivity)
{
	return -ENOTSUP;
}

int get_sensitivity(struct sensing_connection *conn, int8_t index, uint32_t *sensitivity)
{
	return -ENOTSUP;
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


DEVICE_DT_INST_DEFINE(0, sensing_init, NULL, &sensing_ctx, NULL,		\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, NULL);
