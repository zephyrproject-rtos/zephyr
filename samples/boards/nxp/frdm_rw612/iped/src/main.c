/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>

#include <string.h>

/* IPED region boundaries — must match provisioning script */
#ifndef IPED_REGION_START
#define IPED_REGION_START 0x1000U
#endif
#ifndef IPED_REGION_END
#define IPED_REGION_END 0x18000U
#endif

/* Erase/write test area: last 4KB sector of IPED region 0 (padding area) */
#define IPED_ERASE_TEST_OFF  (IPED_REGION_END - 0x1000U)
#define IPED_ERASE_TEST_SIZE 0x1000U

/* IPED region 1 (CTR mode) — must match provisioning script */
#ifndef IPED_REGION1_START
#define IPED_REGION1_START 0x320000U
#endif
#ifndef IPED_REGION1_END
#define IPED_REGION1_END 0x324000U
#endif

/* FlexSPI IPEDCTRL: IPWR_EN enables CTR mode IP-command write encryption */
#define FLEXSPI_IPEDCTRL_ADDR 0x5013442CU
#define IPWR_EN_BIT           (1U << 2)

#define STORAGE_PARTITION    storage_partition
#define STORAGE_PARTITION_ID PARTITION_ID(STORAGE_PARTITION)
#define STORAGE_TEST_SIZE    4096U
#define LARGE_READ_MAX       (64U * 1024U)

#define TEST_PATTERN_START 0xA5

static uint8_t wr_buf[STORAGE_TEST_SIZE];
static uint8_t rd_buf[LARGE_READ_MAX];
static uint8_t rd_buf2[STORAGE_TEST_SIZE];

struct iped_flash_fixture {
	const struct device *flash_dev;
};

static void *iped_flash_setup(void)
{
	static struct iped_flash_fixture f;

	f.flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_true(device_is_ready(f.flash_dev), "flash device not ready");
	return &f;
}

static bool buf_all_val(const uint8_t *buf, size_t len, uint8_t val)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != val) {
			return false;
		}
	}
	return true;
}

ZTEST_F(iped_flash, test_ahb_read_start)
{
	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, rd_buf, 256);

	zassert_equal(rc, 0, "flash_read failed");
	zassert_false(buf_all_val(rd_buf, 256, 0x00), "all zeros");
	zassert_false(buf_all_val(rd_buf, 256, 0xFF), "all 0xFF");
}

ZTEST_F(iped_flash, test_ahb_read_mid)
{
	off_t mid = IPED_REGION_START + (IPED_REGION_END - IPED_REGION_START) / 2;

	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, mid, rd_buf, 256);

	zassert_equal(rc, 0, "flash_read failed");
	zassert_false(buf_all_val(rd_buf, 256, 0x00), "all zeros");
	zassert_false(buf_all_val(rd_buf, 256, 0xFF), "all 0xFF");
}

ZTEST_F(iped_flash, test_ahb_read_near_end)
{
	off_t near_end = IPED_REGION_END - 256;

	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, near_end, rd_buf, 256);

	zassert_equal(rc, 0, "flash_read failed");
}

ZTEST_F(iped_flash, test_ahb_read_single_byte)
{
	uint8_t byte;
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START + 0x100, &byte, 1);

	zassert_equal(rc, 0, "flash_read failed");
}

ZTEST_F(iped_flash, test_ahb_read_4k)
{
	off_t off = IPED_REGION_START + 0x1000;

	if ((off + STORAGE_TEST_SIZE) > IPED_REGION_END) {
		ztest_test_skip();
	}

	memset(rd_buf, 0, STORAGE_TEST_SIZE);
	int rc = flash_read(fixture->flash_dev, off, rd_buf, STORAGE_TEST_SIZE);

	zassert_equal(rc, 0, "flash_read failed");
	zassert_false(buf_all_val(rd_buf, STORAGE_TEST_SIZE, 0x00), "all zeros");
}

