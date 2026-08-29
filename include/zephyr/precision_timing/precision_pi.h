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
 * @version 0.1.0
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
 * @brief Update a PI controller from an error sample.
 *
 * @param pi Controller instance.
 * @param error Current control error.
 *
 * @return Controller output.
 */
double precision_pi_update(struct precision_pi *pi, double error);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_ */
