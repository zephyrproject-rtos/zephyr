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
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <string.h>

#include "crypto_sf32lb_priv.h"

LOG_MODULE_REGISTER(crypto_sifli, CONFIG_CRYPTO_LOG_LEVEL);

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
#define CRYP_SUPPORT                                                                               \
	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_ASYNC_OPS | CAP_NO_IV_PREFIX)
#else
#define CRYP_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#endif

static struct crypto_sifli_session crypto_sifli_sessions[CONFIG_CRYPTO_SIFLI_MAX_SESSION];

#if defined(CONFIG_CRYPTO_SIFLI_HASH)
static struct crypto_sifli_hash_session crypto_sifli_hash_sessions[CONFIG_CRYPTO_SIFLI_MAX_SESSION];

static const uint8_t hash_result_sizes[] = {
	SIFLI_HASH_SHA1_SIZE,
	SIFLI_HASH_SHA224_SIZE, /* 28 bytes output for SHA-224 */
	SIFLI_HASH_SHA256_SIZE,
	SIFLI_HASH_SM3_SIZE,
};
#endif

static inline void crypto_sifli_cache_flush(const void *addr, size_t len)
{
	if (addr == NULL || len == 0U) {
		return;
	}

	/* sys_cache_data_flush_range does not mutate memory but lacks const qualifier. */
	sys_cache_data_flush_range((void *)(uintptr_t)addr, len);
}

static inline void crypto_sifli_cache_flush_invd(void *addr, size_t len)
{
	if (addr == NULL || len == 0U) {
		return;
	}

	sys_cache_data_flush_and_invd_range(addr, len);
}

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)

static void crypto_sifli_isr(const struct device *dev)
{
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = dev->data;
	uint32_t base = config->base;
	uint32_t irq_status = sys_read32(base + AES_IRQ_OFFSET);
	int status = 0;

#if defined(CONFIG_CRYPTO_SIFLI_AES)
	/* Handle AES interrupts */
	if (irq_status &
	    (AES_ACC_IRQ_DONE_STAT | AES_ACC_IRQ_BUS_ERR_STAT | AES_ACC_IRQ_SETUP_ERR_STAT)) {
		/* Clear AES IRQ status */
		sys_write32(irq_status & (AES_ACC_IRQ_DONE_STAT | AES_ACC_IRQ_BUS_ERR_STAT |
					  AES_ACC_IRQ_SETUP_ERR_STAT),
			    base + AES_IRQ_OFFSET);

		/* Disable AES IRQ mask */
		sys_write32(sys_read32(base + AES_SETTING_OFFSET) &
				    ~(AES_ACC_SETTING_DONE_IRQ_MASK |
				      AES_ACC_SETTING_BUS_ERR_IRQ_MASK |
				      AES_ACC_SETTING_SETUP_ERR_IRQ_MASK),
			    base + AES_SETTING_OFFSET);

		/* Check for errors */
		if (irq_status & (AES_ACC_IRQ_BUS_ERR_STAT | AES_ACC_IRQ_SETUP_ERR_STAT)) {
			status = -EIO;
		}

		/* Call cipher callback or signal sync semaphore */
		if (data->cipher_pkt != NULL) {
			struct cipher_pkt *pkt = data->cipher_pkt;

			data->cipher_pkt = NULL;
			data->cipher_status = status;
			crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);

			if (data->cipher_cb != NULL) {
				cipher_completion_cb cb = data->cipher_cb;

				cb(pkt, status);
				k_sem_give(&data->device_sem);
			} else {
				k_sem_give(&data->sync_sem);
			}
		}
	}
#endif /* CONFIG_CRYPTO_SIFLI_AES */