ZTEST_F(iped_flash, test_ahb_read_unaligned)
{
	off_t off = IPED_REGION_START + 0x503;

	memset(rd_buf, 0, 37);
	int rc = flash_read(fixture->flash_dev, off, rd_buf, 37);

	zassert_equal(rc, 0, "flash_read failed");
}

ZTEST_F(iped_flash, test_ahb_read_odd_size)
{
	memset(rd_buf, 0, 127);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START + 0x200, rd_buf, 127);

	zassert_equal(rc, 0, "flash_read failed");
}

ZTEST_F(iped_flash, test_ahb_read_size_sweep)
{
	static const size_t sizes[] = {0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400};

	for (size_t s = 0; s < ARRAY_SIZE(sizes); s++) {
		memset(rd_buf, 0, sizes[s]);
		int rc =
			flash_read(fixture->flash_dev, IPED_REGION_START + 0x400, rd_buf, sizes[s]);
		zassert_equal(rc, 0, "size 0x%zx read failed", sizes[s]);
	}
}

ZTEST_F(iped_flash, test_ahb_read_16k)
{
	if ((IPED_REGION_START + 16384) > IPED_REGION_END) {
		ztest_test_skip();
	}

	memset(rd_buf, 0, 16384);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, rd_buf, 16384);

	zassert_equal(rc, 0, "flash_read failed");
	zassert_false(buf_all_val(rd_buf, 16384, 0x00), "all zeros");
}

ZTEST_F(iped_flash, test_ahb_read_64k)
{
	if ((IPED_REGION_START + LARGE_READ_MAX) > IPED_REGION_END) {
		ztest_test_skip();
	}

	memset(rd_buf, 0, LARGE_READ_MAX);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, rd_buf, LARGE_READ_MAX);

	zassert_equal(rc, 0, "flash_read failed");
	zassert_false(buf_all_val(rd_buf, LARGE_READ_MAX, 0x00), "all zeros");
}

ZTEST_F(iped_flash, test_consistency_3x_read)
{
	off_t addr = IPED_REGION_START + 0x2000;
	uint8_t r1[64], r2[64], r3[64];

	zassert_equal(flash_read(fixture->flash_dev, addr, r1, 64), 0);
	zassert_equal(flash_read(fixture->flash_dev, addr, r2, 64), 0);
	zassert_equal(flash_read(fixture->flash_dev, addr, r3, 64), 0);
	zassert_mem_equal(r1, r2, 64, "read 1 != read 2");
	zassert_mem_equal(r2, r3, 64, "read 2 != read 3");
}

ZTEST_F(iped_flash, test_stability_100x_read)
{
	off_t addr = IPED_REGION_START + 0x2000;
	uint8_t ref[64], tmp[64];

	zassert_equal(flash_read(fixture->flash_dev, addr, ref, 64), 0);

	for (int i = 1; i < 100; i++) {
		zassert_equal(flash_read(fixture->flash_dev, addr, tmp, 64), 0);
		zassert_mem_equal(ref, tmp, 64, "mismatch at iteration %d", i);
	}
}

ZTEST_F(iped_flash, test_chunk_consistency)
{
	off_t addr = IPED_REGION_START + 0x4000;
	uint8_t whole[128], chunks[128];

	zassert_equal(flash_read(fixture->flash_dev, addr, whole, 128), 0);

	memset(chunks, 0, 128);
	for (size_t i = 0; i < 128; i += 16) {
		zassert_equal(flash_read(fixture->flash_dev, addr + i, chunks + i, 16), 0);
	}

	zassert_mem_equal(whole, chunks, 128, "whole read != reassembled chunks");
}

ZTEST_F(iped_flash, test_boundary_read_before_iped)
{
	memset(rd_buf, 0, 64);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START - 64, rd_buf, 64);

	zassert_equal(rc, 0, "read ending at IPED start should succeed");
}

ZTEST_F(iped_flash, test_boundary_read_after_iped)
{
	memset(rd_buf, 0, 64);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_END, rd_buf, 64);

	zassert_equal(rc, 0, "read starting at IPED end should succeed");
}

