/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>

#define TEST_KEY   "test/persist"
#define TEST_VALUE 0xfeedda7a

static uint32_t boot_count __noinit;

ZTEST(settings_persistence, test_preserve_over_reboot)
{
	uint32_t value = 0;
	int rc;

	rc = settings_subsys_init();
	zassert_equal(0, rc, "Failed to initialize settings subsystem");

	if (boot_count == 0) {
		value = TEST_VALUE;

		rc = settings_save_one(TEST_KEY, &value, sizeof(value));
		zassert_equal(0, rc, "Failed to save setting");

		/*
		 * Give backends that defer writes time to commit the value
		 * to persistent storage before rebooting.
		 */
		k_msleep(1000);

		value = 0;

		rc = settings_load_one(TEST_KEY, &value, sizeof(value));
		zassert_equal(sizeof(value), rc, "Failed to read setting before reboot");
		zassert_equal(TEST_VALUE, value, "Wrong setting value before reboot");

		boot_count += 1;

		sys_reboot(SYS_REBOOT_WARM);
		zassert_unreachable("Failed to reboot");
	} else if (boot_count == 1) {
		rc = settings_load_one(TEST_KEY, &value, sizeof(value));
		zassert_equal(sizeof(value), rc, "Failed to read setting after reboot");
		zassert_equal(TEST_VALUE, value, "Setting was not preserved after reboot");
	} else {
		zassert_unreachable("Unexpected boot_count value %u", boot_count);
	}
}

ZTEST_SUITE(settings_persistence, NULL, NULL, NULL, NULL, NULL);
