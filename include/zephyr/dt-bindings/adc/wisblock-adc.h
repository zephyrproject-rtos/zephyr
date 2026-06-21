/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WisBlock ADC channel constants
 * @ingroup wisblock-adc
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_WISBLOCK_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_WISBLOCK_ADC_H_

/**
 * @defgroup wisblock-adc WisBlock ADC
 * @ingroup adc_interface_ext
 * @brief Constants for ADC channels exposed on WisBlock Core modules
 * @{
 */

#define WISBLOCK_ADC_AIN0 0 /**< Analog input 0 (AIN0) */
#define WISBLOCK_ADC_AIN1 1 /**< Analog input 1 (AIN1) */
#define WISBLOCK_ADC_IO1  2 /**< Optional analog-capable IO1 */
#define WISBLOCK_ADC_IO2  3 /**< Optional analog-capable IO2 */
#define WISBLOCK_ADC_IO3  4 /**< Optional analog-capable IO3 */
#define WISBLOCK_ADC_IO4  5 /**< Optional analog-capable IO4 */
#define WISBLOCK_ADC_IO5  6 /**< Optional analog-capable IO5 */
#define WISBLOCK_ADC_IO6  7 /**< Optional analog-capable IO6 */
#define WISBLOCK_ADC_IO7  8 /**< Optional analog-capable IO7 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_WISBLOCK_ADC_H_ */
