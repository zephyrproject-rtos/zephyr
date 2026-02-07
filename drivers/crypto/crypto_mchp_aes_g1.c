/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_aes_g1_crypto

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/cipher.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(mchp_aes, CONFIG_CRYPTO_LOG_LEVEL);

#define MCHP_AES_CAPS_SUPPORT							\
		(CAP_RAW_KEY | CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS |		\
		 CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

#define AES_VECTOR_SIZE 16

struct crypto_mchp_aes_cfg {
	aes_registers_t *const regs;
	const struct sam_clk_cfg clock_cfg;
};

struct crypto_mchp_aes_data {
	struct k_mutex aes_lock;
};

struct crypto_mchp_aes_session {
	bool in_use;
	uint8_t key[32];
	size_t key_len;
	uint8_t reg_mr_keysize;
	uint8_t reg_mr_opmod;
	enum cipher_op dir;
	enum cipher_mode mode;
};

static struct crypto_mchp_aes_session mchp_aes_sessions[2];
static struct k_sem mchp_aes_session_sem;

static ALWAYS_INLINE void aes_write_vector(aes_registers_t *const regs, const uint8_t *vector)
{
	const uint32_t *data_u32 = (const uint32_t *)vector;

	for (uint32_t i = 0; i < ARRAY_SIZE(regs->AES_IVR); i++) {
		regs->AES_IVR[i] = data_u32[i];
	}
}

static ALWAYS_INLINE void aes_write_key(aes_registers_t *const regs, const uint8_t *key,
					uint32_t len)
{
	const uint32_t *data_u32 = (const uint32_t *)key;

	for (uint32_t i = 0; i < len / sizeof(uint32_t); i++) {
		regs->AES_KEYWR[i] = data_u32[i];
	}
}

static void aes_write_input(aes_registers_t *const regs, const uint8_t *data)
{
	uint32_t i, v;
	uint8_t size = 4;

	if ((regs->AES_MR & AES_MR_OPMOD_Msk) == AES_MR_OPMOD_CFB) {
		if ((regs->AES_MR & AES_MR_CFBS_Msk) == AES_MR_CFBS_SIZE_128BIT) {
			size = 4;
		} else if ((regs->AES_MR & AES_MR_CFBS_Msk) == AES_MR_CFBS_SIZE_64BIT) {
			size = 2;
		} else {
			size = 1;
		}
	}

	/* In 32, 16, and 8-bit CFB modes, writing to AES_IDATAR1, AES_IDATAR2 and AES_IDATAR3
	 * is not allowed and may lead to errors in processing.
	 */
	for (i = 0; i < size; i++) {
		memcpy(&v, &data[i * sizeof(uint32_t)], sizeof(uint32_t));
		regs->AES_IDATAR[i] = v;
	}
}

static ALWAYS_INLINE void aes_read_output(aes_registers_t *const regs, uint8_t *data)
{
	uint32_t *data_u32 = (uint32_t *)data;

	for (uint32_t i = 0; i < ARRAY_SIZE(regs->AES_ODATAR); i++) {
		data_u32[i] = regs->AES_ODATAR[i];
	}
}

static int mchp_aes_process(struct cipher_ctx *ctx, int32_t in_len, const uint8_t *const in_buf,
			    int32_t *const out_len, uint8_t *const out_buf, uint8_t *const iv)
{
	struct crypto_mchp_aes_session *session = ctx->drv_sessn_state;
	const struct crypto_mchp_aes_cfg *cfg = ctx->device->config;
	aes_registers_t *const regs = cfg->regs;
	struct crypto_mchp_aes_data *data = ctx->device->data;
	volatile uint32_t val;

	k_mutex_lock(&data->aes_lock, K_FOREVER);

	regs->AES_CR = AES_CR_SWRST_Msk;

	val = regs->AES_MR;
	val &= ~(AES_MR_OPMOD_Msk | AES_MR_KEYSIZE_Msk | AES_MR_CKEY_Msk | AES_MR_CIPHER_Msk);
	regs->AES_MR = val | AES_MR_CKEY_PASSWD | AES_MR_OPMOD(session->reg_mr_opmod) |
		       AES_MR_KEYSIZE(session->reg_mr_keysize) |
		       AES_MR_CIPHER(session->dir == CRYPTO_CIPHER_OP_ENCRYPT ? 1 : 0);

	aes_write_key(regs, session->key, session->key_len);
	if (iv != NULL) {
		aes_write_vector(regs, iv);
	}

	*out_len = 0;
	for (int i = 0; i < in_len; i += 16) {
		aes_write_input(regs, in_buf + i);
		regs->AES_CR = AES_CR_START_Msk;
		while ((regs->AES_ISR & AES_ISR_DATRDY_Msk) != AES_ISR_DATRDY_Msk) {
		}
		aes_read_output(regs, out_buf + i);
		*out_len += 16;
	}

	k_mutex_unlock(&data->aes_lock);

	return 0;
}

static int aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct crypto_mchp_aes_session *session = ctx->drv_sessn_state;

	if (!session || (pkt->in_len % 16U) != 0U) {
		LOG_ERR("Invalid ECB op: session=%p, in_len=%zu", session,
			session ? pkt->in_len : 0);
		return -EINVAL;
	}

	if (pkt->in_len > pkt->out_buf_max) {
		LOG_ERR("Short of output buffer.");
		return -ENOSR;
	}

	return mchp_aes_process(ctx, pkt->in_len, pkt->in_buf, &pkt->out_len, pkt->out_buf, NULL);
}

