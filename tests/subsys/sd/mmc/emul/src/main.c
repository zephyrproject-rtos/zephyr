/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Same block I/O shape as tests/subsys/sd/sdmmc/emul, through the MMC stack.
 * Init faults use CMD1 and CMD9. CMD8 is shared with SD_SEND_IF_COND, so it
 * is not a clean inject target here.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/sd/mmc.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/sd/sd_spec.h>

#define STACK_SIZE 4096

struct rw_ctx {
	uint32_t block;
	int errors;
};

static const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static struct sd_card card;
static uint8_t buf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

K_THREAD_STACK_ARRAY_DEFINE(rw_stacks, 4, STACK_SIZE);
static struct k_thread rw_threads[4];
static struct rw_ctx rw_ctxs[4];

static void card_up(void)
{
	zassert_true(device_is_ready(dev), "SDHC emulator not ready");
	zassert_ok(sd_init(dev, &card), "sd_init failed");
}

ZTEST(mmc_emul, test_inject_cmd1_blocks_init)
{
	sdhc_emul_set_fault(dev, MMC_SEND_OP_COND);
	zassert_not_equal(sd_init(dev, &card), 0, "init should fail");
	sdhc_emul_clear_fault(dev);
	zassert_ok(sd_init(dev, &card), "init after clear");
}

ZTEST(mmc_emul, test_inject_cmd3_blocks_init)
{
	sdhc_emul_set_fault(dev, MMC_SEND_RELATIVE_ADDR);
	zassert_not_equal(sd_init(dev, &card), 0, "init should fail");
	sdhc_emul_clear_fault(dev);
	zassert_ok(sd_init(dev, &card), "init after clear");
}

ZTEST(mmc_emul, test_inject_cmd9_blocks_init)
{
	sdhc_emul_set_fault(dev, SD_SEND_CSD);
	zassert_not_equal(sd_init(dev, &card), 0, "init should fail");
	sdhc_emul_clear_fault(dev);
	zassert_ok(sd_init(dev, &card), "init after clear");
}

ZTEST(mmc_emul, test_inject_cmd17_error)
{
	card_up();
	sdhc_emul_set_fault(dev, SD_READ_SINGLE_BLOCK);
	zassert_equal(mmc_read_blocks(&card, buf, 0, 1), -EIO, "CMD17 fault");
	sdhc_emul_clear_fault(dev);
	zassert_ok(mmc_read_blocks(&card, buf, 0, 1), "read after clear");
}

ZTEST(mmc_emul, test_inject_cmd24_error)
{
	card_up();
	sdhc_emul_set_fault(dev, SD_WRITE_SINGLE_BLOCK);
	zassert_equal(mmc_write_blocks(&card, buf, 0, 1), -EIO, "CMD24 fault");
	sdhc_emul_clear_fault(dev);
	zassert_ok(mmc_read_blocks(&card, buf, 0, 1), "card not usable after fault");
}

ZTEST(mmc_emul, test_card_removal_mid_io)
{
	card_up();
	sdhc_emul_set_card_present(dev, false);
	zassert_equal(mmc_read_blocks(&card, buf, 0, 1), -ENODEV, "removed card");
	sdhc_emul_set_card_present(dev, true);
}

ZTEST(mmc_emul, test_reset_preserves_storage)
{
	static uint8_t wbuf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	static uint8_t rbuf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

	card_up();
	memset(wbuf, 0xA5, sizeof(wbuf));
	zassert_ok(mmc_write_blocks(&card, wbuf, 0, 1));
	zassert_ok(sdhc_hw_reset(dev));
	zassert_ok(sd_init(dev, &card));
	zassert_ok(mmc_read_blocks(&card, rbuf, 0, 1));
	zassert_mem_equal(wbuf, rbuf, sizeof(wbuf), "Storage cleared on reset");
}

ZTEST(mmc_emul, test_multiblock_rw)
{
	static uint8_t wbuf[8 * 512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	static uint8_t rbuf[8 * 512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

	card_up();
	memset(wbuf, 0x5A, sizeof(wbuf));
	zassert_ok(mmc_write_blocks(&card, wbuf, 100, 8), "multi-block write failed");
	memset(rbuf, 0, sizeof(rbuf));
	zassert_ok(mmc_read_blocks(&card, rbuf, 100, 8), "multi-block read failed");
	zassert_mem_equal(wbuf, rbuf, sizeof(wbuf), "multi-block readback mismatch");
}

ZTEST(mmc_emul, test_out_of_range_read)
{
	card_up();
	zassert_equal(mmc_read_blocks(&card, buf, 1023, 2), -EINVAL,
		      "read past end of card should fail");
	zassert_equal(mmc_read_blocks(&card, buf, 1024, 1), -EINVAL,
		      "read starting past end of card should fail");
}

static void rw_thread_disjoint(void *p1, void *p2, void *p3)
{
	struct rw_ctx *ctx = p1;
	uint8_t wbuf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	uint8_t rbuf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int iter = 0; iter < 10; iter++) {
		memset(wbuf, (uint8_t)(ctx->block + iter), sizeof(wbuf));
		if (mmc_write_blocks(&card, wbuf, ctx->block, 1) != 0 ||
		    mmc_read_blocks(&card, rbuf, ctx->block, 1) != 0 ||
		    memcmp(wbuf, rbuf, sizeof(wbuf)) != 0) {
			ctx->errors++;
		}
	}
}

ZTEST(mmc_emul, test_concurrent_disjoint_blocks)
{
	card_up();

	for (int i = 0; i < 4; i++) {
		rw_ctxs[i].block = (uint32_t)(200 + i * 10);
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

ZTEST_SUITE(mmc_emul, NULL, NULL, NULL, NULL, NULL);
