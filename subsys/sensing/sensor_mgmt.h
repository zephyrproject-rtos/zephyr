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

#ifdef __cplusplus
extern "C" {
#endif

#define PHANDLE_DEVICE_BY_IDX(idx, node, prop)					\
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node, prop, idx))

#define PHANDLE_DEVICE_LIST(node, prop)						\
{										\
	LISTIFY(DT_PROP_LEN_OR(node, prop, 0),					\
			PHANDLE_DEVICE_BY_IDX,					\
			(,),							\
			node,							\
			prop)							\
}

#define SENSING_SENSOR_INFO_NAME(node)						\
	_CONCAT(__sensing_sensor_info_, DEVICE_DT_NAME_GET(node))

#define SENSING_SENSOR_INFO_DEFINE(node)					\
	const static STRUCT_SECTION_ITERABLE(sensing_sensor_info,			\
				       SENSING_SENSOR_INFO_NAME(node)) = {	\
		.type = DT_PROP(node, sensor_type),				\
		.name = DT_NODE_FULL_NAME(node),				\
		.friendly_name = DT_PROP(node, friendly_name),			\
		.vendor = DT_NODE_VENDOR_OR(node, NULL),			\
		.model = DT_NODE_MODEL_OR(node, NULL),				\
		.minimal_interval = DT_PROP(node, minimal_interval),		\
	};

#define SENSING_SENSOR_NAME(node)						\
	_CONCAT(__sensing_sensor_, DEVICE_DT_NAME_GET(node))

#define SENSING_SENSOR_DEFINE(node)						\
	static STRUCT_SECTION_ITERABLE(sensing_sensor,				\
				       SENSING_SENSOR_NAME(node)) = {		\
		.dev = DEVICE_DT_GET(node),					\
		.info = &SENSING_SENSOR_INFO_NAME(node),			\
		.reporter_num = DT_PROP_LEN_OR(node, reporters, 0),		\
		.reporters = PHANDLE_DEVICE_LIST(node, reporters),		\
	};

#define for_each_sensor(ctx, i, sensor)						\
	for (i = 0; i < ctx->sensor_num && (sensor = ctx->sensors[i]) != NULL; i++)

enum sensor_trigger_mode {
	SENSOR_TRIGGER_MODE_POLLING = 1,
	SENSOR_TRIGGER_MODE_DATA_READY = 2,
};

/**
 * @struct sensing_connection information
 * @brief sensing_connection indicates connection from reporter(source) to client(sink)
 */
struct sensing_connection {
	struct sensing_sensor *source;
	struct sensing_sensor *sink;
	/* interval and sensitivity set from client(sink) to reporter(source) */
	uint32_t interval;
	int sensitivity[CONFIG_SENSING_MAX_SENSITIVITY_COUNT];
	/* copy sensor data to connection data buf from reporter */
	void *data;
	/* client(sink) next consume time */
	sys_snode_t snode;
	/* post data to application */
	sensing_data_event_t data_evt_cb;
};

/**
 * @struct sensing_sensor
 * @brief Internal sensor instance data structure.
 *
 * Each sensor instance will have its unique data structure for storing all
 * it's related information.
 *
 * Sensor management will enumerate all these instance data structures,
 * build report relationship model base on them, etc.
 */
struct sensing_sensor {
	const struct device *dev;
	const struct sensing_sensor_info *info;
	const uint16_t reporter_num;
	sys_slist_t client_list;
	uint32_t interval;
	uint8_t sensitivity_count;
	int sensitivity[CONFIG_SENSING_MAX_SENSITIVITY_COUNT];
	enum sensing_sensor_state state;
	enum sensor_trigger_mode mode;
	/* runtime info */
	uint16_t sample_size;
	void *data_buf;
	struct sensing_connection *conns;
	const struct device *reporters[];
};

int open_sensor(struct sensing_sensor *sensor, struct sensing_connection **conn);
int close_sensor(struct sensing_connection **conn);
int sensing_register_callback(struct sensing_connection *conn,
			      const struct sensing_callback_list *cb_list);
int set_interval(struct sensing_connection *conn, uint32_t interval);
int get_interval(struct sensing_connection *con, uint32_t *sensitivity);
int set_sensitivity(struct sensing_connection *conn, int8_t index, uint32_t interval);
int get_sensitivity(struct sensing_connection *con, int8_t index, uint32_t *sensitivity);


static inline bool is_phy_sensor(struct sensing_sensor *sensor)
{
	return sensor->reporter_num == 0;
}

static inline uint16_t get_reporter_sample_size(const struct sensing_sensor *sensor, int i)
{
	__ASSERT(i < sensor->reporter_num, "dt index should less than reporter num");

	return ((struct sensing_sensor_ctx *)
		sensor->reporters[i]->data)->register_info->sample_size;
}

static inline struct sensing_sensor *get_sensor_by_dev(const struct device *dev)
{
	return dev ?
		(struct sensing_sensor *)((struct sensing_sensor_ctx *)dev->data)->priv_ptr : NULL;
}

static inline struct sensing_sensor *get_reporter_sensor(struct sensing_sensor *sensor, int index)
{
	if (!sensor || index >= sensor->reporter_num) {
		return NULL;
	}

	return get_sensor_by_dev(sensor->reporters[index]);
}

static inline const struct sensing_sensor_info *get_sensor_info(struct sensing_connection *conn)
{
	__ASSERT(conn, "get sensor info, connection not be NULL");

	__ASSERT(conn->source, "get sensor info, sensing_sensor is NULL");

	return conn->source->info;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* SENSOR_MGMT_H_ */
