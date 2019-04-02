/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ztest.h>

#include <zephyr.h>
#include <misc/reboot.h>
#include <string.h>

#include <settings/settings.h>

static u32_t val32;

static int c1_set(int argc, char **argv, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	int rc;

	if (argc == 1 && !strcmp(argv[0], "val32")) {
		rc = read_cb(cb_arg, &val32, sizeof(val32));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	return -ENOENT;
}

static int c1_export(int (*export_func)(const char *name, void *value,
					size_t val_len))
{
	(void)export_func("hello/val32", &val32, sizeof(val32));

	return 0;
}

static struct settings_handler c1_settings = {
	.name = "hello",
	.h_set = c1_set,
	.h_export = c1_export,
};

void test_init(void)
{
	int err;
	u32_t prev_int;

	val32++;

	err = settings_save();
	zassert_true(err == 0, "can't save settings");

	prev_int = val32;
	val32 = 0U;
	err = settings_load();
	zassert_true(err == 0, "can't load settings");
	zassert_equal(prev_int, val32,
		      "load value doesn't match to what was saved");
}

void test_init_setup(void)
{
	int err;

	settings_subsys_init();

	err = settings_register(&c1_settings);
	zassert_true(err == 0, "can't regsister the settings handler");

	err = settings_load();
	zassert_true(err == 0, "can't load settings");

	if (val32 < 1) {
		val32 = 1U;
		err = settings_save();
		zassert_true(err == 0, "can't save settings");
		k_sleep(250);
		sys_reboot(SYS_REBOOT_COLD);
	}
}

void test_main(void)
{
	/* Bellow call is not used as a test setup intentionally.    */
	/* It causes device reboota at the first device run after it */
	/* was flashed. */
	test_init_setup();

	ztest_test_suite(test_initialization,
			 ztest_unit_test(test_init)
			);

	ztest_run_test_suite(test_initialization);
}
