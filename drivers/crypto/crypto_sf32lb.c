/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_crypto

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "crypto_sf32lb_priv.h"

LOG_MODULE_REGISTER(crypto_sifli, CONFIG_CRYPTO_LOG_LEVEL);

#define CRYP_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

static struct crypto_sifli_session crypto_sifli_sessions[CONFIG_CRYPTO_SIFLI_MAX_SESSION];

#if defined(CONFIG_CRYPTO_SIFLI_HASH)
static struct crypto_sifli_hash_session crypto_sifli_hash_sessions[CONFIG_CRYPTO_SIFLI_MAX_SESSION];
#endif

static inline bool crypto_sifli_aes_busy(uint32_t base)
{
	return (sys_read32(base + AES_STATUS_OFFSET) & AES_ACC_STATUS_BUSY) != 0;
}

static inline bool crypto_sifli_hash_busy(uint32_t base)
{
	return (sys_read32(base + AES_STATUS_OFFSET) & AES_ACC_STATUS_HASH_BUSY) != 0;
}

static void crypto_sifli_aes_reset(uint32_t base)
{
	sys_write32(AES_ACC_COMMAND_AES_ACC_RESET, base + AES_COMMAND_OFFSET);
	sys_write32(0, base + AES_COMMAND_OFFSET);
}

static void crypto_sifli_hash_reset(uint32_t base)
{
	sys_write32(AES_ACC_COMMAND_HASH_RESET, base + AES_COMMAND_OFFSET);
	sys_write32(0, base + AES_COMMAND_OFFSET);
}

#if defined(CONFIG_CRYPTO_SIFLI_AES)

static int crypto_sifli_aes_init(uint32_t base, const uint32_t *key, int key_size,
				 const uint32_t *iv, uint32_t mode)
{
	int ks;
	uint32_t setting = 0;

	if (crypto_sifli_aes_busy(base)) {
		crypto_sifli_aes_reset(base);
	}

	switch (key_size) {
	case 16:
		ks = SIFLI_AES_KEY_LEN_128;
		break;
	case 24:
		ks = SIFLI_AES_KEY_LEN_192;
		break;
	case 32:
		ks = SIFLI_AES_KEY_LEN_256;
		break;
	default:
		LOG_ERR("Unsupported key size: %d", key_size);
		return -EINVAL;
	}

	/* Load key */
	if (key != NULL) {
		int key_words = key_size / sizeof(uint32_t);

		for (int i = 0; i < key_words; i++) {
			sys_write32(key[i], base + AES_EXT_KEY_W0_OFFSET + (i * sizeof(uint32_t)));
		}
	} else {
		/* Use internal root key */
		setting |= AES_ACC_AES_SETTING_KEY_SEL;
	}

	/* Set mode and key length */
	setting |= (mode & AES_ACC_AES_SETTING_AES_MODE_Msk);
	setting |=
		((ks << AES_ACC_AES_SETTING_AES_LENGTH_Pos) & AES_ACC_AES_SETTING_AES_LENGTH_Msk);

	sys_write32(setting, base + AES_AES_SETTING_OFFSET);

	/* Load IV for CBC/CTR modes */
	if (mode != SIFLI_AES_MODE_ECB && iv != NULL) {
		sys_write32(iv[0], base + AES_IV_W0_OFFSET);
		sys_write32(iv[1], base + AES_IV_W1_OFFSET);
		sys_write32(iv[2], base + AES_IV_W2_OFFSET);
		sys_write32(iv[3], base + AES_IV_W3_OFFSET);
	}

	return 0;
}

