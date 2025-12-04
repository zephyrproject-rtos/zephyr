/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_sha_g1_crypto

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cypto_mchp_sha_g1);

#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#define MCHP_SHA_CAPS_SUPPORT (CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

#define FIRST_WORD_4_PADDING_EMPTY_MSG 0x000000080

struct crypto_mchp_sha_cfg {
	sha_registers_t *const regs;
	const struct sam_clk_cfg clock_cfg;
};

struct crypto_mchp_sha_data {
	struct k_sem device_sem;
};

struct crypto_mchp_sha_algo_cfg {
	enum hash_algo algo;
	uint32_t sha_mr_algo;
	uint8_t dgst_len;
	uint8_t block_size;
};

struct crypto_mchp_sha_session {
	struct crypto_mchp_sha_algo_cfg const *algo_cfg;
	bool in_use;
};

static const struct crypto_mchp_sha_algo_cfg mchp_sha_algo_cfgs[4] = {
	{CRYPTO_HASH_ALGO_SHA224, SHA_MR_ALGO_SHA224, 224 / BITS_PER_BYTE, 64},
	{CRYPTO_HASH_ALGO_SHA256, SHA_MR_ALGO_SHA256, 256 / BITS_PER_BYTE, 64},
	{CRYPTO_HASH_ALGO_SHA384, SHA_MR_ALGO_SHA384, 384 / BITS_PER_BYTE, 128},
	{CRYPTO_HASH_ALGO_SHA512, SHA_MR_ALGO_SHA512, 512 / BITS_PER_BYTE, 128},
};

static struct crypto_mchp_sha_session mchp_sha_sessions[2];
static struct k_sem mchp_sha_session_sem;

static inline void mchp_sha_set_input(sha_registers_t *const regs, const unsigned char *data,
				      int len)
{
	const uint32_t *data_u32 = (const uint32_t *)data;
	uint32_t i = 0, v;

	while (len >= sizeof(uint32_t)) {
		v = data_u32[i];
		if (i < ARRAY_SIZE(regs->SHA_IDATAR)) {
			regs->SHA_IDATAR[i] = v;
		} else {
			regs->SHA_IODATAR[i - ARRAY_SIZE(regs->SHA_IDATAR)] = v;
		}
		i++;
		len -= sizeof(uint32_t);
	}

	if (len > 0) {
		v = 0;
		memcpy(&v, &data[i * sizeof(uint32_t)], len);
		if (i < ARRAY_SIZE(regs->SHA_IDATAR)) {
			regs->SHA_IDATAR[i] = v;
		} else {
			regs->SHA_IODATAR[i - ARRAY_SIZE(regs->SHA_IDATAR)] = v;
		}
	}
}

static inline void mchp_sha_get_output(sha_registers_t *const regs, unsigned char *data, int len)
{
	uint32_t *data_u32 = (uint32_t *)data;

	for (uint32_t i = 0; i < MIN(len / sizeof(uint32_t), ARRAY_SIZE(regs->SHA_IODATAR)); i++) {
		data_u32[i] = regs->SHA_IODATAR[i];
	}
}

static int mchp_sha_wait_data_rdy(sha_registers_t *const regs)
{
	uint32_t timeout = 1;

	while ((regs->SHA_ISR & SHA_ISR_DATRDY_Msk) == 0) {
		if (timeout-- == 0) {
			LOG_ERR("MCHP SHA wait data ready timeout");
			return -ETIMEDOUT;
		}
		k_busy_wait(5);
	}

	return 0;
}

static int mchp_sha_process(sha_registers_t *const regs,
			    struct crypto_mchp_sha_algo_cfg const *algo_cfg,
			    const unsigned char *data, unsigned int len, unsigned char *digest)
{
	int block_size = algo_cfg->block_size;
	unsigned int processed = 0;
	unsigned int cnt;
	int ret;

	regs->SHA_CR = SHA_CR_SWRST_Msk;
	regs->SHA_MR = algo_cfg->sha_mr_algo | SHA_MR_SMOD_AUTO_START | SHA_MR_PROCDLY_LONGEST;
	regs->SHA_CR = SHA_CR_FIRST_Msk;
	regs->SHA_MSR = len;
	regs->SHA_BCR = len;

	if (len == 0) {
		/* For the empty message, automatic padding is not quired in this driver
		 * (SHA_MSR.MSGSIZE and SHA_BCR.BYTCNT are configured to 0). The block to
		 * be processed are the padded part (a one bit, then  '1' followed by zeor
		 * bits) and then the message length.
		 */
		regs->SHA_IDATAR[0] = FIRST_WORD_4_PADDING_EMPTY_MSG;
		for (int i = 1; i < block_size / sizeof(uint32_t); i++) {
			regs->SHA_IDATAR[i] = 0;
		}
		goto out;
	}

	while (len - processed != 0) {
		cnt = MIN(block_size, len - processed);
		/* Write the block to be processed in the Input Data Registers */
		mchp_sha_set_input(regs, &data[processed], cnt);
		ret = mchp_sha_wait_data_rdy(regs);
		if (ret < 0) {
			return ret;
		}

		processed += cnt;
	}

out:
	ret = mchp_sha_wait_data_rdy(regs);
	if (ret < 0) {
		return ret;
	}
	mchp_sha_get_output(regs, digest, algo_cfg->dgst_len);

	return ret;
}

