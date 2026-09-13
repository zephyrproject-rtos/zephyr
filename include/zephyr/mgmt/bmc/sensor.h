/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_SENSOR_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_SENSOR_H_

/**
 * @file
 * @brief Sensor registry exposed by the BMC over Redfish and the shell.
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC sensors
 * @defgroup bmc_sensor BMC sensors
 * @ingroup bmc_api
 * @{
 */

/**
 * @brief Longest sensor identifier that can appear in a Redfish URI.
 */
#define BMC_SENSOR_ID_MAX_LEN 32

struct bmc_sensor;

/**
 * @brief A sensor published by the BMC.
 *
 * Do not populate this structure directly, use BMC_SENSOR_DEFINE() or
 * BMC_SENSOR_DT_DEFINE().
 */
struct bmc_sensor {
	/** Identifier used in the Redfish URI, for example "TempBmc". */
	const char *id;
	/** Human readable name. */
	const char *name;
	/** Redfish ReadingType, for example "Temperature" or "Voltage". */
	const char *reading_type;
	/** Redfish ReadingUnits, for example "Cel" or "V". */
	const char *units;
	/** Redfish PhysicalContext, for example "ManagementController". */
	const char *physical_context;
	/** Backend specific context passed back to @ref read. */
	const void *ctx;
	/**
	 * @brief Sample the sensor.
	 *
	 * @param sensor The sensor being read.
	 * @param val Where to store the reading.
	 *
	 * @return 0 on success, negative errno otherwise.
	 */
	int (*read)(const struct bmc_sensor *sensor, struct sensor_value *val);
};

/**
 * @brief Publish a sensor to the BMC.
 *
 * @param _sym Unique C symbol name for the sensor.
 * @param ... Designated initialisers for @ref bmc_sensor.
 */
#define BMC_SENSOR_DEFINE(_sym, ...)                                                               \
	static const STRUCT_SECTION_ITERABLE(bmc_sensor, _sym) = {__VA_ARGS__}

/**
 * @brief Publish a sensor backed by a Zephyr sensor device.
 *
 * Samples channel @p _chan of the device bound to devicetree node @p _node.
 *
 * @param _sym Unique C symbol name for the sensor.
 * @param _node Devicetree node identifier of the sensor device.
 * @param _chan A @c sensor_channel value.
 * @param _id Identifier used in the Redfish URI.
 * @param _name Human readable name.
 * @param _type Redfish ReadingType string.
 * @param _units Redfish ReadingUnits string.
 */
#define BMC_SENSOR_DT_DEFINE(_sym, _node, _chan, _id, _name, _type, _units)                        \
	static const struct bmc_sensor_dt_ctx _sym##_ctx = {                                       \
		.dev = DEVICE_DT_GET(_node),                                                       \
		.chan = (_chan),                                                                   \
	};                                                                                         \
	BMC_SENSOR_DEFINE(_sym, .id = (_id), .name = (_name), .reading_type = (_type),             \
			  .units = (_units), .physical_context = "ManagementController",           \
			  .ctx = &_sym##_ctx, .read = bmc_sensor_dt_read)

/** @brief Context used by BMC_SENSOR_DT_DEFINE(). */
struct bmc_sensor_dt_ctx {
	/** Sensor device to sample. */
	const struct device *dev;
	/** Channel to read. */
	int16_t chan;
};

/**
 * @brief Read helper used by BMC_SENSOR_DT_DEFINE().
 *
 * @param sensor Sensor whose @c ctx points at a @ref bmc_sensor_dt_ctx.
 * @param val Where to store the reading.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_sensor_dt_read(const struct bmc_sensor *sensor, struct sensor_value *val);

/**
 * @brief Look up a registered sensor by its identifier.
 *
 * @param id Identifier to search for.
 *
 * @return The sensor, or NULL if no sensor with that identifier exists.
 */
const struct bmc_sensor *bmc_sensor_get(const char *id);

/**
 * @brief Number of registered sensors.
 *
 * @return The sensor count.
 */
size_t bmc_sensor_count(void);

/**
 * @brief Sample a registered sensor.
 *
 * @param sensor Sensor to read.
 * @param val Where to store the reading.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_sensor_read(const struct bmc_sensor *sensor, struct sensor_value *val);

/**
 * @brief Iterate over every registered sensor.
 *
 * @param _iter Name of the `struct bmc_sensor *` loop variable.
 */
#define BMC_SENSOR_FOREACH(_iter) STRUCT_SECTION_FOREACH(bmc_sensor, _iter)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_SENSOR_H_ */
