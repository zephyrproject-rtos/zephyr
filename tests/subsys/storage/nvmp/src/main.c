/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/nvmp.h>

#define NVMP_FLASH_NODE flash_sim0
#define NVMP_FLASH_PARTITION_NODE storage_partition
#define NVMP_EEPROM_NODE eeprom0
#define NVMP_EEPROM_PARTITION_NODE eeprom0_partition
#define NVMP_RETAINED_MEM_NODE retainedmem0
#define NVMP_RETAINED_MEM_PARTITION_NODE retainedmem0_partition

ZTEST_BMEM static const struct nvmp_info *nvmp;

static void *nvmp_api_setup(void)
{
	return NULL;
}

ZTEST_USER(nvmp_api, test_read_write_erase)
{
	uint8_t *wr = "/a9/9a/a9/9a";
	uint8_t rd[sizeof(wr)];
	enum nvmp_type type = nvmp_get_type(nvmp);
	int rc = 0;

	memset(rd, 0, sizeof(rd));

	zassert_false(type == UNKNOWN, "nvmp has bad type");

	rc = nvmp_read(nvmp, 0, rd, sizeof(rd));
	zassert_equal(rc, 0, "read returned [%d]", rc);

	rc = nvmp_write(nvmp, 0, wr, sizeof(wr));
	zassert_equal(rc, 0, "write returned [%d]", rc);

	rc = nvmp_read(nvmp, 0, rd, sizeof(rd));
	zassert_equal(rc, 0, "read returned [%d]", rc);

	zassert_equal(memcmp(wr, rd, sizeof(rd)), 0, "read/write data differ");

	if ((IS_ENABLED(CONFIG_NVMP_FLASH)) && (type == FLASH)) {
		const struct device *dev =
			(const struct device *)nvmp_get_store(nvmp);
		const struct flash_parameters *flparam =
			flash_get_parameters(dev);

		rc = nvmp_erase(nvmp, 0, nvmp_get_size(nvmp));
		zassert_equal(rc, 0, "erase returned [%d]", rc);

		rc = nvmp_read(nvmp, 0, rd, sizeof(rd));
		zassert_equal(rc, 0, "read returned [%d]", rc);

		for (int i = 0; i < sizeof(rd); i++) {
			zassert_true(rd[i] == flparam->erase_value,
				     "erase failed");
		}
	}

	if ((IS_ENABLED(CONFIG_NVMP_EEPROM) && (type == EEPROM)) ||
	    (IS_ENABLED(CONFIG_NVMP_RETAINED_MEM) && (type == RETAINED_MEM))) {
		rc = nvmp_erase(nvmp, 0, sizeof(rd));
		zassert_equal(rc, 0, "erase returned [%d]", rc);

		rc = nvmp_read(nvmp, 0, rd, sizeof(rd));
		zassert_equal(rc, 0, "read returned [%d]", rc);

		for (int i = 0; i < sizeof(rd); i++) {
			zassert_true(rd[i] == CONFIG_NVMP_ERASE_VALUE,
				     "erase failed");
		}
	}
}

ZTEST_SUITE(nvmp_api, NULL, nvmp_api_setup, NULL, NULL, NULL);

/* Run all of our tests on the given mtd */
static void run_tests_on_nvmp(const struct nvmp_info *info)
{
	nvmp = info;
	k_object_access_grant((const struct device *)nvmp_get_store(info),
			      k_current_get());
	ztest_run_all(NULL);
}

void test_main(void)
{
#ifdef CONFIG_NVMP_FLASH
	run_tests_on_nvmp(NVMP_GET(NVMP_FLASH_NODE));
	run_tests_on_nvmp(NVMP_GET(NVMP_FLASH_PARTITION_NODE));
#endif

#ifdef CONFIG_NVMP_EEPROM
	run_tests_on_nvmp(NVMP_GET(NVMP_EEPROM_NODE));
	run_tests_on_nvmp(NVMP_GET(NVMP_EEPROM_PARTITION_NODE));
#endif

#ifdef CONFIG_NVMP_RETAINED_MEM
	run_tests_on_nvmp(NVMP_GET(NVMP_RETAINED_MEM_NODE));
	run_tests_on_nvmp(NVMP_GET(NVMP_RETAINED_MEM_PARTITION_NODE));
#endif

	ztest_verify_all_test_suites_ran();
}
