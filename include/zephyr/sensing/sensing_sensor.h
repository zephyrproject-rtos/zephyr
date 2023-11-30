/*
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_SENSOR_H_
#define ZEPHYR_INCLUDE_SENSING_SENSOR_H_

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>

/**
 * @defgroup sensing_sensor Sensing Sensor API
 * @ingroup sensing
 * @defgroup sensing_sensor_callbacks Sensor Callbacks
 * @ingroup sensing_sensor
 */

/**
 * @brief Sensing Sensor API
 * @addtogroup sensing_sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sensor registration information
 *
 */
struct sensing_sensor_register_info {
	/**
	 * Sensor flags
	 */
	uint16_t flags;

	/**
	 * Sample size in bytes for a single sample of the registered sensor.
	 * sensing runtime need this information for internal buffer allocation.
	 */
	uint16_t sample_size;

	/**
	 * The number of sensor sensitivities
	 */
	uint8_t sensitivity_count;

	/**
	 * Sensor version.
	 * Version can be used to identify different versions of sensor implementation.
	 */
	struct sensing_sensor_version version;
};

enum {
	EVENT_CONFIG_READY,
};

enum {
	SENSOR_LATER_CFG_BIT,
};

/**
 * @brief Connection between a source and sink of sensor data
 */
struct sensing_connection {
	sys_snode_t snode;
	struct sensing_sensor *source;
	struct sensing_sensor *sink;
	/* interval and sensitivity set from client(sink) to reporter(source) */
	uint32_t interval;
	int sensitivity[CONFIG_SENSING_MAX_SENSITIVITY_COUNT];
	/* copy sensor data to connection data buf from reporter */
	void *data;
	/* client(sink) next consume time */
	uint64_t next_consume_time;
	/* post data to application */
	struct sensing_callback_list callback_list;
};

/**
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
	const struct sensing_sensor_register_info *register_info;
	const uint16_t reporter_num;
	sys_slist_t client_list;
	uint32_t interval;
	uint8_t sensitivity_count;
	int sensitivity[CONFIG_SENSING_MAX_SENSITIVITY_COUNT];
	enum sensing_sensor_state state;
	/* runtime info */
	struct rtio_iodev *iodev;
	struct k_timer timer;
	struct rtio_sqe *stream_sqe;
	atomic_t flag;
	struct sensing_connection *conns;
};

#define SENSING_SENSOR_INFO_NAME(node, idx)					\
	_CONCAT(_CONCAT(__sensing_sensor_info_, idx), DEVICE_DT_NAME_GET(node))

#define SENSING_SENSOR_INFO_DEFINE(node, idx)				\
	const static STRUCT_SECTION_ITERABLE(sensing_sensor_info,	\
			SENSING_SENSOR_INFO_NAME(node, idx)) = {	\
		.type = DT_PROP_BY_IDX(node, sensor_types, idx),		\
		.name = DT_NODE_FULL_NAME(node),			\
		.friendly_name = DT_PROP(node, friendly_name),		\
		.vendor = DT_NODE_VENDOR_OR(node, NULL),		\
		.model = DT_NODE_MODEL_OR(node, NULL),			\
		.minimal_interval = DT_PROP(node, minimal_interval),\
	};

#define SENSING_CONNECTIONS_NAME(node, idx)					\
	_CONCAT(_CONCAT(__sensing_connections_, idx), DEVICE_DT_NAME_GET(node))

#define SENSING_SENSOR_SOURCE_NAME(idx, node)				\
	SENSING_SENSOR_NAME(DT_PHANDLE_BY_IDX(node, reporters, idx),	\
		DT_PROP_BY_IDX(node, reporters_index, idx))

#define SENSING_SENSOR_SOURCE_EXTERN(idx, node)				\
extern struct sensing_sensor SENSING_SENSOR_SOURCE_NAME(idx, node);	\

#define SENSING_CONNECTION_INITIALIZER(source_name, cb_list_ptr)	\
{									\
	.callback_list = *cb_list_ptr,					\
	.source = &source_name,						\
}

#define SENSING_CONNECTION_DEFINE(idx, node, cb_list_ptr)		\
	SENSING_CONNECTION_INITIALIZER(SENSING_SENSOR_SOURCE_NAME(idx, node), \
				       cb_list_ptr)

#define SENSING_CONNECTIONS_DEFINE(node, idx, num, cb_list_ptr)		\
	LISTIFY(num, SENSING_SENSOR_SOURCE_EXTERN,			\
				(), node)				\
	static struct sensing_connection				\
			SENSING_CONNECTIONS_NAME(node, idx)[(num)] = {	\
		LISTIFY(num, SENSING_CONNECTION_DEFINE,			\
				(,), node, cb_list_ptr)			\
	};

#define SENSING_SENSOR_IODEV_NAME(node, idx)				\
	_CONCAT(_CONCAT(__sensing_iodev_, idx), DEVICE_DT_NAME_GET(node))

