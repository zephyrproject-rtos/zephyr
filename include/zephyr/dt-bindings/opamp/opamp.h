/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_OPAMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_OPAMP_H_

/**
 * @brief Enumerations for opamp functional mode.
 */
enum opamp_functional_mode {
	/** Differential amplifier mode */
	OPAMP_DIFFERENTIAL_MODE = 0,
	/** Inverting amplifier mode */
	OPAMP_INVERTING_MODE,
	/** Non-inverting amplifier mode */
	OPAMP_NON_INVERTING_MODE,
	/** Follower mode */
	OPAMP_FOLLOWER_MODE,
};

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_OPAMP_H_ */
