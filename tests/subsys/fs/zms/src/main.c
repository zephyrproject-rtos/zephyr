/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/fs/zms.h>
#include <zephyr/stats/stats.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/crc.h>
#include "zms_priv.h"

#define TEST_ZMS_AREA        storage_partition
#define TEST_ZMS_AREA_OFFSET FIXED_PARTITION_OFFSET(TEST_ZMS_AREA)
#define TEST_ZMS_AREA_ID     FIXED_PARTITION_ID(TEST_ZMS_AREA)
#define TEST_ZMS_AREA_DEV    DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(TEST_ZMS_AREA)))
#define TEST_DATA_ID         1
#define TEST_SECTOR_COUNT    5U

static const struct device *const flash_dev = TEST_ZMS_AREA_DEV;

struct zms_fixture {
	struct zms_fs fs;
#ifdef CONFIG_TEST_ZMS_SIMULATOR
	struct stats_hdr *sim_stats;
	struct stats_hdr *sim_thresholds;
#endif /* CONFIG_TEST_ZMS_SIMULATOR */
};

static void *setup(void)
{
	int err;
	const struct flash_area *fa;
	struct flash_pages_info info;
	static struct zms_fixture fixture;

	__ASSERT_NO_MSG(device_is_ready(flash_dev));

	err = flash_area_open(TEST_ZMS_AREA_ID, &fa);
	zassert_true(err == 0, "flash_area_open() fail: %d", err);

	fixture.fs.offset = TEST_ZMS_AREA_OFFSET;
	err = flash_get_page_info_by_offs(flash_area_get_device(fa), fixture.fs.offset, &info);
	zassert_true(err == 0, "Unable to get page info: %d", err);

	fixture.fs.sector_size = info.size;
	fixture.fs.sector_count = TEST_SECTOR_COUNT;
	fixture.fs.flash_device = flash_area_get_device(fa);

	return &fixture;
}

static void before(void *data)
{
#ifdef CONFIG_TEST_ZMS_SIMULATOR
	struct zms_fixture *fixture = (struct zms_fixture *)data;

	fixture->sim_stats = stats_group_find("flash_sim_stats");
	fixture->sim_thresholds = stats_group_find("flash_sim_thresholds");
#endif /* CONFIG_TEST_ZMS_SIMULATOR */
}

static void after(void *data)
{
	struct zms_fixture *fixture = (struct zms_fixture *)data;

#ifdef CONFIG_TEST_ZMS_SIMULATOR
	if (fixture->sim_stats) {
		stats_reset(fixture->sim_stats);
	}
	if (fixture->sim_thresholds) {
		stats_reset(fixture->sim_thresholds);
	}
#endif /* CONFIG_TEST_ZMS_SIMULATOR */

	/* Clear ZMS */
	if (fixture->fs.ready) {
		int err;

		err = zms_clear(&fixture->fs);
		zassert_true(err == 0, "zms_clear call failure: %d", err);
	}

	fixture->fs.sector_count = TEST_SECTOR_COUNT;
}

ZTEST_SUITE(zms, NULL, setup, before, after, NULL);

ZTEST_F(zms, test_zms_mount)
{
	int err;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);
}

static void execute_long_pattern_write(uint32_t id, struct zms_fs *fs)
{
	char rd_buf[512];
	char wr_buf[512];
	char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
	size_t len;

	len = zms_read(fs, id, rd_buf, sizeof(rd_buf));
	zassert_true(len == -ENOENT, "zms_read unexpected failure: %d", len);

	BUILD_ASSERT((sizeof(wr_buf) % sizeof(pattern)) == 0);
	for (int i = 0; i < sizeof(wr_buf); i += sizeof(pattern)) {
		memcpy(wr_buf + i, pattern, sizeof(pattern));
	}

	len = zms_write(fs, id, wr_buf, sizeof(wr_buf));
	zassert_true(len == sizeof(wr_buf), "zms_write failed: %d", len);

	len = zms_read(fs, id, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf), "zms_read unexpected failure: %d", len);
	zassert_mem_equal(wr_buf, rd_buf, sizeof(rd_buf), "RD buff should be equal to the WR buff");
}

