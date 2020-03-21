/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test is designed to be run using flash-simulator which provide
 * functionality for flash property customization and emulating errors in
 * flash opperation in parallel to regular flash API.
 * Test should be run on qemu_x86 target.
 */

#ifndef CONFIG_BOARD_QEMU_X86
#error "Run on qemu_x86 only"
#endif

#include <stdio.h>
#include <string.h>
#include <ztest.h>

#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <stats/stats.h>
#include <sys/crc.h>
#include <fs/nvs.h>
#include "nvs_priv.h"

#define TEST_FLASH_AREA_STORAGE_OFFSET	FLASH_AREA_OFFSET(storage)
#define TEST_DATA_ID			1
#define TEST_SECTOR_COUNT		5U
#define TEST_GC_BUF_LENGTH		32U

static struct nvs_fs fs;
struct stats_hdr *sim_stats;
struct stats_hdr *sim_thresholds;

void setup(void)
{
	sim_stats = stats_group_find("flash_sim_stats");
	sim_thresholds = stats_group_find("flash_sim_thresholds");

	/* Verify if NVS is initialized. */
	if (fs.sector_count != 0) {
		int err;

		err = nvs_clear(&fs);
		zassert_true(err == 0,  "nvs_clear call failure: %d", err);
	}
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

static void init(uint8_t blocks_per_sector)
{
	int err;
	const struct flash_area *fa;
	struct flash_pages_info info;

	err = flash_area_open(FLASH_AREA_ID(storage), &fa);
	zassert_true(err == 0, "flash_area_open() fail: %d", err);

	fs.offset = TEST_FLASH_AREA_STORAGE_OFFSET;
	err = flash_get_page_info_by_offs(flash_area_get_device(fa), fs.offset,
					  &info);
	zassert_true(err == 0,  "Unable to get page info: %d", err);

	fs.sector_size = blocks_per_sector * info.size;
	fs.sector_count = TEST_SECTOR_COUNT;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);
}

void test_nvs_init(void)
{
	init(3);
}

void test_nvs_init_large_sectors(void)
{
#if NVS_OFFSET_BITS > 16
	init(65);
#else
	ztest_test_skip();
#endif
}

/* return size aligned to fs->write_block_size */
static inline size_t al_size(struct nvs_fs *fs, size_t len)
{
	uint8_t write_block_size = fs->flash_parameters->write_block_size;

	if (write_block_size <= 1U) {
		return len;
	}
	return (len + (write_block_size - 1U)) & ~(write_block_size - 1U);
}

static void execute_long_pattern_write(uint16_t id)
{
	char rd_buf[512];
	char wr_buf[512];
	char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
	size_t len;

	len = nvs_read(&fs, id, rd_buf, sizeof(rd_buf));
	zassert_true(len == -ENOENT,  "nvs_read unexpected failure: %d", len);

	BUILD_ASSERT((sizeof(wr_buf) % sizeof(pattern)) == 0);
	for (int i = 0; i < sizeof(wr_buf); i += sizeof(pattern)) {
		memcpy(wr_buf + i, pattern, sizeof(pattern));
	}

	len = nvs_write(&fs, id, wr_buf, sizeof(wr_buf));
	zassert_true(len == sizeof(wr_buf), "nvs_write failed: %d", len);

	len = nvs_read(&fs, id, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf),  "nvs_read unexpected failure: %d",
			len);
	zassert_mem_equal(wr_buf, rd_buf, sizeof(rd_buf),
			"RD buff should be equal to the WR buff");
}

void test_nvs_write(void)
{
	int err;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	execute_long_pattern_write(TEST_DATA_ID);
}

void test_nvs_write_large_sectors(void)
{
#if NVS_OFFSET_BITS > 16
	int err;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (uint32_t i = 0; i < (TEST_SECTOR_COUNT - 1) * 125; ++i) {
		execute_long_pattern_write(i + 27014);
	}
#else
	ztest_test_skip();
#endif
}