ZTEST_F(iped_flash, test_boundary_cross_start)
{
	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START - 128, rd_buf, 256);

	zassert_equal(rc, -EINVAL, "cross-start overlap must return -EINVAL");
}

ZTEST_F(iped_flash, test_boundary_cross_end)
{
	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_END - 128, rd_buf, 256);

	zassert_equal(rc, -EINVAL, "cross-end overlap must return -EINVAL");
}

ZTEST_F(iped_flash, test_boundary_overlap_start_1byte)
{
	memset(rd_buf, 0, 2);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START - 1, rd_buf, 2);

	zassert_equal(rc, -EINVAL, "1-byte overlap at start must return -EINVAL");
}

ZTEST_F(iped_flash, test_boundary_overlap_end_1byte)
{
	memset(rd_buf, 0, 2);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_END - 1, rd_buf, 2);

	zassert_equal(rc, -EINVAL, "1-byte overlap at end must return -EINVAL");
}

ZTEST_F(iped_flash, test_non_iped_fcb_magic)
{
	memset(rd_buf, 0, 16);
	int rc = flash_read(fixture->flash_dev, 0x400, rd_buf, 16);

	zassert_equal(rc, 0, "flash_read failed");
	zassert_equal(rd_buf[0], 'F');
	zassert_equal(rd_buf[1], 'C');
	zassert_equal(rd_buf[2], 'F');
	zassert_equal(rd_buf[3], 'B');
}

ZTEST_F(iped_flash, test_non_iped_offset_zero)
{
	memset(rd_buf, 0, 16);
	int rc = flash_read(fixture->flash_dev, 0, rd_buf, 16);

	zassert_equal(rc, 0, "flash_read at offset 0 failed");
}

ZTEST_F(iped_flash, test_non_iped_beyond_region)
{
	memset(rd_buf, 0, 16);
	int rc = flash_read(fixture->flash_dev, 0x20000, rd_buf, 16);

	zassert_equal(rc, 0, "flash_read at 0x20000 failed");
}

ZTEST(iped_flash, test_storage_erase_write_cycle)
{
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(STORAGE_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "flash_area_open failed");

	rc = flash_area_erase(fa, 0, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "erase failed");

	memset(rd_buf, 0x00, STORAGE_TEST_SIZE);
	rc = flash_area_read(fa, 0, rd_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "read after erase failed");
	zassert_true(buf_all_val(rd_buf, STORAGE_TEST_SIZE, 0xFF), "erased data not all 0xFF");

	for (size_t i = 0; i < STORAGE_TEST_SIZE; i++) {
		wr_buf[i] = (uint8_t)(TEST_PATTERN_START + i);
	}
	rc = flash_area_write(fa, 0, wr_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "write failed");

	memset(rd_buf, 0x00, STORAGE_TEST_SIZE);
	rc = flash_area_read(fa, 0, rd_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "read-back failed");
	zassert_mem_equal(rd_buf, wr_buf, STORAGE_TEST_SIZE, "written data mismatch");

	rc = flash_area_erase(fa, 0, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "re-erase failed");

	memset(rd_buf, 0x00, STORAGE_TEST_SIZE);
	rc = flash_area_read(fa, 0, rd_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "read after re-erase failed");
	zassert_true(buf_all_val(rd_buf, STORAGE_TEST_SIZE, 0xFF), "re-erased data not all 0xFF");

	flash_area_close(fa);
}

ZTEST_F(iped_flash, test_edge_zero_length_read)
{
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, rd_buf, 0);

	zassert_equal(rc, 0, "zero-length read should be no-op");
}

ZTEST_F(iped_flash, test_edge_last_byte)
{
	uint8_t byte;
	int rc = flash_read(fixture->flash_dev, IPED_REGION_END - 1, &byte, 1);

	zassert_equal(rc, 0, "read last byte of IPED failed");
}