static int aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct crypto_mchp_aes_session *session = ctx->drv_sessn_state;
	int32_t in_len = pkt->in_len;
	uint8_t *in_buf = pkt->in_buf;
	uint8_t *out_buf = pkt->out_buf;

	if (session->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
		out_buf += AES_VECTOR_SIZE;
	} else {
		in_buf += AES_VECTOR_SIZE;
		in_len -= AES_VECTOR_SIZE;
	}

	if (!session || (pkt->in_len % 16) != 0U) {
		LOG_ERR("Invalid CBC op: session=%p, in_len=%zu", session,
			session ? pkt->in_len : 0);
		return -EINVAL;
	}

	return mchp_aes_process(ctx, in_len, in_buf, &pkt->out_len, out_buf, iv);
}

static int aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct crypto_mchp_aes_session *session = ctx->drv_sessn_state;
	uint32_t ctr_len_bits;
	uint8_t counter_blk[16];

	if (!session) {
		return -EINVAL;
	}

	if ((pkt->in_len > 0 && pkt->in_buf == NULL) || pkt->out_buf == NULL) {
		return -EINVAL;
	}

	ctr_len_bits = ctx->mode_params.ctr_info.ctr_len;

	if (ctr_len_bits == 0 || (ctr_len_bits % BITS_PER_BYTE) != 0 || ctr_len_bits > 128) {
		LOG_ERR("Invalid CTR counter length: %u bits", ctr_len_bits);
		return -EINVAL;
	}

	memset(counter_blk, 0, sizeof(counter_blk));
	memcpy(counter_blk, iv, AES_VECTOR_SIZE - ctr_len_bits / BITS_PER_BYTE);

	return mchp_aes_process(ctx, pkt->in_len, pkt->in_buf, &pkt->out_len, pkt->out_buf,
				counter_blk);
}

static struct crypto_mchp_aes_session *crypto_mchp_aes_get_unused_session(void)
{
	k_sem_take(&mchp_aes_session_sem, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(mchp_aes_sessions); i++) {
		if (!mchp_aes_sessions[i].in_use) {
			mchp_aes_sessions[i].in_use = true;
			k_sem_give(&mchp_aes_session_sem);

			return &mchp_aes_sessions[i];
		}
	}

	k_sem_give(&mchp_aes_session_sem);

	return NULL;
}

static int mchp_aes_check_parameters(struct cipher_ctx *ctx, enum cipher_algo algo,
				     enum cipher_mode mode)
{
	if (ctx->flags & ~(MCHP_AES_CAPS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algorithm: %d", algo);
		return -ENOTSUP;
	}

	if (!(mode == CRYPTO_CIPHER_MODE_ECB || mode == CRYPTO_CIPHER_MODE_CBC ||
	      mode == CRYPTO_CIPHER_MODE_CTR)) {
		LOG_ERR("Unsupported mode: %d", mode);
		return -ENOTSUP;
	}

	if (ctx->key.bit_stream == NULL) {
		LOG_ERR("No key provided");
		return -EINVAL;
	}

	if (!(ctx->keylen == 16 || ctx->keylen == 24 || ctx->keylen == 32)) {
		LOG_ERR("Invalid key length: %zu", ctx->keylen);
		return -EINVAL;
	}

	return 0;
}

static int mchp_aes_begin_session(const struct device *dev, struct cipher_ctx *ctx,
				  enum cipher_algo algo, enum cipher_mode mode,
				  enum cipher_op optype)
{
	ARG_UNUSED(dev);

