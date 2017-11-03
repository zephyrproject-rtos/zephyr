/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Shim layer for TinyCrypt, making it complaint to crypto API.
 */

#include <tinycrypt/cbc_mode.h>
#include <tinycrypt/ctr_mode.h>
#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <string.h>
#include <crypto/cipher.h>
#include "crypto_tc_shim_priv.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CRYPTO_LEVEL
#include <logging/sys_log.h>


#define CRYPTO_MAX_SESSION CONFIG_CRYPTO_TINYCRYPT_SHIM_MAX_SESSION

static struct tc_shim_drv_state tc_driver_state[CRYPTO_MAX_SESSION];

static int do_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
			  u8_t *iv)
{
	struct tc_shim_drv_state *data =  ctx->drv_sessn_state;

	if (tc_cbc_mode_encrypt(op->out_buf,
				op->out_buf_max,
				op->in_buf, op->in_len,
				iv,
				&data->session_key) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error during CBC encryption");
		return -EIO;
	}

	return 0;
}

static int do_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
			  u8_t *iv)
{
	struct tc_shim_drv_state *data =  ctx->drv_sessn_state;

	/* TinyCrypt expects the IV and cipher text to be in a contiguous
	 * buffer for efficieny
	 */
	if (iv != op->in_buf) {
		SYS_LOG_ERR("TC needs contiguous iv and ciphertext");
		return -EIO;
	}

	if (tc_cbc_mode_decrypt(op->out_buf,
			op->out_buf_max,
			op->in_buf + TC_AES_BLOCK_SIZE,
			op->in_len,
			op->in_buf, &data->session_key) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("Func TC internal error during CBC decryption");
		return -EIO;
	}

	return 0;
}


static int do_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *op,
		     u8_t *iv)
{
	struct tc_shim_drv_state *data =  ctx->drv_sessn_state;
	u8_t ctr[16] = {0};	/* CTR mode Counter =  iv:ctr */
	int ivlen = ctx->keylen - (ctx->mode_params.ctr_info.ctr_len >> 3);

	/* Tinycrypt takes the last 4 bytes of the counter parameter as the
	 * true counter start. IV forms the first 12 bytes of the split counter.
	 */
	memcpy(ctr, iv, ivlen);

	if (tc_ctr_mode(op->out_buf, op->out_buf_max, op->in_buf,
			op->in_len, ctr,
			&data->session_key) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error during CTR OP");
		return -EIO;
	}

	return 0;
}

static int do_ccm_encrypt_mac(struct cipher_ctx *ctx,
			     struct cipher_aead_pkt *aead_op, u8_t *nonce)
{
	struct tc_ccm_mode_struct ccm;
	struct tc_shim_drv_state *data =  ctx->drv_sessn_state;
	struct ccm_params *ccm_param = &ctx->mode_params.ccm_info;
	struct cipher_pkt *op = aead_op->pkt;

	if (tc_ccm_config(&ccm, &data->session_key, nonce,
			ccm_param->nonce_len,
			ccm_param->tag_len) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error during CCM encryption config");
		return -EIO;
	}

	if (tc_ccm_generation_encryption(op->out_buf, op->out_buf_max,
					 aead_op->ad, aead_op->ad_len, op->in_buf,
					 op->in_len, &ccm) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error during CCM Encryption OP");
		return -EIO;
	}

	/* Looks like TinyCrypt appends the MAC to the end of out_buf as it
	 * does not give a separate hash parameter. The user needs to be aware
	 * of this and provide sufficient buffer space in output buffer to hold
	 * both encrypted output and hash
	 */
	aead_op->tag = op->out_buf + op->in_len;

	return 0;
}

static int do_ccm_decrypt_auth(struct cipher_ctx *ctx,
			       struct cipher_aead_pkt *aead_op, u8_t *nonce)
{
	struct tc_ccm_mode_struct ccm;
	struct tc_shim_drv_state *data =  ctx->drv_sessn_state;
	struct ccm_params *ccm_param = &ctx->mode_params.ccm_info;
	struct cipher_pkt *op = aead_op->pkt;

	if (tc_ccm_config(&ccm, &data->session_key, nonce,
			  ccm_param->nonce_len,
			  ccm_param->tag_len) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error during CCM decryption config");
		return -EIO;
	}