static int crypto_sifli_aes_run(uint32_t base, uint8_t enc, uint8_t *in_data, uint8_t *out_data,
				int size)
{
	uint32_t aes_setting;

	/* Disable interrupts for sync operation */
	sys_write32(0, base + AES_SETTING_OFFSET);

	/* Set DMA addresses and size */
	sys_write32((uint32_t)in_data, base + AES_DMA_IN_OFFSET);
	sys_write32((uint32_t)out_data, base + AES_DMA_OUT_OFFSET);
	sys_write32((size + 15) >> 4, base + AES_DMA_DATA_OFFSET);

	/* Set encrypt/decrypt mode */
	aes_setting = sys_read32(base + AES_AES_SETTING_OFFSET);
	if (enc) {
		aes_setting |= AES_ACC_AES_SETTING_AES_OP_MODE;
	} else {
		aes_setting &= ~AES_ACC_AES_SETTING_AES_OP_MODE;
	}
	sys_write32(aes_setting, base + AES_AES_SETTING_OFFSET);

	/* Start operation */
	sys_write32(sys_read32(base + AES_COMMAND_OFFSET) | AES_ACC_COMMAND_START,
		    base + AES_COMMAND_OFFSET);

	/* Wait for completion */
	if (!WAIT_FOR(crypto_sifli_aes_busy(base) != true, CRYPTO_SIFLI_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_ERR("AES operation timeout");
		return -ETIMEDOUT;
	}

	/* Check for errors */
	uint32_t irq = sys_read32(base + AES_IRQ_OFFSET);

	if (irq & (AES_ACC_IRQ_BUS_ERR_STAT | AES_ACC_IRQ_SETUP_ERR_STAT)) {
		LOG_ERR("AES error: IRQ=0x%08x", irq);
		return -EIO;
	}

	return 0;
}

static int crypto_sifli_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;

	if (pkt->in_len > SIFLI_AES_BLOCK_SIZE) {
		LOG_ERR("ECB mode should not encrypt more than one block");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, NULL,
				    SIFLI_AES_MODE_ECB);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_ENC, pkt->in_buf, pkt->out_buf,
				   pkt->in_len);
	if (ret == 0) {
		pkt->out_len = SIFLI_AES_BLOCK_SIZE;
	}

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;

	if (pkt->in_len > SIFLI_AES_BLOCK_SIZE) {
		LOG_ERR("ECB mode should not decrypt more than one block");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, NULL,
				    SIFLI_AES_MODE_ECB);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_DEC, pkt->in_buf, pkt->out_buf,
				   pkt->in_len);
	if (ret == 0) {
		pkt->out_len = SIFLI_AES_BLOCK_SIZE;
	}

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;
	int out_offset = 0;

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, (uint32_t *)iv,
				    SIFLI_AES_MODE_CBC);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		memcpy(pkt->out_buf, iv, SIFLI_AES_BLOCK_SIZE);
		out_offset = SIFLI_AES_BLOCK_SIZE;
	}

	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_ENC, pkt->in_buf,
				   pkt->out_buf + out_offset, pkt->in_len);
	if (ret == 0) {
		pkt->out_len = pkt->in_len + out_offset;
	}

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;
	int in_offset = 0;

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, (uint32_t *)iv,
				    SIFLI_AES_MODE_CBC);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		in_offset = SIFLI_AES_BLOCK_SIZE;
	}

	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_DEC, pkt->in_buf + in_offset,
				   pkt->out_buf, pkt->in_len - in_offset);
	if (ret == 0) {
		pkt->out_len = pkt->in_len - in_offset;
	}

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_ctr_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;
	uint32_t ctr[4] = {0};
	int ivlen = SIFLI_AES_BLOCK_SIZE - (ctx->mode_params.ctr_info.ctr_len >> 3);

	memcpy(ctr, iv, ivlen);

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, ctr,
				    SIFLI_AES_MODE_CTR);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_ENC, pkt->in_buf, pkt->out_buf,
				   pkt->in_len);
	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_ctr_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;
	uint32_t ctr[4] = {0};
	int ivlen = SIFLI_AES_BLOCK_SIZE - (ctx->mode_params.ctr_info.ctr_len >> 3);

	memcpy(ctr, iv, ivlen);

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, ctr,
				    SIFLI_AES_MODE_CTR);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_DEC, pkt->in_buf, pkt->out_buf,
				   pkt->in_len);
	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_get_unused_session_index(const struct device *dev)
{
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);

	k_sem_take(&data->session_sem, K_FOREVER);

	for (int i = 0; i < CONFIG_CRYPTO_SIFLI_MAX_SESSION; i++) {
		if (!crypto_sifli_sessions[i].in_use) {
			crypto_sifli_sessions[i].in_use = true;
			k_sem_give(&data->session_sem);
			return i;
		}
	}

	k_sem_give(&data->session_sem);
	return -1;
}

static int crypto_sifli_session_setup(const struct device *dev, struct cipher_ctx *ctx,
				      enum cipher_algo algo, enum cipher_mode mode,
				      enum cipher_op op_type)
{
	int ctx_idx;
	struct crypto_sifli_session *session;

	if (ctx->flags & ~(CRYP_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algo: %d", algo);
		return -ENOTSUP;
	}

	if ((mode != CRYPTO_CIPHER_MODE_ECB) && (mode != CRYPTO_CIPHER_MODE_CBC) &&
	    (mode != CRYPTO_CIPHER_MODE_CTR)) {
		LOG_ERR("Unsupported mode: %d", mode);
		return -ENOTSUP;
	}

	if ((ctx->keylen != 16U) && (ctx->keylen != 24U) && (ctx->keylen != 32U)) {
		LOG_ERR("Unsupported key size: %u", ctx->keylen);
		return -ENOTSUP;
	}

	ctx_idx = crypto_sifli_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free session");
		return -ENOSPC;
	}

