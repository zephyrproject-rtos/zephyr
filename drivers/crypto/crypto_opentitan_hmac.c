/*
 * SPDX-FileCopyrightText: 2026 Yousef Kitaneh
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lowrisc_opentitan_hmac

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/hash.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(crypto_opentitan_hmac, CONFIG_CRYPTO_LOG_LEVEL);

/* Register offsets */
#define HMAC_INTR_STATE_OFFSET       0x00U
#define HMAC_INTR_ENABLE_OFFSET      0x04U
#define HMAC_INTR_TEST_OFFSET        0x08U
#define HMAC_ALERT_TEST_OFFSET       0x0cU
#define HMAC_CFG_OFFSET              0x10U
#define HMAC_CMD_OFFSET              0x14U
#define HMAC_STATUS_OFFSET           0x18U
#define HMAC_ERR_CODE_OFFSET         0x1cU
#define HMAC_WIPE_SECRET_OFFSET      0x20U
#define HMAC_KEY_0_OFFSET            0x24U
#define HMAC_DIGEST_0_OFFSET         0x44U
#define HMAC_MSG_LENGTH_LOWER_OFFSET 0x64U
#define HMAC_MSG_LENGTH_UPPER_OFFSET 0x68U
#define HMAC_MSG_FIFO_OFFSET         0x800U

/* INTR_STATE / INTR_ENABLE bits */
#define HMAC_INTR_HMAC_DONE  BIT(0)
#define HMAC_INTR_FIFO_EMPTY BIT(1)
#define HMAC_INTR_HMAC_ERR   BIT(2)
#define HMAC_INTR_ALL        GENMASK(2, 0)

/* CFG register bits */
#define HMAC_CFG_HMAC_EN     BIT(0)
#define HMAC_CFG_SHA_EN      BIT(1)
#define HMAC_CFG_ENDIAN_SWAP BIT(2)
#define HMAC_CFG_DIGEST_SWAP BIT(3)

/* CMD register bits */
#define HMAC_CMD_HASH_START   BIT(0)
#define HMAC_CMD_HASH_PROCESS BIT(1)

/* STATUS register bits */
#define HMAC_STATUS_FIFO_EMPTY BIT(0)
#define HMAC_STATUS_FIFO_FULL  BIT(1)

#define HMAC_SHA256_DIGEST_WORDS 8U
#define HMAC_REG_SIZE            0x4U

#define OPENTITAN_HMAC_CAPS (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

struct crypto_opentitan_hmac_config {
	uintptr_t base;
};

struct crypto_opentitan_hmac_data {
	atomic_t in_use;
};

static void opentitan_hmac_fifo_wait(uintptr_t base)
{
	while ((sys_read32(base + HMAC_STATUS_OFFSET) & HMAC_STATUS_FIFO_FULL) != 0U)
		;
}

static int opentitan_hmac_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct device *dev = ctx->device;
	const struct crypto_opentitan_hmac_config *cfg = dev->config;
	uintptr_t base = cfg->base;

	if (pkt == NULL) {
		return -EINVAL;
	}

	if (pkt->in_len > 0U && pkt->in_buf == NULL) {
		return -EINVAL;
	}

	if (finish && pkt->out_buf == NULL) {
		return -EINVAL;
	}

	if (!ctx->started) {
		sys_write32(HMAC_INTR_ALL, base + HMAC_INTR_STATE_OFFSET);
		/*
		 * Disable ENDIAN_SWAP and use 8-bit writes to bypass
		 * Renode's endianSwap being inverted relative to the RTL.
		 */
		sys_write32(HMAC_CFG_SHA_EN, base + HMAC_CFG_OFFSET);
		sys_write32(HMAC_CMD_HASH_START, base + HMAC_CMD_OFFSET);
		ctx->started = true;
	}

	const uint8_t *buf = pkt->in_buf;
	size_t len = pkt->in_len;

	while (len > 0U) {
		opentitan_hmac_fifo_wait(base);
		sys_write8(*buf++, base + HMAC_MSG_FIFO_OFFSET);
		len--;
	}

	if (finish) {
		sys_write32(HMAC_CMD_HASH_PROCESS, base + HMAC_CMD_OFFSET);

		while ((sys_read32(base + HMAC_INTR_STATE_OFFSET) & HMAC_INTR_HMAC_DONE) == 0U) {
		}

		sys_write32(HMAC_INTR_HMAC_DONE, base + HMAC_INTR_STATE_OFFSET);

		for (size_t i = 0U; i < HMAC_SHA256_DIGEST_WORDS; i++) {
			uint32_t word =
				sys_read32(base + HMAC_DIGEST_0_OFFSET + (i * HMAC_REG_SIZE));

			sys_put_be32(word, &pkt->out_buf[i * sizeof(uint32_t)]);
		}

		ctx->started = false;
	}

	return 0;
}

