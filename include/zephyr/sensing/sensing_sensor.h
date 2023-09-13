/*
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_SENSOR_H_
#define ZEPHYR_INCLUDE_SENSING_SENSOR_H_

#include <zephyr/sensing/sensing.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <zephyr/drivers/sensor.h>

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

/**
 * @brief Sensor context data structure
 *
 */
struct sensing_sensor_ctx {

	/**
	 * For sensing runtime internal private data, sensor should not see and touch
	 */
	void *priv_ptr;

	/**
	 * Pointer to the sensor register information.
	 */
	const struct sensing_sensor_register_info *register_info;

	/**
	 * For sensor private context data, registered by sensor with \ref SENSING_SENSOR_DT_DEFINE.
	 * Sensor could use \ref sensing_sensor_get_ctx_data to fetch out this filed with
	 * struct device.
	 */
	void *const sensor_ctx_ptr;
};

static inline int sensing_sensor_dev_init(
		const struct device *dev)
{
	/**
	 * Nothing need to do in system auto initialization.
	 * Sensor subsystem runtime will call each sensor instance's initialization
	 * function via API callback according sensor reporting dependency sequences.
	 * Sensor subsystem can make sure the depends sensor instances always initialized before
	 * client sensors.
	 */
	return 0;
}

/**
 * @brief Macro for define a sensor instance from device tree node id
 *
 * This macro also defined a struct device for this sensor instance, and registered sensors'
 * private context data, configuration data structure and API.
 *
 * sensing_init will enumerate all sensor instances from device tree, and initialize each sensor
 * instance defined by this macro.
 *
 */

#define SENSING_SENSOR_DT_DEFINE(node_id, reg_ptr, ctx_ptr, api_ptr)			\
	static struct sensing_sensor_ctx							\
		_CONCAT(__sensing_sensor_ctx_, Z_DEVICE_DT_DEV_ID(node_id)) = {		\
			.register_info = reg_ptr,					\
			.sensor_ctx_ptr = ctx_ptr,					\
		};									\
	DEVICE_DT_DEFINE(node_id, sensing_sensor_dev_init, NULL,			\
			&_CONCAT(__sensing_sensor_ctx_, Z_DEVICE_DT_DEV_ID(node_id)),	\
			NULL, POST_KERNEL, 99, api_ptr)

/**
 * @brief Get registered context data pointer for a sensor instance.
 *
 * Used by a sensor instance to get its registered context data pointer with its struct device.
 *
 * @param dev The sensor instance device structure.
 */
static inline void *sensing_sensor_get_ctx_data(
		const struct device *dev)
{
	struct sensing_sensor_ctx *data = dev->data;

	return data->sensor_ctx_ptr;
}

/**
 * @brief Post sensor data, sensor subsystem runtime will deliver to it's
 * clients.
 *
 * Unblocked function, returned immediately.
 *
 * Used by a virtual sensor to post data to it's clients.
 *
 * A reporter sensor can use this API to post data to it's clients.
 * For example, when a virtual sensor computed a data, then can use this API
 * to deliver the data to it's clients.
 * Please note, this API just for reporter post data to the sensor subsystem
 * runtime, the runtime will help delivered the data to it's all clients
 * according clients' configurations such as reporter interval, data change sensitivity.
 *
 * @param dev The sensor instance device structure.
 *
 * @param buf The data buffer.
 *
 * @param size The buffer size in bytes.
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_sensor_post_data(
		const struct device *dev,
		void *buf, int size);

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
		const int *reporter_handles, int max_handles);

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
 * @brief Trigger the data ready event to sensing
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_sensor_notify_data_ready(
		const struct device *dev);

/**
 * @brief Set the data ready mode of the sensor
 *
 * @param dev Pointer to the sensor device
 *
 * @param data_ready Enable/disable the data ready mode. Default:disabled
 *
 * @return 0 on success or negative error value on failure.
 */
int sensing_sensor_set_data_ready(
		const struct device *dev, bool data_ready);

/**
 * @}
 */

/**
 * @brief Sensor Callbacks
 * @addtogroup sensing_sensor_callbacks
 * \{
 */

