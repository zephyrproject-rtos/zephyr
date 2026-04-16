/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic discrete PID controller with anti-windup.
 * @defgroup pid_controller PID Controller
 * @ingroup utilities
 * @{
 */

#ifndef ZEPHYR_INCLUDE_SYS_PID_H_
#define ZEPHYR_INCLUDE_SYS_PID_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of fractional bits in Q16.16 fixed-point representation. */
#define PID_Q16_SHIFT 16

/**
 * @brief Convert an integer value to Q16.16 fixed-point.
 * @param x Integer value.
 * @return Q16.16 fixed-point representation.
 */
#define PID_Q16(x) ((int32_t)(x) << PID_Q16_SHIFT)

/**
 * @brief PID controller configuration.
 *
 * All gain values use Q16.16 fixed-point format.
 * Use @ref PID_Q16 to convert integer gains.
 */
struct pid_cfg {
	/** Proportional gain (Q16.16) */
	int32_t kp;
	/** Integral gain (Q16.16) */
	int32_t ki;
	/** Derivative gain (Q16.16) */
	int32_t kd;
	/** Minimum output value */
	int32_t output_min;
	/** Maximum output value */
	int32_t output_max;
	/** Minimum integral accumulator value (anti-windup) */
	int32_t integral_min;
	/** Maximum integral accumulator value (anti-windup) */
	int32_t integral_max;
};

/**
 * @brief PID controller running state.
 *
 * Maintained across calls to @ref pid_update. Must be initialized
 * with @ref pid_init before first use.
 */
struct pid_state {
	/** Accumulated integral term (Q16.16) */
	int64_t integral;
	/** Previous error value for derivative calculation */
	int32_t prev_error;
};

/**
 * @brief Initialize PID state to zero.
 *
 * @param state Pointer to PID state structure.
 */
void pid_init(struct pid_state *state);

/**
 * @brief Reset PID state, clearing integral and previous error.
 *
 * @param state Pointer to PID state structure.
 */
void pid_reset(struct pid_state *state);

/**
 * @brief Compute one PID control iteration.
 *
 * Pure arithmetic function with no kernel dependencies.
 * Safe to call from any context including ISR.
 *
 * @param cfg PID configuration (gains, clamps). Not modified.
 * @param state PID running state. Updated on each call.
 * @param setpoint Desired target value.
 * @param measured Actual measured value.
 * @return Control output, clamped to [output_min, output_max].
 */
int32_t pid_update(const struct pid_cfg *cfg, struct pid_state *state,
		   int32_t setpoint, int32_t measured);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SYS_PID_H_ */