static struct crypto_mchp_sha_session *crypto_mchp_sha_get_unused_session(void)
{
	struct crypto_mchp_sha_session *session = NULL;

	k_sem_take(&mchp_sha_session_sem, K_FOREVER);
	for (int i = 0; i < ARRAY_SIZE(mchp_sha_sessions); i++) {
		if (!mchp_sha_sessions[i].in_use) {
			session = &mchp_sha_sessions[i];
			break;
		}
	}
	k_sem_give(&mchp_sha_session_sem);

	return session;
}

static int mchp_sha_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct device *dev = ctx->device;
	const struct crypto_mchp_sha_cfg *cfg = dev->config;
	struct crypto_mchp_sha_data *data = dev->data;
	struct crypto_mchp_sha_session *session = ctx->drv_sessn_state;
	bool inplace_ops = (ctx->flags & CAP_INPLACE_OPS) == CAP_INPLACE_OPS;
	int ret;

	if (!pkt || !pkt->in_buf || (!pkt->out_buf && !inplace_ops)) {
		LOG_ERR("Invalid packet buffers");
		return -EINVAL;
	}

	if (!finish) {
		LOG_ERR("Multipart shaing not supported yet");
		return -ENOTSUP;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	if (session->algo_cfg == NULL) {
		k_sem_give(&data->device_sem);
		LOG_ERR("Unsupported algorithm");
		return -ENOTSUP;
	}

	if (inplace_ops && pkt->in_len < session->algo_cfg->dgst_len) {
		k_sem_give(&data->device_sem);
		LOG_ERR("Insufficient in_buf for digest");
		return -EINVAL;
	}

	ret = mchp_sha_process(cfg->regs, session->algo_cfg, pkt->in_buf, pkt->in_len,
			       inplace_ops ? (uint8_t *)(uintptr_t)pkt->in_buf : pkt->out_buf);

	k_sem_give(&data->device_sem);

	return ret;
}

static int mchp_sha_begin_session(const struct device *dev, struct hash_ctx *ctx,
				  enum hash_algo algo)
{
	struct crypto_mchp_sha_algo_cfg const *algo_cfg = NULL;
	struct crypto_mchp_sha_session *session;

	if (ctx->flags & ~(MCHP_SHA_CAPS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	for (int i = 0; i < ARRAY_SIZE(mchp_sha_algo_cfgs); i++) {
		if (algo == mchp_sha_algo_cfgs[i].algo) {
			algo_cfg = &mchp_sha_algo_cfgs[i];
			break;
		}
	}

	if (algo_cfg == NULL) {
		LOG_ERR("Unsupported hash algorithm: %d", algo);
		return -ENOTSUP;
	}

	session = crypto_mchp_sha_get_unused_session();
	if (session == NULL) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}
	session->algo_cfg = algo_cfg;

	ctx->device = dev;
	ctx->drv_sessn_state = session;
	ctx->hash_hndlr = mchp_sha_handler;
	ctx->started = false;

	LOG_DBG("Session started: algo=%d", algo);

	return 0;
}

static int mchp_sha_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	if (!ctx || !ctx->device || !ctx->drv_sessn_state) {
		LOG_ERR("Tried to free a invalid context or session");
		return -EINVAL;
	}

	if (ctx->device != dev) {
		LOG_ERR("The context or session tried to free is not related to the device");
		return -EINVAL;
	}

	k_sem_take(&mchp_sha_session_sem, K_FOREVER);
	memset(ctx->drv_sessn_state, 0, sizeof(struct crypto_mchp_sha_session));
	k_sem_give(&mchp_sha_session_sem);

	ctx->device = NULL;
	ctx->drv_sessn_state = NULL;

	LOG_DBG("Session freed");

	return 0;
}

static int mchp_sha_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MCHP_SHA_CAPS_SUPPORT;
}

static int crypto_mchp_sha_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct crypto_mchp_sha_cfg *cfg = dev->config;
	struct crypto_mchp_sha_data *data = dev->data;

	if (!device_is_ready(pmc)) {
		LOG_ERR("Power Management Controller device not ready");
		return -ENODEV;
	}

	if (clock_control_on(pmc, (clock_control_subsys_t)(uintptr_t)&(cfg->clock_cfg)) != 0) {
		LOG_ERR("Clock op failed\n");
		return -EIO;
	}

	k_sem_init(&data->device_sem, 1, 1);
	k_sem_init(&mchp_sha_session_sem, 1, 1);

	return 0;
}

static DEVICE_API(crypto, mchp_sha_api) = {
	.hash_begin_session = mchp_sha_begin_session,
	.hash_free_session = mchp_sha_free_session,
	.query_hw_caps = mchp_sha_query_caps,
};

#define CRYPTO_MCHP_SHA_INIT(n)							\
		static const struct crypto_mchp_sha_cfg mchp_sha##n##_cfg = {	\
			.regs = (sha_registers_t *)DT_INST_REG_ADDR(n),		\
			.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		};								\
										\
		static struct crypto_mchp_sha_data mchp_sha##n##_data;		\
										\
		DEVICE_DT_INST_DEFINE(n,					\
				      crypto_mchp_sha_init,			\
				      NULL,					\
				      &mchp_sha##n##_data,			\
				      &mchp_sha##n##_cfg,			\
				      POST_KERNEL,				\
				      CONFIG_CRYPTO_INIT_PRIORITY,		\
				      &mchp_sha_api);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_MCHP_SHA_INIT)
