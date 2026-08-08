/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/sd/sd_spec.h>
#include "test_common.h"

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(sdhc_sd));

void *sdhc_rw_setup(void)
{
	zassert_true(device_is_ready(dev), "SDHC emulator not ready");
	sdhc_init_to_transfer(dev);
	return NULL;
}

/* Verify single-block write and read round-trip with bytes_xfered. */
ZTEST(sdhc_rw, test_single_block_write_read)
{
	uint8_t wbuf[512];
	uint8_t rbuf[512];
	size_t written, read;

	for (int i = 0; i < 512; i++) {
		wbuf[i] = (i & 1) ? 0xA5 : 0x5A;
	}

	int ret = sdhc_write_blocks(dev, 0, wbuf, 1, &written);

	zassert_ok(ret, "Write block 0 failed");
	zassert_equal(written, 512, "bytes_xfered mismatch on write");

	memset(rbuf, 0, 512);
	ret = sdhc_read_blocks(dev, 0, rbuf, 1, &read);
	zassert_ok(ret, "Read block 0 failed");
	zassert_equal(read, 512, "bytes_xfered mismatch on read");

	zassert_mem_equal(wbuf, rbuf, 512, "Read data mismatch");
}

/* Verify multi-block write and read round-trip with bytes_xfered. */
ZTEST(sdhc_rw, test_multi_block_write_read)
{
	uint8_t wbuf[4096];
	uint8_t rbuf[4096];
	size_t written, read;

	for (int i = 0; i < 4096; i++) {
		wbuf[i] = (uint8_t)(i ^ 0xA5);
	}

	int ret = sdhc_write_blocks(dev, 10, wbuf, 8, &written);

	zassert_ok(ret, "Write 8 blocks failed");
	zassert_equal(written, 4096, "bytes_xfered mismatch on multi-write");

	memset(rbuf, 0, 4096);
	ret = sdhc_read_blocks(dev, 10, rbuf, 8, &read);
	zassert_ok(ret, "Read 8 blocks failed");
	zassert_equal(read, 4096, "bytes_xfered mismatch on multi-read");

	zassert_mem_equal(wbuf, rbuf, 4096, "Read data mismatch in multi-block");
}

/* Verify last valid block is writable and out-of-bounds block is rejected. */
ZTEST(sdhc_rw, test_last_block_boundary)
{
	uint8_t buf[512] = {0};
	size_t xfer;
	uint32_t cap = sdhc_emul_get_block_count(dev);

	int ret = sdhc_write_blocks(dev, cap - 1, buf, 1, &xfer);

	zassert_ok(ret, "Write to last block failed");
	zassert_equal(xfer, 512, "bytes_xfered mismatch on last block");

	ret = sdhc_write_blocks(dev, cap, buf, 1, NULL);
	zassert_not_equal(ret, 0, "Write out of bounds succeeded");
}

/* Verify erase only touches the specified range, leaving adjacent blocks intact. */
ZTEST(sdhc_rw, test_erase_range)
{
	uint8_t *storage = sdhc_emul_get_storage(dev);

	zassert_not_null(storage, "Storage is NULL");

	/* Fill blocks 4-11 with 0xFF */
	memset(&storage[4 * 512], 0xFF, 512 * 8);

	struct sdhc_command cmd = {0};

	cmd.opcode = SD_ERASE_BLOCK_START;
	cmd.arg = 5;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD32 failed");

	cmd.opcode = SD_ERASE_BLOCK_END;
	cmd.arg = 10;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD33 failed");

	cmd.opcode = SD_ERASE_BLOCK_OPERATION;
	cmd.arg = 0;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD38 failed");

	/* Erased blocks 5-10 should be zero */
	for (int i = 5 * 512; i < 11 * 512; i++) {
		zassert_equal(storage[i], 0x00, "Block not erased at offset %d", i);
	}
	/* Adjacent blocks 4 and 11 must still be 0xFF */
	for (int i = 4 * 512; i < 5 * 512; i++) {
		zassert_equal(storage[i], 0xFF, "Block 4 touched at offset %d", i);
	}
	for (int i = 11 * 512; i < 12 * 512; i++) {
		zassert_equal(storage[i], 0xFF, "Block 11 touched at offset %d", i);
	}
}

/* Verify CMD12 leaves the card in TRANSFER state. */
ZTEST(sdhc_rw, test_stop_transmission)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_STOP_TRANSMISSION;
	cmd.arg = 0;
	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_ok(ret, "CMD12 failed");

	uint32_t rca = 0x1234;
	int state = sdhc_get_card_state(dev, rca);

	zassert_true(state >= 0, "CMD13 failed");
	zassert_equal(state, 4, "State not TRANSFER after CMD12");
}

