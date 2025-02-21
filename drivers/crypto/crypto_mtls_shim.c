/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Shim layer for mbedTLS, crypto API compliant.
 */


#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <errno.h>
#include <zephyr/crypto/crypto.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#include <mbedtls/ccm.h>
#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
#include <mbedtls/gcm.h>
#endif
#include <mbedtls/aes.h>

#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

#define MTLS_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
		      CAP_NO_IV_PREFIX)

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbedtls);

struct mtls_shim_session {
	union {
		mbedtls_ccm_context mtls_ccm;
#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
		mbedtls_gcm_context mtls_gcm;
#endif
		mbedtls_aes_context mtls_aes;
		mbedtls_sha256_context mtls_sha256;
		mbedtls_sha512_context mtls_sha512;
	};
	bool in_use;
	union {
		enum cipher_mode mode;
		enum hash_algo algo;
	};
};

#define CRYPTO_MAX_SESSION CONFIG_CRYPTO_MBEDTLS_SHIM_MAX_SESSION

struct mtls_shim_session mtls_sessions[CRYPTO_MAX_SESSION];

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#else
#error "You need to define MBEDTLS_MEMORY_BUFFER_ALLOC_C"
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */

#define MTLS_GET_CTX(c, m) \
	(&((struct mtls_shim_session *)c->drv_sessn_state)->mtls_ ## m)

#define MTLS_GET_ALGO(c) \
	(((struct mtls_shim_session *)c->drv_sessn_state)->algo)

int mtls_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int ret;
	mbedtls_aes_context *ecb_ctx = MTLS_GET_CTX(ctx, aes);

	/* For security reasons, ECB mode should not be used to encrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot encrypt more than 1 block");
		return -EINVAL;
	}

	ret = mbedtls_aes_crypt_ecb(ecb_ctx, MBEDTLS_AES_ENCRYPT,
				    pkt->in_buf, pkt->out_buf);
	if (ret) {
		LOG_ERR("Could not encrypt (%d)", ret);
		return -EINVAL;
	}

	pkt->out_len = 16;

	return 0;
}

int mtls_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int ret;
	mbedtls_aes_context *ecb_ctx = MTLS_GET_CTX(ctx, aes);

	/* For security reasons, ECB mode should not be used to decrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot decrypt more than 1 block");
		return -EINVAL;
	}

	ret = mbedtls_aes_crypt_ecb(ecb_ctx, MBEDTLS_AES_DECRYPT,
				    pkt->in_buf, pkt->out_buf);
	if (ret) {
		LOG_ERR("Could not encrypt (%d)", ret);
		return -EINVAL;
	}

	pkt->out_len = 16;

	return 0;
}

int mtls_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret, iv_bytes;
	uint8_t *p_iv, iv_loc[16];
	mbedtls_aes_context *cbc_ctx = MTLS_GET_CTX(ctx, aes);

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		/* Prefix IV to ciphertext, which is default behavior of Zephyr
		 * crypto API, unless CAP_NO_IV_PREFIX is requested.
		 */
		iv_bytes = 16;
		memcpy(pkt->out_buf, iv, 16);
		p_iv = iv;
	} else {
		iv_bytes = 0;
		memcpy(iv_loc, iv, 16);
		p_iv = iv_loc;
	}

	ret = mbedtls_aes_crypt_cbc(cbc_ctx, MBEDTLS_AES_ENCRYPT, pkt->in_len,
				    p_iv, pkt->in_buf, pkt->out_buf + iv_bytes);
	if (ret) {
		LOG_ERR("Could not encrypt (%d)", ret);
		return -EINVAL;
	}

	pkt->out_len = pkt->in_len + iv_bytes;

	return 0;
}

int mtls_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret, iv_bytes;
	uint8_t *p_iv, iv_loc[16];
	mbedtls_aes_context *cbc_ctx = MTLS_GET_CTX(ctx, aes);

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		iv_bytes = 16;
		p_iv = iv;
	} else {
		iv_bytes = 0;
		memcpy(iv_loc, iv, 16);
		p_iv = iv_loc;
	}

	ret = mbedtls_aes_crypt_cbc(cbc_ctx, MBEDTLS_AES_DECRYPT, pkt->in_len,
				    p_iv, pkt->in_buf + iv_bytes, pkt->out_buf);
	if (ret) {
		LOG_ERR("Could not encrypt (%d)", ret);
		return -EINVAL;
	}

	pkt->out_len = pkt->in_len - iv_bytes;

	return 0;
}

