/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7
 *
 */

#define DT_DRV_COMPAT realtek_rts5912_sha

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha256_hw_rtk, CONFIG_CRYPTO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one realtek,rts5912-sha compatible node can be supported");

#include "reg/reg_crypto.h"
#include "crypto_rts5912_priv.h"

#define RTS5912_SHA2DMA_MAXIMUM_BLOCK_NUM      (0x1FFUL)
#define RTS5912_SHA2DMA_BLOCK_SIZE             (64)
#define RTS5912_SHA2DMA_BLOCK_SHIFT            (6)
#define RTS5912_SHA2DMA_8Byte_SHIFT            (3)
#define RTS5912_SHA2DMA_DST_WIDTH              (0x3)
#define RTS5912_SHA2DMA_SRC_WIDTH              (0x3)
#define RTS5912_SHA2DMA_HIGH_LEVEL_MSK         (0xFFFF0000)
#define RTS5912_SHA2_BLOCK_EXTEND_CHECK        (56)
#define RTS5912_MAXIMUM_CRYPTO_POLLING_TIME_US (50 * USEC_PER_MSEC)
#define INT_COMPLETE_MASK                                                                          \
	(SHA2DMA_INTSTS_TFR_COMPLETE_Msk | SHA2DMA_INTSTS_BLK_COMPLETE_Msk |                       \
	 SHA2DMA_INTSTS_SCR_COMPLETE_Msk | SHA2DMA_INTSTS_DST_COMPLETE_Msk |                       \
	 SHA2DMA_INTSTS_BUS_COMPLETE_Msk)

static void rts5912_sha256_start(const struct device *dev)
{
	const struct rts5912_sha_config *cfg = dev->config;
	volatile struct sha2_type *sha2_regs = (volatile struct sha2_type *)cfg->cfg_sha2_regs;
	volatile struct sha2dma_type *sha2dma_regs =
		(volatile struct sha2dma_type *)cfg->cfg_sha2dma_regs;
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;

	if (rts5912_sha256_ctx->is224) {
		sha2_regs->ctrl = SHA2_CTRL_BYTEINV_Msk | SHA2_CTRL_ICGEN_Msk;
		for (uint32_t i = 0; i < 8; i++) {
			sha2_regs->digest[i << 1] = rts5912_sha224_digest[i];
			sha2_regs->digest[(i << 1) + 1] = 0x0;
		}
	} else {
		sha2_regs->ctrl = SHA2_CTRL_RST_Msk | SHA2_CTRL_BYTEINV_Msk | SHA2_CTRL_ICGEN_Msk;
	}

	sha2dma_regs->dma_enable = SHA2DMA_DMA_ENABLE_Msk;
	sha2dma_regs->config = 0x0;
	sha2dma_regs->dar = 0x0;
	sha2dma_regs->ctrl_low = (sha2dma_regs->ctrl_low & RTS5912_SHA2DMA_HIGH_LEVEL_MSK) |
				 (SHA2DMA_CTRL_INT_EN_Msk |
				  (RTS5912_SHA2DMA_DST_WIDTH << SHA2DMA_CTRL_DST_WIDTH_Pos) |
				  (RTS5912_SHA2DMA_SRC_WIDTH << SHA2DMA_CTRL_SRC_WIDTH_Pos) |
				  (0x2 << SHA2DMA_CTRL_SRC_BURST_Pos));
	sha2dma_regs->msk_transfer = (sha2dma_regs->msk_transfer & RTS5912_SHA2DMA_HIGH_LEVEL_MSK) |
				     SHA2DMA_MSKTFR_INT_EN_Msk | SHA2DMA_MSKTFR_INT_WRITE_EN_Msk;
	sha2dma_regs->msk_block = 0x0;
}

