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

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/datatypes.h>
#include <zephyr/sensing/sensor_types.h>

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
		uint32_t value;
		struct {
			uint8_t major;
			uint8_t minor;
			uint8_t hotfix;
			uint8_t build;
		};
	};
};

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
 * @brief Sensing subsystem sensor state.
 *
 */
enum sensing_sensor_state {
	SENSING_SENSOR_STATE_READY = 0,
	SENSING_SENSOR_STATE_OFFLINE = 1,
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
 *
 * @param buf The data buffer with sensor data.
 */
typedef void (*sensing_data_event_t)(
		sensing_sensor_handle_t handle,
		const void *buf);

/**
 * @struct sensing_sensor_info
 * @brief Sensor basic constant information
 *
 */
struct sensing_sensor_info {
	const struct sensor_info * info;

	/** Sensor type */
	int32_t type;
};

/**
 * @struct sensing_callback_list
 * @brief Sensing subsystem event callback list
 *
 */
struct sensing_callback_list {
	sensing_data_event_t on_data_event;
};


 /**
  * @brief Get all supported sensor instances' information.
  *
  * This API just returns read only information of sensor instances, pointer info will
  * directly point to internal buffer, no need for caller to allocate buffer,
  * no side effect to sensor instances.
  *
  * @param num_sensors Get number of sensor instances.
  *
  * @param info For receiving sensor instances' information array pointer.
  *
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
 *
 * @param cb_list callback list to be registered to sensing.
 *
 * @param *handle The opened instance handle, if failed will be set to NULL.
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_open_sensor(
		const struct sensing_sensor_info *info,
		const struct sensing_callback_list *cb_list,
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
 *
 * @param cb_list callback list to be registered to sensing.
 *
 * @param *handle The opened instance handle, if failed will be set to NULL.
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_open_sensor_by_dt(
		const struct device *dev, const struct sensing_callback_list *cb_list,
		sensing_sensor_handle_t *handle);

/**
 * @brief Close sensor instance.
 *
 * @param handle The sensor instance handle need to close.
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_close_sensor(
		sensing_sensor_handle_t *handle);

/**
 * @brief Get sensor information from sensor instance handle.
 *
 * @param handle The sensor instance handle.
 *
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