static int mtls_ccm_encrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 uint8_t *nonce)
{
	mbedtls_ccm_context *mtls_ctx = MTLS_GET_CTX(ctx, ccm);
	int ret;

	ret = mbedtls_ccm_encrypt_and_tag(mtls_ctx, apkt->pkt->in_len, nonce,
					  ctx->mode_params.ccm_info.nonce_len,
					  apkt->ad, apkt->ad_len,
					  apkt->pkt->in_buf,
					  apkt->pkt->out_buf, apkt->tag,
					  ctx->mode_params.ccm_info.tag_len);
	if (ret) {
		LOG_ERR("Could not encrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	/* This is equivalent to what the TinyCrypt shim does in
	 * do_ccm_encrypt_mac().
	 */
	apkt->pkt->out_len = apkt->pkt->in_len;
	apkt->pkt->out_len += ctx->mode_params.ccm_info.tag_len;

	return 0;
}

static int mtls_ccm_decrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 uint8_t *nonce)
{
	mbedtls_ccm_context *mtls_ctx = MTLS_GET_CTX(ctx, ccm);
	int ret;

	ret = mbedtls_ccm_auth_decrypt(mtls_ctx, apkt->pkt->in_len, nonce,
				       ctx->mode_params.ccm_info.nonce_len,
				       apkt->ad, apkt->ad_len,
				       apkt->pkt->in_buf,
				       apkt->pkt->out_buf, apkt->tag,
				       ctx->mode_params.ccm_info.tag_len);
	if (ret) {
		if (ret == MBEDTLS_ERR_CCM_AUTH_FAILED) {
			LOG_ERR("Message authentication failed");
			return -EFAULT;
		}

		LOG_ERR("Could not decrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	apkt->pkt->out_len = apkt->pkt->in_len;
	apkt->pkt->out_len += ctx->mode_params.ccm_info.tag_len;

	return 0;
}

#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
static int mtls_gcm_encrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 uint8_t *nonce)
{
	mbedtls_gcm_context *mtls_ctx = MTLS_GET_CTX(ctx, gcm);
	int ret;

	ret = mbedtls_gcm_crypt_and_tag(mtls_ctx, MBEDTLS_GCM_ENCRYPT,
					  apkt->pkt->in_len, nonce,
					  ctx->mode_params.gcm_info.nonce_len,
					  apkt->ad, apkt->ad_len,
					  apkt->pkt->in_buf,
					  apkt->pkt->out_buf,
					  ctx->mode_params.gcm_info.tag_len,
					  apkt->tag);
	if (ret) {
		LOG_ERR("Could not encrypt/auth (%d)", ret);

		return -EINVAL;
	}

	/* This is equivalent to what is done in mtls_ccm_encrypt_auth(). */
	apkt->pkt->out_len = apkt->pkt->in_len;
	apkt->pkt->out_len += ctx->mode_params.gcm_info.tag_len;

	return 0;
}

static int mtls_gcm_decrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 uint8_t *nonce)
{
	mbedtls_gcm_context *mtls_ctx = MTLS_GET_CTX(ctx, gcm);
	int ret;

	ret = mbedtls_gcm_auth_decrypt(mtls_ctx, apkt->pkt->in_len, nonce,
				       ctx->mode_params.gcm_info.nonce_len,
				       apkt->ad, apkt->ad_len,
				       apkt->tag,
				       ctx->mode_params.gcm_info.tag_len,
				       apkt->pkt->in_buf,
				       apkt->pkt->out_buf);
	if (ret) {
		if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
			LOG_ERR("Message authentication failed");
			return -EFAULT;
		}

		LOG_ERR("Could not decrypt/auth (%d)", ret);
		return -EINVAL;
	}

	apkt->pkt->out_len = apkt->pkt->in_len;
	apkt->pkt->out_len += ctx->mode_params.gcm_info.tag_len;

	return 0;
}
#endif /* CONFIG_MBEDTLS_CIPHER_GCM_ENABLED */

static int mtls_get_unused_session_index(void)
{
	int i;

	for (i = 0; i < CRYPTO_MAX_SESSION; i++) {
		if (!mtls_sessions[i].in_use) {
			mtls_sessions[i].in_use = true;
			return i;
		}
	}

	return -1;
}

static int mtls_session_setup(const struct device *dev,
			      struct cipher_ctx *ctx,
			      enum cipher_algo algo, enum cipher_mode mode,
			      enum cipher_op op_type)
{
	mbedtls_aes_context *aes_ctx;
	mbedtls_ccm_context *ccm_ctx;
#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
	mbedtls_gcm_context *gcm_ctx;
#endif
	int ctx_idx;
	int ret;

