/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kvss/nvs.h>
#include <zephyr/stats/stats.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/crc.h>
#include "nvs_priv.h"

#define TEST_NVS_FLASH_AREA		storage_partition
#define TEST_NVS_FLASH_AREA_OFFSET	FIXED_PARTITION_OFFSET(TEST_NVS_FLASH_AREA)
#define TEST_NVS_FLASH_AREA_ID		FIXED_PARTITION_ID(TEST_NVS_FLASH_AREA)
#define TEST_NVS_FLASH_AREA_DEV \
	DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(TEST_NVS_FLASH_AREA)))
#define TEST_DATA_ID			1
#define TEST_SECTOR_COUNT		5U

static const struct device *const flash_dev = TEST_NVS_FLASH_AREA_DEV;

struct nvs_fixture {
	struct nvs_fs fs;
#ifdef CONFIG_TEST_NVS_SIMULATOR
	struct stats_hdr *sim_stats;
	struct stats_hdr *sim_thresholds;
#endif /* CONFIG_TEST_NVS_SIMULATOR */
};

static void *setup(void)
{
	int err;
	const struct flash_area *fa;
	struct flash_pages_info info;
	static struct nvs_fixture fixture;

	__ASSERT_NO_MSG(device_is_ready(flash_dev));

	err = flash_area_open(TEST_NVS_FLASH_AREA_ID, &fa);
	zassert_true(err == 0, "flash_area_open() fail: %d", err);

	fixture.fs.offset = TEST_NVS_FLASH_AREA_OFFSET;
	err = flash_get_page_info_by_offs(flash_area_get_device(fa), fixture.fs.offset,
					  &info);
	zassert_true(err == 0,  "Unable to get page info: %d", err);

	fixture.fs.sector_size = info.size;
	fixture.fs.sector_count = TEST_SECTOR_COUNT;
	fixture.fs.flash_device = flash_area_get_device(fa);

	return &fixture;
}

static void before(void *data)
{
#ifdef CONFIG_TEST_NVS_SIMULATOR
	struct nvs_fixture *fixture = (struct nvs_fixture *)data;

	fixture->sim_stats = stats_group_find("flash_sim_stats");
	fixture->sim_thresholds = stats_group_find("flash_sim_thresholds");
#endif /* CONFIG_TEST_NVS_SIMULATOR */
}

static void after(void *data)
{
	struct nvs_fixture *fixture = (struct nvs_fixture *)data;

#ifdef CONFIG_TEST_NVS_SIMULATOR
	if (fixture->sim_stats) {
		stats_reset(fixture->sim_stats);
	}
	if (fixture->sim_thresholds) {
		stats_reset(fixture->sim_thresholds);
	}
#endif /* CONFIG_TEST_NVS_SIMULATOR */

	/* Clear NVS */
	if (fixture->fs.ready) {
		int err;

		err = nvs_clear(&fixture->fs);
		zassert_true(err == 0, "nvs_clear call failure: %d", err);
	}

	fixture->fs.sector_count = TEST_SECTOR_COUNT;
}

ZTEST_SUITE(nvs, NULL, setup, before, after, NULL);

ZTEST_F(nvs, test_nvs_mount)
{
	int err;
	size_t orig_sector_size = fixture->fs.sector_size;

	fixture->fs.sector_size = KB(128);
	err = nvs_mount(&fixture->fs);
	zassert_true(err == -EINVAL,
		     "nvs_mount did not return expected err for invalid sector_size");

	fixture->fs.sector_size = orig_sector_size;
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);
}

static void execute_long_pattern_write(uint16_t id, struct nvs_fs *fs)
{
	char rd_buf[512];
	char wr_buf[512];
	char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
	size_t len;

	len = nvs_read(fs, id, rd_buf, sizeof(rd_buf));
	zassert_true(len == -ENOENT,  "nvs_read unexpected failure: %d", len);

	BUILD_ASSERT((sizeof(wr_buf) % sizeof(pattern)) == 0);
	for (int i = 0; i < sizeof(wr_buf); i += sizeof(pattern)) {
		memcpy(wr_buf + i, pattern, sizeof(pattern));
	}

	len = nvs_write(fs, id, wr_buf, sizeof(wr_buf));
	zassert_true(len == sizeof(wr_buf), "nvs_write failed: %d", len);

	len = nvs_read(fs, id, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf),  "nvs_read unexpected failure: %d",
			len);
	zassert_mem_equal(wr_buf, rd_buf, sizeof(rd_buf),
			"RD buff should be equal to the WR buff");
}

