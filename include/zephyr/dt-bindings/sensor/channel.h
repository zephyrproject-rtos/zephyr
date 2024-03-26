/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_CHANNEL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_CHANNEL_H_

/** Acceleration.  */
#define SENSOR_CHANNEL_ACCELERATION 0

/** Rotational velocity. */
#define SENSOR_CHANNEL_ROTATIONAL_VELOCITY 1

/** Magnetic field strength. */
#define SENSOR_CHANNEL_MAGNETIC_FIELD 2

/** Temperature. */
#define SENSOR_CHANNEL_TEMPERATURE 3

/** Pressure. */
#define SENSOR_CHANNEL_PRESSURE 4

/** Humidity. */
#define SENSOR_CHANNEL_HUMIDITY 5

/** Light. */
#define SENSOR_CHANNEL_LIGHT 6

/** Altitude. */
#define SENSOR_CHAN_ALTITUDE 7

/** Particulate Matter */
#define SENSOR_CHANNEL_PM 8

/** Distance. From sensor to target, in meters */
#define SENSOR_CHANNEL_DISTANCE 9

/** CO2 level, in parts per million (ppm) **/
#define SENSOR_CHANNEL_CO2 10

/** VOC level, in parts per billion (ppb) **/
#define SENSOR_CHANNEL_VOC 11

/** Gas sensor resistance in ohms. */
#define SENSOR_CHANNEL_GAS_RES 12

/** Voltage **/
#define SENSOR_CHANNEL_VOLTAGE 13

/** Current **/
#define SENSOR_CHANNEL_CURRENT 14

/** Power in watts **/
#define SENSOR_CHANNEL_POWER 15

/** Resistance **/
#define SENSOR_CHANNEL_RESISTANCE 16

/** Angular rotation */
#define SENSOR_CHANNEL_ROTATION 17

/** Position change on the X axis, in points. */
#define SENSOR_CHANNEL_POSITION 18

/** Revolutions per minute, in RPM. */
#define SENSOR_CHANNEL_RPM 19

/**
 * Number of all common sensor channels.
 */
#define SENSOR_CHANNEL_COMMON_COUNT 20

/**
 * This and higher values are sensor specific.
 * Refer to the sensor header file.
 */
#define SENSOR_CHANNEL_PRIV_START SENSOR_CHANNEL_COMMON_COUNT

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_CHANNEL_H_ */

