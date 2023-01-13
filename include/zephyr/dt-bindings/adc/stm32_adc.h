/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linaro Ltd.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32_ADC_H_

#define STM32_ADC_RES_RES_MASK    0x15U
#define STM32_ADC_RES_RES_SHIFT   0U
#define STM32_ADC_RES_VAL_MASK    0x7U
#define STM32_ADC_RES_VAL_SHIFT   4U
#define STM32_ADC_RES_OFF_MASK    0x31U
#define STM32_ADC_RES_OFF_SHIFT   6U

/**
 * @brief STM32 ADC resolutions bit field.
 *
 * - res      (0..14)         [ 0 : 3 ]
 * - val      (0x0..0x3)      [ 4 : 5 ]
 * - offset   (0..31)         [ 6 : 10 ]
 *
 * @param res Supported resolutions [6,8,10,12,14, ..]
 * @param val Value of the supported resolution in ADC register
 * @param offset Offset of the RES bits in ADC register
 */
#define STM32_ADC_RES(res, val, offset)						\
	((((res) & STM32_ADC_RES_RES_MASK) << STM32_ADC_RES_RES_SHIFT) |	\
	 (((val) & STM32_ADC_RES_VAL_MASK) << STM32_ADC_RES_VAL_SHIFT) |	\
	 (((offset) & STM32_ADC_RES_OFF_MASK) << STM32_ADC_RES_OFF_SHIFT))

/**
 * @brief Resolution vamue from ADC resolutions bit field.
 *
 * @param res_val ADC resolution bit field value.
 */
#define STM32_ADC_RES_RES(res_val)						\
	(((res_val) >> STM32_ADC_RES_RES_SHIFT) & STM32_ADC_RES_RES_MASK)

/**
 * @brief Get resolution value in ADC register space
 *
 * @param res_val ADC resolution bit field value.
 */
#define STM32_ADC_RES_VAL(res_val)						\
	(((res_val) >> STM32_ADC_RES_VAL_SHIFT) & STM32_ADC_RES_VAL_MASK)	\
	<< (((res_val) >> STM32_ADC_RES_OFF_SHIFT) & STM32_ADC_RES_OFF_MASK)


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32_ADC_H_ */
