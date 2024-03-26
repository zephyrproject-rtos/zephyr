/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_FIELD_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_FIELD_H_

/** X euclidean axis. */
#define	SENSOR_FIELD_X_AXIS 0

/** Y euclidean axis. */
#define SENSOR_FIELD_Y_AXIS 1

/** Z euclidean axis. */
#define SENSOR_FIELD_Z_AXIS 2

/** On-die (e.g. on die temperature). */
#define SENSOR_FIELD_DIE 3

/** Ambient. */
#define SENSOR_FIELD_AMBIENT 4

/** IR EM frequency range. */
#define SENSOR_FIELD_IR_LIGHT 5

/** Visible EM frequency range. */
#define SENSOR_FIELD_VISIBLE_LIGHT 6

/** Ultra violet EM frequency range. */
#define SENSOR_FIELD_UV_LIGHT 7

/** 1 micrometer particulate. */
#define SENSOR_FIELD_1_0_PARTICULATE 8

/** 2.5 micrometer particulate. */
#define SENSOR_FIELD_2_5_PARTICULATE 9

/** 10 micrometer particulate. */
#define SENSOR_FIELD_10_PARTICULATE 10

/**
 * Number of all common sensor fields.
 */
#define SENSOR_FIELD_COMMON_COUNT 11

/**
 * This and higher values are sensor specific.
 * Refer to the sensor header file.
 */
#define SENSOR_FIELD_PRIV_START SENSOR_FIELD_COMMON_COUNT

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_FIELD_H_ */
