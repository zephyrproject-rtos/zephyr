/*
 * Copyright 2024 Meta Platforms, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_ERROR_TYPES_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_ERROR_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I3C SDR Controller Error Types
 *
 * These are error types defined by the I3C specification.
 *
 * #I3C_ERROR_CE_UNKNOWN and #I3C_ERROR_CE_NONE are not
 * official error types according to the specification.
 * These are there simply to aid in error handling during
 * interactions with the I3C drivers and subsystem.
 */
enum i3c_sdr_controller_error_types {
	/** Transaction after sending CCC */
	I3C_ERROR_CE0,

	/** Monitoring Error */
	I3C_ERROR_CE1,

	/** No response to broadcast address (0x7E) */
	I3C_ERROR_CE2,

	/** Failed Controller Handoff */
	I3C_ERROR_CE3,

	/** Unknown error (not official error type) */
	I3C_ERROR_CE_UNKNOWN,

	/** No error (not official error type) */
	I3C_ERROR_CE_NONE,

	I3C_ERROR_CE_MAX = I3C_ERROR_CE_UNKNOWN,
	I3C_ERROR_CE_INVALID,
};

/**
 * @brief I3C SDR Target Error Types
 *
 * These are error types defined by the I3C specification.
 *
 * #I3C_ERROR_TE_UNKNOWN and #I3C_ERROR_TE_NONE are not
 * official error types according to the specification.
 * These are there simply to aid in error handling during
 * interactions with the I3C drivers and subsystem.
 */
enum i3c_sdr_target_error_types {
	/**
	 * Invalid Broadcast Address or
	 * Dynamic Address after DA assignment
	 */
	I3C_ERROR_TE0,

	/** CCC type */
	I3C_ERROR_TE1,

	/** Write Data */
	I3C_ERROR_TE2,

	/** Assigned Address during Dynamic Address Arbitration */
	I3C_ERROR_TE3,

	/** 0x7E/R missing after RESTART during Dynamic Address Arbitration */
	I3C_ERROR_TE4,

	/** Transaction after detecting CCC */
	I3C_ERROR_TE5,

	/** Monitoring Error */
	I3C_ERROR_TE6,

	/** Dead Bus Recovery */
	I3C_ERROR_DBR,

	/** Unknown error (not official error type) */
	I3C_ERROR_TE_UNKNOWN,

	/** No error (not official error type) */
	I3C_ERROR_TE_NONE,

	I3C_ERROR_TE_MAX = I3C_ERROR_TE_UNKNOWN,
	I3C_ERROR_TE_INVALID,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_ERROR_TYPES_H_ */