ZTEST_F(nvs, test_nvs_write)
{
	int err;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	execute_long_pattern_write(TEST_DATA_ID, &fixture->fs);
}

#ifdef CONFIG_TEST_NVS_SIMULATOR
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

ZTEST_F(nvs, test_nvs_corrupted_write)
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

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	err = nvs_read(&fixture->fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(err == -ENOENT,  "nvs_read unexpected failure: %d", err);

	BUILD_ASSERT((sizeof(wr_buf_1) % sizeof(pattern_1)) == 0);
	for (int i = 0; i < sizeof(wr_buf_1); i += sizeof(pattern_1)) {
		memcpy(wr_buf_1 + i, pattern_1, sizeof(pattern_1));
	}

	len = nvs_write(&fixture->fs, TEST_DATA_ID, wr_buf_1, sizeof(wr_buf_1));
	zassert_true(len == sizeof(wr_buf_1), "nvs_write failed: %d", len);

	len = nvs_read(&fixture->fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
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
	stats_walk(fixture->sim_thresholds, flash_sim_max_write_calls_find,
		   &flash_max_write_calls);
	stats_walk(fixture->sim_stats, flash_sim_write_calls_find, &flash_write_stat);

#if defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
	*flash_max_write_calls = *flash_write_stat - 1;
#else
	/* When there is no explicit erase, erase is done with write, which means
	 * that there are more writes needed. The nvs_write here will cause erase
	 * to be called, which in turn calls the flash_fill; flash_fill will
	 * overwrite data using buffer of size CONFIG_FLASH_FILL_BUFFER_SIZE,
	 * and then two additional real writes are allowed.
	 */
	*flash_max_write_calls = (fixture->fs.sector_size /
				  CONFIG_FLASH_FILL_BUFFER_SIZE) + 2;
#endif
	*flash_write_stat = 0;

	/* Flash simulator will lose part of the data at the end of this write.
	 * This should simulate power down during flash write. The written data
	 * are corrupted at this point and should be discarded by the NVS.
	 */
	len = nvs_write(&fixture->fs, TEST_DATA_ID, wr_buf_2, sizeof(wr_buf_2));
	zassert_true(len == sizeof(wr_buf_2), "nvs_write failed: %d", len);

	/* Reinitialize the NVS. */
	memset(&fixture->fs, 0, sizeof(fixture->fs));
	(void)setup();
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	len = nvs_read(&fixture->fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf),  "nvs_read unexpected failure: %d",
			len);
	zassert_true(memcmp(wr_buf_2, rd_buf, sizeof(rd_buf)) != 0,
			"RD buff should not be equal to the second WR buff because of "
			"corrupted write operation");
	zassert_mem_equal(wr_buf_1, rd_buf, sizeof(rd_buf),
			"RD buff should be equal to the first WR buff because subsequent "
			"write operation has failed");
}

