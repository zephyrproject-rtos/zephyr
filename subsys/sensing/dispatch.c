/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sensing/sensing_sensor.h>
#include "sensor_mgmt.h"

LOG_MODULE_DECLARE(sensing, CONFIG_SENSING_LOG_LEVEL);

/* check whether it is right time for client to consume this sample */
static inline bool sensor_test_consume_time(struct sensing_sensor *sensor,
				     struct sensing_connection *conn,
				     uint64_t cur_time)
{
	LOG_DBG("sensor:%s next_consume_time:%lld cur_time:%lld",
			sensor->dev->name, conn->next_consume_time, cur_time);

	return conn->next_consume_time <= cur_time;
}

static void update_client_consume_time(struct sensing_sensor *sensor,
				       struct sensing_connection *conn)
{
	uint32_t interval = conn->interval;

	if (conn->next_consume_time == 0) {
		conn->next_consume_time = get_us();
	}

	conn->next_consume_time += interval;
}

/* send data to clients based on interval and sensitivity */
static int send_data_to_clients(struct sensing_sensor *sensor,
				void *data)
{
	struct sensing_sensor *client;
	struct sensing_connection *conn;

	for_each_client_conn(sensor, conn) {
		client = conn->sink;
		LOG_DBG("sensor:%s send data to client:%p", conn->source->dev->name, conn);

		if (!is_client_request_data(conn)) {
			continue;
		}

		/* sensor_test_consume_time(), check whether time is ready or not:
		 * true: it's time for client consuming the data
		 * false: client time not arrived yet, not consume the data
		 */
		if (!sensor_test_consume_time(sensor, conn, get_us())) {
			continue;
		}

		update_client_consume_time(sensor, conn);

		if (!conn->callback_list.on_data_event) {
			LOG_WRN("sensor:%s event callback not registered",
					conn->source->dev->name);
			continue;
		}
		conn->callback_list.on_data_event(conn, data,
				conn->callback_list.context);
	}

	return 0;
}

STRUCT_SECTION_START_EXTERN(sensing_sensor);
STRUCT_SECTION_END_EXTERN(sensing_sensor);

static void dispatch_task(void *a, void *b, void *c)
{
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int rc;
	int get_data_rc;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	if (IS_ENABLED(CONFIG_USERSPACE) && !k_is_user_context()) {
		rtio_access_grant(&sensing_rtio_ctx, k_current_get());
		k_thread_user_mode_enter(dispatch_task, a, b, c);
	}

	while (true) {
		struct rtio_cqe cqe;

		rc = rtio_cqe_copy_out(&sensing_rtio_ctx, &cqe, 1, K_FOREVER);
		if (rc < 1) {
			continue;
		}

		/* Cache the data from the CQE */
		rc = cqe.result;

		/* Get the associated data */
		get_data_rc =
			rtio_cqe_get_mempool_buffer(&sensing_rtio_ctx, &cqe, &data, &data_len);
		if (get_data_rc != 0 || data_len == 0) {
			continue;
		}

		if ((uintptr_t)cqe.userdata >=
			    (uintptr_t)STRUCT_SECTION_START(sensing_sensor) &&
		    (uintptr_t)cqe.userdata < (uintptr_t)STRUCT_SECTION_END(sensing_sensor)) {
			struct sensing_sensor *sensor = cqe.userdata;

			send_data_to_clients(sensor, data);
		}

		rtio_release_buffer(&sensing_rtio_ctx, data, data_len);
	}
}

K_THREAD_DEFINE(sensing_dispatch, CONFIG_SENSING_DISPATCH_THREAD_STACK_SIZE, dispatch_task,
		NULL, NULL, NULL, CONFIG_SENSING_DISPATCH_THREAD_PRIORITY, 0, 0);