ZTEST_F(iped_flash, test_edge_first_byte)
{
	uint8_t byte;
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, &byte, 1);

	zassert_equal(rc, 0, "read first byte of IPED failed");
}

ZTEST_F(iped_flash, test_edge_byte_after_iped)
{
	uint8_t byte;
	int rc = flash_read(fixture->flash_dev, IPED_REGION_END, &byte, 1);

	zassert_equal(rc, 0, "read first byte after IPED failed");
}

ZTEST_F(iped_flash, test_edge_byte_before_iped)
{
	uint8_t byte;
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START - 1, &byte, 1);

	zassert_equal(rc, 0, "read last byte before IPED failed");
}

ZTEST_F(iped_flash, test_edge_block_aligned_read)
{
	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, rd_buf, 256);

	zassert_equal(rc, 0, "256B (IPED_BLOCK_SIZE) read failed");
}

ZTEST_F(iped_flash, test_data_different_offsets)
{
	memset(rd_buf, 0, 64);
	memset(rd_buf2, 0, 64);

	zassert_equal(flash_read(fixture->flash_dev, IPED_REGION_START + 0x100, rd_buf, 64), 0);
	zassert_equal(flash_read(fixture->flash_dev, IPED_REGION_START + 0x3000, rd_buf2, 64), 0);
	zassert_true(memcmp(rd_buf, rd_buf2, 64) != 0, "different offsets returned identical data");
}

ZTEST_F(iped_flash, test_data_not_erased)
{
	memset(rd_buf, 0, 256);
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START + 0x800, rd_buf, 256);

	zassert_equal(rc, 0);
	zassert_false(buf_all_val(rd_buf, 256, 0xFF), "IPED data is all 0xFF — not encrypted?");
}

ZTEST_F(iped_flash, test_data_vector_table_sanity)
{
	uint32_t vec[2];
	int rc = flash_read(fixture->flash_dev, IPED_REGION_START, vec, sizeof(vec));

	zassert_equal(rc, 0);

	uint32_t sp = vec[0];
	uint32_t reset = vec[1];

	bool sp_ok = (sp >= 0x10000000 && sp < 0x30000000);
	bool rst_ok = (reset >= 0x08000000 && reset < 0x20000000);

	zassert_true(sp_ok && rst_ok, "SP=0x%08x Reset=0x%08x don't look like ARM pointers", sp,
		     reset);
}

ZTEST_F(iped_flash, test_data_address_uniqueness)
{
	static const off_t offsets[] = {
		0x0800, 0x1000, 0x1800, 0x2000, 0x3000, 0x4000,
		0x6000, 0x8000, 0x9000, 0xA000, 0xB000, 0xC000,
	};
	const size_t n = ARRAY_SIZE(offsets);
	uint8_t bufs[ARRAY_SIZE(offsets)][32];

	for (size_t i = 0; i < n; i++) {
		zassert_equal(
			flash_read(fixture->flash_dev, IPED_REGION_START + offsets[i], bufs[i], 32),
			0);
	}

	for (size_t i = 0; i < n; i++) {
		for (size_t j = i + 1; j < n; j++) {
			zassert_true(memcmp(bufs[i], bufs[j], 32) != 0, "offset 0x%lx == 0x%lx",
				     (long)offsets[i], (long)offsets[j]);
		}
	}
}

ZTEST_F(iped_flash, test_iped_erase_read_0xff)
{
	int rc = flash_erase(fixture->flash_dev, IPED_ERASE_TEST_OFF, IPED_ERASE_TEST_SIZE);

	zassert_equal(rc, 0, "erase in IPED region failed");

	memset(rd_buf, 0x00, IPED_ERASE_TEST_SIZE);
	rc = flash_read(fixture->flash_dev, IPED_ERASE_TEST_OFF, rd_buf, IPED_ERASE_TEST_SIZE);

	zassert_equal(rc, 0, "read after erase failed");
	zassert_true(buf_all_val(rd_buf, IPED_ERASE_TEST_SIZE, 0xFF),
		     "erased IPED sector not all 0xFF");
}

