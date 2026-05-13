/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for STM32 counter capture driver
 * @ingroup counter_stm32_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_STM32_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_STM32_H_

/**
 * @defgroup counter_stm32_interface STM32 counter capture
 * @ingroup counter_interface_ext
 * @brief STM32 counter capture driver
 * @{
 */

/**
 * @name Custom counter flags for STM32
 *
 * These flags can be used with the counter API in the upper 24 bits of counter_capture_flags_t
 * They allow the usage of the filter and prescaler settings of the STM32 timers.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define COUNTER_CAPTURE_STM32_PRESCALER_DIV_POS 8
#define COUNTER_CAPTURE_STM32_PRESCALER_DIV_MSK (0x3 << COUNTER_CAPTURE_STM32_PRESCALER_DIV_POS)
#define COUNTER_CAPTURE_STM32_FILTER_POS        10
#define COUNTER_CAPTURE_STM32_FILTER_MSK        (0xF << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @endcond */

/**
 * @name Prescaler Divider
 * @brief Ratio of the prescaler for input capture.
 * @{
 */

/** @brief Capture on every edge */
#define COUNTER_CAPTURE_STM32_PRESCALER_DIV1 (0 << COUNTER_CAPTURE_STM32_PRESCALER_DIV_POS)
/** @brief Capture on every 2nd edge */
#define COUNTER_CAPTURE_STM32_PRESCALER_DIV2 (1 << COUNTER_CAPTURE_STM32_PRESCALER_DIV_POS)
/** @brief Capture on every 4th edge */
#define COUNTER_CAPTURE_STM32_PRESCALER_DIV4 (2 << COUNTER_CAPTURE_STM32_PRESCALER_DIV_POS)
/** @brief Capture on every 8th edge */
#define COUNTER_CAPTURE_STM32_PRESCALER_DIV8 (3 << COUNTER_CAPTURE_STM32_PRESCALER_DIV_POS)

/** @} */

/**
 * @name Input Capture Sampling Frequency and Digital Filter
 * @brief Configuration for sampling frequency and digital filter length.
 * @{
 */

/** @brief Sampling: fDTS, N=1 (no filter) */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV1_N1   (0 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fTIM_KER_CK, N=2 */
#define COUNTER_CAPTURE_STM32_FILTER_TIM_KER_CK_N2 (1 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fTIM_KER_CK, N=4 */
#define COUNTER_CAPTURE_STM32_FILTER_TIM_KER_CK_N4 (2 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fTIM_KER_CK, N=8 */
#define COUNTER_CAPTURE_STM32_FILTER_TIM_KER_CK_N8 (3 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/2, N=6 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV2_N6   (4 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/2, N=8 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV2_N8   (5 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/4, N=6 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV4_N6   (6 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/4, N=8 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV4_N8   (7 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/8, N=6 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV8_N6   (8 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/8, N=8 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV8_N8   (9 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/16, N=5 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV16_N5  (10 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/16, N=6 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV16_N6  (11 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/16, N=8 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV16_N8  (12 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/32, N=5 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV32_N5  (13 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/32, N=6 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV32_N6  (14 << COUNTER_CAPTURE_STM32_FILTER_POS)
/** @brief Sampling: fDTS/32, N=8 */
#define COUNTER_CAPTURE_STM32_FILTER_DTS_DIV32_N8  (15 << COUNTER_CAPTURE_STM32_FILTER_POS)

/** @} */

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_STM32_H_ */