static int opentitan_hmac_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
					     enum hash_algo algo)
{
	struct crypto_opentitan_hmac_data *data = dev->data;

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_DBG("Unsupported hash algorithm: %d", algo);
		return -ENOTSUP;
	}

	if ((ctx->flags & ~(OPENTITAN_HMAC_CAPS)) != 0U) {
		LOG_DBG("Unsupported session flags: 0x%04x", ctx->flags);
		return -ENOTSUP;
	}

	if (!atomic_cas(&data->in_use, 0, 1)) {
		LOG_DBG("HMAC peripheral already in use");
		return -EBUSY;
	}

	ctx->device = dev;
	ctx->hash_hndlr = opentitan_hmac_hash_handler;
	ctx->started = false;

	return 0;
}

static int opentitan_hmac_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	const struct crypto_opentitan_hmac_config *cfg = dev->config;
	struct crypto_opentitan_hmac_data *data = dev->data;

	if (ctx != NULL) {
		ctx->hash_hndlr = NULL;
		ctx->started = false;
	}

	sys_write32(0U, cfg->base + HMAC_CFG_OFFSET);
	sys_write32(HMAC_INTR_ALL, cfg->base + HMAC_INTR_STATE_OFFSET);

	atomic_clear(&data->in_use);

	return 0;
}

static int opentitan_hmac_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return OPENTITAN_HMAC_CAPS;
}

static int opentitan_hmac_init(const struct device *dev)
{
	const struct crypto_opentitan_hmac_config *cfg = dev->config;

	sys_write32(0U, cfg->base + HMAC_INTR_ENABLE_OFFSET);
	sys_write32(0U, cfg->base + HMAC_CFG_OFFSET);
	sys_write32(HMAC_INTR_ALL, cfg->base + HMAC_INTR_STATE_OFFSET);

	return 0;
}

static DEVICE_API(crypto, opentitan_hmac_crypto_api) = {
	.query_hw_caps = opentitan_hmac_query_hw_caps,
	.hash_begin_session = opentitan_hmac_hash_begin_session,
	.hash_free_session = opentitan_hmac_hash_free_session,
};

#define OPENTITAN_HMAC_INIT(inst)                                                                  \
	static const struct crypto_opentitan_hmac_config opentitan_hmac_cfg_##inst = {             \
		.base = DT_INST_REG_ADDR(inst),                                                    \
	};                                                                                         \
	static struct crypto_opentitan_hmac_data opentitan_hmac_data_##inst;                       \
	DEVICE_DT_INST_DEFINE(inst, opentitan_hmac_init, NULL, &opentitan_hmac_data_##inst,        \
			      &opentitan_hmac_cfg_##inst, POST_KERNEL,                             \
			      CONFIG_CRYPTO_INIT_PRIORITY, &opentitan_hmac_crypto_api);

DT_INST_FOREACH_STATUS_OKAY(OPENTITAN_HMAC_INIT)
