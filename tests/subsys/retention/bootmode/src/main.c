/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define BOOT_MODE_NODE DT_CHOSEN(zephyr_boot_mode)

/* Same untranslated offset as the retention driver uses */
#define AREA_OFFSET DT_PROP_BY_IDX(BOOT_MODE_NODE, reg, 0)
#define AREA_SIZE   DT_REG_SIZE(BOOT_MODE_NODE)
#define PREFIX_LEN  DT_PROP_LEN_OR(BOOT_MODE_NODE, prefix, 0)
#define USER_SIZE   (AREA_SIZE - PREFIX_LEN - DT_PROP(BOOT_MODE_NODE, checksum))
#define PREFIX_ONLY (PREFIX_LEN > 0 && USER_SIZE == 0)

/* Magic value used as prefix by the overlays, 0xf01669ef little-endian */
#define TEST_MAGIC 0xf01669efU

#define OTHER_MODE 0x42U

static const struct device *const parent = DEVICE_DT_GET(DT_PARENT(BOOT_MODE_NODE));
static const uint8_t prefix[] = DT_PROP_OR(BOOT_MODE_NODE, prefix, {0});

static void read_area(uint8_t *buf)
{
	zassert_ok(retained_mem_read(parent, AREA_OFFSET, buf, AREA_SIZE));
}

static bool area_is_zero(void)
{
	uint8_t buf[AREA_SIZE];

	read_area(buf);

	for (size_t i = 0; i < AREA_SIZE; i++) {
		if (buf[i] != 0U) {
			return false;
		}
	}

	return true;
}

static void expect_mode_stored(uint8_t boot_mode)
{
	uint8_t buf[AREA_SIZE];

	read_area(buf);
	zassert_mem_equal(buf, prefix, PREFIX_LEN, "prefix not stored");

	if (PREFIX_LEN == 4) {
		zassert_equal(sys_get_le32(buf), TEST_MAGIC, "unexpected magic value");
	}

	if (!PREFIX_ONLY) {
		zassert_equal(buf[PREFIX_LEN], boot_mode, "boot mode byte not stored");
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bootmode_clear());
}

ZTEST(retention_bootmode, test_cleared)
{
	zassert_true(area_is_zero());
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 0);

	/* A cleared area with a prefix and user data is not valid, no mode matches */
	if (PREFIX_ONLY || PREFIX_LEN == 0) {
		zassert_equal(bootmode_check(BOOT_MODE_TYPE_NORMAL), 1);
	} else {
		zassert_equal(bootmode_check(BOOT_MODE_TYPE_NORMAL), 0);
	}
}

ZTEST(retention_bootmode, test_set_bootloader)
{
	zassert_ok(bootmode_set(BOOT_MODE_TYPE_BOOTLOADER));
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 1);
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_NORMAL), 0);
	expect_mode_stored(BOOT_MODE_TYPE_BOOTLOADER);
}

ZTEST(retention_bootmode, test_set_bootloader_twice)
{
	uint8_t first[AREA_SIZE];
	uint8_t second[AREA_SIZE];

	zassert_ok(bootmode_set(BOOT_MODE_TYPE_BOOTLOADER));
	read_area(first);
	zassert_ok(bootmode_set(BOOT_MODE_TYPE_BOOTLOADER));
	read_area(second);
	zassert_mem_equal(first, second, AREA_SIZE);
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 1);
}

ZTEST(retention_bootmode, test_set_normal)
{
	zassert_ok(bootmode_set(BOOT_MODE_TYPE_BOOTLOADER));
	zassert_ok(bootmode_set(BOOT_MODE_TYPE_NORMAL));
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 0);
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_NORMAL), 1);

	if (PREFIX_ONLY) {
		zassert_true(area_is_zero());
	} else {
		expect_mode_stored(BOOT_MODE_TYPE_NORMAL);
	}
}

ZTEST(retention_bootmode, test_set_other_mode)
{
	if (PREFIX_ONLY) {
		zassert_equal(bootmode_set(OTHER_MODE), -ENOTSUP);
		zassert_true(area_is_zero());
		zassert_equal(bootmode_check(OTHER_MODE), 0);
	} else {
		zassert_ok(bootmode_set(OTHER_MODE));
		zassert_equal(bootmode_check(OTHER_MODE), 1);
		zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 0);
		expect_mode_stored(OTHER_MODE);
	}
}

ZTEST(retention_bootmode, test_clear)
{
	zassert_ok(bootmode_set(BOOT_MODE_TYPE_BOOTLOADER));
	zassert_ok(bootmode_clear());
	zassert_true(area_is_zero());
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 0);
}

/* The magic value is written and consumed by the bootloader behind the back of the
 * retention system, make sure the boot mode interface follows.
 */
ZTEST(retention_bootmode, test_prefix_written_externally)
{
	if (!PREFIX_ONLY) {
		ztest_test_skip();
	}

	zassert_ok(retained_mem_write(parent, AREA_OFFSET, prefix, PREFIX_LEN));
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 1);
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_NORMAL), 0);

	zassert_ok(retained_mem_clear(parent));
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_BOOTLOADER), 0);
	zassert_equal(bootmode_check(BOOT_MODE_TYPE_NORMAL), 1);

	zassert_ok(bootmode_set(BOOT_MODE_TYPE_BOOTLOADER));
	expect_mode_stored(BOOT_MODE_TYPE_BOOTLOADER);
}

ZTEST_SUITE(retention_bootmode, NULL, NULL, before, NULL, NULL);