	session = &crypto_sifli_sessions[ctx_idx];
	memset(session, 0, sizeof(*session));
	session->in_use = true;
	session->key_len = ctx->keylen;
	memcpy(session->key, ctx->key.bit_stream, ctx->keylen);

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		session->mode = SIFLI_AES_MODE_ECB;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		session->mode = SIFLI_AES_MODE_CBC;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		session->mode = SIFLI_AES_MODE_CTR;
		break;
	default:
		break;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_sifli_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_sifli_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_sifli_ctr_encrypt;
			break;
		default:
			break;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_sifli_ecb_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_sifli_cbc_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_sifli_ctr_decrypt;
			break;
		default:
			break;
		}
	}

	ctx->drv_sessn_state = session;
	ctx->device = dev;

	return 0;
}

static int crypto_sifli_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);

	if (session != NULL) {
		session->in_use = false;
	}

	return 0;
}

#endif /* CONFIG_CRYPTO_SIFLI_AES */

#if defined(CONFIG_CRYPTO_SIFLI_HASH)

static const uint8_t hash_result_sizes[] = {
	SIFLI_HASH_SHA1_SIZE,
	SIFLI_HASH_SHA224_SIZE, /* 28 bytes output for SHA-224 */
	SIFLI_HASH_SHA256_SIZE,
	SIFLI_HASH_SM3_SIZE,
};

static int crypto_sifli_hash_get_unused_session_index(const struct device *dev)
{
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);

	k_sem_take(&data->session_sem, K_FOREVER);

	for (int i = 0; i < CONFIG_CRYPTO_SIFLI_MAX_SESSION; i++) {
		if (!crypto_sifli_hash_sessions[i].in_use) {
			crypto_sifli_hash_sessions[i].in_use = true;
			k_sem_give(&data->session_sem);
			return i;
		}
	}

	k_sem_give(&data->session_sem);
	return -1;
}

static int crypto_sifli_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct device *dev = ctx->device;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_hash_session *session = CRYPTO_SIFLI_HASH_SESSN(ctx);
	const struct crypto_sifli_config *config = dev->config;
	uint32_t base = config->base;
	uint32_t hash_setting;
	int result_len;

	if (!pkt || !pkt->out_buf) {
		LOG_ERR("Invalid packet buffers");
		return -EINVAL;
	}

	/* in_buf can be NULL for empty message, but in_len should be 0 */
	if (!pkt->in_buf && pkt->in_len != 0) {
		LOG_ERR("Invalid input buffer");
		return -EINVAL;
	}

	if (!finish) {
		LOG_ERR("Multipart hashing not supported yet");
		return -ENOTSUP;
	}

	LOG_DBG("Hash: algo=%d, in_len=%zu, in_buf=%p", session->algo, pkt->in_len, pkt->in_buf);

	k_sem_take(&data->device_sem, K_FOREVER);

	/* Reset hash module */
	crypto_sifli_hash_reset(base);

	/* Step 1: Set algorithm mode */
	hash_setting = (session->algo << AES_ACC_HASH_SETTING_HASH_MODE_Pos) &
		       AES_ACC_HASH_SETTING_HASH_MODE_Msk;
	sys_write32(hash_setting, base + AES_HASH_SETTING_OFFSET);

	/* Step 2: Trigger IV load (use default IV since DFT_IV_SEL is not set) */
	hash_setting |= AES_ACC_HASH_SETTING_HASH_IV_LOAD;
	sys_write32(hash_setting, base + AES_HASH_SETTING_OFFSET);

	LOG_DBG("HASH_SETTING after init: 0x%08x (wrote 0x%08x)",
		sys_read32(base + AES_HASH_SETTING_OFFSET), hash_setting);

	/* Flush data cache before DMA if there's data to process */
	if (pkt->in_len > 0 && pkt->in_buf != NULL) {
		/* Cast away const - cache flush doesn't modify data content */
		sys_cache_data_flush_range((void *)(uintptr_t)pkt->in_buf, pkt->in_len);
	}

	/* Set input DMA address and size */
	sys_write32((uint32_t)pkt->in_buf, base + AES_HASH_DMA_IN_OFFSET);
	sys_write32(pkt->in_len, base + AES_HASH_DMA_DATA_OFFSET);
	LOG_DBG("DMA_IN=0x%08x, DMA_DATA=0x%08x", sys_read32(base + AES_HASH_DMA_IN_OFFSET),
		sys_read32(base + AES_HASH_DMA_DATA_OFFSET));

	/* Enable padding for final block - use OR to preserve settings */
	sys_write32(sys_read32(base + AES_HASH_SETTING_OFFSET) | AES_ACC_HASH_SETTING_DO_PADDING,
		    base + AES_HASH_SETTING_OFFSET);
	LOG_DBG("HASH_SETTING before start: 0x%08x", sys_read32(base + AES_HASH_SETTING_OFFSET));

	/* Data memory barrier before starting DMA */
	__DSB();

	/* Start hash operation */
	sys_write32(AES_ACC_COMMAND_HASH_START, base + AES_COMMAND_OFFSET);

	/* Wait for completion */
	if (!WAIT_FOR(crypto_sifli_hash_busy(base) != true, CRYPTO_SIFLI_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_ERR("HASH operation timeout");
		k_sem_give(&data->device_sem);
		return -ETIMEDOUT;
	}

	/* Check for errors */
	uint32_t irq = sys_read32(base + AES_IRQ_OFFSET);

	/* Clear IRQ status */
	sys_write32(irq & (AES_ACC_IRQ_HASH_BUS_ERR_STAT | AES_ACC_IRQ_HASH_PAD_ERR_STAT |
			   AES_ACC_IRQ_HASH_DONE_STAT),
		    base + AES_IRQ_OFFSET);

	if (irq & (AES_ACC_IRQ_HASH_BUS_ERR_STAT | AES_ACC_IRQ_HASH_PAD_ERR_STAT)) {
		LOG_ERR("HASH error: IRQ=0x%08x", irq);
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	/* Copy result - hardware stores result as contiguous bytes */
	result_len = hash_result_sizes[session->algo];
	LOG_DBG("Hash algo=%d, result_len=%d", session->algo, result_len);

	/* Use memcpy to read result directly from registers */
	memcpy(pkt->out_buf, (const void *)(base + AES_HASH_RESULT_H0_OFFSET), result_len);

	LOG_HEXDUMP_DBG(pkt->out_buf, result_len, "Hash result");

	k_sem_give(&data->device_sem);
	return 0;
}

static int crypto_sifli_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
					   enum hash_algo algo)
{
	int ctx_idx;
	struct crypto_sifli_hash_session *session;
	uint8_t hw_algo;

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		hw_algo = SIFLI_HASH_ALGO_SHA224;
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		hw_algo = SIFLI_HASH_ALGO_SHA256;
		break;
	default:
		LOG_ERR("Unsupported hash algorithm: %d", algo);
		return -ENOTSUP;
	}

	ctx_idx = crypto_sifli_hash_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free hash session");
		return -ENOSPC;
	}

	session = &crypto_sifli_hash_sessions[ctx_idx];
	memset(session, 0, sizeof(*session));
	session->in_use = true;
	session->algo = hw_algo;

	ctx->drv_sessn_state = session;
	ctx->hash_hndlr = crypto_sifli_hash_handler;
	ctx->started = false;
	ctx->device = dev;

	return 0;
}

