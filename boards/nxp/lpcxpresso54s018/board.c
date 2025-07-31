/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 *
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lpc54s018_board, LOG_LEVEL_INF);

#if DT_NODE_EXISTS(DT_NODELABEL(sdram0)) && DT_NODE_HAS_STATUS(DT_NODELABEL(sdram0), okay)
/* SDRAM initialization would go here if needed at board level */
/* Currently handled by the EMC driver */
#endif

static int lpc54s018_board_init(void)
{
	LOG_DBG("Board initialization");
	
#if DT_NODE_EXISTS(DT_NODELABEL(sdram0)) && DT_NODE_HAS_STATUS(DT_NODELABEL(sdram0), okay)
	LOG_INF("SDRAM memory region enabled at 0x%08x, size %d MB", 
		DT_REG_ADDR(DT_NODELABEL(sdram0)),
		DT_REG_SIZE(DT_NODELABEL(sdram0)) / (1024 * 1024));
#endif

	return 0;
}

/* Board initialization runs after kernel services are up */
SYS_INIT(lpc54s018_board_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);