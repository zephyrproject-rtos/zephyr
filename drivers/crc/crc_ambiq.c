/*
 * Copyright (c) 2025 Ambiq Micro, Inc.
 * Author: Richard S Wheatley
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_hw_crc32

#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(ambiq_hw_crc32, CONFIG_CRC_DRIVER_LOG_LEVEL);

/*
 * This driver decomposes am_hal_crc() into begin / update / finish to fit
 * the Zephyr CRC streaming API. The boundary-checking logic from
 * am_hal_crc_find_next_boundary() is called directly so that each update()
 * call correctly splits at TCM 4 KB and major memory-region boundaries.
 *
 * The SECURITY->RESULT register is the hardware CRC accumulator:
 *   - Seeded in begin() from ctx->seed
 *   - Left running across update() calls (am_hal_crc32 with
 *     g_bCrcInitialized = true skips re-seeding)
 *   - Read in finish() and returned via ctx->result
 */

struct crc_ambiq_data {
	/*
	 * Binary semaphore (not k_mutex) because the Zephyr CRC API does not
	 * require begin() and finish() to run on the same thread. Mutexes are
	 * owned and can only be unlocked by their locking thread, so a begin
	 * in one context with a finish in another would fail. The semaphore
	 * allows cross-thread release.
	 */
	struct k_sem lock;
};

/*
 * Release the resources acquired in crc_ambiq_begin(). Called both on the
 * normal path from crc_ambiq_finish() and on hardware-error paths in
 * crc_ambiq_update() so the caller never has to call finish() after a failure.
 */
static void crc_ambiq_release(const struct device *dev, struct crc_ctx *ctx)
{
	struct crc_ambiq_data *data = dev->data;

	am_hal_crc_finalize();
	ctx->state = CRC_STATE_IDLE;
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	k_sem_give(&data->lock);
}

