/* Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/zms.h>

ZTEST(settings_functional, test_setting_storage_get)
{
	int rc;
	void *storage;
	uint32_t data = 0xdeadbeef;

	rc = settings_storage_get(&storage);
	zassert_equal(0, rc, "Can't fetch storage reference (err=%d)", rc);
	zassert_not_null(storage, "Null reference.");

	rc = zms_write((struct zms_fs *)storage, 512, &data, sizeof(data));
	zassert_true(rc >= 0, "Can't write ZMS entry (err=%d).", rc);
}
ZTEST_SUITE(settings_functional, NULL, NULL, NULL, NULL, NULL);
