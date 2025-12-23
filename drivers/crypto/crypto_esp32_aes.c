/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/cipher.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "hal/aes_hal.h"
#include "hal/aes_ll.h"

LOG_MODULE_REGISTER(esp32_aes, CONFIG_CRYPTO_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_aes

#ifndef ESP_AES_ENCRYPT
#define ESP_AES_ENCRYPT 0
#endif
#ifndef ESP_AES_DECRYPT
#define ESP_AES_DECRYPT 1
#endif

struct esp_aes_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
};

struct esp_aes_dev_data {
	struct k_mutex aes_lock;
};

struct esp_aes_ctx {
	bool in_use;
	uint8_t key[32];
	size_t key_len;
	int dir;
	enum cipher_mode mode;
};

static K_MUTEX_DEFINE(aes_pool_lock);

static struct esp_aes_ctx aes_pool[CONFIG_ESP32_CRYPTO_AES_SESSIONS_MAX];

static inline int aes_setkey_dir(const uint8_t *key, size_t keylen, int dir)
{
	uint8_t written = aes_hal_setkey(key, keylen, dir);

	if (written != keylen) {
		LOG_ERR("HAL setkey failed: wrote %u/%zu bytes", written, keylen);
		return -EIO;
	}

	return 0;
}

static inline void aes_ecb_block(const uint8_t in[16], uint8_t out[16])
{
	aes_hal_transform_block(in, out);
}

static int aes_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (CAP_RAW_KEY | CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS |
	       CAP_NO_IV_PREFIX);
}

static struct esp_aes_ctx *aes_pool_alloc(void)
{
	struct esp_aes_ctx *ret = NULL;

	k_mutex_lock(&aes_pool_lock, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(aes_pool); i++) {
		if (!aes_pool[i].in_use) {
			memset(&aes_pool[i], 0, sizeof(aes_pool[i]));
			aes_pool[i].in_use = true;
			ret = &aes_pool[i];
			break;
		}
	}

	k_mutex_unlock(&aes_pool_lock);

	if (!ret) {
		LOG_WRN("Session pool exhausted (max: %d)", CONFIG_ESP32_CRYPTO_AES_SESSIONS_MAX);
	}

	return ret;
}

static void aes_pool_free(struct esp_aes_ctx *s)
{
	if (!s) {
		return;
	}

	k_mutex_lock(&aes_pool_lock, K_FOREVER);
	memset(s, 0, sizeof(*s));
	k_mutex_unlock(&aes_pool_lock);
}

static int aes_begin_session(const struct device *dev, struct cipher_ctx *zctx,
			     enum cipher_algo algo, enum cipher_mode mode, enum cipher_op op_type)
{
	struct esp_aes_ctx *ctx;
	int rc;

	ARG_UNUSED(dev);

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algorithm: %d", algo);
		return -ENOTSUP;
	}

	if (!(mode == CRYPTO_CIPHER_MODE_ECB || mode == CRYPTO_CIPHER_MODE_CBC ||
	      mode == CRYPTO_CIPHER_MODE_CTR)) {
		LOG_ERR("Unsupported mode: %d", mode);
		return -ENOTSUP;
	}

	if (!(zctx->keylen == 16 || zctx->keylen == 24 || zctx->keylen == 32)) {
		LOG_ERR("Invalid key length: %zu", zctx->keylen);
		return -EINVAL;
	}

	ctx = aes_pool_alloc();

	if (!ctx) {
		return -ENOMEM;
	}

	ctx->mode = mode;
	ctx->dir = (op_type == CRYPTO_CIPHER_OP_ENCRYPT) ? ESP_AES_ENCRYPT : ESP_AES_DECRYPT;
	ctx->key_len = zctx->keylen;
	memcpy(ctx->key, zctx->key.bit_stream, zctx->keylen);

	rc = aes_setkey_dir(ctx->key, ctx->key_len, ctx->dir);

	if (rc) {
		aes_pool_free(ctx);
		return rc;
	}

	zctx->drv_sessn_state = ctx;

	LOG_DBG("Session started: mode=%d, dir=%d, keylen=%zu", mode, ctx->dir, ctx->key_len);

	return 0;
}

static int aes_free_session(const struct device *dev, struct cipher_ctx *zctx)
{
	struct esp_aes_ctx *ctx;

	ARG_UNUSED(dev);

	if (!zctx || !zctx->drv_sessn_state) {
		return 0;
	}

	ctx = zctx->drv_sessn_state;

	aes_pool_free(ctx);

	zctx->drv_sessn_state = NULL;

	LOG_DBG("Session freed");

	return 0;
}

