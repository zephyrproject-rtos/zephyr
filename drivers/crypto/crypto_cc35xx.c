/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_hsm_crypto

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/crypto/crypto.h>

#include <zephyr/drivers/misc/ti_cc35xx_hsm/ti_cc35xx_hsm.h>

#include <string.h>

struct crypto_ti_cc35xx_data {
	const struct device *hsm;
};

static int crypto_ti_cc35xx_do(enum cipher_mode mode, enum cipher_op op_type,
			       struct cipher_ctx *ctx, void *pkt)
{
	struct crypto_ti_cc35xx_data *data;
	struct hsm_ti_cc35xx_driver_api *api;

	if (ctx == NULL || ctx->device == NULL || ctx->device->data == NULL) {
		return -ENODEV;
	}

	data = (struct crypto_ti_cc35xx_data *)ctx->device->data;
	if (data->hsm == NULL || data->hsm->api == NULL) {
		return -EINVAL;
	}

	api = (struct hsm_ti_cc35xx_driver_api *)data->hsm->api;
	if (api->do_crypto == NULL) {
		return -EINVAL;
	}

	return api->do_crypto(data->hsm, CRYPTO_CIPHER_ALGO_AES, mode, op_type, ctx, pkt);
}

static int crypto_ti_cc35xx_do_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int result;

	if (pkt->in_len > HSM_AES_BLOCK_SIZE) {
		return -EINVAL;
	}

	result = crypto_ti_cc35xx_do(CRYPTO_CIPHER_MODE_ECB, CRYPTO_CIPHER_OP_ENCRYPT, ctx, pkt);

	if (result == 0) {
		pkt->out_len = HSM_AES_BLOCK_SIZE;
	}

	return result;
}

static int crypto_ti_cc35xx_do_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int result;

	if (pkt->in_len > HSM_AES_BLOCK_SIZE) {
		return -EINVAL;
	}

	result = crypto_ti_cc35xx_do(CRYPTO_CIPHER_MODE_ECB, CRYPTO_CIPHER_OP_DECRYPT, ctx, pkt);

	if (result == 0) {
		pkt->out_len = HSM_AES_BLOCK_SIZE;
	}

	return result;
}

static int crypto_ti_cc35xx_do_ctr_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					   uint8_t *iv)
{
	int result;
	struct hsm_ti_cc35xx_driver_session *session;
	int ivlen;

	if (ctx == NULL || pkt == NULL || ctx->drv_sessn_state == NULL) {
		return -EINVAL;
	}

	session = (struct hsm_ti_cc35xx_driver_session *const)(ctx->drv_sessn_state);
	ivlen = sizeof(session->ctr) - (ctx->mode_params.ctr_info.ctr_len >> 3);

	if (ivlen < 0 || (sizeof(session->ctr) < ivlen) || ((sizeof(session->ctr) % 4) != 0)) {
		return -EINVAL;
	}

	memcpy(session->ctr, iv, ivlen);

	result = crypto_ti_cc35xx_do(CRYPTO_CIPHER_MODE_CTR, CRYPTO_CIPHER_OP_ENCRYPT, ctx, pkt);
	if (result == 0) {
		pkt->out_len = pkt->in_len;
	}

	return result;
}

static int crypto_ti_cc35xx_do_ctr_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					   uint8_t *iv)
{
	int result;
	struct hsm_ti_cc35xx_driver_session *session;
	int ivlen;

	if (ctx == NULL || pkt == NULL || ctx->drv_sessn_state == NULL) {
		return -EINVAL;
	}

	session = (struct hsm_ti_cc35xx_driver_session *const)(ctx->drv_sessn_state);
	ivlen = sizeof(session->ctr) - (ctx->mode_params.ctr_info.ctr_len >> 3);

	if (ivlen < 0 || (sizeof(session->ctr) < ivlen) || ((sizeof(session->ctr) % 4) != 0)) {
		return -EINVAL;
	}

	memcpy(session->ctr, iv, ivlen);

	result = crypto_ti_cc35xx_do(CRYPTO_CIPHER_MODE_CTR, CRYPTO_CIPHER_OP_ENCRYPT, ctx, pkt);
	if (result == 0) {
		pkt->out_len = pkt->in_len;
	}

	return result;
}