ZTEST_F(nvs, test_nvs_gc)
{
	int err;
	int len;
	uint8_t buf[32];
	uint8_t rd_buf[32];

	const uint16_t max_id = 10;
	/* 25th write will trigger GC. */
	const uint16_t max_writes = 26;

	fixture->fs.sector_count = 2;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	for (uint16_t i = 0; i < max_writes; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		len = nvs_write(&fixture->fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}

	for (uint16_t id = 0; id < max_id; id++) {
		len = nvs_read(&fixture->fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf),
			     "nvs_read unexpected failure: %d", len);

		for (uint16_t i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				"RD buff should be equal to the WR buff");

	}

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	for (uint16_t id = 0; id < max_id; id++) {
		len = nvs_read(&fixture->fs, id, rd_buf, sizeof(buf));
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
		uint8_t id_data = id + max_id * ((i % 256) / max_id);

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
ZTEST_F(nvs, test_nvs_gc_3sectors)
{
	int err;

	const uint16_t max_id = 10;
	/* 50th write will trigger 1st GC. */
	uint16_t max_writes = 51;
	/* 75th write will trigger 2st GC. */
	uint16_t max_writes_2 = 51 + 25;
	/* 100th write will trigger 3st GC. */
	uint16_t max_writes_3 = 51 + 25 + 25;
	/* 125th write will trigger 4st GC. */
	uint16_t max_writes_4 = 51 + 25 + 25 + 25;

	if (fixture->fs.sector_size == KB(64)) {
		/* write 1637 will trigger 1st GC. */
		/* write 3274 will trigger 2nd GC. */
		max_writes = 3275;
		/* write 4911 will trigger 3rd GC. */
		max_writes_2 = 3275 + 1637;
		/* write 6548 will trigger 4th GC. */
		max_writes_3 = 3275 + 1637 + 1637;
		/* write 8185 will trigger 5th GC. */
		max_writes_4 = 3275 + 1637 + 1637 + 1637;
	}

	fixture->fs.sector_count = 3;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 0,
		     "unexpected write sector");

	/* Trigger 1st and 2nd GC */
	write_content(max_id, 0, max_writes, &fixture->fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	/* Trigger 3rd GC */
	write_content(max_id, max_writes, max_writes_2, &fixture->fs);

	/* sector sequence: write, empty, closed */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 0,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 0,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	/* Trigger 4th GC */
	write_content(max_id, max_writes_2, max_writes_3, &fixture->fs);

	/* sector sequence: closed, write, empty */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 1,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 1,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	/* Trigger 5th GC */
	write_content(max_id, max_writes_3, max_writes_4, &fixture->fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2,
		     "unexpected write sector");
	check_content(max_id, &fixture->fs);
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

ZTEST_F(nvs, test_nvs_corrupted_sector_close_operation)
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
	stats_walk(fixture->sim_thresholds, flash_sim_max_write_calls_find,
		   &flash_max_write_calls);
	stats_walk(fixture->sim_thresholds, flash_sim_max_erase_calls_find,
		   &flash_max_erase_calls);
	stats_walk(fixture->sim_thresholds, flash_sim_max_len_find,
		   &flash_max_len);
	stats_walk(fixture->sim_stats, flash_sim_write_calls_find, &flash_write_stat);
	stats_walk(fixture->sim_stats, flash_sim_erase_calls_find, &flash_erase_stat);

	err = nvs_mount(&fixture->fs);
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

		len = nvs_write(&fixture->fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "nvs_write failed: %d", len);
	}

	/* Make the flash simulator functional again. */
	*flash_max_write_calls = 0;
	*flash_max_erase_calls = 0;
	*flash_max_len = 0;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	check_content(max_id, &fixture->fs);

	/* Ensure that the NVS is able to store new content. */
	execute_long_pattern_write(max_id, &fixture->fs);
}
#endif /* CONFIG_TEST_NVS_SIMULATOR */

/**
 * @brief Test case when storage become full, so only deletion is possible.
 */
ZTEST_F(nvs, test_nvs_full_sector)
{
	int err;
	ssize_t len;
	uint16_t filling_id = 0;
	uint16_t i, data_read;

	fixture->fs.sector_count = 3;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	while (1) {
		len = nvs_write(&fixture->fs, filling_id, &filling_id,
				sizeof(filling_id));
		if (len == -ENOSPC) {
			break;
		}
		zassert_true(len == sizeof(filling_id), "nvs_write failed: %d",
			     len);
		filling_id++;
	}

	/* check whether can delete whatever from full storage */
	err = nvs_delete(&fixture->fs, 1);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);

	/* the last sector is full now, test re-initialization */
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	len = nvs_write(&fixture->fs, filling_id, &filling_id, sizeof(filling_id));
	zassert_true(len == sizeof(filling_id), "nvs_write failed: %d", len);

	/* coherence check on NVS content */
	for (i = 0; i <= filling_id; i++) {
		len = nvs_read(&fixture->fs, i, &data_read, sizeof(data_read));
		if (i == 1) {
			zassert_true(len == -ENOENT,
				     "nvs_read shouldn't found the entry: %d",
				     len);
		} else {
			zassert_true(len == sizeof(data_read),
				     "nvs_read #%d failed: len is %zd instead of %zu",
				     i, len, sizeof(data_read));
			zassert_equal(data_read, i,
				      "read unexpected data: %d instead of %d",
				      data_read, i);
		}
	}
}

