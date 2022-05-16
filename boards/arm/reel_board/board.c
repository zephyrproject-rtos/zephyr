/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>

/* Peripheral voltage ON/OFF GPIO */
#define PERIPH_PON_PIN		0

static int board_reel_board_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	volatile NRF_GPIO_Type *gpio = NRF_P1;

	/*
	 * Workaround to enable peripheral voltage.
	 */
	gpio->PIN_CNF[PERIPH_PON_PIN] =
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

	gpio->OUTSET = BIT(PERIPH_PON_PIN);

	return 0;
}

SYS_INIT(board_reel_board_init, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
