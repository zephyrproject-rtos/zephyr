/*
 * SPDX-FileCopyrightText: Copyright 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board initialization for mimxrt700_evk - NPU power on, SRAM1 MPU config
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "fsl_power.h"

static int board_init(void)
{
	/* Power on NPU */
	POWER_DisablePD(kPDRUNCFG_APD_NPU);
	POWER_DisablePD(kPDRUNCFG_PPD_NPU);

	POWER_ApplyPD();
	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_1, 0);
