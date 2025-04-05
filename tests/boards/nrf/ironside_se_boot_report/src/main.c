/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf/ironside_se_boot_report.h>
#include <stdio.h>

int main(void)
{
	int err;
	const struct ironside_se_boot_report *report;

	err = ironside_se_boot_report_get(&report);
	printf("err:  %d\n", err);
	printf("version: 0x%x\n", report->ironside_se_version_int);
	printf("extraversion: %s\n", report->ironside_se_extraversion);

	return 0;
}
