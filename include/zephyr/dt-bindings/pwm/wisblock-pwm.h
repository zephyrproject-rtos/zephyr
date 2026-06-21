/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WisBlock PWM channel constants
 * @ingroup wisblock-pwm
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_WISBLOCK_PWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_WISBLOCK_PWM_H_

/**
 * @defgroup wisblock-pwm WisBlock PWM
 * @ingroup pwm_interface_ext
 * @brief Constants for PWM channels exposed on WisBlock connectors
 * @{
 */

#define WISBLOCK_PWM_IO1 0 /**< PWM channel for IO1 */
#define WISBLOCK_PWM_IO2 1 /**< PWM channel for IO2 */
#define WISBLOCK_PWM_IO3 2 /**< PWM channel for IO3 */
#define WISBLOCK_PWM_IO4 3 /**< PWM channel for IO4 */
#define WISBLOCK_PWM_IO5 4 /**< PWM channel for IO5 */
#define WISBLOCK_PWM_IO6 5 /**< PWM channel for IO6 */
#define WISBLOCK_PWM_IO7 6 /**< PWM channel for IO7 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_WISBLOCK_PWM_H_ */