static int crc_ambiq_begin(const struct device *dev, struct crc_ctx *ctx)
{
	struct crc_ambiq_data *data = dev->data;

	if (ctx == NULL) {
		return -EINVAL;
	}

	/*
	 * The Ambiq SECURITY engine only supports a single fixed configuration:
	 *   type       = CRC32_IEEE
	 *   polynomial = 0x04C11DB7 (baked into silicon)
	 *   reversed   = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT
	 *                (hardware always produces reflected CRC32 IEEE)
	 *
	 * Reject anything else so callers get a visible error instead of a
	 * silently-wrong result.  The seed is configurable via the RESULT
	 * register and is honored below.
	 */
	if (ctx->type != CRC32_IEEE) {
		return -ENOTSUP;
	}

	if (ctx->polynomial != CRC32_IEEE_POLY) {
		return -EINVAL;
	}

	if (ctx->reversed != (CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (SECURITY->CTRL_b.ENABLE) {
		k_sem_give(&data->lock);
		return -EBUSY;
	}

	/*
	 * Prevent the system from entering deep sleep while the CRC
	 * session is active. The SECURITY peripheral lives in the
	 * CRYPTO power domain -- if it gets gated between begin() and
	 * finish(), the RESULT accumulator is lost.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

	/*
	 * Seed the hardware accumulator from ctx->seed so callers can run
	 * incremental / chained CRC32 starting from a prior result.
	 */
	SECURITY->RESULT = ctx->seed;

	/*
	 * Tell am_hal_crc32() NOT to re-seed on subsequent calls.
	 * This keeps the RESULT register accumulating across all chunks
	 * and across multiple update() calls.
	 */
	am_hal_crc_set_init();

	ctx->state = CRC_STATE_IN_PROGRESS;
	ctx->result = ctx->seed;

	return 0;
}

static int crc_ambiq_process_aligned(uint32_t addr, uint32_t remaining,
				     struct crc_ctx *ctx,
				     const struct device *dev)
{
	uint32_t boundary;
	uint32_t chunk;
	uint32_t hal_status;

	while (remaining > 0) {
		boundary = am_hal_crc_find_next_boundary(addr, remaining);
		chunk = (boundary < addr + remaining)
			? (boundary - addr) : remaining;

		if (chunk > 0) {
			hal_status = am_hal_crc32(addr, chunk, &ctx->result);
			if (hal_status != AM_HAL_STATUS_SUCCESS) {
				crc_ambiq_release(dev, ctx);
				return -EIO;
			}
		}
		addr += chunk;
		remaining -= chunk;
	}

	return 0;
}

#define CRC_DMA_CHUNK_SIZE 256

static int crc_ambiq_process_unaligned(const uint8_t *src, uint32_t remaining,
				       struct crc_ctx *ctx,
				       const struct device *dev)
{
	static uint8_t dma_buf[CRC_DMA_CHUNK_SIZE] __aligned(32);
	uint32_t boundary;
	uint32_t chunk;
	uint32_t hal_status;

	while (remaining > 0) {
		uint32_t n = MIN(remaining, sizeof(dma_buf));
		uint32_t local_addr = (uint32_t)dma_buf;
		uint32_t left = n;

		memcpy(dma_buf, src, n);
		sys_cache_data_flush_range(dma_buf, n);

		while (left > 0) {
			boundary = am_hal_crc_find_next_boundary(local_addr,
								 left);
			chunk = (boundary < local_addr + left)
				? (boundary - local_addr) : left;

			if (chunk > 0) {
				hal_status = am_hal_crc32(local_addr, chunk,
							  &ctx->result);
				if (hal_status != AM_HAL_STATUS_SUCCESS) {
					crc_ambiq_release(dev, ctx);
					return -EIO;
				}
			}
			local_addr += chunk;
			left -= chunk;
		}

		src += n;
		remaining -= n;
	}

	return 0;
}

static int crc_ambiq_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
				size_t bufsize)
{
	uint32_t addr;

	/* Basic validation first */
	if (ctx == NULL) {
		return -EINVAL;
	}

	/* If the session is active, any caller error should release resources */
	if (buffer == NULL || bufsize == 0) {
		if (ctx->state == CRC_STATE_IN_PROGRESS) {
			crc_ambiq_release(dev, ctx);
		}
		return -EINVAL;
	}

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	if (bufsize & 0x3) {
		crc_ambiq_release(dev, ctx);
		return -ENOTSUP;
	}

	addr = (uint32_t)(uintptr_t)buffer;

	/*
	 * The SECURITY CRC engine uses DMA which bypasses the CPU data cache.
	 * The DMA source buffer must be 32-byte (cache line) aligned to
	 * guarantee correct cache flush semantics.
	 *
	 * If the caller's buffer is already aligned, flush and feed it
	 * directly. Otherwise, copy through a local aligned staging buffer.
	 *
	 * In both paths the boundary-aware loop from am_hal_crc() splits at
	 * TCM 4 KB and major memory-region crossings.  SECURITY->RESULT
	 * accumulates across all chunks because am_hal_crc_set_init() was
	 * called in begin().
	 */
	if ((addr & 0x1FU) == 0U) {
		sys_cache_data_flush_range((void *)(uintptr_t)buffer, bufsize);
		return crc_ambiq_process_aligned(addr, (uint32_t)bufsize,
						 ctx, dev);
	}

	return crc_ambiq_process_unaligned((const uint8_t *)buffer,
					   (uint32_t)bufsize, ctx, dev);
}

static int crc_ambiq_finish(const struct device *dev, struct crc_ctx *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	crc_ambiq_release(dev, ctx);

	return 0;
}

static int crc_ambiq_init(const struct device *dev)
{
	struct crc_ambiq_data *data = dev->data;

	return k_sem_init(&data->lock, 1, 1);
}

/**
 * @brief Ambiq CRC32 hardware driver API.
 *
 * Provides hardware-accelerated CRC32_IEEE (polynomial 0x04C11DB7) calculation using the
 * SECURITY peripheral. The hardware produces reflected CRC32 with reversed input/output.
 * Operations are protected by PM policy locks to prevent CRYPTO power domain gating during
 * active CRC sessions.
 */
static DEVICE_API(crc, crc_ambiq_api) = {
	.begin = crc_ambiq_begin,
	.update = crc_ambiq_update,
	.finish = crc_ambiq_finish,
};

#define CRC_AMBIQ_INIT(n)                                                                          \
	static struct crc_ambiq_data crc_ambiq_data_##n;                                           \
	DEVICE_DT_INST_DEFINE(n, crc_ambiq_init, NULL, &crc_ambiq_data_##n, NULL, POST_KERNEL,     \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_ambiq_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_AMBIQ_INIT)