static int aes_ecb_op(struct cipher_ctx *zctx, struct cipher_pkt *pkt)
{
	struct esp_aes_ctx *ctx = zctx->drv_sessn_state;
	struct esp_aes_dev_data *data = zctx->device->data;
	const uint8_t *in;
	uint8_t *out;
	size_t blocks;

	if (!ctx || (pkt->in_len % 16U) != 0U) {
		LOG_ERR("Invalid ECB op: ctx=%p, in_len=%zu", ctx, ctx ? pkt->in_len : 0);
		return -EINVAL;
	}

	k_mutex_lock(&data->aes_lock, K_FOREVER);

	in = pkt->in_buf;
	out = pkt->out_buf;
	blocks = pkt->in_len / 16U;

	for (size_t i = 0; i < blocks; i++) {
		aes_ecb_block(in, out);
		in += 16;
		out += 16;
	}

	k_mutex_unlock(&data->aes_lock);

	pkt->out_len = pkt->in_len;

	return 0;
}

static void cbc_xor_block(const uint8_t *a, const uint8_t *b, uint8_t *out)
{
	for (int i = 0; i < 16; i++) {
		out[i] = a[i] ^ b[i];
	}
}

static void cbc_encrypt_blocks(const uint8_t *in, uint8_t *out, size_t blocks, uint8_t *chain)
{
	uint8_t x[16];

	for (size_t i = 0; i < blocks; i++) {
		cbc_xor_block(in, chain, x);
		aes_ecb_block(x, out);
		memcpy(chain, out, 16);
		in += 16;
		out += 16;
	}
}

static void cbc_decrypt_blocks(const uint8_t *in, uint8_t *out, size_t blocks, uint8_t *chain)
{
	uint8_t y[16];
	uint8_t next_chain[16];

	for (size_t i = 0; i < blocks; i++) {
		memcpy(next_chain, in, 16);
		aes_ecb_block(in, y);
		cbc_xor_block(y, chain, out);
		memcpy(chain, next_chain, 16);
		in += 16;
		out += 16;
	}
}

static int aes_cbc_encrypt(struct cipher_ctx *zctx, struct cipher_pkt *pkt, uint8_t *iv,
			   uint8_t *chain)
{
	const uint8_t *in = pkt->in_buf;
	uint8_t *out = pkt->out_buf;
	size_t blocks = pkt->in_len / 16U;
	bool prefix_iv = (zctx->flags & CAP_NO_IV_PREFIX) == 0U;

	memcpy(chain, iv, 16);

	if (prefix_iv) {
		memcpy(out, iv, 16);
		out += 16;
	}

	cbc_encrypt_blocks(in, out, blocks, chain);
	pkt->out_len = pkt->in_len + (prefix_iv ? 16U : 0U);

	return 0;
}

static int aes_cbc_decrypt(struct cipher_ctx *zctx, struct cipher_pkt *pkt, uint8_t *iv,
			   uint8_t *chain)
{
	const uint8_t *in = pkt->in_buf;
	uint8_t *out = pkt->out_buf;
	size_t blocks;
	bool prefix_iv = (zctx->flags & CAP_NO_IV_PREFIX) == 0U;

	if (prefix_iv) {
		memcpy(chain, in, 16);
		in += 16;
		blocks = (pkt->in_len - 16U) / 16U;
		pkt->out_len = pkt->in_len - 16U;
	} else {
		memcpy(chain, iv, 16);
		blocks = pkt->in_len / 16U;
		pkt->out_len = pkt->in_len;
	}

	cbc_decrypt_blocks(in, out, blocks, chain);

	return 0;
}

static int aes_cbc_op(struct cipher_ctx *zctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct esp_aes_ctx *ctx = zctx->drv_sessn_state;
	struct esp_aes_dev_data *data = zctx->device->data;
	uint8_t chain[16];
	int ret;

	if (!ctx || (pkt->in_len % 16U) != 0U) {
		LOG_ERR("Invalid CBC op: ctx=%p, in_len=%zu", ctx, ctx ? pkt->in_len : 0);
		return -EINVAL;
	}

	k_mutex_lock(&data->aes_lock, K_FOREVER);

	if (ctx->dir == ESP_AES_ENCRYPT) {
		ret = aes_cbc_encrypt(zctx, pkt, iv, chain);
	} else {
		ret = aes_cbc_decrypt(zctx, pkt, iv, chain);
	}

	k_mutex_unlock(&data->aes_lock);

	return ret;
}

