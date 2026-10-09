/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic proportional-integral controller.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision PI Controller
 * @defgroup precision_pi Precision PI Controller
 * @since 4.5
 * @version 0.2.0
 * @ingroup precision_timing
 * @{
 */

/** State of an independent proportional-integral controller. */
struct precision_pi {
	/** Proportional gain. */
	double kp;
	/** Integral gain. */
	double ki;
	/** Accumulated integral term. */
	double integral;
	/** Largest magnitude of the integral term, 0 for no limit. */
	double integral_limit;
	/** Largest magnitude of the output, 0 for no limit. */
	double output_limit;
};

/**
 * @brief Initialize a PI controller.
 *
 * @param pi Controller instance.
 * @param kp Proportional gain.
 * @param ki Integral gain.
 */
void precision_pi_init(struct precision_pi *pi, double kp, double ki);

/**
 * @brief Reset the accumulated integral term.
 *
 * The configured gains are preserved.
 *
 * @param pi Controller instance.
 */
void precision_pi_reset(struct precision_pi *pi);

/**
 * @brief Set the limits of a PI controller.
 *
 * The integral term is held within +-@p integral_limit, so it cannot wind up
 * while a large error lasts. The output is held within +-@p output_limit, and
 * while the output is at that limit the integral term does not move further
 * towards it. A limit of 0 disables it, which is the state after
 * precision_pi_init().
 *
 * @param pi Controller instance.
 * @param integral_limit Largest magnitude of the integral term, 0 for no limit.
 * @param output_limit Largest magnitude of the output, 0 for no limit.
 */
void precision_pi_set_limits(struct precision_pi *pi, double integral_limit, double output_limit);

/**
 * @brief Update a PI controller from an error sample.
 *
 * Same as precision_pi_update_interval() with an interval of 1.
 *
 * @param pi Controller instance.
 * @param error Current control error.
 *
 * @return Controller output.
 */
double precision_pi_update(struct precision_pi *pi, double error);

/**
 * @brief Update a PI controller from an error sample taken after an interval.
 *
 * The gains are given for an interval of 1 and are divided by @p interval.
 * When the plant integrates the output over the interval, as a clock does with
 * a rate correction, the loop gains per sample are then kp and ki for any
 * interval, so the loop keeps its stability and damping when the sample rate
 * changes.
 *
 * @param pi Controller instance.
 * @param error Current control error.
 * @param interval Time since the previous sample, in the unit the gains are
 *                 given for. Must be greater than 0.
 *
 * @return Controller output.
 */
double precision_pi_update_interval(struct precision_pi *pi, double error, double interval);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_ */
