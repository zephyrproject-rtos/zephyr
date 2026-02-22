/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bouffalo Lab SEC Engine AES cipher driver
 */

#define DT_DRV_COMPAT bflb_sec_eng_aes

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <bouffalolab/common/sec_eng_reg.h>

LOG_MODULE_REGISTER(crypto_bflb_aes, CONFIG_CRYPTO_LOG_LEVEL);

#define BFLB_AES_BLOCK_SIZE 16

/* AES block cipher mode values (for BLOCK_MODE field) */
#define BFLB_AES_MODE_ECB 0
#define BFLB_AES_MODE_CTR 1
#define BFLB_AES_MODE_CBC 2
#define BFLB_AES_MODE_XTS 3

/* AES key size values (for MODE field) */
#define BFLB_AES_KEY_128 0
#define BFLB_AES_KEY_192 1
#define BFLB_AES_KEY_256 2

/*
 * HAL register offsets are relative to SEC_ENG_BASE. The DT node provides a
 * named "sec_eng" register region with that base address.
 */
#define SEC_ENG_AES_BASE(inst) DT_INST_REG_ADDR_BY_NAME(inst, sec_eng)

struct bflb_aes_session {
	bool in_use;
	uint8_t key[32];
	uint16_t key_len;
	enum cipher_algo algo;
	enum cipher_mode mode;
	enum cipher_op op;
};

struct crypto_bflb_aes_data {
	struct k_mutex lock;
	struct bflb_aes_session sessions[CONFIG_CRYPTO_BFLB_SEC_ENG_AES_MAX_SESSION];
};

struct crypto_bflb_aes_config {
	uintptr_t base;
};

/* Helpers */

static int bflb_aes_wait_not_busy(uintptr_t base)
{
	uint32_t timeout = 100000;

	while (timeout--) {
		if (!(sys_read32(base + SEC_ENG_SE_AES_0_CTRL_OFFSET) & SEC_ENG_SE_AES_0_BUSY)) {
			return 0;
		}
		k_busy_wait(1);
	}

	return -ETIMEDOUT;
}

/* AES implementation */

static void bflb_aes_hw_init(uintptr_t base)
{
	uint32_t regval;

	/* Set endianness to big-endian */
	sys_write32(0x1f, base + SEC_ENG_SE_AES_0_ENDIAN_OFFSET);

	/* Enable AES */
	regval = sys_read32(base + SEC_ENG_SE_AES_0_CTRL_OFFSET);
	regval |= SEC_ENG_SE_AES_0_EN;
	sys_write32(regval, base + SEC_ENG_SE_AES_0_CTRL_OFFSET);
}

