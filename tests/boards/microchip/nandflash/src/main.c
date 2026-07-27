/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/flash/mchp_nand_g1_api.h>

#define NAND_NODE DT_NODELABEL(nand0)

#define TEST_PATTERN 0xaa55aa55U

ZTEST(nandflash, test_nandflash)
{
	const struct device *const dev = DEVICE_DT_GET(NAND_NODE);
	struct nand_info nand;
	char *page_buf, *oob_buf;
	char *verify;
	int page = 0, block = -1;
	int has_bad = 0;
	int i, j, k;
	int ret;

	printf("NAND Flash test on %s\n", CONFIG_BOARD_TARGET);
	zassert_true(device_is_ready(dev), "NAND device is not ready!");

	nand_info(dev, &nand);
	printf("\nNAND Flash info:\n");
	printf(" pagesize   = %d\n", nand.pagesize);
	printf(" oobsize    = %d\n", nand.oobsize);
	printf(" blockpages = %d\n", nand.blockpages);
	printf(" blocknum   = %d\n", nand.blocknum);

	page_buf = k_aligned_alloc(sys_cache_data_line_size_get(), nand.pagesize + nand.oobsize);
	zassert_not_null(page_buf, "Malloc page buffer failed!");

	oob_buf = page_buf + nand.pagesize;
	memset(page_buf, 0, nand.pagesize + nand.oobsize);

	verify = k_aligned_alloc(sys_cache_data_line_size_get(), nand.pagesize);
	zassert_not_null(verify, "Malloc verify buffer failed!");

	printf("\nScaning bad block...\n");
	for (i = 0; i < nand.blocknum; i++) {
		for (j = 0; j < 2; j++) {
			ret = nand_read_oob(dev, i * nand.blockpages + j, oob_buf);
			zassert_ok(ret, "Failed to read oob in page %d, ret=%d\n",
					i * nand.blockpages + j, ret);

			if (oob_buf[0] != 0xff) {
				printf(" block %d is bad\n", i);

				has_bad = 1;
				break;
			}
		}

		/* Try to find a block other than block 0 for testing */
		if (((block == -1) || (block == 0)) &&
		    (j == 2)) {
			block = i;
			page  = i * nand.blockpages;
		}
	}

	if (!has_bad) {
		printf(" No bad block found\n");
	}

	zassert_not_equal(block, -1, "Error, no good block found for test!\n");

	printf("\nErase test on block %d\n", block);
	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to rease block %d, ret=%d\n", block, ret);

	for (i = 0; i < nand.blockpages; i++) {
		ret = nand_read_page(dev, page + i, page_buf, 0);
		zassert_true(ret >= 0, "Failed to read page %d, ret=%d\n", page + i, ret);

		for (j = 0; j < nand.pagesize; j++) {
			zassert_equal(page_buf[j], 0xff,
				      "Error, bytes %d in page %d is 0x%02x (not 0xff)!\n",
				      j, page + i, page_buf[j]);
		}
	}
	printf(" Test done\n");

	printf("\nProgram test on block %d\n", block);
	k = 0;
	for (i = 0; i < nand.blockpages; i++) {
		for (j = 0; j < (nand.pagesize / 4); j++, k++) {
			((int *)page_buf)[j] = k | (TEST_PATTERN & ~((1 << ROUND_UP(find_msb_set(k), 8)) - 1));
		}

		ret = nand_write_page(dev, page + i, page_buf, 0);
		zassert_ok(ret, "Failed to write page %d, ret=%d\n", page + i, ret);
	}

	k = 0;
	for (i = 0; i < nand.blockpages; i++) {
		ret = nand_read_page(dev, page + i, page_buf, 0);
		zassert_true(ret >= 0, "Failed to read page %d, ret=%d\n", page + i, ret);

		for (j = 0; j < (nand.pagesize / 4); j++, k++) {
			zassert_equal(((int *)page_buf)[j],
				      k | (TEST_PATTERN & ~((1 << ROUND_UP(find_msb_set(k), 8)) - 1)),
				      "Error, data[%d - %d] in page %d is 0x%08x (not 0x%08x)!\n",
				      j * 4, j * 4 + 3, page + i, ((int *)page_buf)[j], k - 1);
		}
	}
	printf(" Test done\n");

	printf("\nECC test on page %d\n", page);
	ret = nand_read_page_raw(dev, page, page_buf, 1);
	zassert_ok(ret, "Failed to read page %d, ret=%d\n", page, ret);

	for (i = 0; i < nand.eccbits; i++) {
		printf(" Flip bit %d of data[%d]: 0x%02x -> 0x%02x\n",
		       i & 0x7, i, page_buf[i], page_buf[i] ^ (unsigned char)BIT(i & 0x7));
		page_buf[i] ^= (unsigned char)BIT(i & 0x7);
	}

	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to rease block %d, ret=%d\n", block, ret);

	ret = nand_write_page_raw(dev, page, page_buf, 1);
	zassert_ok(ret, "Failed to write page %d, ret=%d\n", page, ret);

	ret = nand_read_page_raw(dev, page, page_buf, 0);
	zassert_ok(ret, "Failed to read page %d, ret=%d\n", page, ret);

	ret = nand_read_page(dev, page, verify, 0);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d\n", page, ret);

	ret = 0;
	for (i = 0; i < nand.eccbits; i++) {
		int status = ((page_buf[i] ^ (unsigned char)BIT(i & 0x7)) == verify[i]);

		printf(" Verify data[%d] raw: 0x%02x, ecc: 0x%02x [%s]\n",
		       i, page_buf[i], verify[i], status ? "OK" : "FAIL");

		if (!status) {
			ret = -1;
		}
	}
	zassert_ok(ret, "Failed to correct some corrupted bits\n");

	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to erase block %d, ret=%d\n", block, ret);

	printf(" Test done\n");
	printf("\nBlock %d has been erased\n", block);
}

ZTEST_SUITE(nandflash, NULL, NULL, NULL, NULL, NULL);
