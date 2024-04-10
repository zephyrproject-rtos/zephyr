/*
 * Copyright (c) 2021 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>

#include "compiler.h"
#include "at_wrpr.h"
#include "at_pinmux.h"

static int atm_evk_pinmux_init(void)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ble), okay) && CONFIG_BT
#endif

	return 0;
}

SYS_INIT(atm_evk_pinmux_init, PRE_KERNEL_1, 0);