static int crypto_ti_cc35xx_do_ccm_encrypt(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
					   uint8_t *nonce)
{
	int result;
	struct hsm_ti_cc35xx_driver_session *session;

	session = (struct hsm_ti_cc35xx_driver_session *const)(ctx->drv_sessn_state);
	session->nonce = nonce;

	result = crypto_ti_cc35xx_do(CRYPTO_CIPHER_MODE_CCM, CRYPTO_CIPHER_OP_ENCRYPT, ctx, pkt);
	if (result == 0) {
		pkt->pkt->out_len = pkt->pkt->in_len + ctx->mode_params.ccm_info.tag_len;
	}

	return result;
}

static int crypto_ti_cc35xx_do_ccm_decrypt(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
					   uint8_t *nonce)
{
	int result;
	struct hsm_ti_cc35xx_driver_session *session;

	session = (struct hsm_ti_cc35xx_driver_session *const)(ctx->drv_sessn_state);
	session->nonce = nonce;

	result = crypto_ti_cc35xx_do(CRYPTO_CIPHER_MODE_CCM, CRYPTO_CIPHER_OP_DECRYPT, ctx, pkt);

	if (result == 0) {
		pkt->pkt->out_len = pkt->pkt->in_len + ctx->mode_params.ccm_info.tag_len;
	}

	return result;
}

static int crypto_ti_cc35xx_query_caps(const struct device *dev)
{
	struct crypto_ti_cc35xx_data *data;
	struct hsm_ti_cc35xx_driver_api *api;

	if (dev == NULL || dev->data == NULL) {
		return -ENODEV;
	}

	data = (struct crypto_ti_cc35xx_data *)dev->data;
	if (data->hsm == NULL || data->hsm->api == NULL) {
		return -EINVAL;
	}

	api = (struct hsm_ti_cc35xx_driver_api *)data->hsm->api;
	if (api->get_hw_caps == NULL) {
		return -EINVAL;
	}

	return api->get_hw_caps(data->hsm);
}

static int crypto_ti_cc35xx_session_setup(const struct device *dev, struct cipher_ctx *ctx,
					  enum cipher_algo algo, enum cipher_mode mode,
					  enum cipher_op op_type)
{
	static struct hsm_ti_cc35xx_driver_session session = {0};

	if (ctx->key.bit_stream == NULL) {
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_ti_cc35xx_do_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_ti_cc35xx_do_ctr_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			ctx->ops.ccm_crypt_hndlr = crypto_ti_cc35xx_do_ccm_encrypt;
			break;
		default:
			return -ENOTSUP;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_ti_cc35xx_do_ecb_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_ti_cc35xx_do_ctr_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			ctx->ops.ccm_crypt_hndlr = crypto_ti_cc35xx_do_ccm_decrypt;
			break;
		default:
			return -ENOTSUP;
		}
	}

	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = &session;
	ctx->device = dev;

	return 0;
}

static int crypto_ti_cc35xx_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);
	struct hsm_ti_cc35xx_driver_session *session;

	if (ctx == NULL || ctx->drv_sessn_state == NULL) {
		return -EINVAL;
	}

	session = (struct hsm_ti_cc35xx_driver_session *)ctx->drv_sessn_state;
	memset(session, 0, sizeof(struct hsm_ti_cc35xx_driver_session));
	ctx->drv_sessn_state = NULL;
	ctx->device = NULL;

	return 0;
}

static int crypto_ti_cc35xx_init(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_NODELABEL(hsm));
	struct crypto_ti_cc35xx_data *data = dev->data;

	if (!device_is_ready(parent)) {
		return -ENODEV;
	}

	data->hsm = parent;

	return 0;
}

static DEVICE_API(crypto, crypto_ti_cc35xx_driver_api) = {
	.cipher_begin_session = crypto_ti_cc35xx_session_setup,
	.cipher_free_session = crypto_ti_cc35xx_session_free,
	.query_hw_caps = crypto_ti_cc35xx_query_caps,
};

static struct crypto_ti_cc35xx_data crypto_data;

DEVICE_DT_INST_DEFINE(0, crypto_ti_cc35xx_init, PM_DEVICE_DT_INST_GET(0), &crypto_data, NULL,
		      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, &crypto_ti_cc35xx_driver_api);
