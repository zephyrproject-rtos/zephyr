/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief board init functions
 */

#include <init.h>
#include <soc.h>

static const struct acts_pin_config board_pin_config[] = {
	/* uart0 */
	{ 2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) },
	{ 3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) },
};

static int atb1103_golden_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	acts_pinmux_setup_pins(board_pin_config, ARRAY_SIZE(board_pin_config));

	return 0;
}

SYS_INIT(atb1103_golden_pinmux_init, PRE_KERNEL_1, 0);