	struct crypto_mchp_aes_session *session;
	int ret = mchp_aes_check_parameters(ctx, algo, mode);

	if (ret < 0) {
		return ret;
	}

	session = crypto_mchp_aes_get_unused_session();
	if (!session) {
		return -ENOMEM;
	}

	session->mode = mode;
	session->dir = optype;
	session->key_len = ctx->keylen;
	/* calculate values for register AES_MR.KEYSIZE
	 *         | lengths of the keys | AES_MR.KEYSIZE
	 * --------|---------------------|---------------
	 * AES-128 | 128 bits = 16 bytes |        0
	 * AES-192 | 192 bits = 24 bytes |        1
	 * AES-256 | 256 bits = 32 bytes |        2
	 */
	session->reg_mr_keysize = (ctx->keylen / BITS_PER_BYTE) - 2;
	memcpy(session->key, ctx->key.bit_stream, ctx->keylen);

	ctx->ops.cipher_mode = mode;
	ctx->device = dev;
	ctx->drv_sessn_state = session;
	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		session->reg_mr_opmod = AES_MR_OPMOD_ECB_Val;
		ctx->ops.block_crypt_hndlr = aes_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		session->reg_mr_opmod = AES_MR_OPMOD_CBC_Val;
		ctx->ops.cbc_crypt_hndlr = aes_cbc_op;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		session->reg_mr_opmod = AES_MR_OPMOD_CTR_Val;
		ctx->ops.ctr_crypt_hndlr = aes_ctr_op;
		break;
	default:
		LOG_ERR("Crypto mode is invalid, should not run into this line.");
		break;
	}

	LOG_DBG("Session started: mode=%d, op=%d, keylen=%zu", session->mode, session->dir,
		session->key_len);

	return 0;
}

static int mchp_aes_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	if (!ctx || !ctx->device || !ctx->drv_sessn_state) {
		LOG_ERR("Tried to free a invalid context or session");
		return -EINVAL;
	}

	if (ctx->device != dev) {
		LOG_ERR("The context or session tried to free is not related to the device");
		return -EINVAL;
	}

	k_sem_take(&mchp_aes_session_sem, K_FOREVER);
	memset(ctx->drv_sessn_state, 0, sizeof(struct crypto_mchp_aes_session));
	k_sem_give(&mchp_aes_session_sem);

	ctx->device = NULL;
	ctx->drv_sessn_state = NULL;

	LOG_DBG("Session freed");

	return 0;
}

static int mchp_aes_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MCHP_AES_CAPS_SUPPORT;
}

static int crypto_mchp_aes_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct crypto_mchp_aes_cfg *cfg = dev->config;
	struct crypto_mchp_aes_data *data = dev->data;

	if (!device_is_ready(pmc)) {
		LOG_ERR("Power Management Controller device not ready");
		return -ENODEV;
	}

	if (clock_control_on(pmc, (clock_control_subsys_t)&cfg->clock_cfg) != 0) {
		LOG_ERR("Clock op failed\n");
		return -EIO;
	}

	k_mutex_init(&data->aes_lock);
	k_sem_init(&mchp_aes_session_sem, 1, 1);

	return 0;
}

static const struct crypto_driver_api mchp_aes_api = {
	.cipher_begin_session = mchp_aes_begin_session,
	.cipher_free_session = mchp_aes_free_session,
	.query_hw_caps = mchp_aes_query_caps,
};

#define CRYPTO_MCHP_AES_INIT(n)							\
		static const struct crypto_mchp_aes_cfg mchp_aes##n##_cfg = {	\
			.regs = (aes_registers_t *)DT_INST_REG_ADDR(n),		\
			.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		};								\
										\
		static struct crypto_mchp_aes_data mchp_aes##n##_data;		\
										\
		DEVICE_DT_INST_DEFINE(n,					\
				      crypto_mchp_aes_init,			\
				      NULL,					\
				      &mchp_aes##n##_data,			\
				      &mchp_aes##n##_cfg,			\
				      POST_KERNEL,				\
				      CONFIG_CRYPTO_INIT_PRIORITY,		\
				      &mchp_aes_api);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_MCHP_AES_INIT)