	/* TinyCrypt expects the hash/MAC to be present at the end of in_buf
	 * as it doesnt take a separate hash parameter. Ideally this should
	 * be moved to a ctx.flag check during session_setup, later.
	 */
	if (aead_op->tag != op->in_buf + op->in_len) {
		SYS_LOG_ERR("TC needs contiguous hash  at the end of inbuf");
		return -EIO;
	}

	if (tc_ccm_decryption_verification(op->out_buf, op->out_buf_max,
					   aead_op->ad, aead_op->ad_len,
					   op->in_buf,
					   op->in_len + ccm_param->tag_len,
					   &ccm) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error during CCM decryption OP");
		return -EIO;
	}

	return 0;
}

static int get_unused_session(void)
{
	int i;

	for (i = 0; i < CRYPTO_MAX_SESSION; i++) {
		if (tc_driver_state[i].in_use == 0) {
			tc_driver_state[i].in_use = 1;
			break;
		}
	}

	return i;
}

static int tc_session_setup(struct device *dev, struct cipher_ctx *ctx,
		     enum cipher_algo algo, enum cipher_mode mode,
		     enum cipher_op op_type)
{
	struct tc_shim_drv_state *data;
	int idx;

	ARG_UNUSED(dev);

	/* The shim currently supports only CBC or CTR mode for AES */
	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		SYS_LOG_ERR("TC Shim Unsupported algo");
		return -EINVAL;
	}

	/* TinyCrypt being a software library, only synchronous operations
	 * make sense.
	 */
	if (!(ctx->flags & CAP_SYNC_OPS)) {
		SYS_LOG_ERR("Async not supported by this driver");
		return -EINVAL;
	}

	if (ctx->keylen != TC_AES_KEY_SIZE) {
		/* TinyCrypt supports only 128 bits */
		SYS_LOG_ERR("TC Shim Unsupported key size");
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = do_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			if (ctx->mode_params.ctr_info.ctr_len != 32) {
				SYS_LOG_ERR("Tinycrypt supports only 32 bit "
					    "counter");
				return -EINVAL;
			}
			ctx->ops.ctr_crypt_hndlr = do_ctr_op;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			ctx->ops.ccm_crypt_hndlr = do_ccm_encrypt_mac;
			break;
		default:
			SYS_LOG_ERR("TC Shim Unsupported mode");
			return -EINVAL;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = do_cbc_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			/* Maybe validate CTR length */
			if (ctx->mode_params.ctr_info.ctr_len != 32) {
				SYS_LOG_ERR("Tinycrypt supports only 32 bit "
					    "counter");
				return -EINVAL;
			}
			ctx->ops.ctr_crypt_hndlr = do_ctr_op;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			ctx->ops.ccm_crypt_hndlr = do_ccm_decrypt_auth;
			break;
		default:
			SYS_LOG_ERR("TC Shim Unsupported mode");
			return -EINVAL;
		}

	}

	ctx->ops.cipher_mode = mode;

	idx = get_unused_session();
	if (idx == CRYPTO_MAX_SESSION) {
		SYS_LOG_ERR("Max sessions in progress");
		return -ENOSPC;
	}

	data = &tc_driver_state[idx];

	if (tc_aes128_set_encrypt_key(&data->session_key, ctx->key.bit_stream)
			 == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("TC internal error in setting key");
		tc_driver_state[idx].in_use = 0;

		return -EIO;
	}

	ctx->drv_sessn_state = data;

	return 0;
}

static int tc_query_caps(struct device *dev)
{
	return (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}


static int tc_session_free(struct device *dev, struct cipher_ctx *sessn)
{
	struct tc_shim_drv_state *data =  sessn->drv_sessn_state;

	ARG_UNUSED(dev);
	memset(data, 0, sizeof(struct tc_shim_drv_state));
	data->in_use = 0;

	return 0;

}

static int tc_shim_init(struct device *dev)
{
	int i;

	ARG_UNUSED(dev);
	for (i = 0; i < CRYPTO_MAX_SESSION; i++) {
		tc_driver_state[i].in_use = 0;
	}
	return 0;
}




static struct crypto_driver_api crypto_enc_funcs = {
	.begin_session = tc_session_setup,
	.free_session = tc_session_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = tc_query_caps,
};


DEVICE_AND_API_INIT(crypto_tinycrypt, CONFIG_CRYPTO_TINYCRYPT_SHIM_DRV_NAME,
		    &tc_shim_init, NULL, NULL,
		    POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		    (void *)&crypto_enc_funcs);
