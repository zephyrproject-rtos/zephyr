/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2022 Nordic semiconductor ASA */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/nvs.h>

ZTEST(settings_functional, test_setting_storage_get)
{
	int rc;
	void *storage;
	uint16_t data = 0x5a5a;
	k_ssize_t nvs_rc;

	rc = settings_storage_get(&storage);
	zassert_equal(0, rc, "Can't fetch storage reference (err=%d)", rc);

	zassert_not_null(storage, "Null reference.");

	nvs_rc = nvs_write((struct nvs_fs *)storage, 26, &data, sizeof(data));

	zassert_true(nvs_rc >= 0, "Can't read nvs record (err=%d).", rc);
}
ZTEST_SUITE(settings_functional, NULL, NULL, NULL, NULL, NULL);
