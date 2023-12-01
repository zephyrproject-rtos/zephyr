/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_MGMT_H_
#define SENSOR_MGMT_H_

#include <zephyr/sensing/sensing_datatypes.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/ring_buffer.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define for_each_sensor(sensor)							\
	STRUCT_SECTION_FOREACH(sensing_sensor, sensor)

#define for_each_sensor_reverse(sensor)					\
	STRUCT_SECTION_START_EXTERN(sensing_sensor);				\
	STRUCT_SECTION_END_EXTERN(sensing_sensor);				\
	for (struct sensing_sensor *sensor = STRUCT_SECTION_END(sensing_sensor)	\
		- 1;								\
	     ({ __ASSERT(sensor >= STRUCT_SECTION_START(sensing_sensor) - 1,	\
		"unexpected list start location");				\
		sensor >= STRUCT_SECTION_START(sensing_sensor); });		\
	     sensor--)


#define for_each_client_conn(sensor, client)				\
	SYS_SLIST_FOR_EACH_CONTAINER(&sensor->client_list, client, snode)

#define EXEC_TIME_INIT 0
#define EXEC_TIME_OFF UINT64_MAX

extern struct rtio sensing_rtio_ctx;
/**
 * @struct sensing_context
 * @brief sensing subsystem context to include global variables
 */
struct sensing_context {
	bool sensing_initialized;
	struct k_sem event_sem;
	atomic_t event_flag;
};

int open_sensor(struct sensing_sensor *sensor, struct sensing_connection **conn);
int close_sensor(struct sensing_connection **conn);
int sensing_register_callback(struct sensing_connection *conn,
			      struct sensing_callback_list *cb_list);
int set_interval(struct sensing_connection *conn, uint32_t interval);
int get_interval(struct sensing_connection *con, uint32_t *sensitivity);
int set_sensitivity(struct sensing_connection *conn, int8_t index, uint32_t interval);
int get_sensitivity(struct sensing_connection *con, int8_t index, uint32_t *sensitivity);

static inline struct sensing_sensor *get_sensor_by_dev(const struct device *dev)
{
	STRUCT_SECTION_FOREACH(sensing_sensor, sensor) {
		if (sensor->dev == dev) {
			return sensor;
		}
	}

	__ASSERT(true, "device %s is not a sensing sensor", dev->name);

	return NULL;
}

static inline uint16_t get_reporter_sample_size(const struct sensing_sensor *sensor, int i)
{
	const struct sensing_sensor *reporter;

	__ASSERT(i < sensor->reporter_num, "dt index should less than reporter num");

	reporter = sensor->conns[i].source;

	return reporter->register_info->sample_size;
}

static inline struct sensing_sensor *get_reporter_sensor(struct sensing_sensor *sensor, int index)
{
	if (!sensor || index >= sensor->reporter_num) {
		return NULL;
	}

	return sensor->conns[index].source;
}

static inline const struct sensing_sensor_info *get_sensor_info(struct sensing_connection *conn)
{
	__ASSERT(conn, "get sensor info, connection not be NULL");

	__ASSERT(conn->source, "get sensor info, sensing_sensor is NULL");

	return conn->source->info;
}

/* check if client has requested data from reporter */
static inline bool is_client_request_data(struct sensing_connection *conn)
{
	return conn->interval != 0;
}

static inline uint64_t get_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}

static inline bool is_sensor_state_ready(struct sensing_sensor *sensor)
{
	return (sensor->state == SENSING_SENSOR_STATE_READY);
}

/* this function is used to decide whether filtering sensitivity checking
 * for example: filter sensitivity checking if sensitivity value is 0.
 */
static inline bool is_filtering_sensitivity(int *sensitivity)
{
	bool filtering = false;

	__ASSERT(sensitivity, "sensitivity should not be NULL");
	for (int i = 0; i < CONFIG_SENSING_MAX_SENSITIVITY_COUNT; i++) {
		if (sensitivity[i] != 0) {
			filtering = true;
			break;
		}
	}

	return filtering;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MGMT_H_ */
