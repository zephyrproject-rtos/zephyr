/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSS_H_
#define SENSS_H_

/**
 * @defgroup senss Sensor Subsystem APIs
 * @defgroup senss_api API
 * @ingroup senss
 * @defgroup senss_sensor_types  Sensor Types
 * @ingroup senss
 * @defgroup senss_datatypes Data Types
 * @ingroup senss
 */

#include <senss/senss_datatypes.h>
#include <senss/senss_sensor_types.h>

/**
 * @brief Sensor Subsystem API
 * @addtogroup senss_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sensor instance handle
 *
 * Clients using this to handle a opened sensor instance.
 *
 * Application clients can use \ref senss_open_sensor to open a sensor
 * instance and get it's handle.
 */
typedef uint16_t senss_sensor_handle_t;

/**
 * @struct senss_sensor_version
 * @brief Sensor Version
 */
struct senss_sensor_version {
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

#define SENSS_SENSOR_VERSION(_major, _minor, _hotfix, _build)           \
	((_major) << 24 | (_minor) << 16 | (_hotfix) << 8 | (_build))


/**
 * @brief Sensor flag indicating if this sensor is on event reporting data.
 *
 * Reporting sensor data when the sensor event occurs, such as a motion detect sensor reporting
 * a motion or motionless detected event.
 */
#define SENSS_SENSOR_FLAG_REPORT_ON_EVENT			BIT(0)

/**
 * @brief Sensor flag indicating if this sensor is on change reporting data.
 *
 * Reporting sensor data when the sensor data changes.
 *
 * Exclusive with  \ref SENSS_SENSOR_FLAG_REPORT_ON_EVENT
 */
#define SENSS_SENSOR_FLAG_REPORT_ON_CHANGE			BIT(1)


/**
 * @brief Sensor state.
 *
 */
enum senss_sensor_state {
	SENSS_SENOSR_STATE_NOT_READY = 1,
	SENSS_SENOSR_STATE_READY = 2,
};


/**
 * @struct senss_sensor_info
 * @brief Sensor basic constant information
 *
 */
struct senss_sensor_info {
	/** Name of the sensor instance */
	const char *name;

	/** Friendly name of the sensor instance */
	const char *friendly_name;

	/** Vendor name of the sensor instance */
	const char *vendor;

	/** Model name of the sensor instance */
	const char *model;

	/** Sensor type */
	int type;

	/** Sensor index for multiple sensors under same sensor type */
	int sensor_index;

	/** Sensor flags */
	uint32_t flags;

	/** Minimal report interval in micro seconds  */
	uint32_t minimal_interval;

	/** Sensor version */
	struct senss_sensor_version version;
};



/**
 * @brief Initialize sensor subsystem runtime.
 *
 * This will allocate internal resources, such as create internal schedule
 * task, create internal management data structures.
 *
 * It will also enumerate all registered sensor instances in sensor subsystem,
 * build sensors report relational data model, if a virtual sensor's required dependency
 * sensor type not found, will return error.
 * Then call each sensor instances' init callback API.
 *
 * @return 0 on success or negative error value on failure.
 *
 */
int senss_init(void);

 /**
  * @brief De-initialize sensor subsystem runtime.
  *
  * This will release all dynamically allocated internal resources, and call
  * all sensor instances' deinit callback API.
  *
  * @return 0 on success or negative error value on failure.
  */
int senss_deinit(void);

 /**
  * @brief Find sensor instances by sensor type and sensor index.
  *
  * Application can use these API to find sensor instances' information by
  * sensor type and sensor index, or enumerate all sensor instances'
  * information at one call.
  *
  * This API just returned read only information of sensor instances,
  * no side effect to sensor instances.
  *
  * @param type The sensor type which need to find,
  * \ref SENSS_SENSOR_TYPE_ALL for all types.
  *
  * @param sensor_index The index which need to find, -1 for find all
  * instances with type \p type.
  *
  * @param *info Input info array for receiving found sensor instances
  *
  * @param max The max count of the \p infos array input. Can get real count
  * number via \ref senss_get_sensor_count.
  *
  * @return Number of sensor instances found, 0 returned if no found
  */
int senss_find_sensor(
		int type, int sensor_index,
		struct senss_sensor_info *info, int max);

