/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "fcb_test.h"
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>

struct fcb test_fcb = {0};
#if defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
struct fcb test_fcb_crc_disabled = { .f_flags = FCB_FLAGS_CRC_DISABLED };
#endif

uint8_t fcb_test_erase_value;

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	#define SECTOR_SIZE 0x20000 /* 128K */
#else
	#define SECTOR_SIZE 0x4000 /* 16K */
#endif

/* Sectors for FCB are defined far from application code
 * area. This test suite is the non bootable application so 1. image slot is
 * suitable for it.
 */
struct flash_sector test_fcb_sector[] = {
	[0] = {
		.fs_off = 0,
		.fs_size = SECTOR_SIZE
	},
	[1] = {
		.fs_off = SECTOR_SIZE,
		.fs_size = SECTOR_SIZE
	},
	[2] = {
		.fs_off = 2 * SECTOR_SIZE,
		.fs_size = SECTOR_SIZE
	},
	[3] = {
		.fs_off = 3 * SECTOR_SIZE,
		.fs_size = SECTOR_SIZE
	}
};


void test_fcb_wipe(void)
{
	int i;
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(TEST_FCB_FLASH_AREA_ID, &fap);
	zassert_true(rc == 0, "flash area open call failure");

	for (i = 0; i < ARRAY_SIZE(test_fcb_sector); i++) {
		rc = flash_area_flatten(fap, test_fcb_sector[i].fs_off,
				      test_fcb_sector[i].fs_size);
		zassert_true(rc == 0, "erase call failure");
	}
}

int fcb_test_empty_walk_cb(struct fcb_entry_ctx *entry_ctx, void *arg)
{
	zassert_unreachable("fcb_test_empty_walk_cb");
	return 0;
}

uint8_t fcb_test_append_data(int msg_len, int off)
{
	return (msg_len ^ off);
}

int fcb_test_data_walk_cb(struct fcb_entry_ctx *entry_ctx, void *arg)
{
	uint16_t len;
	uint8_t test_data[128];
	int rc;
	int i;
	int *var_cnt = (int *)arg;

	len = entry_ctx->loc.fe_data_len;

	zassert_true(len == *var_cnt, "");

	rc = flash_area_read(entry_ctx->fap,
			     FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc),
			     test_data, len);
	zassert_true(rc == 0, "read call failure");

	for (i = 0; i < len; i++) {
		zassert_true(test_data[i] == fcb_test_append_data(len, i),
		"fcb_test_append_data redout misrepresentation");
	}
	(*var_cnt)++;
	return 0;
}

int fcb_test_cnt_elems_cb(struct fcb_entry_ctx *entry_ctx, void *arg)
{
	struct append_arg *aa = (struct append_arg *)arg;
	int idx;

	idx = entry_ctx->loc.fe_sector - &test_fcb_sector[0];
	aa->elem_cnts[idx]++;
	return 0;
}

void fcb_tc_pretest(int sectors, struct fcb *_fcb)
{
	int rc = 0;

	test_fcb_wipe();
	_fcb->f_erase_value = fcb_test_erase_value;
	_fcb->f_sector_cnt = sectors;
	_fcb->f_sectors = test_fcb_sector; /* XXX */

	rc = 0;
	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, _fcb);
	if (rc != 0) {
		printf("%s rc == %xm, %d\n", __func__, rc, rc);
		zassert_true(rc == 0, "fbc initialization failure");
	}
}

static void fcb_pretest_2_sectors(void *data)
{
	fcb_tc_pretest(2, &test_fcb);
}

static void fcb_pretest_4_sectors(void *data)
{
	fcb_tc_pretest(4, &test_fcb);
}

static void fcb_pretest_crc_disabled(void *data)
{
#if defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
	fcb_tc_pretest(2, &test_fcb_crc_disabled);
#else
	ztest_test_skip();
#endif
}

/*
 * This actually is not a test; the function gets erase value from flash
 * parameters, of the flash device that is used by tests, and stores it in
 * global fcb_test_erase_value.
 */
ZTEST(fcb_test_without_set, test_get_flash_erase_value)
{
	const struct flash_area *fa;
	const struct flash_parameters *fp;
	const struct device *dev;
	int rc = 0;

	rc = flash_area_open(TEST_FCB_FLASH_AREA_ID, &fa);
	zassert_equal(rc, 0, "Failed top open flash area");

	dev = fa->fa_dev;
	flash_area_close(fa);

	zassert_true(dev != NULL, "Failed to obtain device");

	fp = flash_get_parameters(dev);
	zassert_true(fp != NULL, "Failed to get flash device parameters");

	fcb_test_erase_value = fp->erase_value;
}

ZTEST_SUITE(fcb_test_without_set, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(fcb_test_with_2sectors_set, NULL, NULL, fcb_pretest_2_sectors, NULL, NULL);
ZTEST_SUITE(fcb_test_with_4sectors_set, NULL, NULL, fcb_pretest_4_sectors, NULL, NULL);

ZTEST_SUITE(fcb_test_crc_disabled, NULL, NULL, fcb_pretest_crc_disabled, NULL, NULL);