static int crypto_sifli_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_sifli_hash_session *session = CRYPTO_SIFLI_HASH_SESSN(ctx);

	if (session != NULL) {
		session->in_use = false;
	}

	return 0;
}

#endif /* CONFIG_CRYPTO_SIFLI_HASH */

static int crypto_sifli_query_caps(const struct device *dev)
{
	return CRYP_SUPPORT;
}

static int crypto_sifli_init(const struct device *dev)
{
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = dev->data;
	int ret;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret != 0) {
		LOG_ERR("Failed to enable clock");
		return ret;
	}

	k_sem_init(&data->device_sem, 1, 1);
	k_sem_init(&data->session_sem, 1, 1);

	/* Reset AES and HASH modules */
	crypto_sifli_aes_reset(config->base);
	crypto_sifli_hash_reset(config->base);

	LOG_DBG("SiFli crypto driver initialized");

	return 0;
}

static DEVICE_API(crypto, crypto_sifli_funcs) = {
#if defined(CONFIG_CRYPTO_SIFLI_AES)
	.cipher_begin_session = crypto_sifli_session_setup,
	.cipher_free_session = crypto_sifli_session_free,
#endif
#if defined(CONFIG_CRYPTO_SIFLI_HASH)
	.hash_begin_session = crypto_sifli_hash_begin_session,
	.hash_free_session = crypto_sifli_hash_free_session,
#endif
	.cipher_async_callback_set = NULL,
	.query_hw_caps = crypto_sifli_query_caps,
};

#define CRYPTO_SIFLI_INIT(inst)                                                                    \
	static struct crypto_sifli_data crypto_sifli_data_##inst;                                  \
	static const struct crypto_sifli_config crypto_sifli_config_##inst = {                     \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, crypto_sifli_init, NULL, &crypto_sifli_data_##inst,            \
			      &crypto_sifli_config_##inst, POST_KERNEL,                            \
			      CONFIG_CRYPTO_INIT_PRIORITY, &crypto_sifli_funcs);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_SIFLI_INIT)
