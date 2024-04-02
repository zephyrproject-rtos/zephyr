/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/nvmp.h>

#define NVMP_FLASH_PARTITION_NODE0  flash0_partition0
#define NVMP_FLASH_PARTITION_NODE1  flash0_partition1
#define NVMP_EEPROM_PARTITION_NODE0 eeprom0_partition0
#define NVMP_EEPROM_PARTITION_NODE1 eeprom0_partition1

ZTEST_BMEM static const struct nvmp_info *nvmp;

static void *nvmp_api_setup(void)
{
	return NULL;
}

ZTEST_USER(nvmp_api, test_read_write_erase)
{
	uint8_t *wr = "/a9/9a/a9/9a";
	uint8_t rd[sizeof(wr)];
	int rc = 0;

	memset(rd, 0, sizeof(rd));

	rc = nvmp_open(nvmp);
	zassert_equal(rc, 0, "open returned [%d]", rc);

	TC_PRINT("nvmp props: size %d, block-size %d, write-block-size %d\n",
		 nvmp->size, nvmp->block_size, nvmp->write_block_size);

	rc = nvmp_read(nvmp, 0, rd, sizeof(rd));
	zassert_equal(rc, 0, "read returned [%d]", rc);

	rc = nvmp_write(nvmp, 0, wr, sizeof(wr));
	zassert_equal(rc, 0, "write returned [%d]", rc);

	rc = nvmp_read(nvmp, 0, rd, sizeof(rd));
	zassert_equal(rc, 0, "read returned [%d]", rc);

	zassert_equal(memcmp(wr, rd, sizeof(rd)), 0, "read/write data differ");

	if (nvmp->erase != NULL) {
		rc = nvmp_erase(nvmp, 0, nvmp->block_size);
		zassert_equal(rc, 0, "erase returned [%d]", rc);
	}

	if (nvmp->clear != NULL) {
		rc = nvmp_clear(nvmp, rd, sizeof(rd));
		zassert_equal(rc, 0, "clear returned [%d]", rc);
	}
}

ZTEST_SUITE(nvmp_api, NULL, nvmp_api_setup, NULL, NULL, NULL);

/* Run all of our tests on the given nvmp */
static void run_tests_on_nvmp(const struct nvmp_info *info)
{
	nvmp = info;
	ztest_run_all(NULL, false, 1, 1);
}

void test_main(void)
{
#ifdef CONFIG_NVMP_FLASH
	run_tests_on_nvmp(NVMP_GET(NVMP_FLASH_PARTITION_NODE0));
	run_tests_on_nvmp(NVMP_GET(NVMP_FLASH_PARTITION_NODE1));
#endif

#ifdef CONFIG_NVMP_EEPROM
	run_tests_on_nvmp(NVMP_GET(NVMP_EEPROM_PARTITION_NODE0));
	run_tests_on_nvmp(NVMP_GET(NVMP_EEPROM_PARTITION_NODE1));
#endif
	ztest_verify_all_test_suites_ran();
}
