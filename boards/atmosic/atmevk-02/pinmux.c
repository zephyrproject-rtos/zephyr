/*
 * Copyright (c) 2021-2023 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>

#include "at_wrpr.h"
#include "at_pinmux.h"

#include "intisr.h"

static int atm_evk_pinmux_init(void)
{
	CMSDK_WRPR->INTRPT_CFG_6 = INTISR_SRC_GPIO0_COMB;
	CMSDK_WRPR->INTRPT_CFG_11 = INTISR_SRC_TRNG;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ble), okay) && CONFIG_BT
	CMSDK_WRPR->INTRPT_CFG_15 = INTISR_SRC_BLE;
#endif

	return 0;
}

SYS_INIT(atm_evk_pinmux_init, PRE_KERNEL_1, 0);
