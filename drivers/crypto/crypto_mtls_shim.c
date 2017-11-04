/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Shim layer for mbedTLS, crypto API compliant.
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CRYPTO_LEVEL
#include <logging/sys_log.h>

#include <kernel.h>
#include <init.h>
#include <errno.h>
#include <crypto/cipher.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#include <mbedtls/ccm.h>
#include <mbedtls/aes.h>

#define MTLS_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

struct mtls_shim_session {
	mbedtls_ccm_context mtls;
	bool in_use;
};

#define CRYPTO_MAX_SESSION CONFIG_CRYPTO_MBEDTLS_SHIM_MAX_SESSION

struct mtls_shim_session mtls_sessions[CRYPTO_MAX_SESSION];

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#else
#error "You need to define MBEDTLS_MEMORY_BUFFER_ALLOC_C"
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */

static int mtls_ccm_encrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 u8_t *nonce)
{
	mbedtls_ccm_context *mtls_ctx =
		&((struct mtls_shim_session *)ctx->drv_sessn_state)->mtls;
	int ret;

	ret = mbedtls_ccm_encrypt_and_tag(mtls_ctx, apkt->pkt->in_len, nonce,
					  ctx->mode_params.ccm_info.nonce_len,
					  apkt->ad, apkt->ad_len,
					  apkt->pkt->in_buf,
					  apkt->pkt->out_buf, apkt->tag,
					  ctx->mode_params.ccm_info.tag_len);
	if (ret) {
		SYS_LOG_ERR("Could non encrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	return 0;
}

static int mtls_ccm_decrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 u8_t *nonce)
{
	mbedtls_ccm_context *mtls_ctx =
		&((struct mtls_shim_session *)ctx->drv_sessn_state)->mtls;
	int ret;

	ret = mbedtls_ccm_auth_decrypt(mtls_ctx, apkt->pkt->in_len, nonce,
				       ctx->mode_params.ccm_info.nonce_len,
				       apkt->ad, apkt->ad_len,
				       apkt->pkt->in_buf,
				       apkt->pkt->out_buf, apkt->tag,
				       ctx->mode_params.ccm_info.tag_len);
	if (ret) {
		SYS_LOG_ERR("Could non decrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	return 0;
}

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

static int mtls_session_setup(struct device *dev, struct cipher_ctx *ctx,
		       enum cipher_algo algo, enum cipher_mode mode,
		       enum cipher_op op_type)
{
	mbedtls_ccm_context *mtls_ctx;
	int ctx_idx;
	int ret;

	if (ctx->flags & ~(MTLS_SUPPORT)) {
		SYS_LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		SYS_LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	if (mode != CRYPTO_CIPHER_MODE_CCM) {
		SYS_LOG_ERR("Unsupported mode");
		return -EINVAL;
	}

	if (ctx->keylen != 16) {
		SYS_LOG_ERR("%u key size is not supported", ctx->keylen);
		return -EINVAL;
	}

	ctx_idx = mtls_get_unused_session_index();
	if (ctx_idx < 0) {
		SYS_LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	mtls_ctx = &mtls_sessions[ctx_idx].mtls;

	mbedtls_ccm_init(mtls_ctx);

	ret = mbedtls_ccm_setkey(mtls_ctx, MBEDTLS_CIPHER_ID_AES,
				 ctx->key.bit_stream, ctx->keylen * 8);
	if (ret) {
		SYS_LOG_ERR("Could not setup the key (%d)", ret);
		mtls_sessions[ctx_idx].in_use = false;

		return -EINVAL;
	}

	ctx->drv_sessn_state = &mtls_sessions[ctx_idx];

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		ctx->ops.ccm_crypt_hndlr = mtls_ccm_encrypt_auth;
	} else {
		ctx->ops.ccm_crypt_hndlr = mtls_ccm_decrypt_auth;
	}

	return ret;
}

static int mtls_session_free(struct device *dev, struct cipher_ctx *ctx)
{
	struct mtls_shim_session *mtls_session =
		(struct mtls_shim_session *)ctx->drv_sessn_state;

	mbedtls_ccm_free(&mtls_session->mtls);
	mtls_session->in_use = false;

	return 0;
}

static int mtls_query_caps(struct device *dev)
{
	return MTLS_SUPPORT;
}

static int mtls_shim_init(struct device *dev)
{
	return 0;
}

static struct crypto_driver_api mtls_crypto_funcs = {
	.begin_session = mtls_session_setup,
	.free_session = mtls_session_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = mtls_query_caps,
};

DEVICE_AND_API_INIT(crypto_mtls, CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME,
		    &mtls_shim_init, NULL, NULL,
		    POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		    (void *)&mtls_crypto_funcs);
