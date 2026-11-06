/*
 * Copyright (c) 2026 favoritewky
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/settings/settings.h>
#include <cmsis_core.h>

extern char __rom_region_start[];

#define STORAGE_NODE   DT_CHOSEN(zephyr_settings_partition)
#define CODE_NODE      DT_CHOSEN(zephyr_code_partition)

/* The storage partition is expected to sit in the flash between the
 * boot vectors at the base of flash and the application image.
 */
#define STORAGE_ADDR DT_REG_ADDR(STORAGE_NODE)
#define STORAGE_SIZE DT_REG_SIZE(STORAGE_NODE)

ZTEST(romstart_flash_base, test_vector_table_at_flash_base)
{
	/* The vector table must stay at the address the SoC boots from,
	 * which is the base of the flash region, not the start of the
	 * application image in the code partition.
	 */
	zassert_equal(SCB->VTOR, CONFIG_ROMSTART_REGION_ADDRESS,
		      "vector table at 0x%08x, expected 0x%08x",
		      (unsigned int)SCB->VTOR, (unsigned int)FLASH_BASE);
}

ZTEST(romstart_flash_base, test_image_in_code_partition)
{
	/* The application image must be linked at the code partition,
	 * leaving the flash below it untouched.
	 */
	zassert_equal((uintptr_t)__rom_region_start,
		      DT_REG_ADDR(CODE_NODE),
		      "image at 0x%08x, code partition at 0x%08x",
		      (unsigned int)(uintptr_t)__rom_region_start,
		      (unsigned int)DT_REG_ADDR(CODE_NODE));

	/* The storage partition must be fully below the image, so that
	 * flashing the image does not erase it.
	 */
	zassert_true(STORAGE_ADDR + STORAGE_SIZE <= (uintptr_t)__rom_region_start,
		     "storage [0x%08x, 0x%08x) overlaps image at 0x%08x",
		     (unsigned int)STORAGE_ADDR,
		     (unsigned int)(STORAGE_ADDR + STORAGE_SIZE),
		     (unsigned int)(uintptr_t)__rom_region_start);
}

static uint8_t expected_value;
static uint8_t loaded_value;
static bool value_loaded;

static int romstart_settings_set(const char *key, size_t len,
				 settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, "test") != 0) {
		return -ENOENT;
	}
	if (len != sizeof(loaded_value)) {
		return -EINVAL;
	}
	read_cb(cb_arg, &loaded_value, sizeof(loaded_value));
	value_loaded = true;

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(romstart, "romstart", NULL,
			       romstart_settings_set, NULL, NULL);

ZTEST(romstart_flash_base, test_settings_survives_partition)
{
	int ret;

	ret = settings_subsys_init();
	zassert_ok(ret, "settings init failed");

	expected_value = 0xa5;
	ret = settings_save_one("romstart/test", &expected_value,
				sizeof(expected_value));
	zassert_ok(ret, "settings save failed");

	loaded_value = 0;
	value_loaded = false;
	ret = settings_load();
	zassert_ok(ret, "settings load failed");
	zassert_true(value_loaded, "settings entry not loaded back");
	zassert_equal(loaded_value, expected_value, "value mismatch");
}

ZTEST_SUITE(romstart_flash_base, NULL, NULL, NULL, NULL, NULL);