static int flash_sim_write_calls_find(struct stats_hdr *hdr, void *arg,
				      const char *name, uint16_t off)
{
	if (!strcmp(name, "flash_write_calls")) {
		uint32_t **flash_write_stat = (uint32_t **) arg;
		*flash_write_stat = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

static int flash_sim_max_write_calls_find(struct stats_hdr *hdr, void *arg,
					  const char *name, uint16_t off)
{
	if (!strcmp(name, "max_write_calls")) {
		uint32_t **max_write_calls = (uint32_t **) arg;
		*max_write_calls = (uint32_t *)((uint8_t *)hdr + off);
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
	uint32_t *flash_write_stat;
	uint32_t *flash_max_write_calls;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
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
}

void test_nvs_gc(void)
{
	int err;
	int len;
	uint8_t buf[TEST_GC_BUF_LENGTH];
	uint8_t rd_buf[TEST_GC_BUF_LENGTH];

	const uint16_t max_id = 10;
	/* 25th write will trigger GC. */
	const uint16_t max_writes = 26;

	fs.sector_count = 2;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (uint16_t i = 0; i < max_writes; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		len = nvs_write(&fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}

	for (uint16_t id = 0; id < max_id; id++) {
		len = nvs_read(&fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (uint16_t i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				"RD buff should be equal to the WR buff");

	}

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (uint16_t id = 0; id < max_id; id++) {
		len = nvs_read(&fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (uint16_t i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				"RD buff should be equal to the WR buff");

	}
}

static void write_content(uint16_t max_id, uint16_t begin, uint16_t end,
			     struct nvs_fs *fs)
{
	uint8_t buf[TEST_GC_BUF_LENGTH];
	ssize_t len;

	for (uint16_t i = begin; i < end; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		zassert_equal(id_data % max_id, id,
			      "Generating pattern failed\n"
			      "id: 0x%x, id_data: 0x%08x, max_id: 0x%x\n"
			       "i: %d, begin: %d, end: %d",
			      id, id_data, max_id, i, begin, end);

		memset(buf, id_data, sizeof(buf));

		len = nvs_write(fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}
}

static void check_content(uint16_t max_id, struct nvs_fs *fs)
{
	uint8_t rd_buf[TEST_GC_BUF_LENGTH];
	uint8_t buf[TEST_GC_BUF_LENGTH];
	ssize_t len;

	for (uint16_t id = 0; id < max_id; id++) {
		len = nvs_read(fs, id, rd_buf, sizeof(rd_buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (uint16_t i = 0; i < ARRAY_SIZE(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				  "RD buff should be equal to the WR buff\n"
				  "id: %d, wra: 0x%x\n"
				  "expected: 0x%08x, effective: 0x%08x",
				  id, fs->data_wra,
				  ((uint32_t *)buf)[0], ((uint32_t *)rd_buf)[0]);
	}
}

/**
 * Full round of GC over 3 sectors
 */
void test_nvs_gc_3sectors(void)
{
	int err;

	/* Must be power of two or pattern generation will fail */
	const uint16_t max_id = 8;
	/* Number of writes to fill a sector completely */
	const uint16_t sector_writes = fs.sector_size /
				    (sizeof(struct nvs_ate) +
				     TEST_GC_BUF_LENGTH);
	/* Number of writes to trigger 1st GC. */
	const uint16_t max_writes = 2 * sector_writes + 1;
	/* Number of writes to trigger 2nd GC. */
	const uint16_t max_writes_2 = 3 * sector_writes + 1;
	/* Number of writes to trigger 3rd GC. */
	const uint16_t max_writes_3 = 4 * sector_writes + 1;
	/* Number of writes to trigger 4th GC. */
	const uint16_t max_writes_4 = 5 * sector_writes + 1;

	fs.sector_count = 3;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);
	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 0,
		     "unexpected write sector");

	/* Trigger 1st GC */
	write_content(max_id, 0, max_writes, &fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);

	/* Trigger 2nd GC */
	write_content(max_id, max_writes, max_writes_2, &fs);

	/* sector sequence: write, empty, closed */
	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 0,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 0,
		     "unexpected write sector");
	check_content(max_id, &fs);

	/* Trigger 3rd GC */
	write_content(max_id, max_writes_2, max_writes_3, &fs);

	/* sector sequence: closed, write, empty */
	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 1,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 1,
		     "unexpected write sector");
	check_content(max_id, &fs);

	/* Trigger 4th GC */
	write_content(max_id, max_writes_3, max_writes_4, &fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	zassert_equal(fs.ate_wra >> NVS_OFFSET_BITS, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);
}

static int flash_sim_erase_calls_find(struct stats_hdr *hdr, void *arg,
				      const char *name, uint16_t off)
{
	if (!strcmp(name, "flash_erase_calls")) {
		uint32_t **flash_erase_stat = (uint32_t **) arg;
		*flash_erase_stat = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

static int flash_sim_max_erase_calls_find(struct stats_hdr *hdr, void *arg,
					  const char *name, uint16_t off)
{
	if (!strcmp(name, "max_erase_calls")) {
		uint32_t **max_erase_calls = (uint32_t **) arg;
		*max_erase_calls = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

static int flash_sim_max_len_find(struct stats_hdr *hdr, void *arg,
				  const char *name, uint16_t off)
{
	if (!strcmp(name, "max_len")) {
		uint32_t **max_len = (uint32_t **) arg;
		*max_len = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

void test_nvs_corrupted_sector_close_operation(void)
{
	int err;
	int len;
	uint8_t buf[32];
	uint32_t *flash_write_stat;
	uint32_t *flash_erase_stat;
	uint32_t *flash_max_write_calls;
	uint32_t *flash_max_erase_calls;
	uint32_t *flash_max_len;

	const uint16_t max_id = 10;
	/* 25th write will trigger GC. */
	const uint16_t max_writes = 26;

	/* Get the address of simulator parameters. */
	stats_walk(sim_thresholds, flash_sim_max_write_calls_find,
		   &flash_max_write_calls);
	stats_walk(sim_thresholds, flash_sim_max_erase_calls_find,
		   &flash_max_erase_calls);
	stats_walk(sim_thresholds, flash_sim_max_len_find,
		   &flash_max_len);
	stats_walk(sim_stats, flash_sim_write_calls_find, &flash_write_stat);
	stats_walk(sim_stats, flash_sim_erase_calls_find, &flash_erase_stat);

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (uint16_t i = 0; i < max_writes; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		if (i == max_writes - 1) {
			/* Reset stats. */
			*flash_write_stat = 0;
			*flash_erase_stat = 0;

			/* Block write calls and simulate power down during
			 * sector closing operation, so only a part of a NVS
			 * closing ate will be written.
			 */
			*flash_max_write_calls = 1;
			*flash_max_erase_calls = 1;
			*flash_max_len = 4;
		}

		len = nvs_write(&fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}

	/* Make the flash simulator functional again. */
	*flash_max_write_calls = 0;
	*flash_max_erase_calls = 0;
	*flash_max_len = 0;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	check_content(max_id, &fs);

	/* Ensure that the NVS is able to store new content. */
	execute_long_pattern_write(max_id);
}

/**
 * @brief Test case when storage become full, so only deletion is possible.
 */
void test_nvs_full_sector(void)
{
	int err;
	ssize_t len;
	uint16_t filling_id = 0;
	uint16_t i, data_read;

	fs.sector_count = 3;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	while (1) {
		len = nvs_write(&fs, filling_id, &filling_id,
				sizeof(filling_id));
		if (len == -ENOSPC) {
			break;
		}
		zassert_true(len == sizeof(filling_id), "nvs_write failed: %d",
			     len);
		filling_id++;
	}

	/* check whether can delete whatever from full storage */
	err = nvs_delete(&fs, 1);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);

	/* the last sector is full now, test re-initialization */
	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	len = nvs_write(&fs, filling_id, &filling_id, sizeof(filling_id));
	zassert_true(len == sizeof(filling_id), "nvs_write failed: %d", len);

	/* sanitycheck on NVS content */
	for (i = 0; i <= filling_id; i++) {
		len = nvs_read(&fs, i, &data_read, sizeof(data_read));
		if (i == 1) {
			zassert_true(len == -ENOENT,
				     "nvs_read shouldn't found the entry: %d",
				     len);
		} else {
			zassert_true(len == sizeof(data_read),
				     "nvs_read failed: %d", i, len);
			zassert_equal(data_read, i,
				      "read unexpected data: %d instead of %d",
				      data_read, i);
		}
	}
}

void test_delete(void)
{
	int err;
	ssize_t len;
	uint16_t filling_id, data_read;
	uint32_t ate_wra, data_wra;

	fs.sector_count = 3;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	for (filling_id = 0; filling_id < 10; filling_id++) {
		len = nvs_write(&fs, filling_id, &filling_id,
				sizeof(filling_id));

		zassert_true(len == sizeof(filling_id), "nvs_write failed: %d",
			     len);

		if (filling_id != 0) {
			continue;
		}

		/* delete the first entry while it is the most recent one */
		err = nvs_delete(&fs, filling_id);
		zassert_true(err == 0,  "nvs_delete call failure: %d", err);

		len = nvs_read(&fs, filling_id, &data_read, sizeof(data_read));
		zassert_true(len == -ENOENT,
			     "nvs_read shouldn't found the entry: %d", len);
	}

	/* delete existing entry */
	err = nvs_delete(&fs, 1);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);

	len = nvs_read(&fs, 1, &data_read, sizeof(data_read));
	zassert_true(len == -ENOENT, "nvs_read shouldn't found the entry: %d",
		     len);

	ate_wra = fs.ate_wra;
	data_wra = fs.data_wra;

	/* delete already deleted entry */
	err = nvs_delete(&fs, 1);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);
	zassert_true(ate_wra == fs.ate_wra && data_wra == fs.data_wra,
		     "delete already deleted entry should not make"
		     " any footprint in the storage");

	/* delete nonexisting entry */
	err = nvs_delete(&fs, filling_id);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);
	zassert_true(ate_wra == fs.ate_wra && data_wra == fs.data_wra,
		     "delete nonexistent entry should not make"
		     " any footprint in the storage");
}

/*
 * Test that garbage-collection can recover all ate's even when the last ate,
 * ie close_ate, is corrupt. In this test the close_ate is set to point to the
 * last ate at -5. A valid ate is however present at -6. Since the close_ate
 * has an invalid crc8, the offset should not be used and a recover of the
 * last ate should be done instead.
 */
void test_nvs_gc_corrupt_close_ate(void)
{
	struct nvs_ate ate, close_ate;
	struct device *flash_dev;
	uint32_t data;
	ssize_t len;
	int err;
	uint8_t ate_size;
	uint8_t data_size;

	/* align data and ate size to write block size of flash */
	ate_size = al_size(&fs, sizeof(struct nvs_ate));
	data_size = al_size(&fs, sizeof(data));

	flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(flash_dev != NULL, "device_get_binding failure");

	close_ate.id = 0xffff;
	close_ate.offset = fs.sector_size - ate_size * 5;
	close_ate.len = 0;
	close_ate.crc8 = 0xff; /* Incorrect crc8 */

	ate.id = 0x1;
	ate.offset = 0;
	ate.len = data_size;
	ate.crc8 = crc8_ccitt(0xff, &ate,
			      offsetof(struct nvs_ate, crc8));

	flash_write_protection_set(flash_dev, false);

	/* Mark sector 0 as closed */
	err = flash_write(flash_dev, fs.offset + fs.sector_size -
			  ate_size, &close_ate, ate_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write valid ate at -6 */
	err = flash_write(flash_dev, fs.offset + fs.sector_size -
			  ate_size * 6, &ate, ate_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write data for previous ate */
	data = 0xaa55aa55;
	err = flash_write(flash_dev, fs.offset, &data, data_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(flash_dev, fs.offset + (2 * fs.sector_size) -
			  ate_size, &close_ate, ate_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	flash_write_protection_set(flash_dev, true);

	fs.sector_count = 3;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);

	data = 0;
	len = nvs_read(&fs, 1, &data, data_size);
	zassert_true(len == data_size,
		     "nvs_read should have read %d bytes", data_size);
	zassert_true(data == 0xaa55aa55, "unexpected value %d", data);
}

/*
 * Test that garbage-collection correctly handles corrupt ate's.
 */
void test_nvs_gc_corrupt_ate(void)
{
	struct nvs_ate corrupt_ate, close_ate;
	struct device *flash_dev;
	int err;
	uint8_t ate_size;

	/* align ate size to write block size of flash */
	ate_size = al_size(&fs, sizeof(struct nvs_ate));

	flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(flash_dev != NULL,  "device_get_binding failure");

	close_ate.id = 0xffff;
	close_ate.offset = fs.sector_size / 2;
	close_ate.len = 0;
	close_ate.crc8 = crc8_ccitt(0xff, &close_ate,
				    offsetof(struct nvs_ate, crc8));

	corrupt_ate.id = 0xdead;
	corrupt_ate.offset = 0;
	corrupt_ate.len = 20;
	corrupt_ate.crc8 = 0xff; /* Incorrect crc8 */

	flash_write_protection_set(flash_dev, false);

	/* Mark sector 0 as closed */
	err = flash_write(flash_dev, fs.offset + fs.sector_size -
			  ate_size, &close_ate, ate_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write a corrupt ate */
	err = flash_write(flash_dev, fs.offset + (fs.sector_size / 2),
			  &corrupt_ate, ate_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(flash_dev, fs.offset + (2 * fs.sector_size) -
			  ate_size, &close_ate, ate_size);
	zassert_true(err == 0,  "flash_write failed: %d", err);

	flash_write_protection_set(flash_dev, true);

	fs.sector_count = 3;

	err = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	zassert_true(err == 0,  "nvs_init call failure: %d", err);
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
				 test_nvs_gc, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_gc_3sectors, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_corrupted_sector_close_operation,
				 setup, teardown),
			 ztest_unit_test_setup_teardown(test_nvs_full_sector,
				 setup, teardown),
			 ztest_unit_test_setup_teardown(test_delete, setup,
				 teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_gc_corrupt_close_ate, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_gc_corrupt_ate, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_init_large_sectors, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_write_large_sectors, setup, teardown)
			);

	ztest_run_test_suite(test_nvs);
}
