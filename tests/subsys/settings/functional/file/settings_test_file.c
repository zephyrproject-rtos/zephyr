/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2022 Nordic semiconductor ASA */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/fs.h>

ZTEST(settings_functional, test_setting_storage_get)
{
	int rc;
	void *storage;

	struct fs_dirent entry;

	rc = settings_storage_get(&storage);
	zassert_equal(0, rc, "Can't fetch storage reference (err=%d)", rc);

	zassert_not_null(storage, "Null reference.");

	rc = fs_stat((const char *)storage, &entry);

	zassert_true(rc >= 0, "Can't find the file (err=%d)", rc);
}
ZTEST_SUITE(settings_functional, NULL, NULL, NULL, NULL, NULL);