ZTEST_F(zms, test_zms_write)
{
	int err;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	execute_long_pattern_write(TEST_DATA_ID, &fixture->fs);
}

#ifdef CONFIG_TEST_ZMS_SIMULATOR
static int flash_sim_write_calls_find(struct stats_hdr *hdr, void *arg, const char *name,
				      uint16_t off)
{
	if (!strcmp(name, "flash_write_calls")) {
		uint32_t **flash_write_stat = (uint32_t **)arg;
		*flash_write_stat = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

static int flash_sim_max_write_calls_find(struct stats_hdr *hdr, void *arg, const char *name,
					  uint16_t off)
{
	if (!strcmp(name, "max_write_calls")) {
		uint32_t **max_write_calls = (uint32_t **)arg;
		*max_write_calls = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

ZTEST_F(zms, test_zms_corrupted_write)
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

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	err = zms_read(&fixture->fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(err == -ENOENT, "zms_read unexpected failure: %d", err);

	BUILD_ASSERT((sizeof(wr_buf_1) % sizeof(pattern_1)) == 0);
	for (int i = 0; i < sizeof(wr_buf_1); i += sizeof(pattern_1)) {
		memcpy(wr_buf_1 + i, pattern_1, sizeof(pattern_1));
	}

	len = zms_write(&fixture->fs, TEST_DATA_ID, wr_buf_1, sizeof(wr_buf_1));
	zassert_true(len == sizeof(wr_buf_1), "zms_write failed: %d", len);

	len = zms_read(&fixture->fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf), "zms_read unexpected failure: %d", len);
	zassert_mem_equal(wr_buf_1, rd_buf, sizeof(rd_buf),
			  "RD buff should be equal to the first WR buff");

	BUILD_ASSERT((sizeof(wr_buf_2) % sizeof(pattern_2)) == 0);
	for (int i = 0; i < sizeof(wr_buf_2); i += sizeof(pattern_2)) {
		memcpy(wr_buf_2 + i, pattern_2, sizeof(pattern_2));
	}

	/* Set the maximum number of writes that the flash simulator can
	 * execute.
	 */
	stats_walk(fixture->sim_thresholds, flash_sim_max_write_calls_find, &flash_max_write_calls);
	stats_walk(fixture->sim_stats, flash_sim_write_calls_find, &flash_write_stat);

	*flash_max_write_calls = *flash_write_stat - 1;
	*flash_write_stat = 0;

	/* Flash simulator will lose part of the data at the end of this write.
	 * This should simulate power down during flash write. The written data
	 * are corrupted at this point and should be discarded by the ZMS.
	 */
	len = zms_write(&fixture->fs, TEST_DATA_ID, wr_buf_2, sizeof(wr_buf_2));
	zassert_true(len == sizeof(wr_buf_2), "zms_write failed: %d", len);

	/* Reinitialize the ZMS. */
	memset(&fixture->fs, 0, sizeof(fixture->fs));
	(void)setup();
	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	len = zms_read(&fixture->fs, TEST_DATA_ID, rd_buf, sizeof(rd_buf));
	zassert_true(len == sizeof(rd_buf), "zms_read unexpected failure: %d", len);
	zassert_true(memcmp(wr_buf_2, rd_buf, sizeof(rd_buf)) != 0,
		     "RD buff should not be equal to the second WR buff because of "
		     "corrupted write operation");
	zassert_mem_equal(wr_buf_1, rd_buf, sizeof(rd_buf),
			  "RD buff should be equal to the first WR buff because subsequent "
			  "write operation has failed");
}

ZTEST_F(zms, test_zms_gc)
{
	int err;
	int len;
	uint8_t buf[32];
	uint8_t rd_buf[32];
	const uint8_t max_id = 10;
	/* 21st write will trigger GC. */
	const uint16_t max_writes = 21;

	fixture->fs.sector_count = 2;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	for (int i = 0; i < max_writes; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		len = zms_write(&fixture->fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "zms_write failed: %d", len);
	}

	for (int id = 0; id < max_id; id++) {
		len = zms_read(&fixture->fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf), "zms_read unexpected failure: %d", len);

		for (int i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				  "RD buff should be equal to the WR buff");
	}

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	for (int id = 0; id < max_id; id++) {
		len = zms_read(&fixture->fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf), "zms_read unexpected failure: %d", len);

		for (int i = 0; i < sizeof(rd_buf); i++) {
			rd_buf[i] = rd_buf[i] % max_id;
			buf[i] = id;
		}
		zassert_mem_equal(buf, rd_buf, sizeof(rd_buf),
				  "RD buff should be equal to the WR buff");
	}
}

static void write_content(uint32_t max_id, uint32_t begin, uint32_t end, struct zms_fs *fs)
{
	uint8_t buf[32];
	ssize_t len;

	for (int i = begin; i < end; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		len = zms_write(fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "zms_write failed: %d", len);
	}
}

static void check_content(uint32_t max_id, struct zms_fs *fs)
{
	uint8_t rd_buf[32];
	uint8_t buf[32];
	ssize_t len;

	for (int id = 0; id < max_id; id++) {
		len = zms_read(fs, id, rd_buf, sizeof(buf));
		zassert_true(len == sizeof(rd_buf), "zms_read unexpected failure: %d", len);

		for (int i = 0; i < ARRAY_SIZE(rd_buf); i++) {
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
ZTEST_F(zms, test_zms_gc_3sectors)
{
	int err;
	const uint16_t max_id = 10;
	/* 41st write will trigger 1st GC. */
	const uint16_t max_writes = 41;
	/* 61st write will trigger 2nd GC. */
	const uint16_t max_writes_2 = 41 + 20;
	/* 81st write will trigger 3rd GC. */
	const uint16_t max_writes_3 = 41 + 20 + 20;
	/* 101st write will trigger 4th GC. */
	const uint16_t max_writes_4 = 41 + 20 + 20 + 20;

	fixture->fs.sector_count = 3;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 0, "unexpected write sector");

	/* Trigger 1st GC */
	write_content(max_id, 0, max_writes, &fixture->fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	/* Trigger 2nd GC */
	write_content(max_id, max_writes, max_writes_2, &fixture->fs);

	/* sector sequence: write, empty, closed */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 0, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 0, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	/* Trigger 3rd GC */
	write_content(max_id, max_writes_2, max_writes_3, &fixture->fs);

	/* sector sequence: closed, write, empty */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 1, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 1, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	/* Trigger 4th GC */
	write_content(max_id, max_writes_3, max_writes_4, &fixture->fs);

	/* sector sequence: empty,closed, write */
	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2, "unexpected write sector");
	check_content(max_id, &fixture->fs);

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	zassert_equal(fixture->fs.ate_wra >> ADDR_SECT_SHIFT, 2, "unexpected write sector");
	check_content(max_id, &fixture->fs);
}

static int flash_sim_max_len_find(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off)
{
	if (!strcmp(name, "max_len")) {
		uint32_t **max_len = (uint32_t **)arg;
		*max_len = (uint32_t *)((uint8_t *)hdr + off);
	}

	return 0;
}

ZTEST_F(zms, test_zms_corrupted_sector_close_operation)
{
	int err;
	int len;
	uint8_t buf[32];
	uint32_t *flash_write_stat;
	uint32_t *flash_max_write_calls;
	uint32_t *flash_max_len;
	const uint16_t max_id = 10;
	/* 21st write will trigger GC. */
	const uint16_t max_writes = 21;

	/* Get the address of simulator parameters. */
	stats_walk(fixture->sim_thresholds, flash_sim_max_write_calls_find, &flash_max_write_calls);
	stats_walk(fixture->sim_thresholds, flash_sim_max_len_find, &flash_max_len);
	stats_walk(fixture->sim_stats, flash_sim_write_calls_find, &flash_write_stat);

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	for (int i = 0; i < max_writes; i++) {
		uint8_t id = (i % max_id);
		uint8_t id_data = id + max_id * (i / max_id);

		memset(buf, id_data, sizeof(buf));

		if (i == max_writes - 1) {
			/* Reset stats. */
			*flash_write_stat = 0;

			/* Block write calls and simulate power down during
			 * sector closing operation, so only a part of a ZMS
			 * closing ate will be written.
			 */
			*flash_max_write_calls = 1;
			*flash_max_len = 4;
		}
		len = zms_write(&fixture->fs, id, buf, sizeof(buf));
		zassert_true(len == sizeof(buf), "zms_write failed: %d", len);
	}

	/* Make the flash simulator functional again. */
	*flash_max_write_calls = 0;
	*flash_max_len = 0;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	check_content(max_id, &fixture->fs);

	/* Ensure that the ZMS is able to store new content. */
	execute_long_pattern_write(max_id, &fixture->fs);
}
#endif /* CONFIG_TEST_ZMS_SIMULATOR */

/**
 * @brief Test case when storage become full, so only deletion is possible.
 */
ZTEST_F(zms, test_zms_full_sector)
{
	int err;
	ssize_t len;
	uint32_t filling_id = 0;
	uint32_t data_read;

	fixture->fs.sector_count = 3;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	while (1) {
		len = zms_write(&fixture->fs, filling_id, &filling_id, sizeof(filling_id));
		if (len == -ENOSPC) {
			break;
		}
		zassert_true(len == sizeof(filling_id), "zms_write failed: %d", len);
		filling_id++;
	}

	/* check whether can delete whatever from full storage */
	err = zms_delete(&fixture->fs, 1);
	zassert_true(err == 0, "zms_delete call failure: %d", err);

	/* the last sector is full now, test re-initialization */
	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	len = zms_write(&fixture->fs, filling_id, &filling_id, sizeof(filling_id));
	zassert_true(len == sizeof(filling_id), "zms_write failed: %d", len);

	/* sanitycheck on ZMS content */
	for (int i = 0; i <= filling_id; i++) {
		len = zms_read(&fixture->fs, i, &data_read, sizeof(data_read));
		if (i == 1) {
			zassert_true(len == -ENOENT, "zms_read shouldn't found the entry: %d", len);
		} else {
			zassert_true(len == sizeof(data_read),
				     "zms_read #%d failed: len is %zd instead of %zu", i, len,
				     sizeof(data_read));
			zassert_equal(data_read, i, "read unexpected data: %d instead of %d",
				      data_read, i);
		}
	}
}

ZTEST_F(zms, test_delete)
{
	int err;
	ssize_t len;
	uint32_t filling_id;
	uint32_t data_read;
	uint32_t ate_wra;
	uint32_t data_wra;

	fixture->fs.sector_count = 3;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	for (filling_id = 0; filling_id < 10; filling_id++) {
		len = zms_write(&fixture->fs, filling_id, &filling_id, sizeof(filling_id));

		zassert_true(len == sizeof(filling_id), "zms_write failed: %d", len);

		if (filling_id != 0) {
			continue;
		}

		/* delete the first entry while it is the most recent one */
		err = zms_delete(&fixture->fs, filling_id);
		zassert_true(err == 0, "zms_delete call failure: %d", err);

		len = zms_read(&fixture->fs, filling_id, &data_read, sizeof(data_read));
		zassert_true(len == -ENOENT, "zms_read shouldn't found the entry: %d", len);
	}

	/* delete existing entry */
	err = zms_delete(&fixture->fs, 1);
	zassert_true(err == 0, "zms_delete call failure: %d", err);

	len = zms_read(&fixture->fs, 1, &data_read, sizeof(data_read));
	zassert_true(len == -ENOENT, "zms_read shouldn't found the entry: %d", len);

	ate_wra = fixture->fs.ate_wra;
	data_wra = fixture->fs.data_wra;

#ifdef CONFIG_ZMS_NO_DOUBLE_WRITE
	/* delete already deleted entry */
	err = zms_delete(&fixture->fs, 1);
	zassert_true(err == 0, "zms_delete call failure: %d", err);
	zassert_true(ate_wra == fixture->fs.ate_wra && data_wra == fixture->fs.data_wra,
		     "delete already deleted entry should not make"
		     " any footprint in the storage");

	/* delete nonexisting entry */
	err = zms_delete(&fixture->fs, filling_id);
	zassert_true(err == 0, "zms_delete call failure: %d", err);
	zassert_true(ate_wra == fixture->fs.ate_wra && data_wra == fixture->fs.data_wra,
		     "delete nonexistent entry should not make"
		     " any footprint in the storage");
#endif
}

#ifdef CONFIG_TEST_ZMS_SIMULATOR
/*
 * Test that garbage-collection can recover all ate's even when the last ate,
 * ie close_ate, is corrupt. In this test the close_ate is set to point to the
 * last ate at -5. A valid ate is however present at -6. Since the close_ate
 * has an invalid crc8, the offset should not be used and a recover of the
 * last ate should be done instead.
 */
ZTEST_F(zms, test_zms_gc_corrupt_close_ate)
{
	struct zms_ate ate;
	struct zms_ate close_ate;
	struct zms_ate empty_ate;
	uint32_t data;
	ssize_t len;
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_FLASH_SIMULATOR_DOUBLE_WRITES);
	close_ate.id = 0xffffffff;
	close_ate.offset = fixture->fs.sector_size - sizeof(struct zms_ate) * 5;
	close_ate.len = 0;
	close_ate.metadata = 0xffffffff;
	close_ate.cycle_cnt = 1;
	close_ate.crc8 = 0xff; /* Incorrect crc8 */

	empty_ate.id = 0xffffffff;
	empty_ate.offset = 0;
	empty_ate.len = 0xffff;
	empty_ate.metadata = 0x4201;
	empty_ate.cycle_cnt = 1;
	empty_ate.crc8 =
		crc8_ccitt(0xff, (uint8_t *)&empty_ate + SIZEOF_FIELD(struct zms_ate, crc8),
			   sizeof(struct zms_ate) - SIZEOF_FIELD(struct zms_ate, crc8));

	memset(&ate, 0, sizeof(struct zms_ate));
	ate.id = 0x1;
	ate.len = sizeof(data);
	ate.cycle_cnt = 1;
	data = 0xaa55aa55;
	memcpy(&ate.data, &data, sizeof(data));
	ate.crc8 = crc8_ccitt(0xff, (uint8_t *)&ate + SIZEOF_FIELD(struct zms_ate, crc8),
			      sizeof(struct zms_ate) - SIZEOF_FIELD(struct zms_ate, crc8));

	/* Add empty ATE */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + fixture->fs.sector_size - sizeof(struct zms_ate),
			  &empty_ate, sizeof(empty_ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	/* Mark sector 0 as closed */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + fixture->fs.sector_size - 2 * sizeof(struct zms_ate),
			  &close_ate, sizeof(close_ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	/* Write valid ate at -6 */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + fixture->fs.sector_size - 6 * sizeof(struct zms_ate),
			  &ate, sizeof(ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + (2 * fixture->fs.sector_size) -
				  2 * sizeof(struct zms_ate),
			  &close_ate, sizeof(close_ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	fixture->fs.sector_count = 3;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	data = 0;
	len = zms_read(&fixture->fs, 1, &data, sizeof(data));
	zassert_true(len == sizeof(data), "zms_read should have read %d bytes", sizeof(data));
	zassert_true(data == 0xaa55aa55, "unexpected value %d", data);
}
#endif /* CONFIG_TEST_ZMS_SIMULATOR */

/*
 * Test that garbage-collection correctly handles corrupt ate's.
 */
ZTEST_F(zms, test_zms_gc_corrupt_ate)
{
	struct zms_ate corrupt_ate;
	struct zms_ate close_ate;
	int err;

	close_ate.id = 0xffffffff;
	close_ate.offset = fixture->fs.sector_size / 2;
	close_ate.len = 0;
	close_ate.crc8 =
		crc8_ccitt(0xff, (uint8_t *)&close_ate + SIZEOF_FIELD(struct zms_ate, crc8),
			   sizeof(struct zms_ate) - SIZEOF_FIELD(struct zms_ate, crc8));

	corrupt_ate.id = 0xdeadbeef;
	corrupt_ate.offset = 0;
	corrupt_ate.len = 20;
	corrupt_ate.crc8 = 0xff; /* Incorrect crc8 */

	/* Mark sector 0 as closed */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + fixture->fs.sector_size - 2 * sizeof(struct zms_ate),
			  &close_ate, sizeof(close_ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	/* Write a corrupt ate */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + (fixture->fs.sector_size / 2), &corrupt_ate,
			  sizeof(corrupt_ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	/* Mark sector 1 as closed */
	err = flash_write(fixture->fs.flash_device,
			  fixture->fs.offset + (2 * fixture->fs.sector_size) -
				  2 * sizeof(struct zms_ate),
			  &close_ate, sizeof(close_ate));
	zassert_true(err == 0, "flash_write failed: %d", err);

	fixture->fs.sector_count = 3;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);
}

#ifdef CONFIG_ZMS_LOOKUP_CACHE
static size_t num_matching_cache_entries(uint64_t addr, bool compare_sector_only, struct zms_fs *fs)
{
	size_t num = 0;
	uint64_t mask = compare_sector_only ? ADDR_SECT_MASK : UINT64_MAX;

	for (int i = 0; i < CONFIG_ZMS_LOOKUP_CACHE_SIZE; i++) {
		if ((fs->lookup_cache[i] & mask) == addr) {
			num++;
		}
	}

	return num;
}

static size_t num_occupied_cache_entries(struct zms_fs *fs)
{
	return CONFIG_ZMS_LOOKUP_CACHE_SIZE -
	       num_matching_cache_entries(ZMS_LOOKUP_CACHE_NO_ADDR, false, fs);
}
#endif

/*
 * Test that ZMS lookup cache is properly rebuilt on zms_mount(), or initialized
 * to ZMS_LOOKUP_CACHE_NO_ADDR if the store is empty.
 */
ZTEST_F(zms, test_zms_cache_init)
{
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	int err;
	size_t num;
	uint64_t ate_addr;
	uint8_t data = 0;

	/* Test cache initialization when the store is empty */

	fixture->fs.sector_count = 3;
	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	num = num_occupied_cache_entries(&fixture->fs);
	zassert_equal(num, 0, "uninitialized cache");

	/* Test cache update after zms_write() */

	ate_addr = fixture->fs.ate_wra;
	err = zms_write(&fixture->fs, 1, &data, sizeof(data));
	zassert_equal(err, sizeof(data), "zms_write call failure: %d", err);

	num = num_occupied_cache_entries(&fixture->fs);
	zassert_equal(num, 1, "cache not updated after write");

	num = num_matching_cache_entries(ate_addr, false, &fixture->fs);
	zassert_equal(num, 1, "invalid cache entry after write");

	/* Test cache initialization when the store is non-empty */

	memset(fixture->fs.lookup_cache, 0xAA, sizeof(fixture->fs.lookup_cache));
	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	num = num_occupied_cache_entries(&fixture->fs);
	zassert_equal(num, 1, "uninitialized cache after restart");

	num = num_matching_cache_entries(ate_addr, false, &fixture->fs);
	zassert_equal(num, 1, "invalid cache entry after restart");
#endif
}

/*
 * Test that even after writing more ZMS IDs than the number of ZMS lookup cache
 * entries they all can be read correctly.
 */
ZTEST_F(zms, test_zms_cache_collission)
{
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	int err;
	uint16_t data;

	fixture->fs.sector_count = 4;
	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	for (int id = 0; id < CONFIG_ZMS_LOOKUP_CACHE_SIZE + 1; id++) {
		data = id;
		err = zms_write(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "zms_write call failure: %d", err);
	}

	for (int id = 0; id < CONFIG_ZMS_LOOKUP_CACHE_SIZE + 1; id++) {
		err = zms_read(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "zms_read call failure: %d", err);
		zassert_equal(data, id, "incorrect data read");
	}
#endif
}

/*
 * Test that ZMS lookup cache does not contain any address from gc-ed sector
 */
ZTEST_F(zms, test_zms_cache_gc)
{
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	int err;
	size_t num;
	uint16_t data = 0;

	fixture->fs.sector_count = 3;
	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	/* Fill the first sector with writes of ID 1 */

	while (fixture->fs.data_wra + sizeof(data) + sizeof(struct zms_ate) <=
	       fixture->fs.ate_wra) {
		++data;
		err = zms_write(&fixture->fs, 1, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "zms_write call failure: %d", err);
	}

	/* Verify that cache contains a single entry for sector 0 */

	num = num_matching_cache_entries(0ULL << ADDR_SECT_SHIFT, true, &fixture->fs);
	zassert_equal(num, 1, "invalid cache content after filling sector 0");

	/* Fill the second sector with writes of ID 2 */

	while ((fixture->fs.ate_wra >> ADDR_SECT_SHIFT) != 2) {
		++data;
		err = zms_write(&fixture->fs, 2, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "zms_write call failure: %d", err);
	}

	/*
	 * At this point sector 0 should have been gc-ed. Verify that action is
	 * reflected by the cache content.
	 */

	num = num_matching_cache_entries(0ULL << ADDR_SECT_SHIFT, true, &fixture->fs);
	zassert_equal(num, 0, "not invalidated cache entries aftetr gc");

	num = num_matching_cache_entries(2ULL << ADDR_SECT_SHIFT, true, &fixture->fs);
	zassert_equal(num, 2, "invalid cache content after gc");
#endif
}

/*
 * Test ZMS lookup cache hash quality.
 */
ZTEST_F(zms, test_zms_cache_hash_quality)
{
#ifdef CONFIG_ZMS_LOOKUP_CACHE
	const size_t MIN_CACHE_OCCUPANCY = CONFIG_ZMS_LOOKUP_CACHE_SIZE * 6 / 10;
	int err;
	size_t num;
	uint32_t id;
	uint16_t data;

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	/* Write ZMS IDs from 0 to CONFIG_ZMS_LOOKUP_CACHE_SIZE - 1 */

	for (int i = 0; i < CONFIG_ZMS_LOOKUP_CACHE_SIZE; i++) {
		id = i;
		data = 0;

		err = zms_write(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "zms_write call failure: %d", err);
	}

	/* Verify that at least 60% cache entries are occupied */

	num = num_occupied_cache_entries(&fixture->fs);
	TC_PRINT("Cache occupancy: %u\n", (unsigned int)num);
	zassert_between_inclusive(num, MIN_CACHE_OCCUPANCY, CONFIG_ZMS_LOOKUP_CACHE_SIZE,
				  "too low cache occupancy - poor hash quality");

	err = zms_clear(&fixture->fs);
	zassert_true(err == 0, "zms_clear call failure: %d", err);

	err = zms_mount(&fixture->fs);
	zassert_true(err == 0, "zms_mount call failure: %d", err);

	/* Write CONFIG_ZMS_LOOKUP_CACHE_SIZE ZMS IDs that form the following series: 0, 4, 8... */

	for (int i = 0; i < CONFIG_ZMS_LOOKUP_CACHE_SIZE; i++) {
		id = i * 4;
		data = 0;

		err = zms_write(&fixture->fs, id, &data, sizeof(data));
		zassert_equal(err, sizeof(data), "zms_write call failure: %d", err);
	}

	/* Verify that at least 60% cache entries are occupied */

	num = num_occupied_cache_entries(&fixture->fs);
	TC_PRINT("Cache occupancy: %u\n", (unsigned int)num);
	zassert_between_inclusive(num, MIN_CACHE_OCCUPANCY, CONFIG_ZMS_LOOKUP_CACHE_SIZE,
				  "too low cache occupancy - poor hash quality");

#endif
}
