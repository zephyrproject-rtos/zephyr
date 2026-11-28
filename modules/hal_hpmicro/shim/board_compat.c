/*
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "board.h"

void board_delay_ms(uint32_t ms)
{
	k_busy_wait(ms * 1000U);
}