static int aes_ctr_op(struct cipher_ctx *zctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct esp_aes_ctx *ctx = zctx->drv_sessn_state;
	struct esp_aes_dev_data *data = zctx->device->data;
	uint32_t ctr_len_bits;
	size_t ctr_bytes;
	size_t iv_bytes;
	uint8_t counter_blk[16];
	int restore_dir;
	const uint8_t *in;
	uint8_t *out;
	size_t len;

	if (!ctx) {
		return -EINVAL;
	}

	if ((pkt->in_len > 0 && pkt->in_buf == NULL) || pkt->out_buf == NULL) {
		return -EINVAL;
	}

	ctr_len_bits = zctx->mode_params.ctr_info.ctr_len;

	if (ctr_len_bits == 0 || (ctr_len_bits % 8) != 0 || ctr_len_bits > 128) {
		LOG_ERR("Invalid CTR counter length: %u bits", ctr_len_bits);
		return -EINVAL;
	}

	ctr_bytes = ctr_len_bits / 8U;
	iv_bytes = 16U - ctr_bytes;

	memset(counter_blk, 0, sizeof(counter_blk));
	memcpy(counter_blk, iv, iv_bytes);

	k_mutex_lock(&data->aes_lock, K_FOREVER);

	restore_dir = 0;

	if (ctx->dir != ESP_AES_ENCRYPT) {
		aes_setkey_dir(ctx->key, ctx->key_len, ESP_AES_ENCRYPT);
		restore_dir = 1;
	}

	in = pkt->in_buf;
	out = pkt->out_buf;
	len = pkt->in_len;

	while (len > 0) {
		uint8_t ks[16];
		size_t chunk = (len < 16U) ? len : 16U;

		aes_ecb_block(counter_blk, ks);

		for (size_t i = 0; i < chunk; i++) {
			out[i] = in[i] ^ ks[i];
		}

		for (int i = 15; i >= (int)iv_bytes; i--) {
			if (++counter_blk[i] != 0) {
				break;
			}
		}

		in += chunk;
		out += chunk;
		len -= chunk;
	}

	if (restore_dir) {
		aes_setkey_dir(ctx->key, ctx->key_len, ctx->dir);
	}

	k_mutex_unlock(&data->aes_lock);

	pkt->out_len = pkt->in_len;

	return 0;
}

static int aes_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
				    enum cipher_algo algo, enum cipher_mode mode,
				    enum cipher_op optype)
{
	int rc = aes_begin_session(dev, ctx, algo, mode, optype);

	if (rc) {
		return rc;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		ctx->ops.block_crypt_hndlr = aes_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		ctx->ops.cbc_crypt_hndlr = aes_cbc_op;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		ctx->ops.ctr_crypt_hndlr = aes_ctr_op;
		break;
	default:
		break;
	}

	return 0;
}

static int aes_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	return aes_free_session(dev, ctx);
}

static int aes_cipher_async_cb_set(const struct device *dev, cipher_completion_cb cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	return -ENOTSUP;
}

static int aes_init(const struct device *dev)
{
	struct esp_aes_dev_data *data = dev->data;
	const struct esp_aes_config *cfg = dev->config;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys) != 0) {
		LOG_ERR("Failed to enable AES peripheral clock");
		return -EIO;
	}

	k_mutex_init(&data->aes_lock);

	LOG_INF("ESP32 AES hardware accelerator initialized");

	return 0;
}

static const struct crypto_driver_api aes_crypto_api = {
	.query_hw_caps = aes_query_hw_caps,
	.cipher_begin_session = aes_cipher_begin_session,
	.cipher_free_session = aes_cipher_free_session,
	.cipher_async_callback_set = aes_cipher_async_cb_set,
};

#define ESP_AES_DEVICE_INIT(inst)                                                                  \
	static struct esp_aes_dev_data aes_data_##inst;                                            \
	static const struct esp_aes_config aes_cfg_##inst = {                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset)};        \
	DEVICE_DT_INST_DEFINE(inst, aes_init, NULL, &aes_data_##inst, &aes_cfg_##inst,             \
			      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, &aes_crypto_api);

DT_INST_FOREACH_STATUS_OKAY(ESP_AES_DEVICE_INIT)
