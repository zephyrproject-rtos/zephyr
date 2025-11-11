/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/devicetree.h>
#include <nrf_ironside/boot_report.h>

#define IRONSIDE_SE_BOOT_REPORT_ADDR DT_REG_ADDR(DT_NODELABEL(ironside_se_boot_report))

int ironside_boot_report_get(const struct ironside_boot_report **report)
{
	const struct ironside_boot_report *tmp_report = (void *)IRONSIDE_SE_BOOT_REPORT_ADDR;

	if (tmp_report->magic != IRONSIDE_BOOT_REPORT_MAGIC) {
		return -EINVAL;
	}

	*report = tmp_report;

	return 0;
}
