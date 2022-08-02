/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2022 Nordic semiconductor ASA */

#include <zephyr/zephyr.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/fcb.h>

void test_setting_storage_get(void)
{
	int rc;
	void *storage;
	struct fcb_entry loc;

	rc = settings_storage_get(&storage);
	zassert_equal(0, rc, "Can't fetch storage reference (err=%d)", rc);

	zassert_not_null(storage, "Null reference.");

	loc.fe_sector = NULL;
	rc = fcb_getnext((struct fcb *)storage, &loc);

	zassert_equal(rc, 0, "Can't read fcb (err=%d)", rc);
}
