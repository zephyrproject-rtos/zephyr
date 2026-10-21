/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <ironside_client/layout.h>
#include <ironside_client/services/boot_report.h>

LOG_MODULE_DECLARE(ironside_te, CONFIG_IRONSIDE_TE_LOG_LEVEL);

static bool check_region_matches(const char *name, const struct ironside_boot_region *region,
			   uint32_t addr, uint32_t size)
{
	if (region->addr == addr && region->size == size) {
		return true;
	}

	LOG_ERR("%s is 0x%08x+0x%x on this device but 0x%08x+0x%x in the devicetree", name,
		(unsigned int)region->addr, (unsigned int)region->size,
		(unsigned int)addr, (unsigned int)size);

	return false;
}

#define CHECK_PARTITION_MATCHES(region, label)                       \
	check_region_matches(#label, &report.region,                 \
			     DT_PARTITION_ADDR(DT_NODELABEL(label)), \
			     DT_REG_SIZE(DT_NODELABEL(label)))

static int boot_layout_check(void)
{
	struct ironside_boot_report report;
	size_t len;
	int32_t err;

	err = ironside_boot_report_get(&report, sizeof(report), &len);
	if (err != 0 || len < sizeof(report)) {
		LOG_WRN("ironside_boot_report_get() failed: %d", (int)err);
		return 0;
	}

	CHECK_PARTITION_MATCHES(app, slot0_partition);
	CHECK_PARTITION_MATCHES(dfubank, dfubank_partition);
	CHECK_PARTITION_MATCHES(appstorage, storage_partition);
	CHECK_PARTITION_MATCHES(dfuimage, dfuimage_partition);
	/* The reported non-secure RAM includes the APP-XRAM window above sram_ns. */
	check_region_matches("sram_ns", &report.ram_ns, DT_REG_ADDR(DT_NODELABEL(sram_ns)),
		       DT_REG_SIZE(DT_NODELABEL(sram_ns)) + IRONSIDE_RAM_XRAM_SIZE);

	return 0;
}
SYS_INIT(boot_layout_check, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
