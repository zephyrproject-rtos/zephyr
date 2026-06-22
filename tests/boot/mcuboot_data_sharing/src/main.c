/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <bootutil/boot_status.h>
#include <bootutil/image.h>
#include <zephyr/mcuboot_version.h>

#define FLASH_SECTOR_SIZE 1024
#define FLASH_SECTOR_SIZE_KB 4
#define FLASH_MAX_APP_SECTORS 34
#define FLASH_RESERVED_SECTORS 1
#define FLASH_MAX_APP_SIZE ((FLASH_MAX_APP_SECTORS - FLASH_RESERVED_SECTORS) \
			    * FLASH_SECTOR_SIZE_KB)
#define RUNNING_SLOT 0

ZTEST(mcuboot_shared_data, test_mode)
{
	uint8_t var[1];
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/mode", var, sizeof(var));
	zassert_equal(rc, sizeof(var), "Expected data length mismatch");
	zassert_equal(var[0], MCUBOOT_MODE_SWAP_USING_OFFSET, "Expected data mismatch");
}

ZTEST(mcuboot_shared_data, test_signature_type)
{
	uint8_t var[1];
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/signature_type", var, sizeof(var));
	zassert_equal(rc, sizeof(var), "Expected data length mismatch");
	zassert_equal(var[0], MCUBOOT_SIGNATURE_TYPE_RSA, "Expected data mismatch");
}

ZTEST(mcuboot_shared_data, test_recovery)
{
	uint8_t var[1];
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/recovery", var, sizeof(var));
	zassert_equal(rc, sizeof(var), "Expected data length mismatch");
	zassert_equal(var[0], MCUBOOT_RECOVERY_MODE_NONE, "Expected data mismatch");
}

ZTEST(mcuboot_shared_data, test_running_slot)
{
	uint8_t var[1];
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/running_slot", var, sizeof(var));
	zassert_equal(rc, sizeof(var), "Expected data length mismatch");
	zassert_equal(var[0], RUNNING_SLOT, "Expected data mismatch");
}

ZTEST(mcuboot_shared_data, test_bootloader_version)
{
	uint8_t var[8];
	int rc;
	struct image_version *version = (void *)var;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/bootloader_version", var, sizeof(var));
	zassert_equal(rc, sizeof(var), "Expected data length mismatch");

	zassert_equal(version->iv_major, MCUBOOT_VERSION_MAJOR,
		      "Expected version (major) mismatch");
	zassert_equal(version->iv_minor, MCUBOOT_VERSION_MINOR,
		      "Expected version (minor) mismatch");
	zassert_equal(version->iv_revision, MCUBOOT_PATCHLEVEL,
		      "Expected version (patch level) mismatch");
	zassert_equal(version->iv_build_num, 0, "Expected version (build number) mismatch");
}

ZTEST(mcuboot_shared_data, test_max_application_size)
{
	uint8_t var[4];
	uint32_t value;
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/max_application_size", var, sizeof(var));
	zassert_equal(rc, sizeof(var), "Expected data length mismatch");
	memcpy(&value, var, sizeof(value));
	value /= FLASH_SECTOR_SIZE;
	zassert_equal(value, FLASH_MAX_APP_SIZE, "Expected data mismatch");
}

ZTEST(mcuboot_shared_data, test_invalid)
{
	uint8_t var[4];
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/does_not_exist", var, sizeof(var));
	zassert_not_equal(rc, sizeof(var), "Expected data length (error) mismatch");
	zassert_not_equal(rc, 0, "Expected data length (error) mismatch");
}

ZTEST(mcuboot_shared_data, test_bootloader_version_limited)
{
	uint8_t var[2];
	int rc;

	memset(var, 0xff, sizeof(var));
	rc = settings_runtime_get("blinfo/bootloader_version", var, sizeof(var));
	zassert_not_equal(rc, sizeof(var), "Expected data length mismatch");
}

ZTEST_SUITE(mcuboot_shared_data, NULL, NULL, NULL, NULL, NULL);
