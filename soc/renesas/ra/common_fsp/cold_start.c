/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <bsp_api.h>
#include "cold_start.h"

#define RSTSR2_CWSF_BIT_MASK BIT(0)

static uint8_t rstsr2_state_at_boot;

bool is_power_on_reset_happen(void)
{
	return ((rstsr2_state_at_boot & RSTSR2_CWSF_BIT_MASK) == 0);
}

void cold_start_handler(void)
{
	/* Detect power on reset */
	rstsr2_state_at_boot = R_SYSTEM->RSTSR2;
	if (R_SYSTEM->RSTSR2_b.CWSF == 0) {
#if defined(CONFIG_RENESAS_RA_BATTERY_BACKUP_MANUAL_CONFIGURE)
		battery_backup_init();
#endif /* CONFIG_RENESAS_RA_BATTERY_BACKUP_MANUAL_CONFIGURE */
		R_SYSTEM->RSTSR2_b.CWSF = 1;
	}
}
