/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_ecb.h>

#define DT_DRV_COMPAT nordic_nrf_ecb

#define ECB_AES_KEY_SIZE   16
#define ECB_AES_BLOCK_SIZE 16

LOG_MODULE_REGISTER(crypto_nrf_ecb, CONFIG_CRYPTO_LOG_LEVEL);

struct ecb_data {
	uint8_t key[ECB_AES_KEY_SIZE];
	uint8_t cleartext[ECB_AES_BLOCK_SIZE];
	uint8_t ciphertext[ECB_AES_BLOCK_SIZE];
};

struct nrf_ecb_drv_state {
	struct ecb_data data;
	bool in_use;
};

static struct nrf_ecb_drv_state drv_state;

static int do_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	ARG_UNUSED(ctx);

	if (pkt->in_len != ECB_AES_BLOCK_SIZE) {
		LOG_ERR("only 16-byte blocks are supported");
		return -EINVAL;
	}
	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("output buffer too small");
		return -EINVAL;
	}

	if (pkt->in_buf != drv_state.data.cleartext) {
		memcpy(drv_state.data.cleartext, pkt->in_buf,
		       ECB_AES_BLOCK_SIZE);
	}

	nrf_ecb_event_clear(NRF_ECB, NRF_ECB_EVENT_ENDECB);
	nrf_ecb_event_clear(NRF_ECB, NRF_ECB_EVENT_ERRORECB);
	nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STARTECB);
	while (!(nrf_ecb_event_check(NRF_ECB, NRF_ECB_EVENT_ENDECB) ||
		 nrf_ecb_event_check(NRF_ECB, NRF_ECB_EVENT_ERRORECB))) {
	}
	if (nrf_ecb_event_check(NRF_ECB, NRF_ECB_EVENT_ERRORECB)) {
		LOG_ERR("ECB operation error");
		return -EIO;
	}
	if (pkt->out_buf != drv_state.data.ciphertext) {
		memcpy(pkt->out_buf, drv_state.data.ciphertext,
		       ECB_AES_BLOCK_SIZE);
	}
	pkt->out_len = pkt->in_len;
	return 0;
}

static int nrf_ecb_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	nrf_ecb_data_pointer_set(NRF_ECB, &drv_state.data);
	drv_state.in_use = false;
	return 0;
}

static int nrf_ecb_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int nrf_ecb_session_setup(const struct device *dev,
				 struct cipher_ctx *ctx,
				 enum cipher_algo algo, enum cipher_mode mode,
				 enum cipher_op op_type)
{
	ARG_UNUSED(dev);

	if ((algo != CRYPTO_CIPHER_ALGO_AES) ||
	    !(ctx->flags & CAP_SYNC_OPS) ||
	    (ctx->keylen != ECB_AES_KEY_SIZE) ||
	    (op_type != CRYPTO_CIPHER_OP_ENCRYPT) ||
	    (mode != CRYPTO_CIPHER_MODE_ECB)) {
		LOG_ERR("This driver only supports 128-bit AES ECB encryption"
			" in synchronous mode");
		return -EINVAL;
	}

	if (ctx->key.bit_stream == NULL) {
		LOG_ERR("No key provided");
		return -EINVAL;
	}

	if (drv_state.in_use) {
		LOG_ERR("Peripheral in use");
		return -EBUSY;
	}

	drv_state.in_use = true;

	ctx->ops.block_crypt_hndlr = do_ecb_encrypt;
	ctx->ops.cipher_mode = mode;

	if (ctx->key.bit_stream != drv_state.data.key) {
		memcpy(drv_state.data.key, ctx->key.bit_stream,
		       ECB_AES_KEY_SIZE);
	}

	return 0;
}

static int nrf_ecb_session_free(const struct device *dev,
				struct cipher_ctx *sessn)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sessn);

	drv_state.in_use = false;

	return 0;
}

static const struct crypto_driver_api crypto_enc_funcs = {
	.cipher_begin_session = nrf_ecb_session_setup,
	.cipher_free_session = nrf_ecb_session_free,
	.cipher_async_callback_set = NULL,
	.query_hw_caps = nrf_ecb_query_caps,
};

DEVICE_DT_INST_DEFINE(0, nrf_ecb_driver_init, NULL,
		      NULL, NULL,
		      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &crypto_enc_funcs);
