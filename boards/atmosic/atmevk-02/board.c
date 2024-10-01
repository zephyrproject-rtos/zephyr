/*
 * Copyright (c) 2021 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board, CONFIG_SOC_LOG_LEVEL);

#include "nvm.h"
#include "at_apb_nvm_regs_core_macro.h"

static int board_init(void)
{
	bool stacked = NVM_EFUSE_AUTOREAD__STACKED_FLASH__READ(nvm_EFUSE_AUTOREAD);
	bool pkg = NVM_EFUSE_AUTOREAD__PKG_5X5__READ(nvm_EFUSE_AUTOREAD);
	bool non_harv = NVM_EFUSE_AUTOREAD__OTP_HARV_DISABLED__READ(nvm_EFUSE_AUTOREAD);
	bool csp = NVM_EFUSE_AUTOREAD__CSP__READ(nvm_EFUSE_AUTOREAD);
	bool pkg_7x7 = NVM_EFUSE_AUTOREAD__PKG_7X7__READ(nvm_EFUSE_AUTOREAD);

	LOG_INF("ATM%d2%d%d-x1x silicon: %s pkg, %s", non_harv ? 2 : 3,
		pkg ? (csp ? 5 : 0) : (pkg_7x7 ? 3 : 2), stacked ? 2 : 1,
		pkg ? (csp ? "CSP" : "5x5") : (pkg_7x7 ? "7x7" : "6x6"),
		stacked ? "Stacked flash" : "External flash");

	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_2, 0);
