/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree constants for the Sensirion STCC4 CO2 sensor
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STCC4_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STCC4_H_

/** Continuous measurement mode (~1 Hz) */
#define STCC4_DT_MODE_CONTINUOUS  0
/** Single-shot measurement mode (on demand, 500ms per measurement) */
#define STCC4_DT_MODE_SINGLE_SHOT 1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STCC4_H_ */