	if (ctx->flags & ~(MTLS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	if (mode != CRYPTO_CIPHER_MODE_CCM &&
	    mode != CRYPTO_CIPHER_MODE_CBC &&
#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
	    mode != CRYPTO_CIPHER_MODE_GCM &&
#endif
	    mode != CRYPTO_CIPHER_MODE_ECB) {
		LOG_ERR("Unsupported mode");
		return -EINVAL;
	}

	if (ctx->keylen != 16U) {
		LOG_ERR("%u key size is not supported", ctx->keylen);
		return -EINVAL;
	}

	ctx_idx = mtls_get_unused_session_index();
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}


	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		aes_ctx = &mtls_sessions[ctx_idx].mtls_aes;
		mbedtls_aes_init(aes_ctx);
		if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
			ret = mbedtls_aes_setkey_enc(aes_ctx,
					ctx->key.bit_stream, ctx->keylen * 8U);
			ctx->ops.block_crypt_hndlr = mtls_ecb_encrypt;
		} else {
			ret = mbedtls_aes_setkey_dec(aes_ctx,
					ctx->key.bit_stream, ctx->keylen * 8U);
			ctx->ops.block_crypt_hndlr = mtls_ecb_decrypt;
		}
		if (ret) {
			LOG_ERR("AES_ECB: failed at setkey (%d)", ret);
			ctx->ops.block_crypt_hndlr = NULL;
			mtls_sessions[ctx_idx].in_use = false;
			return -EINVAL;
		}
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		aes_ctx = &mtls_sessions[ctx_idx].mtls_aes;
		mbedtls_aes_init(aes_ctx);
		if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
			ret = mbedtls_aes_setkey_enc(aes_ctx,
					ctx->key.bit_stream, ctx->keylen * 8U);
			ctx->ops.cbc_crypt_hndlr = mtls_cbc_encrypt;
		} else {
			ret = mbedtls_aes_setkey_dec(aes_ctx,
					ctx->key.bit_stream, ctx->keylen * 8U);
			ctx->ops.cbc_crypt_hndlr = mtls_cbc_decrypt;
		}
		if (ret) {
			LOG_ERR("AES_CBC: failed at setkey (%d)", ret);
			ctx->ops.cbc_crypt_hndlr = NULL;
			mtls_sessions[ctx_idx].in_use = false;
			return -EINVAL;
		}
		break;
	case CRYPTO_CIPHER_MODE_CCM:
		ccm_ctx = &mtls_sessions[ctx_idx].mtls_ccm;
		mbedtls_ccm_init(ccm_ctx);
		ret = mbedtls_ccm_setkey(ccm_ctx, MBEDTLS_CIPHER_ID_AES,
					 ctx->key.bit_stream, ctx->keylen * 8U);
		if (ret) {
			LOG_ERR("AES_CCM: failed at setkey (%d)", ret);
			mtls_sessions[ctx_idx].in_use = false;

			return -EINVAL;
		}
		if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
			ctx->ops.ccm_crypt_hndlr = mtls_ccm_encrypt_auth;
		} else {
			ctx->ops.ccm_crypt_hndlr = mtls_ccm_decrypt_auth;
		}
		break;
#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
	case CRYPTO_CIPHER_MODE_GCM:
		gcm_ctx = &mtls_sessions[ctx_idx].mtls_gcm;
		mbedtls_gcm_init(gcm_ctx);
		ret = mbedtls_gcm_setkey(gcm_ctx, MBEDTLS_CIPHER_ID_AES,
					 ctx->key.bit_stream, ctx->keylen * 8U);
		if (ret) {
			LOG_ERR("AES_GCM: failed at setkey (%d)", ret);
			mtls_sessions[ctx_idx].in_use = false;

			return -EINVAL;
		}
		if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
			ctx->ops.gcm_crypt_hndlr = mtls_gcm_encrypt_auth;
		} else {
			ctx->ops.gcm_crypt_hndlr = mtls_gcm_decrypt_auth;
		}
		break;
#endif /* CONFIG_MBEDTLS_CIPHER_GCM_ENABLED */
	default:
		LOG_ERR("Unhandled mode");
		mtls_sessions[ctx_idx].in_use = false;
		return -EINVAL;
	}

	mtls_sessions[ctx_idx].mode = mode;
	ctx->drv_sessn_state = &mtls_sessions[ctx_idx];

	return ret;
}

static int mtls_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	struct mtls_shim_session *mtls_session =
		(struct mtls_shim_session *)ctx->drv_sessn_state;

	if (mtls_session->mode == CRYPTO_CIPHER_MODE_CCM) {
		mbedtls_ccm_free(&mtls_session->mtls_ccm);
#ifdef CONFIG_MBEDTLS_CIPHER_GCM_ENABLED
	} else if (mtls_session->mode == CRYPTO_CIPHER_MODE_GCM) {
		mbedtls_gcm_free(&mtls_session->mtls_gcm);
#endif
	} else {
		mbedtls_aes_free(&mtls_session->mtls_aes);
	}
	mtls_session->in_use = false;

	return 0;
}

