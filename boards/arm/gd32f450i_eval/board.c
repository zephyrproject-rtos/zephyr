/*
 * Copyright (c) 2021, Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>

static int board_init(const struct device *dev)
{
	rcu_periph_clock_enable(RCU_GPIOA);

	/* PA9: USART0 TX */
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_9);

	/* PA10: USART0 RX */
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_10);

	rcu_periph_clock_disable(RCU_GPIOA);

	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
