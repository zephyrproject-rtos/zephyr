/*
 * Copyright (c) 2023 STMicrelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STM32_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STM32_ADC_H_

#include <zephyr/dt-bindings/adc/adc.h>

#define STM32_ADC_REG_MASK		BIT_MASK(8)
#define STM32_ADC_REG_SHIFT		0U
#define STM32_ADC_SHIFT_MASK		BIT_MASK(5)
#define STM32_ADC_SHIFT_SHIFT		8U
#define STM32_ADC_MASK_MASK		BIT_MASK(3)
#define STM32_ADC_MASK_SHIFT		13U
#define STM32_ADC_REG_VAL_MASK		BIT_MASK(3)
#define STM32_ADC_REG_VAL_SHIFT		16U
#define STM32_ADC_REAL_VAL_MASK		BIT_MASK(13)
#define STM32_ADC_REAL_VAL_SHIFT	19U

/**
 * @brief STM32 ADC configuration bit field.
 *
 * - reg      (0..0xFF)       [ 0 : 7 ]
 * - shift    (0..31)         [ 8 : 12 ]
 * - mask     (0x1, 0x3, 0x7) [ 13 : 15 ]
 * - reg_val  (0..7)          [ 16 : 18 ]
 * - real_val (0..8191)       [ 19 : 31 ]
 *
 * @param reg ADC_x register offset
 * @param shift Position within ADC_x.
 * @param mask Mask for the ADC_x field.
 * @param reg_val Register value (0, 1, ... 7).
 * @param real_val Real corresponding value (0, 1, ... 8191).
 */
#define STM32_ADC(real_val, reg_val, mask, shift, reg)				\
	((((reg) & STM32_ADC_REG_MASK) << STM32_ADC_REG_SHIFT) |		\
	 (((shift) & STM32_ADC_SHIFT_MASK) << STM32_ADC_SHIFT_SHIFT) |		\
	 (((mask) & STM32_ADC_MASK_MASK) << STM32_ADC_MASK_SHIFT) |		\
	 (((reg_val) & STM32_ADC_REG_VAL_MASK) << STM32_ADC_REG_VAL_SHIFT) |	\
	 (((real_val) & STM32_ADC_REAL_VAL_MASK) << STM32_ADC_REAL_VAL_SHIFT))

#define STM32_ADC_GET_REAL_VAL(val)	\
	(((val) >> STM32_ADC_REAL_VAL_SHIFT) & STM32_ADC_REAL_VAL_MASK)

#define STM32_ADC_GET_REG_VAL(val)	\
	(((val) >> STM32_ADC_REG_VAL_SHIFT) & STM32_ADC_REG_VAL_MASK)

#define STM32_ADC_GET_MASK(val)		\
	(((val) >> STM32_ADC_MASK_SHIFT) & STM32_ADC_MASK_MASK)

#define STM32_ADC_GET_SHIFT(val)	\
	(((val) >> STM32_ADC_SHIFT_SHIFT) & STM32_ADC_SHIFT_MASK)

#define STM32_ADC_GET_REG(val)		\
	(((val) >> STM32_ADC_REG_SHIFT) & STM32_ADC_REG_MASK)

/*
 * Macro used to store resolution info. STM32_ADC_RES_* macros are defined in
 * respective stm32xx_adc.h files
 */
#define STM32_ADC_RES(resolution, reg_val)	\
	STM32_ADC(resolution, reg_val, STM32_ADC_RES_MASK, STM32_ADC_RES_SHIFT, \
		  STM32_ADC_RES_REG)

/**
 * @name STM32 ADC clock source
 * This value is to set <st,adc-clock-source>
 * One or both values may not apply to all series. Refer to the RefMan
 * @{
 */
#define SYNC  1
#define ASYNC 2
/** @} */

/**
 * @name STM32 ADC sequencer type
 * This value is to set <st,adc-sequencer>
 * One or both values may not apply to all series. Refer to the RefMan
 * @{
 */
#define NOT_FULLY_CONFIGURABLE	0
#define FULLY_CONFIGURABLE	1
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STM32_ADC_H_ */
