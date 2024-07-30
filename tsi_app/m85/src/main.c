/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

#define PRINT_TSI_BOOT_BANNER()  printk("***!!WELCOME TO TSAVORITE SCALABLE INTELLIGENCe!!***\n")
#define LOG_MODULE_NAME tsi_m85_
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

int main(void)
{

	/* TSI banner */
	PRINT_TSI_BOOT_BANNER();
	LOG_INF("Test Platform: %s", CONFIG_BOARD_TARGET);

	LOG_WRN("Testing on FPGA; Multi module init TBD");
	LOG_DBG("Debugging - tsi_app module");

	return 0;
}
