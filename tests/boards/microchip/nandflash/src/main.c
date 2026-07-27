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

#define F_ZERO	0
#define F_FF	1
#define F_I	2

static int verify_data(const unsigned char *buf, size_t size, unsigned int flag)
{
	unsigned char expected;

	zassert_true(flag == F_ZERO || flag == F_FF || flag == F_I,
		     "Invalid verify flag %u", flag);

	expected = (flag == F_FF) ? 0xff : 0;

	for (size_t i = 0; i < size; i++) {
		if (flag == F_I) {
			expected = (unsigned char)(i & 0xff);
		}

		if (buf[i] != expected) {
			printf(" error: data[%zu]=0x%02x (expected 0x%02x)\n",
			       i, buf[i], expected);
			return -1;
		}
	}

	return 0;
}

static int verify_page_data(const unsigned char *buf,
			    size_t page_size, unsigned int page_flag,
			    size_t oob_size, unsigned int oob_flag)
{
	int ret;

	ret = verify_data(buf, page_size, page_flag);
	if (ret) {
		printf(" page data error\n");
		return -1;
	}

	ret = verify_data(buf + page_size, oob_size, oob_flag);
	if (ret) {
		printf(" oob data error\n");
		return -1;
	}

	return 0;
}

ZTEST(nandflash, test_nandflash)
{
	const struct device *const dev = DEVICE_DT_GET(NAND_NODE);
	struct nand_info nand;
	unsigned char *page_buf, *oob_buf;
	unsigned char *verify;
	int page = 0, block = -1;
	int has_bad = 0;
	int i, j, k;
	int ret;

	printf("NAND Flash test on %s\n", CONFIG_BOARD_TARGET);
	zassert_true(device_is_ready(dev), "NAND device is not ready!");

	/*
	 * Test: Retrieve NAND Flash information.
	 * API under test:
	 * - nand_info()
	 */
	nand_info(dev, &nand);
	printf("\nNAND Flash info:\n");
	printf(" pagesize   = %d\n", nand.pagesize);
	printf(" oobsize    = %d\n", nand.oobsize);
	printf(" blockpages = %d\n", nand.blockpages);
	printf(" blocknum   = %d\n", nand.blocknum);
	printf(" eccbits    = %d\n", nand.eccbits);
	printf(" eccsize    = %d\n", nand.eccsize);
	printf(" eccaddr    = %d\n", nand.eccaddr);

	page_buf = k_aligned_alloc(sys_cache_data_line_size_get(), nand.pagesize + nand.oobsize);
	zassert_not_null(page_buf, "Malloc page buffer failed!");
	oob_buf = page_buf + nand.pagesize;

	verify = k_aligned_alloc(sys_cache_data_line_size_get(), nand.pagesize + nand.oobsize);
	zassert_not_null(verify, "Malloc verify buffer failed!");

	/*
	 * Test: Scanning bad block.
	 * API under test:
	 * - nand_read_oob()
	 */
	printf("\nScaning bad block...\n");

	for (i = 0; i < nand.blocknum; i++) {
		for (j = 0; j < 2; j++) {
			memset(oob_buf, 0, nand.oobsize);
			ret = nand_read_oob(dev, i * nand.blockpages + j, oob_buf);
			zassert_ok(ret, "Failed to read oob in page %d, ret=%d!",
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
		printf(" No bad block found.\n");
	}

	zassert_not_equal(block, -1, "Error, no good block found for test!");

	/*
	 * Test: Block erase test.
	 * APIs under test:
	 * - nand_erase()
	 * - nand_read_page()
	 */
	printf("\nBlock erase test on block %d:\n", block);

	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to rease block %d, ret=%d!", block, ret);

	for (i = 0; i < nand.blockpages; i++) {
		memset(page_buf, 0, nand.pagesize + nand.oobsize);
		ret = nand_read_page(dev, page + i, page_buf, 1);
		zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page + i, ret);

		ret = verify_data(page_buf, nand.pagesize + nand.oobsize, F_FF);
		zassert_true(ret == 0, "Failed to verify data in page %d!", page);
	}
	printf(" Test done.\n");

	/*
	 * Test: Block mark bad test.
	 * APIs under test:
	 * - nand_erase()
	 * - nand_read_oob()
	 * - nand_write_oob()
	 */
	printf("\nBlock mark bad test on block %d:\n", block);

	memset(oob_buf, 0xff, nand.oobsize);
	oob_buf[0] = 0;
	ret = nand_write_oob(dev, page, oob_buf);
	zassert_ok(ret, "Failed to write oob in page %d, ret=%d!", page, ret);

	memset(oob_buf, 0xff, nand.oobsize);
	ret = nand_read_oob(dev, page, oob_buf);
	zassert_ok(ret, "Failed to read oob in page %d, ret=%d!", page, ret);

	zassert_true(oob_buf[0] != 0xff, "Failed to mark block %d bad (oob[0]=0x%02x)!",
					 block, oob_buf[0]);
	printf(" block %d has been marked as bad.\n", block);

	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to rease block %d, ret=%d!", block, ret);
	printf(" block %d has been erased (scrubbed).\n", block);

	memset(oob_buf, 0, nand.oobsize);
	ret = nand_read_oob(dev, page, oob_buf);
	zassert_ok(ret, "Failed to read oob in page %d, ret=%d!", page, ret);

	zassert_true(oob_buf[0] == 0xff, "Failed: block %d is not marked as good (oob[0]=0x%02x)!",
					 block, oob_buf[0]);
	printf(" block %d is now marked as good.\n", block);
	printf(" Test done.\n");

	/*
	 * Test: Page program test.
	 * APIs under test:
	 * - nand_read_page()
	 * - nand_write_page()
	 */
	printf("\nPage program test on block %d:\n", block);

	k = 0;
	for (i = 0; i < nand.blockpages; i++) {
		for (j = 0; j < (nand.pagesize / 4); j++, k++) {
			((int *)page_buf)[j] = k | (TEST_PATTERN &
						    ~((1 << ROUND_UP(find_msb_set(k), 8)) - 1));
		}

		ret = nand_write_page(dev, page + i, page_buf, 0);
		zassert_ok(ret, "Failed to write page %d, ret=%d!", page + i, ret);
	}

	k = 0;
	for (i = 0; i < nand.blockpages; i++) {
		memset(page_buf, 0, nand.pagesize);
		ret = nand_read_page(dev, page + i, page_buf, 0);
		zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page + i, ret);

		for (j = 0; j < (nand.pagesize / 4); j++, k++) {
			zassert_equal(((int *)page_buf)[j],
				      k | (TEST_PATTERN &
					   ~((1 << ROUND_UP(find_msb_set(k), 8)) - 1)),
				      "Failed: data[%d - %d] in page %d is 0x%08x (not 0x%08x)",
				      j * 4, j * 4 + 3, page + i, ((int *)page_buf)[j], k - 1);
		}
	}
	printf(" Test done.\n");

	/*
	 * Test: Page ECC test.
	 * APIs under test:
	 * - nand_erase()
	 * - nand_read_page()
	 * - nand_read_page_raw()
	 * - nand_write_page_raw()
	 */
	printf("\nPage ECC test on page %d:\n", page);

	memset(page_buf, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page_raw(dev, page, page_buf, 1);
	zassert_ok(ret, "Failed to read page %d, ret=%d!", page, ret);

	for (i = 0; i < nand.eccbits; i++) {
		printf(" Flip bit %d of data[%d]: 0x%02x -> 0x%02x\n",
		       i & 0x7, i, page_buf[i], page_buf[i] ^ (unsigned char)BIT(i & 0x7));
		page_buf[i] ^= (unsigned char)BIT(i & 0x7);
	}

	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to rease block %d, ret=%d!", block, ret);

	ret = nand_write_page_raw(dev, page, page_buf, 1);
	zassert_ok(ret, "Failed to write page %d, ret=%d!", page, ret);

	memset(page_buf, 0, nand.pagesize);
	ret = nand_read_page_raw(dev, page, page_buf, 0);
	zassert_ok(ret, "Failed to read page %d, ret=%d!", page, ret);

	memset(verify, 0, nand.pagesize);
	ret = nand_read_page(dev, page, verify, 0);
	zassert_true(ret == nand.eccbits, "Failed to read page %d, ret=%d!", page, ret);

	ret = 0;
	for (i = 0; i < nand.eccbits; i++) {
		int status = ((page_buf[i] ^ (unsigned char)BIT(i & 0x7)) == verify[i]);

		printf(" Verify data[%d] raw: 0x%02x, ecc: 0x%02x [%s]\n",
		       i, page_buf[i], verify[i], status ? "OK" : "FAIL");

		if (!status) {
			ret = -1;
		}
	}
	zassert_ok(ret, "Failed to correct some corrupted bits!");
	printf(" Test done.\n");

	/*
	 * Test: Page read/write APIs.
	 * API under test:
	 * - nand_read_page()
	 * - nand_write_page()
	 * - nand_read_page_raw()
	 * - nand_write_page_raw()
	 * - nand_read_oob()
	 * - nand_write_oob()
	 */
	printf("\nPage read/write APIs test on page %d - %d:\n", page, page + 4);

	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to rease block %d, ret=%d!", block, ret);

	memset(page_buf, 0, nand.pagesize + nand.oobsize);
	for (i = 0; i < nand.pagesize; i++) {
		page_buf[i] = i & 0xff;
	}
	for (i = 0; i < nand.oobsize; i++) {
		oob_buf[i] = i & 0xff;
	}

	/*
	 * - nand_read_page()
	 * - nand_write_page()
	 */
	ret = nand_write_page(dev, page, page_buf, 0);
	zassert_ok(ret, "Failed to write page %d, ret=%d!", page, ret);

	memset(verify, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page(dev, page, verify, 0);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_page_data(verify, nand.pagesize, F_I, nand.oobsize, F_ZERO);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);

	memset(verify, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page(dev, page, verify, 1);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_page_data(verify, nand.pagesize, F_I, nand.eccaddr, F_FF);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);

	/*
	 * - nand_read_page() with oob_req
	 * - nand_write_page() with oob_req
	 */
	page++;
	ret = nand_write_page(dev, page, page_buf, 1);
	zassert_ok(ret, "Failed to write page %d, ret=%d!", page, ret);

	memset(verify, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page(dev, page, verify, 1);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_page_data(verify, nand.pagesize, F_I, nand.eccaddr, F_I);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);

	/*
	 * - nand_read_page_raw()
	 * - nand_write_page_raw()
	 */
	page++;
	ret = nand_write_page_raw(dev, page, page_buf, 0);
	zassert_ok(ret, "Failed to write page %d, ret=%d!", page, ret);

	memset(verify, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page_raw(dev, page, verify, 0);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_page_data(verify, nand.pagesize, F_I, nand.oobsize, F_ZERO);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);

	memset(verify, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page_raw(dev, page, verify, 1);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_page_data(verify, nand.pagesize, F_I, nand.oobsize, F_FF);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);

	/*
	 * - nand_read_page_raw() with oob_req
	 * - nand_write_page_raw() with oob_req
	 */
	page++;
	ret = nand_write_page_raw(dev, page, page_buf, 1);
	zassert_ok(ret, "Failed to write page %d, ret=%d!", page, ret);

	memset(verify, 0, nand.pagesize + nand.oobsize);
	ret = nand_read_page_raw(dev, page, verify, 1);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_page_data(verify, nand.pagesize, F_I, nand.oobsize, F_I);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);

	/*
	 * - nand_read_oob()
	 * - nand_write_oob()
	 */
	page++;
	ret = nand_write_oob(dev, page, oob_buf);
	zassert_ok(ret, "Failed to write page %d, ret=%d!", page, ret);

	memset(verify, 0, nand.oobsize);
	ret = nand_read_oob(dev, page, verify);
	zassert_true(ret >= 0, "Failed to read page %d, ret=%d!", page, ret);
	ret = verify_data(verify, nand.pagesize, F_I);
	zassert_true(ret == 0, "Failed to verify data in page %d!", page);
	printf(" Test done.\n");

	/*
	 * Test: Finished.
	 * API under test:
	 * - nand_erase()
	 */
	printf("\nTest finished on block %d:\n", block);
	ret = nand_erase(dev, block);
	zassert_ok(ret, "Failed to erase block %d, ret=%d!", block, ret);
	printf(" block %d has been erased.\n", block);
}

ZTEST_SUITE(nandflash, NULL, NULL, NULL, NULL, NULL);