static int mtls_sha256_compute(struct hash_ctx *ctx, struct hash_pkt *pkt,
			       bool finish)
{
	int ret;
	mbedtls_sha256_context *sha256_ctx = MTLS_GET_CTX(ctx, sha256);


	if (!ctx->started) {
		ret = mbedtls_sha256_starts(sha256_ctx,
					    MTLS_GET_ALGO(ctx) == CRYPTO_HASH_ALGO_SHA224);
		if (ret != 0) {
			LOG_ERR("Could not compute the hash");
			return -EINVAL;
		}
		ctx->started = true;
	}

	ret = mbedtls_sha256_update(sha256_ctx, pkt->in_buf, pkt->in_len);
	if (ret != 0) {
		LOG_ERR("Could not update the hash");
		ctx->started = false;
		return -EINVAL;
	}

	if (finish) {
		ctx->started = false;
		ret = mbedtls_sha256_finish(sha256_ctx, pkt->out_buf);
		if (ret != 0) {
			LOG_ERR("Could not compute the hash");
			return -EINVAL;
		}
	}

	return 0;
}

static int mtls_sha512_compute(struct hash_ctx *ctx, struct hash_pkt *pkt,
			       bool finish)
{
	int ret;
	mbedtls_sha512_context *sha512_ctx = MTLS_GET_CTX(ctx, sha512);

	if (!ctx->started) {
		ret = mbedtls_sha512_starts(sha512_ctx,
					    MTLS_GET_ALGO(ctx) == CRYPTO_HASH_ALGO_SHA384);
		if (ret != 0) {
			LOG_ERR("Could not compute the hash");
			return -EINVAL;
		}
		ctx->started = true;
	}

	ret = mbedtls_sha512_update(sha512_ctx, pkt->in_buf, pkt->in_len);
	if (ret != 0) {
		LOG_ERR("Could not update the hash");
		ctx->started = false;
		return -EINVAL;
	}

	if (finish) {
		ctx->started = false;
		ret = mbedtls_sha512_finish(sha512_ctx, pkt->out_buf);
		if (ret != 0) {
			LOG_ERR("Could not compute the hash");
			return -EINVAL;
		}
	}

	return 0;
}

static int mtls_hash_session_setup(const struct device *dev,
				   struct hash_ctx *ctx,
				   enum hash_algo algo)
{
	int ctx_idx;

	if (ctx->flags & ~(MTLS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if ((algo != CRYPTO_HASH_ALGO_SHA224) &&
	    (algo != CRYPTO_HASH_ALGO_SHA256) &&
	    (algo != CRYPTO_HASH_ALGO_SHA384) &&
	    (algo != CRYPTO_HASH_ALGO_SHA512)) {
		LOG_ERR("Unsupported algo: %d", algo);
		return -EINVAL;
	}

	ctx_idx = mtls_get_unused_session_index();
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	mtls_sessions[ctx_idx].algo = algo;
	ctx->drv_sessn_state = &mtls_sessions[ctx_idx];
	ctx->started = false;

	if ((algo == CRYPTO_HASH_ALGO_SHA224) ||
	    (algo == CRYPTO_HASH_ALGO_SHA256)) {
		mbedtls_sha256_context *sha256_ctx =
			&mtls_sessions[ctx_idx].mtls_sha256;
		mbedtls_sha256_init(sha256_ctx);
		ctx->hash_hndlr = mtls_sha256_compute;
	} else {
		mbedtls_sha512_context *sha512_ctx =
			&mtls_sessions[ctx_idx].mtls_sha512;
		mbedtls_sha512_init(sha512_ctx);
		ctx->hash_hndlr = mtls_sha512_compute;
	}

	return 0;
}

static int mtls_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct mtls_shim_session *mtls_session =
		(struct mtls_shim_session *)ctx->drv_sessn_state;

	if (mtls_session->algo == CRYPTO_HASH_ALGO_SHA256) {
		mbedtls_sha256_free(&mtls_session->mtls_sha256);
	} else {
		mbedtls_sha512_free(&mtls_session->mtls_sha512);
	}
	mtls_session->in_use = false;

	return 0;
}


static int mtls_query_caps(const struct device *dev)
{
	return MTLS_SUPPORT;
}

static DEVICE_API(crypto, mtls_crypto_funcs) = {
	.cipher_begin_session = mtls_session_setup,
	.cipher_free_session = mtls_session_free,
	.cipher_async_callback_set = NULL,
	.hash_begin_session = mtls_hash_session_setup,
	.hash_free_session = mtls_hash_session_free,
	.query_hw_caps = mtls_query_caps,
};

DEVICE_DEFINE(crypto_mtls, CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME,
	      NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
	      (void *)&mtls_crypto_funcs);