ZTEST_F(nvs, test_delete)
{
	int err;
	ssize_t len;
	uint16_t filling_id, data_read;
	uint32_t ate_wra, data_wra;

	fixture->fs.sector_count = 3;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	for (filling_id = 0; filling_id < 10; filling_id++) {
		len = nvs_write(&fixture->fs, filling_id, &filling_id,
				sizeof(filling_id));

		zassert_true(len == sizeof(filling_id), "nvs_write failed: %d",
			     len);

		if (filling_id != 0) {
			continue;
		}

		/* delete the first entry while it is the most recent one */
		err = nvs_delete(&fixture->fs, filling_id);
		zassert_true(err == 0,  "nvs_delete call failure: %d", err);

		len = nvs_read(&fixture->fs, filling_id, &data_read, sizeof(data_read));
		zassert_true(len == -ENOENT,
			     "nvs_read shouldn't found the entry: %d", len);
	}

	/* delete existing entry */
	err = nvs_delete(&fixture->fs, 1);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);

	len = nvs_read(&fixture->fs, 1, &data_read, sizeof(data_read));
	zassert_true(len == -ENOENT, "nvs_read shouldn't found the entry: %d",
		     len);

	ate_wra = fixture->fs.ate_wra;
	data_wra = fixture->fs.data_wra;

	/* delete already deleted entry */
	err = nvs_delete(&fixture->fs, 1);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);
	zassert_true(ate_wra == fixture->fs.ate_wra && data_wra == fixture->fs.data_wra,
		     "delete already deleted entry should not make"
		     " any footprint in the storage");

	/* delete nonexisting entry */
	err = nvs_delete(&fixture->fs, filling_id);
	zassert_true(err == 0,  "nvs_delete call failure: %d", err);
	zassert_true(ate_wra == fixture->fs.ate_wra && data_wra == fixture->fs.data_wra,
		     "delete nonexistent entry should not make"
		     " any footprint in the storage");
}

#ifdef CONFIG_TEST_NVS_SIMULATOR
/*
 * Test that garbage-collection can recover all ate's even when the last ate,
 * ie close_ate, is corrupt. In this test the close_ate is set to point to the
 * last ate at -5. A valid ate is however present at -6. Since the close_ate
 * has an invalid crc8, the offset should not be used and a recover of the
 * last ate should be done instead.
 */
