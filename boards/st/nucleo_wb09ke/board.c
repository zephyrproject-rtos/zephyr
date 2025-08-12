/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "stm32wb0x_ll_pwr.h"

/* Implement the correct Pull-up/pull-down for GPIOA and GPIOB
 * to reach the minimum power consumption.
 */
static int board_pupd_init(void)
{
	LL_PWR_EnableGPIOPullUp(LL_PWR_GPIO_A, 
				LL_PWR_GPIO_BIT_0|
				LL_PWR_GPIO_BIT_1|
				LL_PWR_GPIO_BIT_2|
				LL_PWR_GPIO_BIT_3);
  
	LL_PWR_EnableGPIOPullDown(LL_PWR_GPIO_A, 
				  LL_PWR_GPIO_BIT_8|
				  LL_PWR_GPIO_BIT_9|
				  LL_PWR_GPIO_BIT_10|
				  LL_PWR_GPIO_BIT_11);
  
	LL_PWR_EnableGPIOPullDown(LL_PWR_GPIO_B, 
				  LL_PWR_GPIO_BIT_0|
				  LL_PWR_GPIO_BIT_3|
				  LL_PWR_GPIO_BIT_6|
				  LL_PWR_GPIO_BIT_7);
  
	LL_PWR_EnableGPIOPullUp(LL_PWR_GPIO_B, 
				LL_PWR_GPIO_BIT_1|
				LL_PWR_GPIO_BIT_2|
				LL_PWR_GPIO_BIT_4|
				LL_PWR_GPIO_BIT_5|
				LL_PWR_GPIO_BIT_14|
				LL_PWR_GPIO_BIT_15);

	return 0;
}

SYS_INIT(board_pupd_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