static int rts5912_sha256_process(const struct device *dev, uint8_t *input, size_t blk_size)
{
	const struct rts5912_sha_config *cfg = dev->config;
	volatile struct sha2_type *sha2_regs = (volatile struct sha2_type *)cfg->cfg_sha2_regs;
	volatile struct sha2dma_type *sha2dma_regs =
		(volatile struct sha2dma_type *)cfg->cfg_sha2dma_regs;
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;
	uint32_t idx = 0;

	for (; blk_size > 0; blk_size -= RTS5912_SHA2DMA_MAXIMUM_BLOCK_NUM) {
		sha2dma_regs->sar = (uint32_t)(&(input[idx]));
		if (blk_size <= RTS5912_SHA2DMA_MAXIMUM_BLOCK_NUM) {
			sha2dma_regs->ctrl_high = blk_size << RTS5912_SHA2DMA_8Byte_SHIFT;
		} else {
			sha2dma_regs->ctrl_high = RTS5912_SHA2DMA_MAXIMUM_BLOCK_NUM
						  << RTS5912_SHA2DMA_8Byte_SHIFT;
		}
		sha2dma_regs->channel_enable = SHA2DMA_CHEN_EN_Msk | SHA2DMA_CHEN_WRITE_EN_Msk;

		uint32_t _wf_cycle_count =
			k_us_to_cyc_ceil32(RTS5912_MAXIMUM_CRYPTO_POLLING_TIME_US);
		uint32_t _wf_start = k_cycle_get_32();
		uint32_t _wf_now = _wf_start;

		while (!((sha2dma_regs->interrupt_status & INT_COMPLETE_MASK) != 0) &&
		       (_wf_cycle_count > (_wf_now - _wf_start))) {
			k_msleep(1);
			Z_SPIN_DELAY(10);
			_wf_now = k_cycle_get_32();
		}

		if (_wf_cycle_count < (_wf_now - _wf_start)) {
			LOG_ERR("SHA2DMA reach timeout and breach");
			return -EIO;
		}

		if (sha2dma_regs->interrupt_status & SHA2DMA_INTSTS_BUS_COMPLETE_Msk) {
			return -EIO;
		}

		sha2dma_regs->clear_transfer = SHA2DMA_INTCLR_CLRTFR_Msk;
		idx += RTS5912_SHA2DMA_MAXIMUM_BLOCK_NUM << RTS5912_SHA2DMA_BLOCK_SHIFT;

		if (blk_size <= RTS5912_SHA2DMA_MAXIMUM_BLOCK_NUM) {
			break;
		}
	}
	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	for (uint32_t i = 0; i < 8; i++) {
		rts5912_sha256_ctx->state[i] = sha2_regs->digest[i << 1];
	}
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
	return 0;
}

static int rts5912_sha256_update(const struct device *dev, uint8_t *input, size_t len)
{
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;
	uint32_t remain, fill, blk_size = 0, ret_val = 0;

	remain = rts5912_sha256_ctx->total[0] & (RTS5912_SHA2DMA_BLOCK_SIZE - 1);
	fill = RTS5912_SHA2DMA_BLOCK_SIZE - remain;

	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	rts5912_sha256_ctx->total[0] += len;
	if (rts5912_sha256_ctx->total[0] < len) {
		rts5912_sha256_ctx->total[1]++;
	}
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));

	if ((len >= fill) && (remain != 0)) {
		k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
		memcpy((void *)(&(rts5912_sha256_ctx->buffer[remain])), input, fill);
		k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
		ret_val = rts5912_sha256_process(dev, (&(rts5912_sha256_ctx->buffer[0])), 0x1);
		if (ret_val != 0) {
			return ret_val;
		}

		input += fill;
		len -= fill;
		remain = 0;
	}

	while (len >= RTS5912_SHA2DMA_BLOCK_SIZE) {
		blk_size = len >> RTS5912_SHA2DMA_BLOCK_SHIFT;
		ret_val = rts5912_sha256_process(dev, input, blk_size);
		if (ret_val != 0) {
			return ret_val;
		}
		input += (len & ~(RTS5912_SHA2DMA_BLOCK_SIZE - 1));
		len &= (RTS5912_SHA2DMA_BLOCK_SIZE - 1);
	}

	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	if (len > 0) {
		memcpy((void *)(&(rts5912_sha256_ctx->buffer[remain])), input, len);
	}
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));

	return ret_val;
}

static int rts5912_sha256_finish(const struct device *dev, uint8_t *output)
{
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;
	uint32_t remain, be_high, be_low, ret_val = 0;

	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	remain = rts5912_sha256_ctx->total[0] & (RTS5912_SHA2DMA_BLOCK_SIZE - 1);
	rts5912_sha256_ctx->buffer[remain++] = 0x80U;

	if (remain <= RTS5912_SHA2_BLOCK_EXTEND_CHECK) {
		memset((void *)(&(rts5912_sha256_ctx->buffer[remain])), 0x0,
		       RTS5912_SHA2_BLOCK_EXTEND_CHECK - remain);
	} else {
		memset((void *)(&(rts5912_sha256_ctx->buffer[remain])), 0x0,
		       RTS5912_SHA2DMA_BLOCK_SIZE - remain);
		k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
		ret_val = rts5912_sha256_process(dev, (uint8_t *)(&(rts5912_sha256_ctx->buffer[0])),
						 0x1);
		if (ret_val != 0) {
			return ret_val;
		}
		k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
		memset((void *)(&(rts5912_sha256_ctx->buffer[0])), 0x0,
		       RTS5912_SHA2_BLOCK_EXTEND_CHECK);
	}

	be_high = (rts5912_sha256_ctx->total[0] >> (32 - RTS5912_SHA2DMA_8Byte_SHIFT)) |
		  (rts5912_sha256_ctx->total[1] << RTS5912_SHA2DMA_8Byte_SHIFT);
	be_low = rts5912_sha256_ctx->total[0] << RTS5912_SHA2DMA_8Byte_SHIFT;
	*(uint32_t *)(&(rts5912_sha256_ctx->buffer[56])) = sys_cpu_to_be32(be_high);
	*(uint32_t *)(&(rts5912_sha256_ctx->buffer[60])) = sys_cpu_to_be32(be_low);

	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
	ret_val = rts5912_sha256_process(dev, (uint8_t *)(&(rts5912_sha256_ctx->buffer[0])), 0x1);
	if (ret_val != 0) {
		return ret_val;
	}
	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);

	volatile uint32_t *output_buf = (volatile uint32_t *)output;

	for (uint32_t i = 0; i < 7; i++) {
		output_buf[i] = sys_cpu_to_be32(rts5912_sha256_ctx->state[i]);
	}
	if (rts5912_sha256_ctx->is224 == 0) {
		output_buf[7] = sys_cpu_to_be32(rts5912_sha256_ctx->state[7]);
	}
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
	return ret_val;
}