static int bflb_aes_do_op(uintptr_t base, const struct bflb_aes_session *sess, const uint8_t *in,
			  uint8_t *out, uint32_t len, const uint8_t *iv, bool decrypt)
{
	uint32_t regval;
	uint8_t block_mode;
	uint8_t key_mode;
	const uint8_t *key = sess->key;
	int ret;

	if (len % BFLB_AES_BLOCK_SIZE != 0) {
		return -EINVAL;
	}

	switch (sess->mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		block_mode = BFLB_AES_MODE_ECB;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		block_mode = BFLB_AES_MODE_CBC;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		block_mode = BFLB_AES_MODE_CTR;
		break;
	default:
		return -ENOTSUP;
	}

	regval = sys_read32(base + SEC_ENG_SE_AES_0_CTRL_OFFSET);
	regval &= ~SEC_ENG_SE_AES_0_BLOCK_MODE_MASK;
	regval |= (block_mode << SEC_ENG_SE_AES_0_BLOCK_MODE_SHIFT);
	if (sess->mode == CRYPTO_CIPHER_MODE_CTR) {
		regval |= SEC_ENG_SE_AES_0_DEC_KEY_SEL;
	} else {
		regval &= ~SEC_ENG_SE_AES_0_DEC_KEY_SEL;
	}
	sys_write32(regval, base + SEC_ENG_SE_AES_0_CTRL_OFFSET);

	/* Set key size */
	regval = sys_read32(base + SEC_ENG_SE_AES_0_CTRL_OFFSET);
	regval &= ~SEC_ENG_SE_AES_0_MODE_MASK;
	regval &= ~SEC_ENG_SE_AES_0_HW_KEY_EN;
	switch (sess->key_len) {
	case 16:
		key_mode = BFLB_AES_KEY_128;
		break;
	case 24:
		key_mode = BFLB_AES_KEY_192;
		break;
	case 32:
		key_mode = BFLB_AES_KEY_256;
		break;
	default:
		return -EINVAL;
	}
	regval |= (key_mode << SEC_ENG_SE_AES_0_MODE_SHIFT);
	sys_write32(regval, base + SEC_ENG_SE_AES_0_CTRL_OFFSET);

	/* Write key registers */
	sys_write32(sys_get_le32(key + 0), base + SEC_ENG_SE_AES_0_KEY_0_OFFSET);
	sys_write32(sys_get_le32(key + 4), base + SEC_ENG_SE_AES_0_KEY_1_OFFSET);
	sys_write32(sys_get_le32(key + 8), base + SEC_ENG_SE_AES_0_KEY_2_OFFSET);
	sys_write32(sys_get_le32(key + 12), base + SEC_ENG_SE_AES_0_KEY_3_OFFSET);
	if (sess->key_len >= 24) {
		sys_write32(sys_get_le32(key + 16), base + SEC_ENG_SE_AES_0_KEY_4_OFFSET);
		sys_write32(sys_get_le32(key + 20), base + SEC_ENG_SE_AES_0_KEY_5_OFFSET);
	}
	if (sess->key_len == 32) {
		sys_write32(sys_get_le32(key + 24), base + SEC_ENG_SE_AES_0_KEY_6_OFFSET);
		sys_write32(sys_get_le32(key + 28), base + SEC_ENG_SE_AES_0_KEY_7_OFFSET);
	}

	/* Configure operation */
	regval = sys_read32(base + SEC_ENG_SE_AES_0_CTRL_OFFSET);
	regval &= ~SEC_ENG_SE_AES_0_TRIG_1T;
	if (iv) {
		regval &= ~SEC_ENG_SE_AES_0_IV_SEL;
	} else {
		regval |= SEC_ENG_SE_AES_0_IV_SEL;
	}
	if (decrypt) {
		regval |= SEC_ENG_SE_AES_0_DEC_EN;
	} else {
		regval &= ~SEC_ENG_SE_AES_0_DEC_EN;
	}
	regval &= ~SEC_ENG_SE_AES_0_MSG_LEN_MASK;
	regval |= SEC_ENG_SE_AES_0_INT_CLR_1T;
	regval |= ((len / BFLB_AES_BLOCK_SIZE) << SEC_ENG_SE_AES_0_MSG_LEN_SHIFT);
	sys_write32(regval, base + SEC_ENG_SE_AES_0_CTRL_OFFSET);

	/* Write IV */
	if (iv) {
		sys_write32(sys_get_le32(iv + 0), base + SEC_ENG_SE_AES_0_IV_0_OFFSET);
		sys_write32(sys_get_le32(iv + 4), base + SEC_ENG_SE_AES_0_IV_1_OFFSET);
		sys_write32(sys_get_le32(iv + 8), base + SEC_ENG_SE_AES_0_IV_2_OFFSET);
		sys_write32(sys_get_le32(iv + 12), base + SEC_ENG_SE_AES_0_IV_3_OFFSET);
	}

	/* Set source and destination addresses */
	sys_write32((uint32_t)(uintptr_t)in, base + SEC_ENG_SE_AES_0_MSA_OFFSET);
	sys_write32((uint32_t)(uintptr_t)out, base + SEC_ENG_SE_AES_0_MDA_OFFSET);

	/* The operation done here writes and reads autonomously to and from RAM, which is cached.
	 * This is handled automatically on L1C SoCs (BL60x, BL70x/L)
	 * But on XTHeadCMO SoCs (BL61x) this requires manual cache handling
	 */
	sys_cache_data_flush_range((void *)in, (size_t)len);

	/* Trigger */
	regval = sys_read32(base + SEC_ENG_SE_AES_0_CTRL_OFFSET);
	regval |= SEC_ENG_SE_AES_0_TRIG_1T;
	sys_write32(regval, base + SEC_ENG_SE_AES_0_CTRL_OFFSET);

	ret = bflb_aes_wait_not_busy(base);

	sys_cache_data_invd_range((void *)out, (size_t)len);

	return ret;
}

