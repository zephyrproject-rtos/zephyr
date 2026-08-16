/*
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

static int board_init(void)
{
	return 0;
}

/*
 * This runs after the devices it needs, which initialize at PRE_KERNEL, so
 * neither platform init hook fits: the early one runs before any of them and
 * the late one only after all of POST_KERNEL. An anchored entry runs at the
 * end of PRE_KERNEL instead, ordered after the SoC initialization where the
 * SoC registers one.
 */
#define SYS_ANCHOR_board_init                                                  \
	SYS_ANCHOR_AFTER_IF(CONFIG_SOC_INIT_ANCHOR, SYS_ANCHOR_soc_init,       \
			    board_init)
SYS_INIT_ANCHORED(board_init, board_init, PRE_KERNEL);
