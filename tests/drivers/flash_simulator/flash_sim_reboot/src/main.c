/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

#define MAGIC_WORD 0xABDE2134

#define SOC_NV_FLASH_NODE          DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_sim_0)
#define FLASH_SIMULATOR_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

static const struct device *const flash_dev = DEVICE_DT_GET(DT_NODELABEL(sim_flash_controller));
static uint32_t boot_count __noinit;

ZTEST(flash_sim_reboot, test_preserve_over_reboot)
{
	uint32_t word = MAGIC_WORD;
	int rc;

	if (boot_count == 0) {
		printk("First boot, erasing flash\n");
		rc = flash_erase(flash_dev, 0, FLASH_SIMULATOR_FLASH_SIZE);
		zassert_equal(0, rc, "Failed to erase flash");
		printk("Writing magic word to offset 0\n");
		rc = flash_write(flash_dev, 0, &word, sizeof(word));
		zassert_equal(0, rc, "Failed to write flash");
		printk("Rebooting device...\n");
		boot_count += 1;
		sys_reboot(SYS_REBOOT_WARM);
		zassert_unreachable("Failed to reboot");
	} else if (boot_count == 1) {
		printk("Second boot, reading magic word\n");
		rc = flash_read(flash_dev, 0, &word, sizeof(word));
		zassert_equal(0, rc, "Failed to read flash");
		zassert_equal(MAGIC_WORD, word, "Magic word not preserved");
	} else {
		zassert_unreachable("Unexpected boot_count value %d", boot_count);
	}
}

void *flash_sim_setup(void)
{
	zassert_true(device_is_ready(flash_dev), "Simulated flash device not ready");

	return NULL;
}

ZTEST_SUITE(flash_sim_reboot, NULL, flash_sim_setup, NULL, NULL, NULL);
