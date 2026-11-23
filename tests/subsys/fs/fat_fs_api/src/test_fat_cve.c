/*
 * Copyright (c) 2026 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Regression tests for vendored FatFs (ff.c) security fixes that are only
 * reachable through the FatFs API directly. The Zephyr fs_* layer clamps
 * fs_seek() to the current file size and zero-fills fs_truncate() growth, so
 * those wrappers cannot reach the underlying bug; these tests therefore drive
 * ff.c's f_lseek() extend path directly on the already-mounted volume.
 */

#include "test_fat.h"
#include <string.h>

#define CVE_FILE DISK_NAME ":/cve6686.bin"

/*
 * CVE-2026-6686 (GHSA-h94p-vjqx-v7rr): extending a file past end-of-file with
 * f_lseek() must not expose residual on-disk data. The newly in-file range has
 * to read back as zeros, covering both freshly allocated clusters and the
 * unused tail (slack) of an already-allocated cluster.
 */
static int test_cve_2026_6686(void)
{
	static FIL fp;
	static uint8_t buf[512];
	int res;
	unsigned int bw, br;

	TC_PRINT("\nCVE-2026-6686 (f_lseek past-EOF zeroing):\n");

	/* Scenario 1: extend an empty file; the whole range must read zeroed. */
	res = f_open(&fp, CVE_FILE, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
	if (res != FR_OK) {
		TC_PRINT("f_open failed [%d]\n", res);
		return TC_FAIL;
	}
	res = f_lseek(&fp, 400);
	if (res != FR_OK || f_size(&fp) != 400) {
		TC_PRINT("f_lseek extend failed [%d]\n", res);
		f_close(&fp);
		return TC_FAIL;
	}
	f_lseek(&fp, 0);
	memset(buf, 0xAA, sizeof(buf));
	res = f_read(&fp, buf, 400, &br);
	if (res != FR_OK || br != 400) {
		TC_PRINT("f_read failed [%d] (%u)\n", res, br);
		f_close(&fp);
		return TC_FAIL;
	}
	for (int i = 0; i < 400; i++) {
		if (buf[i] != 0) {
			TC_PRINT("empty-extend leaked residual at %d (0x%02x)\n", i, buf[i]);
			f_close(&fp);
			return TC_FAIL;
		}
	}
	f_close(&fp);

	/*
	 * Scenario 2 (partial-cluster / straddle): write real data, truncate so
	 * its tail becomes unused slack that stays on disk, then re-extend within
	 * the same cluster. The re-exposed slack must read zeroed while the kept
	 * byte survives. This is the case whole-cluster zeroing alone would miss.
	 */
	res = f_open(&fp, CVE_FILE, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
	if (res != FR_OK) {
		TC_PRINT("f_open(2) failed [%d]\n", res);
		return TC_FAIL;
	}
	memset(buf, 0xCC, 400);
	res = f_write(&fp, buf, 400, &bw);
	if (res != FR_OK || bw != 400) {
		TC_PRINT("f_write failed [%d] (%u)\n", res, bw);
		f_close(&fp);
		return TC_FAIL;
	}
	f_lseek(&fp, 1);
	res = f_truncate(&fp);		/* objsize == 1, [1, 400) slack still on disk */
	if (res != FR_OK) {
		TC_PRINT("f_truncate failed [%d]\n", res);
		f_close(&fp);
		return TC_FAIL;
	}
	res = f_lseek(&fp, 300);		/* re-extend within the same cluster */
	if (res != FR_OK) {
		TC_PRINT("f_lseek re-extend failed [%d]\n", res);
		f_close(&fp);
		return TC_FAIL;
	}
	f_lseek(&fp, 0);
	memset(buf, 0xAA, sizeof(buf));
	res = f_read(&fp, buf, 300, &br);
	if (res != FR_OK || br != 300) {
		TC_PRINT("f_read(2) failed [%d] (%u)\n", res, br);
		f_close(&fp);
		return TC_FAIL;
	}
	f_close(&fp);
	(void)f_unlink(CVE_FILE);

	if (buf[0] != 0xCC) {
		TC_PRINT("straddle kept byte clobbered (0x%02x)\n", buf[0]);
		return TC_FAIL;
	}
	for (int i = 1; i < 300; i++) {
		if (buf[i] != 0) {
			TC_PRINT("straddle leaked slack at %d (0x%02x)\n", i, buf[i]);
			return TC_FAIL;
		}
	}

	TC_PRINT("CVE-2026-6686: PASS\n");
	return TC_PASS;
}

void test_fat_cve(void)
{
	zassert_true(test_cve_2026_6686() == TC_PASS);
}