/**
 * @brief Sensor initialize.
 *
 * Sensor can initialize it's runtime context in this callback.
 *
 * @param dev The sensor instance device structure.
 *
 * @param info The sensor instance's constant information.
 *
 * @param reporter_handles The reporters handles for this sensor, NULL for physical sensors.
 *
 * @param reporters_count The number of reporters, zero for physical sensors.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_init_t)(
		const struct device *dev, const struct sensing_sensor_info *info,
		const sensing_sensor_handle_t *reporter_handles, int reporters_count);

/**
 * @brief Sensor's de-initialize.
 *
 * Sensor can release it's runtime context in this callback.
 *
 * @param dev The sensor instance device structure.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_deinit_t)(
		const struct device *dev);

/**
 * @brief Sensor reset.
 *
 * Sensor can reset its runtime context in this callback to default values without resources
 * release and re-allocation.
 *
 * Its very useful for a virtual sensor to quickly reset its runtime context to a default state.
 *
 * @param dev The sensor instance device structure.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_reset_t)(
		const struct device *dev);

/**
 * @brief Sensor read sample.
 *
 * Only physical sensor need implement this callback.
 * Physical sensor can fetch sample data from sensor device in this callback
 *
 * @param dev The sensor instance device structure.
 *
 * @param buf Sensor subsystem runtime allocated buffer, and passed its pointer
 * to this sensor for store fetched sample.
 *
 * @param size The size of the buffer in bytes.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_read_sample_t)(
		const struct device *dev,
		void *buf, int size);

/**
 * @brief Sensor process data.
 *
 * Only virtual sensor need implement this callback.
 * Virtual sensor can receive reporter's data and do fusion computing
 * in this callback.
 *
 * @param dev The sensor instance device structure.
 *
 * @param reporter The reporter handle who delivered this sensor data
 *
 * @param buf The buffer stored the reporter's sensor data.
 *
 * @param size The size of the buffer in bytes.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_process_t)(
		const struct device *dev,
		int reporter,
		void *buf, int size);

/**
 * @brief Trigger a sensor to do self calibration
 *
 * If not support self calibration, can not implement this callback.
 *
 * @param dev The sensor instance device structure.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_self_calibration_t)(
		const struct device *dev);

/**
 * @brief Sensitivity arbitration.
 *
 * This callback API provides a chance for sensor to do customized arbitration on data change
 * sensitivity.
 * The sensor can check two sequential samples with client's sensitivity value (passed with
 * parameters in this callback) and decide if can pass the sensor sample to its client.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param sensitivity The sensitivity value.
 *
 * @param last_sample_buf The buffer stored last sample data.
 *
 * @param last_sample_size The size of last sample's data buffer in bytes
 *
 * @param current_sample_buf The buffer stored current sample data.
 *
 * @param current_sample_size The size of current sample's data buffer in bytes
 *
 * @return 0 on test passed or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_sensitivity_test_t)(
		const struct device *dev,
		int index, uint32_t sensitivity,
		void *last_sample_buf, int last_sample_size,
		void *current_sample_buf, int current_sample_size);

/**
 * @brief Set current report interval.
 *
 * @param dev The sensor instance device structure.
 *
 * @param value The value to be set.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_set_interval_t)(
		const struct device *dev,
		uint32_t value);

/**
 * @brief Get current report interval.
 *
 * @param dev The sensor instance device structure.
 *
 * @param value The data buffer to receive value.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_get_interval_t)(
		const struct device *dev,
		uint32_t *value);

/**
 * @brief Set data change sensitivity.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support set separated sensitivity for each data field, or global
 * sensitivity for all data fields.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The value to be set.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
typedef int (*sensing_sensor_set_sensitivity_t)(
		const struct device *dev,
		int index, uint32_t value);

/**
 * @brief Get current data change sensitivity.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support get separated sensitivity for each data field, or global
 * sensitivity for all data fields.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The data buffer to receive value.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_get_sensitivity_t)(
		const struct device *dev,
		int index, uint32_t *value);

/**
 * @brief Set data range.
 *
 * Some sensors especially for physical sensors, support data range
 * configuration, this may change data resolution.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support set separated range for each data field, or global range for
 * all data fields.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The value to be set.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_set_range_t)(
		const struct device *dev,
		int index, uint32_t value);

/**
 * @brief Get current data range.
 *
 * Some sensors especially for physical sensors, support data range
 * configuration, this may change data resolution.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support get separated range for each data field, or global range for
 * all data fields.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The data buffer to receive value.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_get_range_t)(
		const struct device *dev,
		int index, uint32_t *value);

/**
 * @brief Set current sensor's hardware fifo size
 *
 * Some sensors especially for physical sensors, support hardware fifo, this API can
 * configure the current fifo size.
 *
 * @param dev The sensor instance device structure.
 *
 * @param samples The sample number to set for fifo.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_set_fifo_t)(
		const struct device *dev,
		uint32_t samples);

/**
 * @brief Get current sensor's hardware fifo size
 *
 * Some sensors especially for physical sensors, support fifo, this API can
 * get the current fifo size.
 *
 * @param dev The sensor instance device structure.
 *
 * @param samples The data buffer to receive the fifo sample number.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_get_fifo_t)(
		const struct device *dev,
		uint32_t *samples);

/**
 * @brief Set current sensor data offset
 *
 * Some sensors especially for physical sensors, such as accelerometer senors,
 * as data drift, need configure offset calibration.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support set separated offset for each data field, or global offset for
 * all data fields.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The offset value to be set.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_set_offset_t)(
		const struct device *dev,
		int index, int32_t value);

/**
 * @brief Get current sensor data offset
 *
 * Some sensors especially for physical sensors, such as accelerometer senors,
 * as data drift, need configure offset calibration.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support get separated offset for each data field, or global offset for
 * all data fields.
 *
 * @param dev The sensor instance device structure.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The data buffer to receive the offset value.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
typedef int (*sensing_sensor_get_offset_t)(
		const struct device *dev,
		int index, int32_t *value);
/**
 * @struct sensing_sensor_api
 * @brief Sensor callback api
 *
 * A sensor must register this callback API during sensor registration.
 */
struct sensing_sensor_api {
	sensing_sensor_init_t init;
	sensing_sensor_reset_t reset;
	sensing_sensor_deinit_t deinit;
	sensing_sensor_set_interval_t set_interval;
	sensing_sensor_get_interval_t get_interval;
	sensing_sensor_set_range_t set_range;
	sensing_sensor_get_range_t get_range;
	sensing_sensor_set_offset_t set_offset;
	sensing_sensor_get_offset_t get_offset;
	sensing_sensor_get_fifo_t get_fifo;
	sensing_sensor_set_fifo_t set_fifo;
	sensing_sensor_set_sensitivity_t set_sensitivity;
	sensing_sensor_get_sensitivity_t get_sensitivity;
	sensing_sensor_read_sample_t read_sample;
	sensing_sensor_process_t process;
	sensing_sensor_sensitivity_test_t sensitivity_test;
	sensing_sensor_self_calibration_t self_calibration;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*ZEPHYR_INCLUDE_SENSING_SENSOR_H_*/
