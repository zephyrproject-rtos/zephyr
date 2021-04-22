/*
 * Copyright (c) 2021 Laczen
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/flash.h>
#include <drivers/mtd.h>

/* Helper functions */
/* Get the master device of a partition (contains the device) */
static struct mtd_info *mtd_get_master(const struct mtd_info *mtd)
{
	struct mtd_info *master = (struct mtd_info *)mtd;

	while (master->cfg->parent) {
		master = (struct mtd_info *)master->cfg->parent;
	}

	return master;
}

/* Recalculate offset to master offset */
static off_t mtd_get_master_offset(const struct mtd_info *mtd, off_t off)
{
	struct mtd_info *master = (struct mtd_info *)mtd;
	off_t ret = off;

	while (master->cfg->parent) {
		ret += master->cfg->off;
		master = (struct mtd_info *)master->cfg->parent;
	}

	ret += master->cfg->off;

	return ret;
}
/* End Helper functions */

/**
 * @brief Test mtd partition read/write
 */
void test_mtd_rw_on(const struct mtd_info *mtd)
{
	int rc;
	struct mtd_info *master = mtd_get_master(mtd);
	off_t off, off_m;
	size_t sec_size;
	uint8_t wd[256];
	uint8_t rd[256];

	/* First erase the area so it's ready for use. */
	off_m = mtd_get_master_offset(mtd, 0);
	rc = flash_erase(master->cfg->device, off_m, mtd->cfg->size);
	zassert_true(rc == 0, "hal_flash_erase() fail [rc: %d]", rc);

	(void)memset(wd, 0xa5, sizeof(wd));

	rc = mtd_get_ebs(mtd, &sec_size);
	zassert_true(rc == 0, "mtd_get_ebs() fail [rc: %d]", rc);

	/* write stuff to beginning of every sector */
	off = 0;
	while (off < mtd->cfg->size) {
		rc = mtd_write(mtd, off, wd, sizeof(wd));
		zassert_true(rc == 0, "mtd_write() fail [rc: %d]", rc);

		/* read it back via hal_flash_read() */
		off_m = mtd_get_master_offset(mtd, off);
		rc = flash_read(master->cfg->device, off_m, rd, sizeof(rd));
		zassert_true(rc == 0, "hal_flash_read() fail [rc: %d]", rc);

		rc = memcmp(wd, rd, sizeof(wd));
		zassert_true(rc == 0, "read data != write data");

		/* write stuff to end of area via hal_flash_write */
		off_m = mtd_get_master_offset(mtd, off + sec_size - sizeof(wd));
		rc = flash_write(master->cfg->device, off_m, wd, sizeof(wd));
		zassert_true(rc == 0, "hal_flash_write() fail [rc: %d]", rc);

		/* and read it back */
		(void)memset(rd, 0, sizeof(rd));
		rc = mtd_read(mtd, off + sec_size - sizeof(rd), rd, sizeof(rd));
		zassert_true(rc == 0, "mtd_read() fail [rc: %d]", rc);

		rc = memcmp(wd, rd, sizeof(rd));
		zassert_true(rc == 0, "read data != write data");

		off += sec_size;
	}

	/* erase it */
	rc = mtd_erase(mtd, 0, mtd->cfg->size);
	zassert_true(rc == 0, "mtd_erase() fail");

	/* should read back ff all throughout*/
	(void)memset(wd, 0xff, sizeof(wd));
	for (off = 0; off < mtd->cfg->size; off += sizeof(rd)) {
		rc = mtd_read(mtd, off, rd, sizeof(rd));
		zassert_true(rc == 0, "mtd_read() fail");

		rc = memcmp(wd, rd, sizeof(rd));
		zassert_true(rc == 0, "area not erased");
	}
}

void test_mtd_rw(void)
{
	test_mtd_rw_on(MTD_MASTER_GET(flash0));
	test_mtd_rw_on(MTD_PARTITION_GET(image_0));
	test_mtd_rw_on(MTD_PARTITION_GET(mcu_sub));
	test_mtd_rw_on(MTD_PARTITION_GET(image_0_sub));
}

void test_mtd_erased_val(void)
{
	const struct mtd_info *mtd = MTD_PARTITION_GET(image_0);
	const struct flash_parameters *param;
	uint8_t val;
	int rc;

	rc = mtd_get_edv(mtd, &val);
	zassert_true(rc == 0, "mtd_get_edv fail [%d]", rc);

	param = flash_get_parameters(mtd->cfg->device);

	zassert_equal(param->erase_value, val,
		      "value different than the flash erase value");

}

void test_mtd_get_block_on(const struct mtd_info *mtd)
{
	struct mtd_block block;
	off_t off = 0U;
	int rc;

	while (off < mtd->cfg->size) {
		/* get block from start */
		rc = mtd_get_block_at(mtd, off, &block);
		zassert_true(rc == 0, "mtd_get_block_of fail");
		zassert_true(off == block.offset, "wrong block offset");

		/* get block from start + 1 */
		rc = mtd_get_block_at(mtd, off + 1, &block);
		zassert_true(rc == 0, "mtd_get_block_of fail");
		zassert_true(off == block.offset, "wrong block offset");

		/* get block from end */
		rc = mtd_get_block_at(mtd, off + block.size - 1, &block);
		zassert_true(rc == 0, "mtd_get_block_of fail");
		zassert_true(off == block.offset, "wrong block offset");
		off += block.size;
	}
}

void test_mtd_get_block(void)
{
	test_mtd_get_block_on(MTD_PARTITION_GET(image_0));
	test_mtd_get_block_on(MTD_MASTER_GET(flash0));
	test_mtd_get_block_on(MTD_PARTITION_GET(image_0_sub));

}

void test_main(void)
{
	ztest_test_suite(test_mtd,
			 ztest_unit_test(test_mtd_erased_val),
			 ztest_unit_test(test_mtd_get_block),
			 ztest_unit_test(test_mtd_rw)
	);
	ztest_run_test_suite(test_mtd);
}