 /**
  * @brief Get sensor instance count of given type and sensor index.
  *
  * @param type The sensor type for checking, \ref SENSS_SENSOR_TYPE_ALL
  * for all types.
  *
  * @param sensor_index The sensor index for checking, -1 for all instances with
  * \p type type.
  *
  * @return Count of matched sensor instances, 0 returned if no matched.
  */
int senss_get_sensor_count(
		int type, int sensor_index);

/**
 * @brief Open sensor instance by sensor type and sensor index.
 *
 * Application clients use it to open a sensor instance and get its handle.
 * Support multiple Application clients for open same sensor instance,
 * in this case, the returned handle will different for different clients.
 *
 * @param type The sensor type which need to find.
 *
 * @param sensor_index The index which need to open.
 *
 * @param *handle The opened instance handle, if failed will be set to NULL.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_open_sensor(
		int type, int sensor_index,
		senss_sensor_handle_t *handle);

/**
 * @brief Close sensor instance.
 *
 * @param handle The sensor instance handle need to close.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_close_sensor(
		senss_sensor_handle_t handle);


/**
 * @brief Get current report interval of given client handle.
 *
 * @param handle The sensor instance handle.
 *
 * @param value The data buffer to receive value (micro seconds unit).
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_get_interval(
		senss_sensor_handle_t handle,
		uint32_t *value);

/**
 * @brief Set current report interval of given client handle.
 *
 * @param handle The sensor instance handle.
 *
 * @param value The value to be set (micro seconds unit).
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
int senss_set_interval(
		senss_sensor_handle_t handle,
		uint32_t value);

/**
 * @brief Set current data change sensitivity of given client handle.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support set separated sensitivity for each data field, or global
 * sensitivity for all data fields.
 *
 * @param handle The sensor instance handle.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The value to be set.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
int senss_set_sensitivity(
		senss_sensor_handle_t handle,
		int index, uint32_t value);

/**
 * @brief Get current data change sensitivity of given client handle.
 *
 * Since each sensor type may have multiple data fields for it's value, this
 * API support get separated sensitivity for each data field, or global
 * sensitivity for all data fields.
 *
 * @param handle The sensor instance handle.
 *
 * @param index The value fields index to be set, -1 for all fields (global).
 *
 * @param value The data buffer to receive value.
 *
 * @return 0 on success or negative error value on failure, not support etc.
 */
int senss_get_sensitivity(
		senss_sensor_handle_t handle,
		int index, uint32_t *value);

/**
 * @brief Read a data sample
 *
 * This API will trigger target sensor read a sample each time.
 *
 * @param handle The sensor instance handle.
 *
 * @param buf The data buffer to receive sensor data sample.
 *
 * @param size The buffer size in bytes.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_read_sample(
		senss_sensor_handle_t handle,
		void *buf, int size);

/**
 * @brief Sensor data event receive callback.
 *
 * @param handle The sensor instance handle.
 *
 * @param buf The data buffer with sensor data.
 *
 * @param size The buffer size in bytes.
 *
 * @param param The user private parameter.
 *
 * @return 0 on success or negative error value on failure.
 */
typedef int (*senss_data_event_t)(
		senss_sensor_handle_t handle,
		void *buf, int size, void *param);

/**
 * @brief Register sensor data event receive callback.
 *
 * User can use this API to register a data event receive callback function.
 *
 * @param handle The sensor instance handle.
 *
 * @param callback The data event receive callback function.
 *
 * @param param The user private parameter.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_register_data_event_callback(
		senss_sensor_handle_t handle,
		senss_data_event_t callback, void *param);


/**
 * @brief Set report latency of a sensor instance.
 *
 * Report latency controls how long the sensor data events of the sensor instance will be buffered
 * in the batching buffer before deliver to its client.
 *
 * For 0 latency value, it's means sensor data event will not buffered, and will report to its client
 * immediately.
 *
 * The maximum sensor data events of this sensor instance is calculated as: latency /
 * report interval.
 *
 * @param handle The sensor instance handle.
 *
 * @param latency The latency in micro seconds.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_set_report_latency(
		senss_sensor_handle_t handle, uint64_t latency);

/**
 * @brief Get current report latency value of a sensor instance.
 *
 * @param handle The sensor instance handle.
 *
 * @param latency The value buffer to receive report latency in micro seconds.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_get_report_latency(
		senss_sensor_handle_t handle, uint64_t *latency);

/**
 * @brief Flush batching buffer for a sensor instance.
 *
 * @param handle The sensor instance handle.
 *
 * If \p handle is -1, will flush all buffer.
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_batching_flush(
		senss_sensor_handle_t handle);

/**
 * @brief Get sensor information from sensor instance handle.
 *
 * @param handle The sensor instance hande.
 *
 * @return a const pointer to \ref senss_sensor_info on success or NULL on failure.
 */
const struct senss_sensor_info *senss_get_sensor_info(
		senss_sensor_handle_t handle);

/**
 * @brief Get sensor instance's state
 *
 * @param handle The sensor instance hande.
 *
 * @param state Returned sensor state value
 *
 * @return 0 on success or negative error value on failure.
 */
int senss_get_sensor_state(
		senss_sensor_handle_t handle,
		enum senss_sensor_state * state);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* SENSS_H_ */
