/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2022 Nordic semiconductor ASA */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/fcb.h>

ZTEST(settings_functional, test_setting_storage_get)
{
	int rc;
	void *storage;
	struct fcb_entry loc;

	memset(&loc, 0, sizeof(struct fcb_entry));
	rc = settings_storage_get(&storage);
	zassert_equal(0, rc, "Can't fetch storage reference (err=%d)", rc);

	zassert_not_null(storage, "Null reference.");

	loc.fe_sector = NULL;
	rc = fcb_getnext((struct fcb *)storage, &loc);

	zassert_equal(rc, 0, "Can't read fcb (err=%d)", rc);
}
ZTEST_SUITE(settings_functional, NULL, NULL, NULL, NULL, NULL);
