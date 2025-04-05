/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/devicetree.h>
#include <nrf/ironside_se_boot_report.h>

#define IRONSIDE_SE_BOOT_REPORT_ADDR  DT_REG_ADDR(DT_NODELABEL(cpuapp_ironside_se_boot_report))
#define IRONSIDE_SE_BOOT_REPORT_MAGIC CONFIG_SOC_NRF54H20_IRONSIDE_SE_BOOT_REPORT_MAGIC

int ironside_se_boot_report_get(const struct ironside_se_boot_report **report)
{
	const struct ironside_se_boot_report *tmp_report =
		(const struct ironside_se_boot_report *)IRONSIDE_SE_BOOT_REPORT_ADDR;

	if (tmp_report->magic != IRONSIDE_SE_BOOT_REPORT_MAGIC) {
		return -EINVAL;
	}

	*report = tmp_report;

	return 0;
}
