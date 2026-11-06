/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

#define MAGIC_WORD 0xABDE2134

static const struct device *const eeprom_dev = DEVICE_DT_GET(DT_NODELABEL(sim_eeprom));
static uint32_t boot_count __noinit;

ZTEST(eeprom_sim_reboot, test_preserve_over_reboot)
{
	uint32_t word = MAGIC_WORD;
	int rc;

	if (boot_count == 0) {
		printk("First boot, writing magic word to offset 0\n");
		rc = eeprom_write(eeprom_dev, 0, &word, sizeof(word));
		zassert_equal(0, rc, "Failed to write EEPROM");
		printk("Rebooting device...\n");
		boot_count += 1;
		sys_reboot(SYS_REBOOT_WARM);
		zassert_unreachable("Failed to reboot");
	} else if (boot_count == 1) {
		printk("Second boot, reading magic word\n");
		rc = eeprom_read(eeprom_dev, 0, &word, sizeof(word));
		zassert_equal(0, rc, "Failed to read EEPROM");
		zassert_equal(MAGIC_WORD, word, "Magic word not preserved");
	} else {
		zassert_unreachable("Unexpected boot_count value %u", boot_count);
	}
}

void *eeprom_sim_setup(void)
{
	zassert_true(device_is_ready(eeprom_dev), "Simulated EEPROM device not ready");

	return NULL;
}

ZTEST_SUITE(eeprom_sim_reboot, NULL, eeprom_sim_setup, NULL, NULL, NULL);
