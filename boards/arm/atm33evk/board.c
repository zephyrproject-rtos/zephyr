/*
 * Copyright (c) 2021 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board, CONFIG_SOC_LOG_LEVEL);

static int board_init(void)
{
	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_2, 0);
