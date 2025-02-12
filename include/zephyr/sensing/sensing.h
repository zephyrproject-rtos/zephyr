/*
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_H_
#define ZEPHYR_INCLUDE_SENSING_H_

/**
 * @defgroup sensing Sensing
 * @defgroup sensing_api Sensing Subsystem API
 * @ingroup sensing
 * @defgroup sensing_sensor_types Sensor Types
 * @ingroup sensing
 * @defgroup sensing_datatypes Data Types
 * @ingroup sensing
 */

#include <zephyr/sensing/sensing_datatypes.h>
#include <zephyr/sensing/sensing_sensor_types.h>
#include <zephyr/device.h>

/**
 * @brief Sensing Subsystem API
 * @addtogroup sensing_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @struct sensing_sensor_version
 * @brief Sensor Version
 */
struct sensing_sensor_version {
	union {
		uint32_t value; /**< The version represented as a 32-bit value. */
		struct {
			uint8_t major;  /**< The major version number. */
			uint8_t minor;  /**< The minor version number. */
			uint8_t hotfix; /**< The hotfix version number. */
			uint8_t build;  /**< The build version number. */
		};
	};
};

/**
 * @brief Macro to create a sensor version value.
 *
 */
#define SENSING_SENSOR_VERSION(_major, _minor, _hotfix, _build)         \
				(FIELD_PREP(GENMASK(31, 24), _major) |  \
				 FIELD_PREP(GENMASK(23, 16), _minor) |  \
				 FIELD_PREP(GENMASK(15, 8), _hotfix) |  \
				 FIELD_PREP(GENMASK(7, 0), _build))


/**
 * @brief Sensor flag indicating if this sensor is on event reporting data.
 *
 * Reporting sensor data when the sensor event occurs, such as a motion detect sensor reporting
 * a motion or motionless detected event.
 */
#define SENSING_SENSOR_FLAG_REPORT_ON_EVENT			BIT(0)

/**
 * @brief Sensor flag indicating if this sensor is on change reporting data.
 *
 * Reporting sensor data when the sensor data changes.
 *
 * Exclusive with \ref SENSING_SENSOR_FLAG_REPORT_ON_EVENT
 */
#define SENSING_SENSOR_FLAG_REPORT_ON_CHANGE			BIT(1)

/**
 * @brief SENSING_SENSITIVITY_INDEX_ALL indicating sensitivity of each data field should be set
 *
 */
#define SENSING_SENSITIVITY_INDEX_ALL -1

/**
 * @brief Sensing subsystem sensor state.
 *
 */
enum sensing_sensor_state {
	SENSING_SENSOR_STATE_READY = 0,   /**< The sensor is ready. */
	SENSING_SENSOR_STATE_OFFLINE = 1, /**< The sensor is offline. */
};

/**
 * @brief Sensing subsystem sensor config attribute
 *
 */
enum sensing_sensor_attribute {
	/** The interval attribute of a sensor configuration. */
	SENSING_SENSOR_ATTRIBUTE_INTERVAL = 0,
	/** The sensitivity attribute of a sensor configuration. */
	SENSING_SENSOR_ATTRIBUTE_SENSITIVITY = 1,
	/** The latency attribute of a sensor configuration. */
	SENSING_SENSOR_ATTRIBUTE_LATENCY = 2,
	/** The maximum number of attributes that a sensor configuration can have. */
	SENSING_SENSOR_ATTRIBUTE_MAX,
};

/**
 * @brief Define Sensing subsystem sensor handle
 *
 */
typedef void *sensing_sensor_handle_t;

/**
 * @brief Sensor data event receive callback.
 *
 * @param handle The sensor instance handle.
 * @param buf The data buffer with sensor data.
 * @param context User provided context pointer.
 */
typedef void (*sensing_data_event_t)(
		sensing_sensor_handle_t handle,
		const void *buf,
		void *context);

/**
 * @struct sensing_sensor_info
 * @brief Sensor basic constant information
 *
 */
struct sensing_sensor_info {
	/** Name of the sensor instance */
	const char *name;

	/** Friendly name of the sensor instance */
	const char *friendly_name;

	/** Vendor name of the sensor instance */
	const char *vendor;

	/** Model name of the sensor instance */
	const char *model;

	/** Sensor type */
	const int32_t type;

	/** Minimal report interval in micro seconds */
	const uint32_t minimal_interval;
};