#if defined(CONFIG_CRYPTO_SIFLI_HASH)
	/* Handle HASH interrupts */
	if (irq_status & (AES_ACC_IRQ_HASH_DONE_STAT | AES_ACC_IRQ_HASH_BUS_ERR_STAT |
			  AES_ACC_IRQ_HASH_PAD_ERR_STAT)) {
		/* Clear HASH IRQ status */
		sys_write32(irq_status &
				    (AES_ACC_IRQ_HASH_DONE_STAT | AES_ACC_IRQ_HASH_BUS_ERR_STAT |
				     AES_ACC_IRQ_HASH_PAD_ERR_STAT),
			    base + AES_IRQ_OFFSET);

		/* Disable HASH IRQ mask */
		sys_write32(sys_read32(base + AES_SETTING_OFFSET) &
				    ~(AES_ACC_SETTING_HASH_DONE_MASK |
				      AES_ACC_SETTING_HASH_BUS_ERR_MASK |
				      AES_ACC_SETTING_HASH_PAD_ERR_MASK),
			    base + AES_SETTING_OFFSET);

		/* Check for errors */
		if (irq_status & (AES_ACC_IRQ_HASH_BUS_ERR_STAT | AES_ACC_IRQ_HASH_PAD_ERR_STAT)) {
			status = -EIO;
		}

		/* Call hash callback or signal sync semaphore */
		if (data->hash_pkt != NULL) {
			struct hash_pkt *pkt = data->hash_pkt;
			uint8_t algo = data->hash_algo;

			data->hash_pkt = NULL;
			data->hash_status = status;

			if ((status == 0) && (pkt->out_buf != NULL) &&
			    (algo < ARRAY_SIZE(hash_result_sizes))) {
				memcpy(pkt->out_buf,
				       (const void *)(base + AES_HASH_RESULT_H0_OFFSET),
				       hash_result_sizes[algo]);
			}

			if (data->hash_cb != NULL) {
				hash_completion_cb cb = data->hash_cb;

				cb(pkt, status);
				k_sem_give(&data->device_sem);
			} else {
				k_sem_give(&data->sync_sem);
			}
		}
	}
#endif /* CONFIG_CRYPTO_SIFLI_HASH */
}

#endif /* CONFIG_CRYPTO_SIFLI_ASYNC */

#if defined(CONFIG_CRYPTO_SIFLI_AES)

static inline bool crypto_sifli_aes_busy(uint32_t base)
{
	return sys_test_bit(base + AES_STATUS_OFFSET, AES_ACC_STATUS_BUSY_Pos);
}

static void crypto_sifli_aes_reset(uint32_t base)
{
	sys_set_bit(base + AES_COMMAND_OFFSET, AES_ACC_COMMAND_AES_ACC_RESET_Pos);
	sys_clear_bit(base + AES_COMMAND_OFFSET, AES_ACC_COMMAND_AES_ACC_RESET_Pos);
}

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

#if !defined(CONFIG_CRYPTO_SIFLI_ASYNC)
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
#endif /* !CONFIG_CRYPTO_SIFLI_ASYNC */

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
static int crypto_sifli_aes_run_async(const struct device *dev, uint8_t enc, uint8_t *in_data,
				      uint8_t *out_data, int size, struct cipher_pkt *pkt,
				      cipher_completion_cb cb)
{
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = dev->data;
	uint32_t base = config->base;
	uint32_t aes_setting;

	/* Clear pending IRQ */
	sys_write32(sys_read32(base + AES_IRQ_OFFSET), base + AES_IRQ_OFFSET);

	/* Enable IRQ masks */
	sys_write32(AES_ACC_SETTING_DONE_IRQ_MASK | AES_ACC_SETTING_BUS_ERR_IRQ_MASK |
			    AES_ACC_SETTING_SETUP_ERR_IRQ_MASK,
		    base + AES_SETTING_OFFSET);

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

	/* Store callback and packet for ISR */
	data->cipher_status = 0;
	data->cipher_pkt = pkt;

	/* Start operation */
	barrier_dsync_fence_full();
	sys_write32(sys_read32(base + AES_COMMAND_OFFSET) | AES_ACC_COMMAND_START,
		    base + AES_COMMAND_OFFSET);

	if (cb == NULL) {
		/* Synchronous mode: wait for completion via semaphore */
		if (k_sem_take(&data->sync_sem, K_USEC(CRYPTO_SIFLI_TIMEOUT_US)) != 0) {
			LOG_ERR("AES operation timeout");
			/* Disable IRQ mask */
			sys_write32(0, base + AES_SETTING_OFFSET);
			data->cipher_pkt = NULL;
			return -ETIMEDOUT;
		}

		if (data->cipher_status != 0) {
			return data->cipher_status;
		}
	}

	return 0;
}
#endif /* CONFIG_CRYPTO_SIFLI_ASYNC */