static int bflb_aes_cipher_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	struct crypto_bflb_aes_data *data = dev->data;
	const struct crypto_bflb_aes_config *cfg = dev->config;
	struct bflb_aes_session *sess = ctx->drv_sessn_state;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = bflb_aes_do_op(cfg->base, sess, pkt->in_buf, pkt->out_buf, pkt->in_len, NULL,
			     sess->op == CRYPTO_CIPHER_OP_DECRYPT);
	k_mutex_unlock(&data->lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int bflb_aes_cipher_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	struct crypto_bflb_aes_data *data = dev->data;
	const struct crypto_bflb_aes_config *cfg = dev->config;
	struct bflb_aes_session *sess = ctx->drv_sessn_state;
	bool decrypt = (sess->op == CRYPTO_CIPHER_OP_DECRYPT);
	int in_offset = 0;
	int out_offset = 0;
	int ret;

	if (!decrypt && (ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		memcpy(pkt->out_buf, iv, BFLB_AES_BLOCK_SIZE);
		out_offset = BFLB_AES_BLOCK_SIZE;
	}

	if (decrypt && (ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		in_offset = BFLB_AES_BLOCK_SIZE;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = bflb_aes_do_op(cfg->base, sess, pkt->in_buf + in_offset, pkt->out_buf + out_offset,
			     pkt->in_len - in_offset, iv, decrypt);
	k_mutex_unlock(&data->lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len - in_offset + out_offset;
	}

	return ret;
}

static int bflb_aes_cipher_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	struct crypto_bflb_aes_data *data = dev->data;
	const struct crypto_bflb_aes_config *cfg = dev->config;
	struct bflb_aes_session *sess = ctx->drv_sessn_state;
	uint8_t ctr[BFLB_AES_BLOCK_SIZE] = {0};
	int ivlen = BFLB_AES_BLOCK_SIZE - (ctx->mode_params.ctr_info.ctr_len >> 3);
	int ret;

	memcpy(ctr, iv, ivlen);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = bflb_aes_do_op(cfg->base, sess, pkt->in_buf, pkt->out_buf, pkt->in_len, ctr,
			     sess->op == CRYPTO_CIPHER_OP_DECRYPT);
	k_mutex_unlock(&data->lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

/* Crypto API */

static int crypto_bflb_aes_query_caps(const struct device *dev)
{
	return (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX);
}

static int crypto_bflb_aes_begin_session(const struct device *dev, struct cipher_ctx *ctx,
					 enum cipher_algo algo, enum cipher_mode mode,
					 enum cipher_op op)
{
	struct crypto_bflb_aes_data *data = dev->data;
	struct bflb_aes_session *sess = NULL;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		return -ENOTSUP;
	}

	if (mode != CRYPTO_CIPHER_MODE_ECB && mode != CRYPTO_CIPHER_MODE_CBC &&
	    mode != CRYPTO_CIPHER_MODE_CTR) {
		return -ENOTSUP;
	}

	if (ctx->keylen != 16 && ctx->keylen != 24 && ctx->keylen != 32) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < CONFIG_CRYPTO_BFLB_SEC_ENG_AES_MAX_SESSION; i++) {
		if (!data->sessions[i].in_use) {
			sess = &data->sessions[i];
			break;
		}
	}

	if (!sess) {
		k_mutex_unlock(&data->lock);
		return -ENOSPC;
	}

	sess->in_use = true;

	k_mutex_unlock(&data->lock);

	sess->algo = algo;
	sess->mode = mode;
	sess->op = op;
	sess->key_len = ctx->keylen;
	memcpy(sess->key, ctx->key.bit_stream, ctx->keylen);

	ctx->drv_sessn_state = sess;
	ctx->device = dev;

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		ctx->ops.block_crypt_hndlr = bflb_aes_cipher_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		ctx->ops.cbc_crypt_hndlr = bflb_aes_cipher_cbc_op;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		ctx->ops.ctr_crypt_hndlr = bflb_aes_cipher_ctr_op;
		break;
	default:
		sess->in_use = false;
		return -ENOTSUP;
	}

	return 0;
}

static int crypto_bflb_aes_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	struct crypto_bflb_aes_data *data = dev->data;
	struct bflb_aes_session *sess = ctx->drv_sessn_state;

	if (sess) {
		k_mutex_lock(&data->lock, K_FOREVER);
		sess->in_use = false;
		k_mutex_unlock(&data->lock);
	}

	return 0;
}

static int crypto_bflb_aes_init(const struct device *dev)
{
	struct crypto_bflb_aes_data *data = dev->data;
	const struct crypto_bflb_aes_config *cfg = dev->config;

	k_mutex_init(&data->lock);
	bflb_aes_hw_init(cfg->base);

	return 0;
}

static DEVICE_API(crypto, crypto_bflb_aes_api) = {
	.query_hw_caps = crypto_bflb_aes_query_caps,
	.cipher_begin_session = crypto_bflb_aes_begin_session,
	.cipher_free_session = crypto_bflb_aes_free_session,
};

#define BFLB_AES_INIT(inst)                                                                        \
	static struct crypto_bflb_aes_data crypto_bflb_aes_data_##inst;                            \
	static const struct crypto_bflb_aes_config crypto_bflb_aes_config_##inst = {               \
		.base = SEC_ENG_AES_BASE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, crypto_bflb_aes_init, NULL, &crypto_bflb_aes_data_##inst,      \
			      &crypto_bflb_aes_config_##inst, POST_KERNEL,                         \
			      CONFIG_CRYPTO_INIT_PRIORITY, &crypto_bflb_aes_api);

DT_INST_FOREACH_STATUS_OKAY(BFLB_AES_INIT)