#define SENSING_SENSOR_IODEV_DEFINE(node, idx)						\
	COND_CODE_1(DT_PROP(node, stream_mode),						\
		(SENSOR_DT_STREAM_IODEV(SENSING_SENSOR_IODEV_NAME(node, idx), node)),	\
		(SENSOR_DT_READ_IODEV(SENSING_SENSOR_IODEV_NAME(node, idx), node)));

#define SENSING_SENSOR_NAME(node, idx)					\
	_CONCAT(_CONCAT(__sensing_sensor_, idx), DEVICE_DT_NAME_GET(node))

#define SENSING_SENSOR_DEFINE(node, prop, idx, reg_ptr, cb_list_ptr)	\
	SENSING_SENSOR_INFO_DEFINE(node, idx)				\
	SENSING_CONNECTIONS_DEFINE(node, idx,				\
			DT_PROP_LEN_OR(node, reporters, 0), cb_list_ptr)\
	SENSING_SENSOR_IODEV_DEFINE(node, idx)				\
	STRUCT_SECTION_ITERABLE(sensing_sensor,				\
				SENSING_SENSOR_NAME(node, idx)) = {	\
		.dev = DEVICE_DT_GET(node),				\
		.info = &SENSING_SENSOR_INFO_NAME(node, idx),		\
		.register_info = reg_ptr,				\
		.reporter_num = DT_PROP_LEN_OR(node, reporters, 0),	\
		.conns = SENSING_CONNECTIONS_NAME(node, idx),		\
		.iodev = &SENSING_SENSOR_IODEV_NAME(node, idx),		\
	};

#define SENSING_SENSORS_DEFINE(node, reg_ptr, cb_list_ptr)		\
	DT_FOREACH_PROP_ELEM_VARGS(node, sensor_types,			\
			SENSING_SENSOR_DEFINE, reg_ptr, cb_list_ptr)
/**
 * @brief Like SENSOR_DEVICE_DT_DEFINE() with sensing specifics.
 *
 * @details Defines a sensor which implements the sensor API. May define an
 * element in the sensing sensor iterable section used to enumerate all sensing
 * sensors.
 *
 * @param node_id The devicetree node identifier.
 *
 * @param reg_ptr Pointer to the device's sensing_sensor_register_info.
 *
 * @param init_fn Name of the init function of the driver.
 *
 * @param pm_device PM device resources reference (NULL if device does not use
 * PM).
 *
 * @param data_ptr Pointer to the device's private data.
 *
 * @param cfg_ptr The address to the structure containing the configuration
 * information for this instance of the driver.
 *
 * @param level The initialization level. See SYS_INIT() for details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api_ptr Provides an initial pointer to the API function struct used
 * by the driver. Can be NULL.
 */
#define SENSING_SENSORS_DT_DEFINE(node_id, reg_ptr, cb_list_ptr,	\
				init_fn, pm_device,			\
				data_ptr, cfg_ptr, level, prio,		\
				api_ptr, ...)				\
	SENSOR_DEVICE_DT_DEFINE(node_id, init_fn, pm_device,		\
			 data_ptr, cfg_ptr, level, prio,		\
			 api_ptr, __VA_ARGS__);				\
									\
	SENSING_SENSORS_DEFINE(node_id, reg_ptr, cb_list_ptr);

/**
 * @brief Like SENSING_SENSORS_DT_DEFINE() for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number. This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to SENSING_SENSORS_DT_DEFINE().
 *
 * @param ... other parameters as expected by SENSING_SENSORS_DT_DEFINE().
 */
#define SENSING_SENSORS_DT_INST_DEFINE(inst, reg_ptr, cb_list_ptr, ...)	\
	SENSING_SENSORS_DT_DEFINE(DT_DRV_INST(inst), reg_ptr,		\
			cb_list_ptr, __VA_ARGS__)

/**
 * @brief Get reporter handles	of a given sensor instance by sensor type.
 *
 * @param dev The sensor instance device structure.
 *
 * @param type The given type, \ref SENSING_SENSOR_TYPE_ALL to get reporters
 * with all types.
 *
 * @param max_handles The max count of the \p reporter_handles array input. Can
 * get real count number via \ref sensing_sensor_get_reporters_count
 *
 * @param reporter_handles Input handles array for receiving found reporter
 * sensor instances
 *
 * @return number of reporters found, 0 returned if not found.
 */
int sensing_sensor_get_reporters(
		const struct device *dev, int type,
		sensing_sensor_handle_t *reporter_handles, int max_handles);

/**
 * @brief Get reporters count of a given sensor instance by sensor type.
 *
 * @param dev The sensor instance device structure.
 *
 * @param type The sensor type for checking, \ref SENSING_SENSOR_TYPE_ALL
 *
 * @return Count of reporters by \p type, 0 returned if no reporters by \p type.
 */
int sensing_sensor_get_reporters_count(
		const struct device *dev, int type);

/**
 * @brief Get this sensor's state
 *
 * @param dev The sensor instance device structure.
 *
 * @param state Returned sensor state value
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_sensor_get_state(
		const struct device *dev,
		enum sensing_sensor_state *state);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*ZEPHYR_INCLUDE_SENSING_SENSOR_H_*/