static int rts5912_sha256_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct rts5912_sha256_context *rts5912_sha256_ctx =
		(struct rts5912_sha256_context *)(ctx->device)->data;
	int ret_val = 0;

	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	memcpy(rts5912_sha256_ctx->sha2_data_in_sram, pkt->in_buf, pkt->in_len);
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
	ret_val = rts5912_sha256_update(ctx->device, rts5912_sha256_ctx->sha2_data_in_sram,
					pkt->in_len);

	if (ret_val) {
		return ret_val;
	} else if (finish) {
		ret_val = rts5912_sha256_finish(ctx->device, pkt->out_buf);
	}

	return ret_val;
}

static int rts5912_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
				      enum hash_algo algo)
{
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;

	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	if (rts5912_sha256_ctx->in_use == true) {
		LOG_ERR("Crypto driver is busy!");
		k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
		return -EBUSY;
	}

	rts5912_sha256_ctx->in_use = true;
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
	case CRYPTO_HASH_ALGO_SHA256:
		k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
		memset((void *)(rts5912_sha256_ctx), 0x0, sizeof(struct rts5912_sha256_context));
		rts5912_sha256_ctx->is224 = (algo == CRYPTO_HASH_ALGO_SHA224);
		k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));
		ctx->hash_hndlr = rts5912_sha256_handler;
		rts5912_sha256_start(dev);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rts5912_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;

	k_mutex_lock(&(rts5912_sha256_ctx->crypto_rts5912_in_use), K_FOREVER);
	rts5912_sha256_ctx->in_use = false;
	k_mutex_unlock(&(rts5912_sha256_ctx->crypto_rts5912_in_use));

	return 0;
}

static inline int rts5912_query_hw_caps(const struct device *dev)
{
	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int rts5912_sha_init(const struct device *dev)
{
	uint32_t ret;
	struct rts5912_sha256_context *rts5912_sha256_ctx = dev->data;
	uint8_t init_buf[32] = {0};
	struct hash_pkt pkt = {
		.in_buf = init_buf,
		.out_buf = init_buf,
		.in_len = 32,
		.ctx = NULL,
	};

	k_mutex_init(&(rts5912_sha256_ctx->crypto_rts5912_in_use));

	/* For Realtek crypto driver, it has to be inited by running one time
	 * to clear the register of the crypto driver and make sure the date in
	 * the driver are cleaned.
	 */

	ret = rts5912_hash_begin_session(dev, pkt.ctx, CRYPTO_HASH_ALGO_SHA256);
	if (ret != 0) {
		LOG_ERR("Crypto driver init begin fail!");
		return ret;
	}

	ret = rts5912_sha256_update(dev, pkt.in_buf, pkt.in_len);
	if (ret != 0) {
		LOG_ERR("Crypto driver init update fail!");
		return ret;
	}

	ret = rts5912_sha256_finish(dev, pkt.out_buf);
	if (ret != 0) {
		LOG_ERR("Crypto driver init finish fail!");
		return ret;
	}

	ret = rts5912_hash_free_session(dev, pkt.ctx);
	if (ret != 0) {
		LOG_ERR("Crypto driver init free fail!");
		return ret;
	}

	return 0;
}

static DEVICE_API(crypto, rts5912_hash_funcs) = {
	.hash_begin_session = rts5912_hash_begin_session,
	.hash_free_session = rts5912_hash_free_session,
	.query_hw_caps = rts5912_query_hw_caps,
};

const struct rts5912_sha_config cfg_0 = {
	.cfg_sha2_regs = (volatile struct sha2_type *)DT_INST_REG_ADDR_BY_NAME(0, sha2),
	.cfg_sha2dma_regs = (volatile struct sha2dma_type *)DT_INST_REG_ADDR_BY_NAME(0, sha2dma),
};

static struct rts5912_sha256_context data_0;

DEVICE_DT_INST_DEFINE(0, &rts5912_sha_init, NULL, &data_0, &cfg_0, POST_KERNEL,
		      CONFIG_CRYPTO_INIT_PRIORITY, (void *)&rts5912_hash_funcs);
