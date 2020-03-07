/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ztest.h>

#include <zephyr.h>
#include <power/reboot.h>
#include <string.h>

#include <settings/settings.h>
#include <storage/flash_map.h>
#include <drivers/flash.h>

static u32_t val32;

#if defined(CONFIG_SOC_SERIES_STM32L0X) || defined(CONFIG_SOC_SERIES_STM32L0X)
#define ERASED_VAL 0x00
#else
#define ERASED_VAL 0xFF
#endif

/* leverage that this area has to be embededd flash part */
#ifdef DT_FLASH_AREA_IMAGE_0_ID
static const volatile __attribute__((section(".rodata")))
__aligned(DT_FLASH_WRITE_BLOCK_SIZE)
u8_t prepared_mark[DT_FLASH_WRITE_BLOCK_SIZE] = {ERASED_VAL};
#endif

static int c1_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	int rc;
	const char *next;

	if (settings_name_steq(name, "val32", &next) && !next) {
		rc = read_cb(cb_arg, &val32, sizeof(val32));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	return -ENOENT;
}

static int c1_export(int (*export_func)(const char *name,
					const void *value, size_t val_len))
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


void test_prepare_storage(void)
{
#ifdef DT_FLASH_AREA_IMAGE_0_ID
/* This procedure uses mark which is stored inside SoC embedded program
 * flash. It will not work on devices on which read/write to them is not
 * possible.
 */
	int err;
	const struct flash_area *fa;
	struct device *dev;
	u8_t new_val[DT_FLASH_WRITE_BLOCK_SIZE];

	if (prepared_mark[0] == ERASED_VAL) {
		TC_PRINT("First run: erasing the storage\r\n");
		err = flash_area_open(DT_FLASH_AREA_STORAGE_ID, &fa);
		zassert_true(err == 0, "Can't open storage flash area");

		err = flash_area_erase(fa, 0, fa->fa_size);
		zassert_true(err == 0, "Can't erase storage flash area");

		err = flash_area_open(DT_FLASH_AREA_IMAGE_0_ID, &fa);
		zassert_true(err == 0, "Can't open storage flash area");

		dev = flash_area_get_device(fa);

		err = flash_write_protection_set(dev, false);
		zassert_true(err == 0, "can't unprotect flash");

		(void)memset(new_val, (~ERASED_VAL) & 0xFF,
			     DT_FLASH_WRITE_BLOCK_SIZE);
		err = flash_write(dev, (off_t)&prepared_mark, &new_val,
				  sizeof(new_val));
		zassert_true(err == 0, "can't write prepared_mark");
	}
#else
	TC_PRINT("Storage preparation can't be performed\r\n");
	TC_PRINT("Erase storage manually before test flashin\r\n");
#endif
}

void test_init_setup(void)
{
	int err;

	test_prepare_storage();

	settings_subsys_init();

	err = settings_register(&c1_settings);
	zassert_true(err == 0, "can't regsister the settings handler");

	err = settings_load();
	zassert_true(err == 0, "can't load settings");

	if (val32 < 1) {
		val32 = 1U;
		err = settings_save();
		zassert_true(err == 0, "can't save settings");
		k_sleep(K_MSEC(250));
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