static int crypto_sifli_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;

	if ((pkt->in_buf == NULL) || (pkt->out_buf == NULL)) {
		LOG_ERR("Invalid input/output buffer");
		return -EINVAL;
	}

	if (pkt->in_len != SIFLI_AES_BLOCK_SIZE) {
		LOG_ERR("ECB mode requires a single block");
		return -EINVAL;
	}

	if (pkt->out_buf_max < SIFLI_AES_BLOCK_SIZE) {
		LOG_ERR("Output buffer too small");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, NULL,
				    SIFLI_AES_MODE_ECB);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	pkt->out_len = SIFLI_AES_BLOCK_SIZE;
	crypto_sifli_cache_flush(pkt->in_buf, pkt->in_len);
	crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	ret = crypto_sifli_aes_run_async(dev, SIFLI_AES_ENC, pkt->in_buf, pkt->out_buf, pkt->in_len,
					 pkt, data->cipher_cb);
	if (ret != 0 || data->cipher_cb != NULL) {
		/* Async mode: callback will release semaphore, or error occurred */
		if (ret != 0) {
			k_sem_give(&data->device_sem);
		}
		return ret;
	}
#else
	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_ENC, pkt->in_buf, pkt->out_buf,
				   pkt->in_len);
	if (ret == 0) {
		crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);
	}
#endif

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

	if ((pkt->in_buf == NULL) || (pkt->out_buf == NULL)) {
		LOG_ERR("Invalid input/output buffer");
		return -EINVAL;
	}

	if (pkt->in_len != SIFLI_AES_BLOCK_SIZE) {
		LOG_ERR("ECB mode requires a single block");
		return -EINVAL;
	}

	if (pkt->out_buf_max < SIFLI_AES_BLOCK_SIZE) {
		LOG_ERR("Output buffer too small");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, NULL,
				    SIFLI_AES_MODE_ECB);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	pkt->out_len = SIFLI_AES_BLOCK_SIZE;
	crypto_sifli_cache_flush(pkt->in_buf, pkt->in_len);
	crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	ret = crypto_sifli_aes_run_async(dev, SIFLI_AES_DEC, pkt->in_buf, pkt->out_buf, pkt->in_len,
					 pkt, data->cipher_cb);
	if (ret != 0 || data->cipher_cb != NULL) {
		if (ret != 0) {
			k_sem_give(&data->device_sem);
		}
		return ret;
	}
#else
	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_DEC, pkt->in_buf, pkt->out_buf,
				   pkt->in_len);
	if (ret == 0) {
		crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);
	}
#endif

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
	int out_len;

	if ((pkt->in_buf == NULL) || (pkt->out_buf == NULL)) {
		LOG_ERR("Invalid input/output buffer");
		return -EINVAL;
	}

	if (iv == NULL) {
		LOG_ERR("Missing IV");
		return -EINVAL;
	}

	if ((pkt->in_len <= 0) || ((pkt->in_len % SIFLI_AES_BLOCK_SIZE) != 0)) {
		LOG_ERR("Invalid input length");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, (uint32_t *)iv,
				    SIFLI_AES_MODE_CBC);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		out_offset = SIFLI_AES_BLOCK_SIZE;
	}

	out_len = pkt->in_len + out_offset;
	if (pkt->out_buf_max < out_len) {
		LOG_ERR("Output buffer too small");
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	if (out_offset != 0) {
		memcpy(pkt->out_buf, iv, SIFLI_AES_BLOCK_SIZE);
	}

	pkt->out_len = out_len;
	crypto_sifli_cache_flush(pkt->in_buf, pkt->in_len);
	crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	ret = crypto_sifli_aes_run_async(dev, SIFLI_AES_ENC, pkt->in_buf, pkt->out_buf + out_offset,
					 pkt->in_len, pkt, data->cipher_cb);
	if (ret != 0 || data->cipher_cb != NULL) {
		if (ret != 0) {
			k_sem_give(&data->device_sem);
		}
		return ret;
	}
#else
	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_ENC, pkt->in_buf,
				   pkt->out_buf + out_offset, pkt->in_len);
	if (ret == 0) {
		crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);
	}
