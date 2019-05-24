/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <ztest.h>

#include "flash.h"
#include "flash_map.h"
#include "stats.h"
#include "nvs/nvs.h"

#define TEST_FLASH_AREA_STORAGE_OFFSET	DT_FLASH_AREA_STORAGE_OFFSET
#define TEST_DATA_ID			1
#define TEST_SECTOR_COUNT		5U

static struct nvs_fs fs;
struct stats_hdr *sim_stats;
struct stats_hdr *sim_thresholds;

void setup(void)
{
	sim_stats = stats_group_find("flash_sim_stats");
	sim_thresholds = stats_group_find("flash_sim_thresholds");
}

void teardown(void)
{
	if (sim_stats) {
		stats_reset(sim_stats);
	}
	if (sim_thresholds) {
		stats_reset(sim_thresholds);
	}
}

void test_nvs_init(void)
{
	int err;
	const struct flash_area *fa;
	struct flash_pages_info info;

	err = flash_area_open(DT_FLASH_AREA_STORAGE_ID, &fa);
	zassert_true(err == 0, "flash_area_open() fail: %d", err);

	fs.offset = TEST_FLASH_AREA_STORAGE_OFFSET;
	err = flash_get_page_info_by_offs(flash_area_get_device(fa), fs.offset,
					  &info);
	zassert_true(err == 0,  "Unable to get page info: %d", err);

	fs.sector_size = info.size;
	fs.sector_count = TEST_SECTOR_COUNT;

	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);
}

void test_nvs_write(void)
{
	int err;
	char rd_buf[512];
	char wr_buf[512];
	char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
	size_t len;

	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	err = nvs_read(&fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(err == -ENOENT,  "nvs_read unexpected failure: %d", err);

	BUILD_ASSERT((sizeof(wr_buf) % sizeof(pattern)) == 0);
	for (int i = 0; i < sizeof(wr_buf); i += sizeof(pattern)) {
		memcpy(wr_buf + i, pattern, sizeof(pattern));
	}

	len = nvs_write(&fs, TEST_DATA_ID, wr_buf, sizeof(wr_buf));
	zassert_true(len == sizeof(wr_buf), "nvs_write failed: %d", len);

	len = nvs_read(&fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf),  "nvs_read unexpected failure: %d",
			len);
	zassert_mem_equal(wr_buf, rd_buf, sizeof(rd_buf),
			"RD buff should be equal to the WR buff");

	err = nvs_clear(&fs);
	zassert_true(err == 0,  "nvs_clear call failure: %d", err);
}

static int flash_sim_write_calls_find(struct stats_hdr *hdr, void *arg,
				      const char *name, uint16_t off)
{
	if (!strcmp(name, "flash_write_calls")) {
		u32_t **flash_write_stat = (u32_t **) arg;
		*flash_write_stat = (u32_t *)((u8_t *)hdr + off);
	}

	return 0;
}

static int flash_sim_max_write_calls_find(struct stats_hdr *hdr, void *arg,
					  const char *name, uint16_t off)
{
	if (!strcmp(name, "max_write_calls")) {
		u32_t **max_write_calls = (u32_t **) arg;
		*max_write_calls = (u32_t *)((u8_t *)hdr + off);
	}

	return 0;
}

void test_nvs_corrupted_write(void)
{
	int err;
	size_t len;
	char rd_buf[512];
	char wr_buf_1[512];
	char wr_buf_2[512];
	char pattern_1[] = {0xDE, 0xAD, 0xBE, 0xEF};
	char pattern_2[] = {0x03, 0xAA, 0x85, 0x6F};
	u32_t *flash_write_stat;
	u32_t *flash_max_write_calls;

	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	err = nvs_read(&fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(err == -ENOENT,  "nvs_read unexpected failure: %d", err);

	BUILD_ASSERT((sizeof(wr_buf_1) % sizeof(pattern_1)) == 0);
	for (int i = 0; i < sizeof(wr_buf_1); i += sizeof(pattern_1)) {
		memcpy(wr_buf_1 + i, pattern_1, sizeof(pattern_1));
	}

	len = nvs_write(&fs, TEST_DATA_ID, wr_buf_1, sizeof(wr_buf_1));
	zassert_true(len == sizeof(wr_buf_1), "nvs_write failed: %d", len);

	len = nvs_read(&fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf),  "nvs_read unexpected failure: %d",
			len);
	zassert_mem_equal(wr_buf_1, rd_buf, sizeof(rd_buf),
			"RD buff should be equal to the first WR buff");

	BUILD_ASSERT((sizeof(wr_buf_2) % sizeof(pattern_2)) == 0);
	for (int i = 0; i < sizeof(wr_buf_2); i += sizeof(pattern_2)) {
		memcpy(wr_buf_2 + i, pattern_2, sizeof(pattern_2));
	}

	/* Set the maximum number of writes that the flash simulator can
	 * execute.
	 */
	stats_walk(sim_thresholds, flash_sim_max_write_calls_find,
		   &flash_max_write_calls);
	stats_walk(sim_stats, flash_sim_write_calls_find, &flash_write_stat);

	*flash_max_write_calls = *flash_write_stat - 1;
	*flash_write_stat = 0;

	/* Flash simulator will lose part of the data at the end of this write.
	 * This should simulate power down during flash write. The written data
	 * are corrupted at this point and should be discarded by the NVS.
	 */
	len = nvs_write(&fs, TEST_DATA_ID, wr_buf_2, sizeof(wr_buf_2));
	zassert_true(len == sizeof(wr_buf_2), "nvs_write failed: %d", len);

	/* Reinitialize the NVS. */
	memset(&fs, 0, sizeof(fs));
	test_nvs_init();

	len = nvs_read(&fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf),  "nvs_read unexpected failure: %d",
			len);
	zassert_true(memcmp(wr_buf_2, rd_buf, sizeof(rd_buf)) != 0,
			"RD buff should not be equal to the second WR buff because of "
			"corrupted write operation");
	zassert_mem_equal(wr_buf_1, rd_buf, sizeof(rd_buf),
			"RD buff should be equal to the first WR buff because subsequent "
			"write operation has failed");

	err = nvs_clear(&fs);
	zassert_true(err == 0,  "nvs_clear call failure: %d", err);
}

void test_nvs_gc(void)
{
	int err;
	int len;
	u8_t buf[32];
	u8_t rd_buf[32];

	const u16_t max_id = 10;
	/* 25th write will trigger GC. */
	const u16_t max_writes = 26;

	fs.sector_count = 2;

	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (u16_t i = 0; i < max_writes; i++) {
		u8_t id = (i % max_id);
		u8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		len = nvs_write(&fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}

	for (u16_t id = 0; id < max_id; id++) {
		len = nvs_read(&fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (u16_t i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				"RD buff should be equal to the WR buff");

	}

	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (u16_t id = 0; id < max_id; id++) {
		len = nvs_read(&fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (u16_t i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				"RD buff should be equal to the WR buff");

	}

	err = nvs_clear(&fs);
	zassert_true(err == 0,  "nvs_clear call failure: %d", err);
}


void test_main(void)
{
	ztest_test_suite(test_nvs,
			 ztest_unit_test_setup_teardown(test_nvs_init, setup,
				 teardown),
			 ztest_unit_test_setup_teardown(test_nvs_write, setup,
				 teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_corrupted_write, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_gc, setup, teardown)
			 );

	ztest_run_test_suite(test_nvs);
}
