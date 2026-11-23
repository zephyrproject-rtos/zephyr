/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#define N  4
#define SZ 32U
#define CH 0U

static const struct device *const dma = DEVICE_DT_GET(DT_NODELABEL(tst_dma0));
static K_SEM_DEFINE(done, 0, 1);
static int cbs;
static __aligned(32) char src[N][SZ];
static __aligned(32) char dst[N][SZ];

static void on_done(const struct device *d, void *a, uint32_t id, int status)
{
	cbs++;
	k_sem_give(&done);
}

static struct dma_config base_cfg(struct dma_block_config *blk)
{
	struct dma_config c = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.head_block = blk,
		.dma_callback = on_done,
		.user_data = &done,
	};
	return c;
}

/* A channel must deliver a completion for every reload, moving each new block. */
ZTEST(dma_reload, test_reload_completes_each_time)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);

	cbs = 0;
	memset(src[0], 'A', SZ);
	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_start(dma, CH));
	zassert_ok(k_sem_take(&done, K_MSEC(1000)));

	for (int i = 1; i < N; i++) {
		memset(src[i], 'A' + i, SZ);
		k_sem_reset(&done);
		zassert_ok(dma_reload(dma, CH, (uintptr_t)src[i], (uintptr_t)dst[i], SZ),
			   "reload %d failed", i);
		zassert_ok(k_sem_take(&done, K_MSEC(1000)), "no completion for reload %d", i);
		zassert_mem_equal(dst[i], src[i], SZ, "reload %d moved wrong data", i);
	}
	zassert_equal(cbs, N, "expected %d completions, got %d", N, cbs);
}

/* dma_get_status() must report a defined struct, not leave the caller's stack. */
ZTEST(dma_reload, test_get_status_conformance)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status st;

	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_get_status(dma, CH, &st), "get_status must be implemented");
	zassert_equal(st.dir, MEMORY_TO_MEMORY, "dir not reported");
}

ZTEST(dma_reload, test_get_status_after_stop)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status st = {
		.busy = true,
		.dir = PERIPHERAL_TO_MEMORY,
		.pending_length = UINT32_MAX,
		.free = UINT32_MAX,
		.write_position = UINT32_MAX,
		.read_position = UINT32_MAX,
		.total_copied = UINT64_MAX,
	};

	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_stop(dma, CH));
	zassert_ok(dma_get_status(dma, CH, &st));
	zassert_false(st.busy, "channel is still busy after stop");
	zassert_equal(st.pending_length, 0U, "pending length was not initialized");
	zassert_equal(st.free, 0U, "free space was not initialized");
	zassert_equal(st.dir, MEMORY_TO_MEMORY, "direction was not initialized");
	zassert_equal(st.read_position, 0U, "read position was not initialized");
	zassert_equal(st.write_position, 0U, "write position was not initialized");
	zassert_equal(st.total_copied, 0U, "total copied was not initialized");
}

ZTEST(dma_reload, test_emul_rejects_null_config_and_invalid_status_arguments)
{
	struct dma_block_config blk = {
		.source_address = (uintptr_t)src[0],
		.dest_address = (uintptr_t)dst[0],
		.block_size = SZ,
	};
	struct dma_config c = base_cfg(&blk);
	struct dma_status before;
	struct dma_status after;

	zassert_ok(dma_config(dma, CH, &c));
	zassert_ok(dma_get_status(dma, CH, &before));
	zassert_equal(dma_config(dma, CH, NULL), -EINVAL, "null config was accepted");
	zassert_equal(dma_config(dma, UINT32_MAX, &c), -EINVAL,
		      "invalid config channel was accepted");
	zassert_equal(dma_start(dma, UINT32_MAX), -EINVAL, "invalid start channel was accepted");
	zassert_equal(dma_stop(dma, UINT32_MAX), -EINVAL, "invalid stop channel was accepted");
	zassert_equal(dma_reload(dma, UINT32_MAX, 0U, 0U, SZ), -EINVAL,
		      "invalid reload channel was accepted");
	zassert_equal(dma_get_status(dma, UINT32_MAX, &after), -EINVAL,
		      "invalid status channel was accepted");
	zassert_equal(dma_get_status(dma, CH, NULL), -EINVAL, "null status was accepted");
	zassert_ok(dma_get_status(dma, CH, &after));
	zassert_equal(after.busy, before.busy, "rejection changed busy state");
	zassert_equal(after.dir, before.dir, "rejection changed direction");
	zassert_equal(after.pending_length, before.pending_length,
		      "rejection changed pending length");
	zassert_equal(after.free, before.free, "rejection changed free space");
	zassert_equal(after.read_position, before.read_position, "rejection changed read position");
	zassert_equal(after.write_position, before.write_position,
		      "rejection changed write position");
	zassert_equal(after.total_copied, before.total_copied, "rejection changed total copied");
}

static void *setup(void)
{
	zassert_true(device_is_ready(dma), "DMA controller not ready");
	return NULL;
}

/* Stop after each test so the next one can (re)configure a fresh channel.
 * This runs only once a channel has been configured, so it never stops an
 * unconfigured channel.
 */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)dma_stop(dma, CH);
}

ZTEST_SUITE(dma_reload, NULL, setup, NULL, after, NULL);
