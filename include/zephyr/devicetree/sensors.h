/**
 * @file
 * @brief Sensor Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_SENSORS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_SENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-sensors Devicetree Sensor API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the node identifier for the sensor device from a
 *        sensors property at an index
 *
 * Example devicetree fragment:
 *
 *     temp_amb: amb-sensor@... { ... };
 *
 *     temp_cpu: cpu-sensor@... { ... };
 *
 *     n: node {
 *             sensors = <&temp_cpu DT_SENSOR_CHAN_DIE_TEMP>,
 *                       <&temp_amb DT_SENSOR_CHAN_AMBIENT_TEMP>;
 *     };
 *
 * Example usage:
 *
 *     DT_SENSOR_DEV_BY_IDX(DT_NODELABEL(n), 0) // DT_NODELABEL(temp_cpu)
 *     DT_SENSOR_DEV_BY_IDX(DT_NODELABEL(n), 1) // DT_NODELABEL(temp_amb)
 *
 * @param node_id node identifier for a node with a sensors property
 * @param idx logical index into sensors property
 * @return the node identifier for the sensor device referenced at index "idx"
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_SENSOR_DEV_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, sensors, idx)

/**
 * @brief Equivalent to DT_SENSOR_DEV_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a sensors property
 * @return the node identifier for the sensor device at index 0
 *         in the node's "sensors" property
 * @see DT_SENSOR_DEV_BY_IDX()
 */
#define DT_SENSOR_DEV(node_id) DT_SENSOR_DEV_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the sensor device from a
 *        sensors property by name
 *
 * Example devicetree fragment:
 *
 *     temp_amb: amb-sensor@... { ... };
 *
 *     temp_cpu: cpu-sensor@... { ... };
 *
 *     n: node {
 *             sensors = <&temp_cpu DT_SENSOR_CHAN_DIE_TEMP>,
 *                       <&temp_amb DT_SENSOR_CHAN_AMBIENT_TEMP>;
 *             sensor-names = "cpu", "ambient";
 *     };
 *
 * Example usage:
 *
 *     DT_SENSOR_DEV_BY_NAME(DT_NODELABEL(n), cpu) // DT_NODELABEL(temp_cpu)
 *     DT_SENSOR_DEV_BY_NAME(DT_NODELABEL(n), ambient)  // DT_NODELABEL(temp_amb)
 *
 * @param node_id node identifier for a node with a sensors property
 * @param name lowercase-and-underscores name of a sensors element
 *             as defined by the node's sensor-names property
 * @return the node identifier for the sensor device in the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_SENSOR_DEV_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, sensors, name)

/**
 * @brief Get a sensor specifier's channel cell value at an index
 *
 * This macro only works for sensor specifiers with cells named "channel".
 * Refer to the node's binding to check if necessary.
 *
 * @param node_id node identifier for a node with a sensor property
 * @param idx logical index into sensor property
 * @return the channel cell value at index "idx"
 */
#define DT_SENSOR_CHANNEL_BY_IDX(node_id, idx) \
	DT_PHA_BY_IDX(node_id, sensors, idx, channel)

/**
 * @brief Equivalent to DT_SENSOR_CHANNEL_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a sensors property
 * @return the channel cell value at index 0
 * @see DT_SENSOR_CHANNEL_BY_IDX()
 */
#define DT_SENSOR_CHANNEL(node_id) DT_SENSOR_CHANNEL_BY_IDX(node_id, 0)

/**
 * @brief Get a sensor specifier's channel cell value by name
 *
 * This macro only works for sensor specifiers with cells named "channel".
 * Refer to the node's binding to check if necessary.
 *
 * @param node_id node identifier for a node with a sensor property
 * @param name lowercase-and-underscores name of a sensor element
 *             as defined by the node's sensor-names property
 * @return the channel cell value in the specifier at the named element
 */
#define DT_SENSOR_CHANNEL_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME(node_id, sensors, name, channel)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_SENSORS_H_ */
