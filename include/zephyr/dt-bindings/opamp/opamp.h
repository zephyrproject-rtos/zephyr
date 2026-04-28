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
	OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL = 0,
	/** Inverting amplifier mode */
	OPAMP_FUNCTIONAL_MODE_INVERTING,
	/** Non-inverting amplifier mode */
	OPAMP_FUNCTIONAL_MODE_NON_INVERTING,
	/** Follower mode */
	OPAMP_FUNCTIONAL_MODE_FOLLOWER,
	/**
	 * @brief Standalone mode.
	 * The gain is set by external resistors. The API call to set the gain
	 * is ignored in this mode or has no impact.
	 */
	OPAMP_FUNCTIONAL_MODE_STANDALONE,
};

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_OPAMP_H_ */
