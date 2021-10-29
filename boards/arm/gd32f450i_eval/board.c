/*
 * Copyright (c) 2021 BrainCo Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>

#include <gd32f4xx.h>

/** Initialize the board's hardware through GD32 HAL */
static int board_init(const struct device *dev)
{
	/* Enable GPIOA clock for PA9, PA10 */
	rcu_periph_clock_enable(RCU_GPIOA);

	/* Pin AF definition can be found in datasheet Device overview section */
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);

	/* Configure USART0 Tx as alternate function push-pull */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

	/* Configure USART0 Rx as alternate function push-pull */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