#endif

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
	int out_len;

	if ((pkt->in_buf == NULL) || (pkt->out_buf == NULL)) {
		LOG_ERR("Invalid input/output buffer");
		return -EINVAL;
	}

	if (iv == NULL) {
		LOG_ERR("Missing IV");
		return -EINVAL;
	}

	if ((pkt->in_len <= 0) || ((pkt->in_len % SIFLI_AES_BLOCK_SIZE) != 0)) {
		LOG_ERR("Invalid input length");
		return -EINVAL;
	}

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

	if (pkt->in_len <= in_offset) {
		LOG_ERR("Invalid input length");
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	out_len = pkt->in_len - in_offset;
	if (pkt->out_buf_max < out_len) {
		LOG_ERR("Output buffer too small");
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	pkt->out_len = out_len;
	crypto_sifli_cache_flush(pkt->in_buf + in_offset, out_len);
	crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	ret = crypto_sifli_aes_run_async(dev, SIFLI_AES_DEC, pkt->in_buf + in_offset, pkt->out_buf,
					 pkt->in_len - in_offset, pkt, data->cipher_cb);
	if (ret != 0 || data->cipher_cb != NULL) {
		if (ret != 0) {
			k_sem_give(&data->device_sem);
		}
		return ret;
	}
#else
	ret = crypto_sifli_aes_run(config->base, SIFLI_AES_DEC, pkt->in_buf + in_offset,
				   pkt->out_buf, pkt->in_len - in_offset);
	if (ret == 0) {
		crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);
	}
#endif

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_ctr_crypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv,
				  uint8_t enc)
{
	const struct device *dev = ctx->device;
	const struct crypto_sifli_config *config = dev->config;
	struct crypto_sifli_data *data = CRYPTO_SIFLI_DATA(dev);
	struct crypto_sifli_session *session = CRYPTO_SIFLI_SESSN(ctx);
	int ret;
	uint32_t ctr[4] = {0};
	uint32_t ctr_len = ctx->mode_params.ctr_info.ctr_len;
	int nonce_len;

	if ((pkt->in_buf == NULL) || (pkt->out_buf == NULL)) {
		LOG_ERR("Invalid input/output buffer");
		return -EINVAL;
	}

	if ((ctr_len == 0U) || ((ctr_len & 0x7U) != 0U) ||
	    (ctr_len > (SIFLI_AES_BLOCK_SIZE * 8U))) {
		LOG_ERR("Invalid CTR length");
		return -EINVAL;
	}

	nonce_len = SIFLI_AES_BLOCK_SIZE - (ctr_len >> 3);
	if ((nonce_len > 0) && (iv == NULL)) {
		LOG_ERR("Missing IV");
		return -EINVAL;
	}

	if ((pkt->in_len <= 0) || ((pkt->in_len % SIFLI_AES_BLOCK_SIZE) != 0)) {
		LOG_ERR("Invalid input length");
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("Output buffer too small");
		return -EINVAL;
	}

	if (nonce_len > 0) {
		memcpy(ctr, iv, nonce_len);
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_sifli_aes_init(config->base, session->key, session->key_len, ctr,
				    SIFLI_AES_MODE_CTR);
	if (ret != 0) {
		k_sem_give(&data->device_sem);
		return ret;
	}

	pkt->out_len = pkt->in_len;
	crypto_sifli_cache_flush(pkt->in_buf, pkt->in_len);
	crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	ret = crypto_sifli_aes_run_async(dev, enc, pkt->in_buf, pkt->out_buf, pkt->in_len, pkt,
					 data->cipher_cb);
	if (ret != 0 || data->cipher_cb != NULL) {
		if (ret != 0) {
			k_sem_give(&data->device_sem);
		}
		return ret;
	}
#else
	ret = crypto_sifli_aes_run(config->base, enc, pkt->in_buf, pkt->out_buf, pkt->in_len);
	if (ret == 0) {
		crypto_sifli_cache_flush_invd(pkt->out_buf, pkt->out_len);
	}
#endif

	k_sem_give(&data->device_sem);
	return ret;
}

static int crypto_sifli_ctr_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	return crypto_sifli_ctr_crypt(ctx, pkt, iv, SIFLI_AES_ENC);
}

