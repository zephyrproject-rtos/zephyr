/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Shim layer for TinyCrypt, making it complaint to crypto API.
 */

#include <tinycrypt/aes.h>
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/hmac_prng.h>
#include <tinycrypt/cbc_mode.h>
#include <tinycrypt/ctr_mode.h>
#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <string.h>
#include <crypto/cipher.h>
#include <kernel.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CRYPTO_LEVEL
#include <logging/sys_log.h>

struct tc_shim_drv_state {
	struct tc_aes_key_sched_struct session_key;
	union {
		struct tc_cmac_struct cmac_state;
		struct tc_hmac_prng_struct prng_state;
	};
};

K_MEM_POOL_DEFINE(tc_shim_session_pool,
	(sizeof(struct tc_shim_drv_state) + sizeof(struct k_mem_block_id)),
	(sizeof(struct tc_shim_drv_state) + sizeof(struct k_mem_block_id)),
	CONFIG_CRYPTO_MAX_SESSION,
	__alignof__(struct tc_shim_drv_state));

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

	/* out_len is the same as in_len in CBC mode */
	op->out_len = op->in_len;

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
			op->in_len - TC_AES_BLOCK_SIZE,
			op->in_buf, &data->session_key) == TC_CRYPTO_FAIL) {
		SYS_LOG_ERR("Func TC internal error during CBC decryption");
		return -EIO;
	}

	/* out_len is the same as in_len in CBC mode */
	op->out_len = op->in_len;

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

	/* out_len is the same as in_len in CTR mode */
	op->out_len = op->in_len;

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

	/* Before returning TC_CRYPTO_SUCCESS, tc_ccm_generation_encryption()
	 * will advance the output buffer pointer by op->in_len bytes,
	 * and then increment it ccm.mlen times (while writing to it).
	 */
	op->out_len = op->in_len + ccm.mlen;

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

	op->out_len = op->in_len + ccm_param->tag_len;

	return 0;
}

static int do_mac_op(struct cipher_ctx *ctx, struct cipher_mac_pkt *pkt)
{
	struct tc_shim_drv_state *state = ctx->drv_sessn_state;
	int r;

	if (pkt->finalize) {
		r = tc_cmac_final(pkt->data, &state->cmac_state);
	} else {
		r = tc_cmac_update(&state->cmac_state, pkt->data,
				   pkt->data_len);
	}

	return r == TC_CRYPTO_SUCCESS ? 0 : -EIO;
}

static int do_prng_op(struct cipher_ctx *ctx,
		      struct cipher_prng_pkt *pkt)
{
	struct tc_shim_drv_state *state = ctx->drv_sessn_state;
	int r;

	if (pkt->reseed) {
		r = tc_hmac_prng_reseed(&state->prng_state,
					pkt->data, pkt->data_len,
					pkt->additional_input,
					pkt->additional_input_len);
	} else {
		r = tc_hmac_prng_generate(pkt->data, pkt->data_len,
					  &state->prng_state);
	}

	switch (r) {
	case TC_CRYPTO_SUCCESS:
		return 0;
	case TC_HMAC_PRNG_RESEED_REQ:
		return -EAGAIN;
	default:
		return -EIO;
	}
}

static int do_ecc_op_gen_key(struct cipher_ctx *ctx,
			     struct cipher_ecc_pkt *pkt)
{
	int r;

	if (pkt->curve != CIPHER_ECC_CURVE_SECP256R1) {
		return -EINVAL;
	}
	if (pkt->public_key_len != 64) {
		return -EINVAL;
	}
	if (pkt->private_key_len != 32) {
		return -EINVAL;
	}

	r = uECC_make_key(pkt->public_key,
			  pkt->private_key,
			  &curve_secp256r1);

	return r == TC_CRYPTO_SUCCESS ? 0 : -EINVAL;
}

static int do_ecc_op_validate_pubkey(struct cipher_ctx *ctx,
				     struct cipher_ecc_pkt *pkt)
{
	int r;

	if (pkt->curve != CIPHER_ECC_CURVE_SECP256R1) {
		return -EINVAL;
	}
	if (pkt->public_key_len != 64) {
		return -EINVAL;
	}

	r = uECC_valid_public_key(pkt->public_key,
				  &curve_secp256r1);

	return r == 0 ? 0 : -EINVAL;
}


static int do_ecc_op_gen_shared_key(struct cipher_ctx *ctx,
				    struct cipher_ecc_pkt *pkt)
{
	int r;

	if (pkt->curve != CIPHER_ECC_CURVE_SECP256R1) {
		return -EINVAL;
	}
	if (pkt->public_key_len != 64) {
		return -EINVAL;
	}
	if (pkt->private_key_len != 32) {
		return -EINVAL;
	}
	if (pkt->shared_secret_len != 32) {
		return -EINVAL;
	}

	r = uECC_shared_secret(pkt->public_key,
			       pkt->private_key,
			       pkt->shared_secret,
			       &curve_secp256r1);

	return r == TC_CRYPTO_SUCCESS ? 0 : -EINVAL;
}

static struct tc_shim_drv_state *get_unused_session(void)
{
	return k_mem_pool_malloc(&tc_shim_session_pool,
				 sizeof(struct tc_shim_drv_state));
}

static int tc_session_setup_aes(struct cipher_ctx *ctx,
				enum cipher_mode mode,
				enum cipher_algo algo,
				enum cipher_op op)
{
	struct tc_shim_drv_state *data;

