/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>

/* Peripheral voltage ON/OFF GPIO */
#define PERIPH_PON_PIN		0

static int board_reel_board_init(void)
{
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

/*
 * Writes the GPIO registers directly, so it depends on no device. An anchored
 * entry keeps it at the end of PRE_KERNEL, where the deprecated level put it.
 */
#define SYS_ANCHOR_reel_board SYS_ANCHOR(reel_board)
SYS_INIT_ANCHORED(reel_board, board_reel_board_init, PRE_KERNEL);
