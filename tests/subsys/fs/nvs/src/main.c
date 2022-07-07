/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test is designed to be run using flash-simulator which provide
 * functionality for flash property customization and emulating errors in
 * flash operation in parallel to regular flash API.
 * Test should be run on qemu_x86 or native_posix target.
 */

#if !defined(CONFIG_BOARD_QEMU_X86) && !defined(CONFIG_BOARD_NATIVE_POSIX)
#error "Run on qemu_x86 or native_posix only"
#endif

#include <stdio.h>
#include <string.h>
#include <ztest.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/stats/stats.h>
#include <zephyr/sys/crc.h>
#include <zephyr/fs/nvs.h>
#include "nvs_priv.h"

#define TEST_FLASH_AREA_STORAGE_OFFSET	FLASH_AREA_OFFSET(storage)
#define TEST_DATA_ID			1
#define TEST_SECTOR_COUNT		5U

static const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
static struct nvs_fs fs;
struct stats_hdr *sim_stats;
struct stats_hdr *sim_thresholds;

void setup(void)
{
	sim_stats = stats_group_find("flash_sim_stats");
	sim_thresholds = stats_group_find("flash_sim_thresholds");

	/* Verify if NVS is initialized. */
	if (fs.ready) {
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

void test_nvs_mount(void)
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

	fs.sector_size = info.size;
	fs.sector_count = TEST_SECTOR_COUNT;
	fs.flash_device = flash_area_get_device(fa);

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);
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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	execute_long_pattern_write(TEST_DATA_ID);
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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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
	test_nvs_mount();

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
	uint8_t buf[32];
	uint8_t rd_buf[32];

	const uint16_t max_id = 10;
	/* 25th write will trigger GC. */
	const uint16_t max_writes = 26;

	fs.sector_count = 2;

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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
	uint8_t buf[32];
	ssize_t len;

	for (uint16_t i = begin; i < end; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		len = nvs_write(fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}
}

static void check_content(uint16_t max_id, struct nvs_fs *fs)
{
	uint8_t rd_buf[32];
	uint8_t buf[32];
	ssize_t len;

	for (uint16_t id = 0; id < max_id; id++) {
		len = nvs_read(fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (uint16_t i = 0; i < ARRAY_SIZE(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				  "RD buff should be equal to the WR buff");

	}
}

/**
 * Full round of GC over 3 sectors
 */
void test_nvs_gc_3sectors(void)
{
	int err;

	const uint16_t max_id = 10;
	/* 50th write will trigger 1st GC. */
	const uint16_t max_writes = 51;
	/* 75th write will trigger 2st GC. */
	const uint16_t max_writes_2 = 51 + 25;
	/* 100th write will trigger 3st GC. */
	const uint16_t max_writes_3 = 51 + 25 + 25;
	/* 125th write will trigger 4st GC. */
	const uint16_t max_writes_4 = 51 + 25 + 25 + 25;

	fs.sector_count = 3;

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);
	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 0,
		     "unexpected write sector");

	/* Trigger 1st GC */
	write_content(max_id, 0, max_writes, &fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);

	/* Trigger 2nd GC */
	write_content(max_id, max_writes, max_writes_2, &fs);

	/* sector sequence: write, empty, closed */
	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 0,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 0,
		     "unexpected write sector");
	check_content(max_id, &fs);

	/* Trigger 3rd GC */
	write_content(max_id, max_writes_2, max_writes_3, &fs);

	/* sector sequence: closed, write, empty */
	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 1,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 1,
		     "unexpected write sector");
	check_content(max_id, &fs);

	/* Trigger 4th GC */
	write_content(max_id, max_writes_3, max_writes_4, &fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fs);

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fs.ate_wra >> ADDR_SECT_SHIFT, 2,
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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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
	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

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
	uint32_t data;
	ssize_t len;
	int err;

	close_ate.id = 0xffff;
	close_ate.offset = fs.sector_size - sizeof(struct nvs_ate) * 5;
	close_ate.len = 0;
	close_ate.crc8 = 0xff; /* Incorrect crc8 */

	ate.id = 0x1;
	ate.offset = 0;
	ate.len = sizeof(data);
	ate.crc8 = crc8_ccitt(0xff, &ate,
			      offsetof(struct nvs_ate, crc8));

	/* Mark sector 0 as closed */
	err = flash_write(flash_dev, fs.offset + fs.sector_size -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write valid ate at -6 */
	err = flash_write(flash_dev, fs.offset + fs.sector_size -
			  sizeof(struct nvs_ate) * 6, &ate, sizeof(ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write data for previous ate */
	data = 0xaa55aa55;
	err = flash_write(flash_dev, fs.offset, &data, sizeof(data));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(flash_dev, fs.offset + (2 * fs.sector_size) -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	fs.sector_count = 3;

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	data = 0;
	len = nvs_read(&fs, 1, &data, sizeof(data));
	zassert_true(len == sizeof(data),
		     "nvs_read should have read %d bytes", sizeof(data));
	zassert_true(data == 0xaa55aa55, "unexpected value %d", data);
}

/*
 * Test that garbage-collection correctly handles corrupt ate's.
 */
void test_nvs_gc_corrupt_ate(void)
{
	struct nvs_ate corrupt_ate, close_ate;
	int err;

	close_ate.id = 0xffff;
	close_ate.offset = fs.sector_size / 2;
	close_ate.len = 0;
	close_ate.crc8 = crc8_ccitt(0xff, &close_ate,
				    offsetof(struct nvs_ate, crc8));

	corrupt_ate.id = 0xdead;
	corrupt_ate.offset = 0;
	corrupt_ate.len = 20;
	corrupt_ate.crc8 = 0xff; /* Incorrect crc8 */

	/* Mark sector 0 as closed */
	err = flash_write(flash_dev, fs.offset + fs.sector_size -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write a corrupt ate */
	err = flash_write(flash_dev, fs.offset + (fs.sector_size / 2),
			  &corrupt_ate, sizeof(corrupt_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(flash_dev, fs.offset + (2 * fs.sector_size) -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	fs.sector_count = 3;

	err = nvs_mount(&fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);
}

#ifdef CONFIG_NVS_LOOKUP_CACHE
static size_t num_matching_cache_entries(uint32_t addr, bool compare_sector_only)
{
	size_t i, num = 0;
	uint32_t mask = compare_sector_only ? ADDR_SECT_MASK : UINT32_MAX;

	for (i = 0; i < CONFIG_NVS_LOOKUP_CACHE_SIZE; i++) {
		if ((fs.lookup_cache[i] & mask) == addr) {
			num++;
		}
	}

	return num;
}
#endif

/*
 * Test that NVS lookup cache is properly rebuilt on nvs_mount(), or initialized
 * to NVS_LOOKUP_CACHE_NO_ADDR if the store is empty.
 */
void test_nvs_cache_init(void)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	int err;
	size_t num;
	uint32_t ate_addr;
	uint8_t data = 0;

	/* Test cache initialization when the store is empty */

	fs.sector_count = 3;
	err = nvs_mount(&fs);
	zassert_true(err == 0, "nvs_init call failure: %d", err);

	num = num_matching_cache_entries(NVS_LOOKUP_CACHE_NO_ADDR, false);
	zassert_equal(num, CONFIG_NVS_LOOKUP_CACHE_SIZE, "uninitialized cache");

	/* Test cache update after nvs_write() */

	ate_addr = fs.ate_wra;
	err = nvs_write(&fs, 1, &data, sizeof(data));
	zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);

	num = num_matching_cache_entries(NVS_LOOKUP_CACHE_NO_ADDR, false);
	zassert_equal(num, CONFIG_NVS_LOOKUP_CACHE_SIZE - 1, "cache not updated after write");

	num = num_matching_cache_entries(ate_addr, false);
	zassert_equal(num, 1, "invalid cache entry after write");

	/* Test cache initialization when the store is non-empty */

	memset(fs.lookup_cache, 0xAA, sizeof(fs.lookup_cache));
	err = nvs_mount(&fs);
	zassert_true(err == 0, "nvs_init call failure: %d", err);

	num = num_matching_cache_entries(NVS_LOOKUP_CACHE_NO_ADDR, false);
	zassert_equal(num, CONFIG_NVS_LOOKUP_CACHE_SIZE - 1, "uninitialized cache after restart");

	num = num_matching_cache_entries(ate_addr, false);
	zassert_equal(num, 1, "invalid cache entry after restart");
#endif
}

/*
 * Test that even after writing more NVS IDs than the number of NVS lookup cache
 * entries they all can be read correctly.
 */
void test_nvs_cache_collission(void)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	int err;
	uint16_t id;
	uint16_t data;

	fs.sector_count = 3;
	err = nvs_mount(&fs);
	zassert_true(err == 0, "nvs_init call failure: %d", err);

	for (id = 0; id < CONFIG_NVS_LOOKUP_CACHE_SIZE + 1; id++) {
		data = id;
		err = nvs_write(&fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	for (id = 0; id < CONFIG_NVS_LOOKUP_CACHE_SIZE + 1; id++) {
		err = nvs_read(&fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_read call failure: %d", err);
		zassert_equal(data, id, "incorrect data read");
	}
#endif
}

/*
 * Test that NVS lookup cache does not contain any address from gc-ed sector
 */
void test_nvs_cache_gc(void)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	int err;
	size_t num;
	uint16_t data = 0;

	fs.sector_count = 3;
	err = nvs_mount(&fs);
	zassert_true(err == 0, "nvs_init call failure: %d", err);

	/* Fill the first sector with writes of ID 1 */

	while (fs.data_wra + sizeof(data) <= fs.ate_wra) {
		++data;
		err = nvs_write(&fs, 1, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	/* Verify that cache contains a single entry for sector 0 */

	num = num_matching_cache_entries(0 << ADDR_SECT_SHIFT, true);
	zassert_equal(num, 1, "invalid cache content after filling sector 0");

	/* Fill the second sector with writes of ID 2 */

	while ((fs.ate_wra >> ADDR_SECT_SHIFT) != 2) {
		++data;
		err = nvs_write(&fs, 2, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	/*
	 * At this point sector 0 should have been gc-ed. Verify that action is
	 * reflected by the cache content.
	 */

	num = num_matching_cache_entries(0 << ADDR_SECT_SHIFT, true);
	zassert_equal(num, 0, "not invalidated cache entries aftetr gc");

	num = num_matching_cache_entries(2 << ADDR_SECT_SHIFT, true);
	zassert_equal(num, 2, "invalid cache content after gc");
#endif
}

void test_main(void)
{
	__ASSERT_NO_MSG(device_is_ready(flash_dev));

	ztest_test_suite(test_nvs,
			 ztest_unit_test_setup_teardown(test_nvs_mount, setup,
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
				 test_nvs_cache_init, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_cache_collission, setup, teardown),
			 ztest_unit_test_setup_teardown(
				 test_nvs_cache_gc, setup, teardown)
			);

	ztest_run_test_suite(test_nvs);
}