ZTEST_F(nvs, test_nvs_gc_corrupt_close_ate)
{
	struct nvs_ate ate, close_ate;
	uint32_t data;
	ssize_t len;
	int err;
#ifdef CONFIG_NVS_DATA_CRC
	uint32_t data_crc;
#endif

	close_ate.id = 0xffff;
	close_ate.offset = fixture->fs.sector_size - sizeof(struct nvs_ate) * 5;
	close_ate.len = 0;
	close_ate.part = 0xff;
	close_ate.crc8 = 0xff; /* Incorrect crc8 */

	ate.id = 0x1;
	ate.offset = 0;
	ate.len = sizeof(data);
#ifdef CONFIG_NVS_DATA_CRC
	ate.len += sizeof(data_crc);
#endif
	ate.part = 0xff;
	ate.crc8 = crc8_ccitt(0xff, &ate,
			      offsetof(struct nvs_ate, crc8));

	/* Mark sector 0 as closed */
	err = flash_write(fixture->fs.flash_device, fixture->fs.offset + fixture->fs.sector_size -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write valid ate at -6 */
	err = flash_write(fixture->fs.flash_device, fixture->fs.offset + fixture->fs.sector_size -
			  sizeof(struct nvs_ate) * 6, &ate, sizeof(ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write data for previous ate */
	data = 0xaa55aa55;
	err = flash_write(fixture->fs.flash_device, fixture->fs.offset, &data, sizeof(data));
	zassert_true(err == 0,  "flash_write failed: %d", err);
#ifdef CONFIG_NVS_DATA_CRC
	data_crc = crc32_ieee((const uint8_t *) &data, sizeof(data));
	err = flash_write(fixture->fs.flash_device, fixture->fs.offset + sizeof(data), &data_crc,
			  sizeof(data_crc));
	zassert_true(err == 0,  "flash_write for data CRC failed: %d", err);
#endif

	/* Mark sector 1 as closed */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + (2 * fixture->fs.sector_size) -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	fixture->fs.sector_count = 3;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);

	data = 0;
	len = nvs_read(&fixture->fs, 1, &data, sizeof(data));
	zassert_true(len == sizeof(data),
		     "nvs_read should have read %d bytes", sizeof(data));
	zassert_true(data == 0xaa55aa55, "unexpected value %d", data);
}

/*
 * Test that garbage-collection correctly handles corrupt ate's.
 */
ZTEST_F(nvs, test_nvs_gc_corrupt_ate)
{
	struct nvs_ate corrupt_ate, close_ate;
	int err;

	close_ate.id = 0xffff;
	close_ate.offset = fixture->fs.sector_size / 2;
	close_ate.len = 0;
	close_ate.part = 0xff;
	close_ate.crc8 = crc8_ccitt(0xff, &close_ate,
				    offsetof(struct nvs_ate, crc8));

	corrupt_ate.id = 0xdead;
	corrupt_ate.offset = 0;
	corrupt_ate.len = 20;
	corrupt_ate.part = 0xff;
	corrupt_ate.crc8 = 0xff; /* Incorrect crc8 */

	/* Mark sector 0 as closed */
	err = flash_write(fixture->fs.flash_device, fixture->fs.offset + fixture->fs.sector_size -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Write a corrupt ate */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + (fixture->fs.sector_size / 2),
			  &corrupt_ate, sizeof(corrupt_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + (2 * fixture->fs.sector_size) -
			  sizeof(struct nvs_ate), &close_ate,
			  sizeof(close_ate));
	zassert_true(err == 0,  "flash_write failed: %d", err);

	fixture->fs.sector_count = 3;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0,  "nvs_mount call failure: %d", err);
}

/*
 * Test that nvs_startup() correctly handles a sector that becomes
 * logically full due to a power loss during the last ATE write.
 *
 * In this scenario, no erased space remains between data_wra and ate_wra,
 * but the sector should be treated as closed and garbage collection
 * should be triggered on the next write.
 */
ZTEST_F(nvs, test_nvs_startup_full_sector_after_ate_power_loss)
{
	struct nvs_ate valid_gc_ate, valid_ate, corrupt_ate;
	int err;
	off_t sector_end = fixture->fs.offset + fixture->fs.sector_size;
	off_t data_size = fixture->fs.sector_size - 5 * sizeof(struct nvs_ate);
	uint32_t simple_data = 0xaa55aa55U;
	size_t offset = 0U;
	uint8_t data[32];

	fixture->fs.sector_count = 2;

	/* Valid GC ATE */
	valid_gc_ate.id = 0xffff;
	valid_gc_ate.offset = 0;
	valid_gc_ate.len = 0;
	valid_gc_ate.part = 0xff;
	valid_gc_ate.crc8 = crc8_ccitt(0xff, &valid_gc_ate,
				       offsetof(struct nvs_ate, crc8));

	/* A valid data ATE */
	valid_ate.id = 0x0001;
	valid_ate.offset = 0x0000;
	valid_ate.len = data_size / 2;
	valid_ate.part = 0xff;
	valid_ate.crc8 = crc8_ccitt(0xff, &valid_ate,
				    offsetof(struct nvs_ate, crc8));

	/* Corrupt ATE simulating power loss during ATE programming */
	corrupt_ate.id = 0x0002;
	corrupt_ate.offset = valid_ate.len;
	corrupt_ate.len = data_size / 2;
	corrupt_ate.part = 0xff;
	corrupt_ate.crc8 = 0xff; /* Invalid CRC */

	/*
	 * Sector layout after simulated power loss:
	 *
	 * [ Data #1 ][ Data #2 ][ ATE RSV ][ ATE #2 (corrupt) ][ ATE #1 ][ ATE GC ][ATE Close]
	 *
	 * data_wra catches up to ate_wra, leaving no erased space for data,
	 * but the sector should still be mountable.
	 */

	/* Write valid GC ATE */
	err = flash_write(fixture->fs.flash_device,
			  sector_end - 2 * sizeof(struct nvs_ate),
			  &valid_gc_ate, sizeof(valid_gc_ate));
	zassert_ok(err, "flash_write failed: %d", err);

	for (int i = 0; i < sizeof(data); i++) {
		data[i] = ~(fixture->fs.flash_parameters->erase_value);
	}

	/* Write data #1 #2 */
	while (data_size) {
		size_t wr = MIN(data_size, sizeof(data));

		err = flash_write(fixture->fs.flash_device,
				  fixture->fs.offset + offset,
				  &data, wr);
		zassert_ok(err, "flash_write failed: %d", err);

		data_size -= wr;
		offset    += wr;
	}

	/* Write valid data ATE */
	err = flash_write(fixture->fs.flash_device,
			  sector_end - 3 * sizeof(struct nvs_ate),
			  &valid_ate, sizeof(valid_ate));
	zassert_ok(err, "flash_write failed: %d", err);

	/* Write corrupt ATE (simulating power loss) */
	err = flash_write(fixture->fs.flash_device,
			  sector_end - 4 * sizeof(struct nvs_ate),
			  &corrupt_ate, sizeof(corrupt_ate));
	zassert_ok(err, "flash_write failed: %d", err);

	/* nvs_mount() must succeed */
	err = nvs_mount(&fixture->fs);
	zassert_ok(err, "nvs_mount failed: %d", err);

	/* nvs_write() must succeed and trigger garbage collection.
	 * GC is verified by checking that the active write address
	 * moves back to sector 1 after the write.
	 */
	err = nvs_write(&fixture->fs, 0x0003, &simple_data, sizeof(simple_data));
	zassert_equal(err, sizeof(simple_data), "nvs_write call failure: %d", err);

	/* After GC, the active ATE write address should be in sector 1 */
	zassert_equal((fixture->fs.ate_wra & ADDR_SECT_MASK) >> ADDR_SECT_SHIFT,
		      1, "nvs_gc() not trigger");
}
#endif /* CONFIG_TEST_NVS_SIMULATOR */

#ifdef CONFIG_NVS_LOOKUP_CACHE
static size_t num_matching_cache_entries(uint32_t addr, bool compare_sector_only, struct nvs_fs *fs)
{
	size_t i, num = 0;
	uint32_t mask = compare_sector_only ? ADDR_SECT_MASK : UINT32_MAX;

	for (i = 0; i < CONFIG_NVS_LOOKUP_CACHE_SIZE; i++) {
		if ((fs->lookup_cache[i] & mask) == addr) {
			num++;
		}
	}

	return num;
}

static size_t num_occupied_cache_entries(struct nvs_fs *fs)
{
	return CONFIG_NVS_LOOKUP_CACHE_SIZE -
	       num_matching_cache_entries(NVS_LOOKUP_CACHE_NO_ADDR, false, fs);
}
#endif

/*
 * Test that NVS lookup cache is properly rebuilt on nvs_mount(), or initialized
 * to NVS_LOOKUP_CACHE_NO_ADDR if the store is empty.
 */
ZTEST_F(nvs, test_nvs_cache_init)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	int err;
	size_t num;
	uint32_t ate_addr;
	uint8_t data = 0;

	/* Test cache initialization when the store is empty */

	fixture->fs.sector_count = 3;
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	num = num_occupied_cache_entries(&fixture->fs);
	zassert_equal(num, 0, "uninitialized cache");

	/* Test cache update after nvs_write() */

	ate_addr = fixture->fs.ate_wra;
	err = nvs_write(&fixture->fs, 1, &data, sizeof(data));
	zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);

	num = num_occupied_cache_entries(&fixture->fs);
	zassert_equal(num, 1, "cache not updated after write");

	num = num_matching_cache_entries(ate_addr, false, &fixture->fs);
	zassert_equal(num, 1, "invalid cache entry after write");

	/* Test cache initialization when the store is non-empty */

	memset(fixture->fs.lookup_cache, 0xAA, sizeof(fixture->fs.lookup_cache));
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	num = num_occupied_cache_entries(&fixture->fs);
	zassert_equal(num, 1, "uninitialized cache after restart");

	num = num_matching_cache_entries(ate_addr, false, &fixture->fs);
	zassert_equal(num, 1, "invalid cache entry after restart");
#endif
}