static int crypto_sifli_ctr_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	return crypto_sifli_ctr_crypt(ctx, pkt, iv, SIFLI_AES_DEC);
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
	memset(session->key, 0, sizeof(session->key));
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

static inline bool crypto_sifli_hash_busy(uint32_t base)
{
	return sys_test_bit(base + AES_STATUS_OFFSET, AES_ACC_STATUS_HASH_BUSY_Pos);
}

static void crypto_sifli_hash_reset(uint32_t base)
{
	sys_set_bit(base + AES_COMMAND_OFFSET, AES_ACC_COMMAND_HASH_RESET_Pos);
	sys_clear_bit(base + AES_COMMAND_OFFSET, AES_ACC_COMMAND_HASH_RESET_Pos);
}

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
		/* HW requires non-final chunks to be 4-byte aligned; multipart not supported. */
		LOG_ERR("Multipart hashing not supported yet");
		return -ENOTSUP;
	}

	if (session->algo >= ARRAY_SIZE(hash_result_sizes)) {
		LOG_ERR("Invalid hash algorithm: %u", session->algo);
		return -EINVAL;
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
	crypto_sifli_cache_flush(pkt->in_buf, pkt->in_len);

	/* Set input DMA address and size */
	sys_write32((uint32_t)pkt->in_buf, base + AES_HASH_DMA_IN_OFFSET);
	sys_write32(pkt->in_len, base + AES_HASH_DMA_DATA_OFFSET);
	LOG_DBG("DMA_IN=0x%08x, DMA_DATA=0x%08x", sys_read32(base + AES_HASH_DMA_IN_OFFSET),
		sys_read32(base + AES_HASH_DMA_DATA_OFFSET));

	/* Enable padding for final block - use OR to preserve settings */
	sys_write32(sys_read32(base + AES_HASH_SETTING_OFFSET) | AES_ACC_HASH_SETTING_DO_PADDING,
		    base + AES_HASH_SETTING_OFFSET);
	LOG_DBG("HASH_SETTING before start: 0x%08x", sys_read32(base + AES_HASH_SETTING_OFFSET));

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	/* Clear pending IRQ */
	sys_write32(sys_read32(base + AES_IRQ_OFFSET), base + AES_IRQ_OFFSET);

	/* Enable HASH IRQ masks */
	sys_write32(sys_read32(base + AES_SETTING_OFFSET) | AES_ACC_SETTING_HASH_DONE_MASK |
			    AES_ACC_SETTING_HASH_BUS_ERR_MASK | AES_ACC_SETTING_HASH_PAD_ERR_MASK,
		    base + AES_SETTING_OFFSET);

	/* Store context for ISR */
	data->hash_pkt = pkt;
	data->hash_algo = session->algo;
	data->hash_status = 0;

	/* Start hash operation */
	barrier_dsync_fence_full();
	sys_write32(AES_ACC_COMMAND_HASH_START, base + AES_COMMAND_OFFSET);

	if (data->hash_cb == NULL) {
		/* Wait for completion via semaphore */
		if (k_sem_take(&data->sync_sem, K_USEC(CRYPTO_SIFLI_TIMEOUT_US)) != 0) {
			LOG_ERR("HASH operation timeout");
			/* Disable IRQ mask */
			sys_write32(sys_read32(base + AES_SETTING_OFFSET) &
					    ~(AES_ACC_SETTING_HASH_DONE_MASK |
					      AES_ACC_SETTING_HASH_BUS_ERR_MASK |
					      AES_ACC_SETTING_HASH_PAD_ERR_MASK),
				    base + AES_SETTING_OFFSET);
			data->hash_pkt = NULL;
			k_sem_give(&data->device_sem);
			return -ETIMEDOUT;
		}

		if (data->hash_status != 0) {
			k_sem_give(&data->device_sem);
			return data->hash_status;
		}

		k_sem_give(&data->device_sem);
	}

	return 0;
#else
	/* Start hash operation */
	barrier_dsync_fence_full();
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
	int result_len = hash_result_sizes[session->algo];

	LOG_DBG("Hash algo=%d, result_len=%d", session->algo, result_len);
	memcpy(pkt->out_buf, (const void *)(base + AES_HASH_RESULT_H0_OFFSET), result_len);
	LOG_HEXDUMP_DBG(pkt->out_buf, result_len, "Hash result");

	k_sem_give(&data->device_sem);
	return 0;
#endif
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

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC) && defined(CONFIG_CRYPTO_SIFLI_AES)
static int crypto_sifli_cipher_callback_set(const struct device *dev, cipher_completion_cb cb)
{
	struct crypto_sifli_data *data = dev->data;

	data->cipher_cb = cb;
	return 0;
}
#endif

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC) && defined(CONFIG_CRYPTO_SIFLI_HASH)
static int crypto_sifli_hash_callback_set(const struct device *dev, hash_completion_cb cb)
{
	struct crypto_sifli_data *data = dev->data;

	data->hash_cb = cb;
	return 0;
}
#endif

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

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	k_sem_init(&data->sync_sem, 0, 1);
	config->irq_config_func();
