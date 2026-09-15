/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CORDIC_ADOPTION_STM32_H_
#define ZEPHYR_INCLUDE_CORDIC_ADOPTION_STM32_H_

#include <zephyr/drivers/cordic.h>


enum stm32_cordic_data_mode {
	/* Directly reads the result from the CORDIC hardware -
	 * bus wait status will be introduced by HW itself s.t. CPU stalls for computation time duration
	 * NOTE: This is default mode
	 */
	STM32_CORDIC_DATA_MODE_ZERO_OVERHEAD,
	/* Reads the result by polling the CORDIC hardware ready register status */
	STM32_CORDIC_DATA_MODE_POLLING,
	/* Reads the result using interrupts */
	STM32_CORDIC_DATA_MODE_INTERRUPT,
	/* Reads and writes data using DMA without CPU intervention
	 * NOTE: DMA is functiionality is not part of the driver.
	 *       A DMA shall be configured and managed by the application.
	 * TODO: What about writing by DMA and reading by polling etc or
	 * 	writing by CPU and reading by DMA etc? Shall we support all combinations?
	 */
	STM32_CORDIC_DATA_MODE_DMA,
};

struct cordic_config_options {
	enum stm32_cordic_data_mode data_mode;
};

#endif /* ZEPHYR_INCLUDE_CORDIC_ADOPTION_STM32_H_ */