ZTEST_F(iped_flash, test_iped_erase_tracking_persists)
{
	int rc = flash_erase(fixture->flash_dev, IPED_ERASE_TEST_OFF, IPED_ERASE_TEST_SIZE);

	zassert_equal(rc, 0);

	for (int i = 0; i < 3; i++) {
		memset(rd_buf, 0x00, 256);
		rc = flash_read(fixture->flash_dev, IPED_ERASE_TEST_OFF + (i * 256), rd_buf, 256);
		zassert_equal(rc, 0, "read %d failed", i);
		zassert_true(buf_all_val(rd_buf, 256, 0xFF), "read %d not 0xFF", i);
	}
}

ZTEST_F(iped_flash, test_iped_gcm_write_rejected)
{
	int rc;

	rc = flash_erase(fixture->flash_dev, IPED_ERASE_TEST_OFF, IPED_ERASE_TEST_SIZE);
	zassert_equal(rc, 0, "erase failed");

	for (size_t i = 0; i < 256; i++) {
		wr_buf[i] = (uint8_t)(TEST_PATTERN_START + i);
	}
	rc = flash_write(fixture->flash_dev, IPED_ERASE_TEST_OFF, wr_buf, 256);
	zassert_equal(rc, -ENOTSUP,
		      "GCM write should be rejected (plaintext corrupts auth tag)");

	rc = flash_erase(fixture->flash_dev, IPED_ERASE_TEST_OFF, IPED_ERASE_TEST_SIZE);
	zassert_equal(rc, 0, "re-erase failed");
}

ZTEST_F(iped_flash, test_iped_erase_write_erase_cycle)
{
	int rc;

	rc = flash_erase(fixture->flash_dev, IPED_ERASE_TEST_OFF, IPED_ERASE_TEST_SIZE);
	zassert_equal(rc, 0);

	memset(rd_buf, 0x00, IPED_ERASE_TEST_SIZE);
	rc = flash_read(fixture->flash_dev, IPED_ERASE_TEST_OFF, rd_buf, IPED_ERASE_TEST_SIZE);
	zassert_equal(rc, 0);
	zassert_true(buf_all_val(rd_buf, IPED_ERASE_TEST_SIZE, 0xFF), "first erase: not 0xFF");

	for (size_t i = 0; i < 256; i++) {
		wr_buf[i] = (uint8_t)(0x55 + i);
	}
	rc = flash_write(fixture->flash_dev, IPED_ERASE_TEST_OFF, wr_buf, 256);
	zassert_equal(rc, -ENOTSUP, "GCM write should be rejected");

	/* Erase should still succeed after rejected write */
	rc = flash_erase(fixture->flash_dev, IPED_ERASE_TEST_OFF, IPED_ERASE_TEST_SIZE);
	zassert_equal(rc, 0, "re-erase failed");

	memset(rd_buf, 0x00, IPED_ERASE_TEST_SIZE);
	rc = flash_read(fixture->flash_dev, IPED_ERASE_TEST_OFF, rd_buf, IPED_ERASE_TEST_SIZE);
	zassert_equal(rc, 0);
	zassert_true(buf_all_val(rd_buf, IPED_ERASE_TEST_SIZE, 0xFF), "re-erase: not 0xFF");
}

static void ipwr_enable(void)
{
	volatile uint32_t *ipedctrl = (volatile uint32_t *)FLEXSPI_IPEDCTRL_ADDR;
	unsigned int key = irq_lock();

	*ipedctrl |= IPWR_EN_BIT;
	irq_unlock(key);
}

static void ipwr_disable(void)
{
	volatile uint32_t *ipedctrl = (volatile uint32_t *)FLEXSPI_IPEDCTRL_ADDR;
	unsigned int key = irq_lock();

	*ipedctrl &= ~IPWR_EN_BIT;
	irq_unlock(key);
}

