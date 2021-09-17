/*
 * Copyright (c) 2021 ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>

static int board_init(const struct device *dev)
{
	rcu_periph_clock_enable(RCU_GPIOA);

	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