#endif

#if defined(CONFIG_CRYPTO_SIFLI_AES)
	crypto_sifli_aes_reset(config->base);
#endif
#if defined(CONFIG_CRYPTO_SIFLI_HASH)
	crypto_sifli_hash_reset(config->base);
#endif

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
#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	.hash_async_callback_set = crypto_sifli_hash_callback_set,
#endif
#endif
#if defined(CONFIG_CRYPTO_SIFLI_ASYNC) && defined(CONFIG_CRYPTO_SIFLI_AES)
	.cipher_async_callback_set = crypto_sifli_cipher_callback_set,
#else
	.cipher_async_callback_set = NULL,
#endif
	.query_hw_caps = crypto_sifli_query_caps,
};

#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
#define CRYPTO_SIFLI_IRQ_CONFIG(inst)                                                              \
	static void crypto_sifli_irq_config_##inst(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), crypto_sifli_isr,     \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define CRYPTO_SIFLI_IRQ_CONFIG_INIT(inst) .irq_config_func = crypto_sifli_irq_config_##inst,
#else
#define CRYPTO_SIFLI_IRQ_CONFIG(inst)
#define CRYPTO_SIFLI_IRQ_CONFIG_INIT(inst)
#endif

#define CRYPTO_SIFLI_INIT(inst)                                                                    \
	CRYPTO_SIFLI_IRQ_CONFIG(inst)                                                              \
	static struct crypto_sifli_data crypto_sifli_data_##inst;                                  \
	static const struct crypto_sifli_config crypto_sifli_config_##inst = {                     \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(inst),                                      \
		CRYPTO_SIFLI_IRQ_CONFIG_INIT(inst)};                                               \
	DEVICE_DT_INST_DEFINE(inst, crypto_sifli_init, NULL, &crypto_sifli_data_##inst,            \
			      &crypto_sifli_config_##inst, POST_KERNEL,                            \
			      CONFIG_CRYPTO_INIT_PRIORITY, &crypto_sifli_funcs);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_SIFLI_INIT)