ZTEST_F(iped_flash, test_iped_ctr_write_read_roundtrip)
{
	int rc;

	rc = flash_erase(fixture->flash_dev, IPED_REGION1_START,
			 IPED_REGION1_END - IPED_REGION1_START);
	zassert_equal(rc, 0, "erase region 1 failed");

	memset(rd_buf, 0x00, STORAGE_TEST_SIZE);
	rc = flash_read(fixture->flash_dev, IPED_REGION1_START, rd_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "read after erase failed");
	zassert_true(buf_all_val(rd_buf, STORAGE_TEST_SIZE, 0xFF), "erased region not 0xFF");

	ipwr_enable();

	for (size_t i = 0; i < STORAGE_TEST_SIZE; i++) {
		wr_buf[i] = (uint8_t)(TEST_PATTERN_START + i);
	}
	rc = flash_write(fixture->flash_dev, IPED_REGION1_START, wr_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "write to CTR IPED region failed");

	memset(rd_buf, 0x00, STORAGE_TEST_SIZE);
	rc = flash_read(fixture->flash_dev, IPED_REGION1_START, rd_buf, STORAGE_TEST_SIZE);
	zassert_equal(rc, 0, "read-back failed");
	zassert_mem_equal(rd_buf, wr_buf, STORAGE_TEST_SIZE, "write+read roundtrip data mismatch");

	ipwr_disable();
}

ZTEST_F(iped_flash, test_iped_ctr_reread_after_write)
{
	int rc;

	rc = flash_erase(fixture->flash_dev, IPED_REGION1_START,
			 IPED_REGION1_END - IPED_REGION1_START);
	zassert_equal(rc, 0);

	ipwr_enable();

	for (size_t i = 0; i < 256; i++) {
		wr_buf[i] = (uint8_t)(0xBE + i);
	}
	rc = flash_write(fixture->flash_dev, IPED_REGION1_START, wr_buf, 256);
	zassert_equal(rc, 0);

	uint8_t r1[256], r2[256];

	rc = flash_read(fixture->flash_dev, IPED_REGION1_START, r1, 256);
	zassert_equal(rc, 0);
	rc = flash_read(fixture->flash_dev, IPED_REGION1_START, r2, 256);
	zassert_equal(rc, 0);
	zassert_mem_equal(r1, r2, 256, "consecutive re-reads differ");
	zassert_mem_equal(r1, wr_buf, 256, "re-read doesn't match written data");

	ipwr_disable();
}

ZTEST_F(iped_flash, test_iped_ctr_write_clears_erased_tracking)
{
	int rc;

	rc = flash_erase(fixture->flash_dev, IPED_REGION1_START,
			 IPED_REGION1_END - IPED_REGION1_START);
	zassert_equal(rc, 0);

	memset(rd_buf, 0x00, 256);
	rc = flash_read(fixture->flash_dev, IPED_REGION1_START, rd_buf, 256);
	zassert_equal(rc, 0);
	zassert_true(buf_all_val(rd_buf, 256, 0xFF), "not 0xFF after erase");

	ipwr_enable();

	for (size_t i = 0; i < 256; i++) {
		wr_buf[i] = (uint8_t)(0x42 + i);
	}
	rc = flash_write(fixture->flash_dev, IPED_REGION1_START, wr_buf, 256);
	zassert_equal(rc, 0);

	memset(rd_buf, 0x00, 256);
	rc = flash_read(fixture->flash_dev, IPED_REGION1_START, rd_buf, 256);
	zassert_equal(rc, 0);
	zassert_false(buf_all_val(rd_buf, 256, 0xFF),
		      "still 0xFF after write — tracking not cleared?");
	zassert_mem_equal(rd_buf, wr_buf, 256, "data mismatch after write");

	ipwr_disable();
}

static void iped_flash_after(void *f)
{
	ARG_UNUSED(f);

	/* Ensure IPWR_EN is cleared even if a test fails mid-way */
	ipwr_disable();
}

ZTEST_SUITE(iped_flash, NULL, iped_flash_setup, NULL, iped_flash_after, NULL);