	if (ctx->keylen != TC_AES_KEY_SIZE) {
		/* TinyCrypt supports only 128 bits */
		SYS_LOG_ERR("Unsupported key size for Tinycript shim: %d",
			    ctx->keylen);
		return -EINVAL;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_CBC:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			ctx->ops.cbc_crypt_hndlr = do_cbc_encrypt;
		} else {
			ctx->ops.cbc_crypt_hndlr = do_cbc_decrypt;
		}
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		if (ctx->mode_params.ctr_info.ctr_len != 32) {
			SYS_LOG_ERR("Tinycrypt supports only 32 bit "
				    "counter");
			return -EINVAL;
		}
		/* Same operation works for encryption/decryption */
		ctx->ops.ctr_crypt_hndlr = do_ctr_op;
		break;
	case CRYPTO_CIPHER_MODE_CCM:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			ctx->ops.ccm_crypt_hndlr = do_ccm_encrypt_mac;
		} else {
			ctx->ops.ccm_crypt_hndlr = do_ccm_decrypt_auth;
		}
		break;
	default:
		SYS_LOG_ERR("Unsupported mode for AES algo: %d", mode);
		return -EINVAL;
	}

	data = get_unused_session();
	if (!data) {
		return -ENOSPC;
	}

	if (tc_aes128_set_encrypt_key(&data->session_key,
				      ctx->key.bit_stream) == TC_CRYPTO_FAIL) {
		k_free(data);
		return -EIO;
	}

	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = data;

	return 0;
}

static int tc_session_setup_prng(struct cipher_ctx *ctx,
				 enum cipher_mode mode)
{
	struct tc_shim_drv_state *data;
	int r;

	if (mode != CRYPTO_CIPHER_MODE_PRNG_HMAC) {
		SYS_LOG_ERR("Only HMAC PRNG supported by TC shim driver");
		return -EINVAL;
	}

	data = get_unused_session();
	if (!data) {
		return -ENOSPC;
	}

	r = tc_hmac_prng_init(&data->prng_state,
			      ctx->key.personalization.data,
			      ctx->key.personalization.len);
	if (r != TC_CRYPTO_SUCCESS) {
		k_free(data);
		return -EIO;
	}

	ctx->ops.prng_crypt_hndlr = do_prng_op;
	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = data;

	return 0;
}

static int tc_session_setup_mac(struct cipher_ctx *ctx,
				enum cipher_mode mode)
{
	struct tc_shim_drv_state *data;
	int r;

	if (!(mode & CRYPTO_CIPHER_MODE_CMAC)) {
		SYS_LOG_ERR("Only CMAC supported for MAC algorithms");
		return -EINVAL;
	}

	data = get_unused_session();
	if (!data) {
		return -ENOSPC;
	}

	r = tc_cmac_setup(&data->cmac_state,
			  ctx->key.bit_stream,
			  &data->session_key);
	if (r != TC_CRYPTO_SUCCESS) {
		k_free(data);
		return -EIO;
	}

	ctx->ops.mac_crypt_hndlr = do_mac_op;
	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = data;

	return 0;
}

static int tc_session_setup_ecc(struct cipher_ctx *ctx,
				enum cipher_mode mode,
				enum cipher_op op)
{
	struct tc_shim_drv_state *data;

	if (mode != CRYPTO_CIPHER_MODE_NONE) {
		SYS_LOG_ERR("Only NONE mode supported for ECC algo");
		return -EINVAL;
	}

	switch (op) {
	case CRYPTO_CIPHER_OP_ECC_GEN_KEY:
		ctx->ops.ecc_crypt_hndlr = do_ecc_op_gen_key;
		break;
	case CRYPTO_CIPHER_OP_ECC_GEN_SHARED_KEY:
		ctx->ops.ecc_crypt_hndlr = do_ecc_op_gen_shared_key;
		break;
	case CRYPTO_CIPHER_OP_ECC_VALIDATE_PUBKEY:
		ctx->ops.ecc_crypt_hndlr = do_ecc_op_validate_pubkey;
		break;
	default:
		return -EINVAL;
	}

	data = get_unused_session();
	if (!data) {
		return -ENOSPC;
	}

	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = data;

	return 0;
}

static int tc_session_setup(struct device *dev,
			    struct cipher_ctx *ctx,
			    enum cipher_algo algo,
			    enum cipher_mode mode,
			    enum cipher_op op)
{
	ARG_UNUSED(dev);

	/* TinyCrypt being a software library, only synchronous operations
	 * make sense.
	 */
	if (!(ctx->flags & CAP_SYNC_OPS)) {
		SYS_LOG_ERR("Async not supported by this driver");
		return -EINVAL;
	}

	switch (algo) {
	case CRYPTO_CIPHER_ALGO_AES:
		return tc_session_setup_aes(ctx, mode, algo, op);
	case CRYPTO_CIPHER_ALGO_ECC:
		return tc_session_setup_ecc(ctx, mode, op);
	case CRYPTO_CIPHER_ALGO_PRNG:
		return tc_session_setup_prng(ctx, mode);
	case CRYPTO_CIPHER_ALGO_MAC:
		return tc_session_setup_mac(ctx, mode);
	default:
		SYS_LOG_ERR("Unsupported algo for Tinycrypt shim: %d", algo);
		return -ENOTSUP;
	}
}

static int tc_query_caps(struct device *dev)
{
	return (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int tc_session_free(struct device *dev, struct cipher_ctx *sessn)
{
	struct tc_shim_drv_state *data = sessn->drv_sessn_state;

	ARG_UNUSED(dev);
	(void)memset(data, 0, sizeof(struct tc_shim_drv_state));
	k_free(data);

	return 0;
}

static int tc_shim_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static struct crypto_driver_api crypto_enc_funcs = {
	.begin_session = tc_session_setup,
	.free_session = tc_session_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = tc_query_caps,
};

DEVICE_AND_API_INIT(crypto_tinycrypt, CONFIG_CRYPTO_NAME,
		    &tc_shim_init, NULL, NULL,
		    POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		    (void *)&crypto_enc_funcs);