/*
 * Test that even after writing more NVS IDs than the number of NVS lookup cache
 * entries they all can be read correctly.
 */
ZTEST_F(nvs, test_nvs_cache_collission)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	int err;
	uint16_t id;
	uint16_t data;

	fixture->fs.sector_count = 3;
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	for (id = 0; id < CONFIG_NVS_LOOKUP_CACHE_SIZE + 1; id++) {
		data = id;
		err = nvs_write(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	for (id = 0; id < CONFIG_NVS_LOOKUP_CACHE_SIZE + 1; id++) {
		err = nvs_read(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_read call failure: %d", err);
		zassert_equal(data, id, "incorrect data read");
	}
#endif
}

/*
 * Test that NVS lookup cache does not contain any address from gc-ed sector
 */
ZTEST_F(nvs, test_nvs_cache_gc)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	int err;
	size_t num;
	uint16_t data = 0;

	fixture->fs.sector_count = 3;
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	/* Fill the first sector with writes of ID 1 */

	while (fixture->fs.data_wra + sizeof(data) + sizeof(struct nvs_ate)
	       <= fixture->fs.ate_wra) {
		++data;
		err = nvs_write(&fixture->fs, 1, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	/* Verify that cache contains a single entry for sector 0 */

	num = num_matching_cache_entries(0 << ADDR_SECT_SHIFT, true, &fixture->fs);
	zassert_equal(num, 1, "invalid cache content after filling sector 0");

	/* Fill the second sector with writes of ID 2 */

	while ((fixture->fs.ate_wra >> ADDR_SECT_SHIFT) != 2) {
		++data;
		err = nvs_write(&fixture->fs, 2, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	/*
	 * At this point sector 0 should have been gc-ed. Verify that action is
	 * reflected by the cache content.
	 */

	num = num_matching_cache_entries(0 << ADDR_SECT_SHIFT, true, &fixture->fs);
	zassert_equal(num, 0, "not invalidated cache entries aftetr gc");

	num = num_matching_cache_entries(2 << ADDR_SECT_SHIFT, true, &fixture->fs);
	zassert_equal(num, 2, "invalid cache content after gc");
#endif
}

/*
 * Test NVS lookup cache hash quality.
 */
ZTEST_F(nvs, test_nvs_cache_hash_quality)
{
#ifdef CONFIG_NVS_LOOKUP_CACHE
	const size_t MIN_CACHE_OCCUPANCY = CONFIG_NVS_LOOKUP_CACHE_SIZE * 6 / 10;
	int err;
	size_t num;
	uint16_t id;
	uint16_t data;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	/* Write NVS IDs from 0 to CONFIG_NVS_LOOKUP_CACHE_SIZE - 1 */

	for (uint16_t i = 0; i < CONFIG_NVS_LOOKUP_CACHE_SIZE; i++) {
		id = i;
		data = 0;

		err = nvs_write(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	/* Verify that at least 60% cache entries are occupied */

	num = num_occupied_cache_entries(&fixture->fs);
	TC_PRINT("Cache occupancy: %u\n", (unsigned int)num);
	zassert_between_inclusive(num, MIN_CACHE_OCCUPANCY, CONFIG_NVS_LOOKUP_CACHE_SIZE,
				  "too low cache occupancy - poor hash quality");

	err = nvs_clear(&fixture->fs);
	zassert_true(err == 0, "nvs_clear call failure: %d", err);

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	/* Write CONFIG_NVS_LOOKUP_CACHE_SIZE NVS IDs that form the following series: 0, 4, 8... */

	for (uint16_t i = 0; i < CONFIG_NVS_LOOKUP_CACHE_SIZE; i++) {
		id = i * 4;
		data = 0;

		err = nvs_write(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "nvs_write call failure: %d", err);
	}

	/* Verify that at least 60% cache entries are occupied */

	num = num_occupied_cache_entries(&fixture->fs);
	TC_PRINT("Cache occupancy: %u\n", (unsigned int)num);
	zassert_between_inclusive(num, MIN_CACHE_OCCUPANCY, CONFIG_NVS_LOOKUP_CACHE_SIZE,
				  "too low cache occupancy - poor hash quality");

#endif
}

#ifdef CONFIG_TEST_NVS_SIMULATOR
/*
 * Test NVS bad region initialization recovery.
 */
ZTEST_F(nvs, test_nvs_init_bad_memory_region)
{
	int err;
	uint32_t data;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	/* Write bad ATE to each sector */
	for (uint16_t i = 0; i < TEST_SECTOR_COUNT; i++) {
		data = 0xdeadbeef;
		err = flash_write(fixture->fs.flash_device,
				  fixture->fs.offset + (fixture->fs.sector_size * (i + 1)) -
					  sizeof(struct nvs_ate),
				  &data, sizeof(data));
		zassert_true(err == 0, "flash_write failed: %d", err);
	}

	/* Reinitialize the NVS. */
	memset(&fixture->fs, 0, sizeof(fixture->fs));
	(void)setup();

#ifdef CONFIG_NVS_INIT_BAD_MEMORY_REGION
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	/* Ensure that the NVS is able to store new content. */
	execute_long_pattern_write(TEST_DATA_ID, &fixture->fs);
#else
	err = nvs_mount(&fixture->fs);
	zassert_true(err == -EDEADLK, "nvs_mount call ok, expect fail: %d", err);
#endif
}

/*
 * Test NVS GC Pack per entry
 */
ZTEST_F(nvs, test_nvs_gc_pack_per_entry)
{
	int err;
	const size_t wbs = fixture->fs.flash_parameters->write_block_size;
	uint16_t id = 0;
	uint8_t write_data[NVS_BLOCK_SIZE];
	uint8_t update_data[NVS_BLOCK_SIZE];

	fixture->fs.sector_count = 2;
	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	/* Fill NVS with variable-length entries until full */
	while (1) {
		size_t len = id % wbs;

		if (len == 0) {
			len = 1;
		}

		memset(write_data, id, len);

		err = nvs_write(&fixture->fs, id, write_data, len);
		if (err < 0) {
			zassert_equal(err, -ENOSPC, "Expected -ENOSPC, got %d", err);
			break;
		}

		id++;
	}

	/* Limit the number of entries to avoid ZTEST timeout on large erase block sizes(64KB)
	 * Testing wbs * 2 entries is sufficient because each entry length already covers
	 * the range.
	 */
	id = MIN(id, wbs * 2);

	/* Delete and update each entry immediately */
	for (uint16_t i = 0; i < id; i++) {
		size_t len = i % wbs;

		if (len == 0) {
			len = 1;
		}

		err = nvs_delete(&fixture->fs, i);
		zassert_equal(err, 0, "nvs_delete failed for id %d: %d", i, err);

		/* Prepare updated data: different content */
		memset(update_data, i + 1, len);

		err = nvs_write(&fixture->fs, i, update_data, len);
		zassert_equal(err, len, "nvs_write failed for id %d: %d", i, err);
	}
}

/*
 * Test NVS write stream
 */
ZTEST_F(nvs, test_nvs_write_stream)
{
	uint8_t head[15] = {
		0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
		0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF
	};
	uint8_t data[16] = {
		0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
		0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0,
	};
	uint8_t tail[17] = {
		0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
		0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0,
		0xE1,
	};
	uint8_t expected[sizeof(head) + sizeof(data) + sizeof(tail)];
	uint8_t read_buf[sizeof(expected)];

	struct {
		bool use_head;
		bool use_data;
		bool use_tail;
	} cases[] = {
		{false, true,  false},
		{true,  false, false},
		{false, false, true },
		{true,  true,  false},
		{false, true,  true },
		{true,  false, true },
		{true,  true,  true },
		{false, false, false},
	};
	int err;

	err = nvs_mount(&fixture->fs);
	zassert_true(err == 0, "nvs_mount call failure: %d", err);

	for (int i = 0; i < ARRAY_SIZE(cases); i++) {
		memset(expected, 0, sizeof(expected));
		memset(read_buf, 0, sizeof(read_buf));

		struct nvs_flash_wrt_stream strm = {0};

		size_t offset = 0;

		if (cases[i].use_head) {
			strm.head.ptr = head;
			strm.head.len = sizeof(head);
			memcpy(expected + offset, head, sizeof(head));
			offset += sizeof(head);
		}

		if (cases[i].use_data) {
			strm.data.ptr = data;
			strm.data.len = sizeof(data);
			memcpy(expected + offset, data, sizeof(data));
			offset += sizeof(data);
		}

		if (cases[i].use_tail) {
			strm.tail.ptr = tail;
			strm.tail.len = sizeof(tail);
			memcpy(expected + offset, tail, sizeof(tail));
			offset += sizeof(tail);
		}

		err = nvs_flash_al_wrt_streams(&fixture->fs, 0, &strm);

		zassert_true(err == 0, "write failed err %d", err);

		err = flash_read(fixture->fs.flash_device, fixture->fs.offset,
				 read_buf, offset);
		zassert_true(err == 0, "read failed err %d", err);

		zassert_mem_equal(read_buf, expected, offset, "mismatch");

		err = flash_erase(fixture->fs.flash_device, fixture->fs.offset,
				  fixture->fs.sector_size);
		zassert_true(err == 0, "erase failed err %d", err);
	}
}
#endif /* CONFIG_TEST_NVS_SIMULATOR */