/* Verify CMD23 SET_BLOCK_COUNT precedes a valid multi-block transfer. */
ZTEST(sdhc_rw, test_set_block_count_then_multi)
{
	uint8_t wbuf[1024];
	uint8_t rbuf[1024];
	size_t written, read;

	for (int i = 0; i < 1024; i++) {
		wbuf[i] = (uint8_t)(i + 1);
	}

	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SET_BLOCK_COUNT;
	cmd.arg = 2; /* 2 blocks */
	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_ok(ret, "CMD23 failed");

	ret = sdhc_write_blocks(dev, 20, wbuf, 2, &written);
	zassert_ok(ret, "Write after CMD23 failed");
	zassert_equal(written, 1024, "bytes_xfered mismatch");

	memset(rbuf, 0, 1024);
	ret = sdhc_read_blocks(dev, 20, rbuf, 2, &read);
	zassert_ok(ret, "Read after CMD23 failed");
	zassert_equal(read, 1024, "bytes_xfered mismatch on read");
	zassert_mem_equal(wbuf, rbuf, 1024, "Data mismatch after CMD23");
}

struct rw_ctx {
	uint32_t block;
	int errors;
};

static void rw_thread_disjoint(void *p1, void *p2, void *p3)
{
	struct rw_ctx *ctx = p1;
	uint32_t blk = ctx->block;
	uint8_t buf[512];

	for (int iter = 0; iter < 10; iter++) {
		buf[0] = (uint8_t)(blk + iter);
		size_t xfer;

		if (sdhc_write_blocks(dev, blk, buf, 1, &xfer) != 0 || xfer != 512) {
			ctx->errors++;
		}
		memset(buf, 0, 512);
		if (sdhc_read_blocks(dev, blk, buf, 1, &xfer) != 0 || xfer != 512) {
			ctx->errors++;
		}
		if (buf[0] != (uint8_t)(blk + iter)) {
			ctx->errors++;
		}
	}
}

static void rw_thread_contended(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct rw_ctx *ctx = p1;
	uint32_t blk = ctx->block;
	uint8_t buf[512];

	for (int iter = 0; iter < 20; iter++) {
		buf[0] = (uint8_t)(iter + 1);
		size_t xfer;

		if (sdhc_write_blocks(dev, blk, buf, 1, &xfer) != 0 || xfer != 512) {
			ctx->errors++;
		}
		memset(buf, 0, 512);
		if (sdhc_read_blocks(dev, blk, buf, 1, &xfer) != 0 || xfer != 512) {
			ctx->errors++;
		}
		/* With mutex contention, buf[0] may not equal our value, but */
		/* the operation should not crash and xfer should be correct. */
	}
}

#define STACK_SIZE 1024
K_THREAD_STACK_ARRAY_DEFINE(rw_stacks, 4, STACK_SIZE);
struct k_thread rw_threads[4];
struct rw_ctx rw_ctxs[4];

/* Verify concurrent threads on disjoint blocks read back correct data. */
ZTEST(sdhc_rw, test_concurrent_disjoint_blocks)
{
	for (int i = 0; i < 4; i++) {
		rw_ctxs[i].block = (uint32_t)(i * 10);
		rw_ctxs[i].errors = 0;
		k_thread_create(&rw_threads[i], rw_stacks[i], STACK_SIZE,
				rw_thread_disjoint, &rw_ctxs[i], NULL, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	for (int i = 0; i < 4; i++) {
		k_thread_join(&rw_threads[i], K_FOREVER);
		zassert_equal(rw_ctxs[i].errors, 0, "Thread %d had errors", i);
	}
}

/* Verify concurrent threads on the same block do not crash or corrupt xfer sizes. */
ZTEST(sdhc_rw, test_concurrent_contended_block)
{
	for (int i = 0; i < 4; i++) {
		rw_ctxs[i].block = 100; /* same block for all */
		rw_ctxs[i].errors = 0;
		k_thread_create(&rw_threads[i], rw_stacks[i], STACK_SIZE,
				rw_thread_contended, &rw_ctxs[i], NULL, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	for (int i = 0; i < 4; i++) {
		k_thread_join(&rw_threads[i], K_FOREVER);
		/* Only assert structural safety (no crashes, valid xfer sizes) */
		zassert_true(rw_ctxs[i].errors == 0,
			   "Contended thread %d had structural errors", i);
	}
}