/**
 * @struct sensing_callback_list
 * @brief Sensing subsystem event callback list
 *
 */
struct sensing_callback_list {
	sensing_data_event_t on_data_event; /**< Callback function for a sensor data event. */
	void *context;                      /**< Associated context with on_data_event */
};

/**
 * @struct sensing_sensor_config
 * @brief Sensing subsystem sensor configure, including interval, sensitivity, latency
 *
 */
struct sensing_sensor_config {
	enum sensing_sensor_attribute attri; /**< Attribute of the sensor configuration. */

	/** \ref SENSING_SENSITIVITY_INDEX_ALL */
	int8_t data_field; /**< Data field of the sensor configuration. */

	union {
		/** Interval between two sensor samples in microseconds (us). */
		uint32_t interval;

		/**
		 * Sensitivity threshold for reporting new data. A new sensor sample is reported
		 * only if the difference between it and the previous sample exceeds this
		 * sensitivity value.
		 */
		uint32_t sensitivity;

		/**
		 * Maximum duration for batching sensor samples before reporting in
		 * microseconds (us). This defines how long sensor samples can be
		 * accumulated before they must be reported.
		 */
		uint64_t latency;
	};
};

/**
 * @brief Get all supported sensor instances' information.
 *
 * This API just returns read only information of sensor instances, pointer info will
 * directly point to internal buffer, no need for caller to allocate buffer,
 * no side effect to sensor instances.
 *
 * @param num_sensors Get number of sensor instances.
 * @param info For receiving sensor instances' information array pointer.
 * @return 0 on success or negative error value on failure.
 */
int sensing_get_sensors(int *num_sensors, const struct sensing_sensor_info **info);

/**
 * @brief Open sensor instance by sensing sensor info
 *
 * Application clients use it to open a sensor instance and get its handle.
 * Support multiple Application clients for open same sensor instance,
 * in this case, the returned handle will different for different clients.
 * meanwhile, also register sensing callback list
 *
 * @param info The sensor info got from \ref sensing_get_sensors
 * @param cb_list callback list to be registered to sensing, must have a static
 *                lifetime.
 * @param handle The opened instance handle, if failed will be set to NULL.
 * @return 0 on success or negative error value on failure.
 */
int sensing_open_sensor(
		const struct sensing_sensor_info *info,
		struct sensing_callback_list *cb_list,
		sensing_sensor_handle_t *handle);

/**
 * @brief Open sensor instance by device.
 *
 * Application clients use it to open a sensor instance and get its handle.
 * Support multiple Application clients for open same sensor instance,
 * in this case, the returned handle will different for different clients.
 * meanwhile, also register sensing callback list.
 *
 * @param dev pointer device get from device tree.
 * @param cb_list callback list to be registered to sensing, must have a static
 *                lifetime.
 * @param handle The opened instance handle, if failed will be set to NULL.
 * @return 0 on success or negative error value on failure.
 */
int sensing_open_sensor_by_dt(
		const struct device *dev, struct sensing_callback_list *cb_list,
		sensing_sensor_handle_t *handle);

/**
 * @brief Close sensor instance.
 *
 * @param handle The sensor instance handle need to close.
 * @return 0 on success or negative error value on failure.
 */
int sensing_close_sensor(
		sensing_sensor_handle_t *handle);

/**
 * @brief Set current config items to Sensing subsystem.
 *
 * @param handle The sensor instance handle.
 * @param configs The configs to be set according to config attribute.
 * @param count count of configs.
 * @return 0 on success or negative error value on failure, not support etc.
 */
int sensing_set_config(
		sensing_sensor_handle_t handle,
		struct sensing_sensor_config *configs, int count);

/**
 * @brief Get current config items from Sensing subsystem.
 *
 * @param handle The sensor instance handle.
 * @param configs The configs to be get according to config attribute.
 * @param count count of configs.
 * @return 0 on success or negative error value on failure, not support etc.
 */
int sensing_get_config(
		sensing_sensor_handle_t handle,
		struct sensing_sensor_config *configs, int count);

/**
 * @brief Get sensor information from sensor instance handle.
 *
 * @param handle The sensor instance handle.
 * @return a const pointer to \ref sensing_sensor_info on success or NULL on failure.
 */
const struct sensing_sensor_info *sensing_get_sensor_info(
		sensing_sensor_handle_t handle);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*ZEPHYR_INCLUDE_SENSING_H_*/
