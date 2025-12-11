/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Shim layer for Mbed TLS, crypto API compliant.
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

#include <psa/crypto.h>

#define MBEDTLS_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
		      CAP_NO_IV_PREFIX)

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbedtls);

struct mbedtls_shim_session {
	union {
		psa_key_id_t key_id;
		psa_hash_operation_t hash_op;
	};
	bool in_use;
	enum cipher_op cipher_op;
	psa_algorithm_t psa_alg;
};

#define CRYPTO_MAX_SESSION CONFIG_CRYPTO_MBEDTLS_SHIM_MAX_SESSION

struct mbedtls_shim_session mbedtls_sessions[CRYPTO_MAX_SESSION];

static K_MUTEX_DEFINE(mbedtls_sessions_lock);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#else
#error "You need to define MBEDTLS_MEMORY_BUFFER_ALLOC_C"
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */

static struct mbedtls_shim_session *mbedtls_get_unused_session(void)
{
	struct mbedtls_shim_session *session = NULL;
	int i;

	k_mutex_lock(&mbedtls_sessions_lock, K_FOREVER);

	for (i = 0; i < CRYPTO_MAX_SESSION; i++) {
		if (!mbedtls_sessions[i].in_use) {
			mbedtls_sessions[i].in_use = true;
			session = &mbedtls_sessions[i];
			break;
		}
	}

	k_mutex_unlock(&mbedtls_sessions_lock);
	return session;
}

static inline void mbedtls_free_session(struct mbedtls_shim_session *session)
{
	k_mutex_lock(&mbedtls_sessions_lock, K_FOREVER);
	session->in_use = false;
	k_mutex_unlock(&mbedtls_sessions_lock);
}

#if CONFIG_PSA_WANT_KEY_TYPE_AES
#if CONFIG_PSA_WANT_ALG_ECB_NO_PADDING
static int mbedtls_ecb(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct mbedtls_shim_session *session = ctx->drv_sessn_state;
	psa_status_t status;

	/* For security reasons, ECB mode should not be used to encrypt/decrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot encrypt more than 1 block");
		return -EINVAL;
	}

	if (session->cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) {
		status = psa_cipher_encrypt(session->key_id, session->psa_alg,
					    pkt->in_buf, pkt->in_len,
					    pkt->out_buf, pkt->out_buf_max,
					    (size_t *) &pkt->out_len);
	} else {
		status = psa_cipher_decrypt(session->key_id, session->psa_alg,
					    pkt->in_buf, pkt->in_len,
					    pkt->out_buf, pkt->out_buf_max,
					    (size_t *) &pkt->out_len);
	}

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_[en|de]crypt() failed (%d)", status);
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_PSA_WANT_ALG_ECB_NO_PADDING */

#if CONFIG_PSA_WANT_ALG_CBC_NO_PADDING
int mbedtls_cbc(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct mbedtls_shim_session *session = ctx->drv_sessn_state;
	psa_cipher_operation_t psa_op = PSA_CIPHER_OPERATION_INIT;
	int iv_bytes = 0;
	uint8_t *in_buf_ptr = pkt->in_buf;
	size_t in_buf_size = pkt->in_len;
	uint8_t *out_buf_ptr = pkt->out_buf;
	size_t out_buf_size = pkt->out_buf_max;
	size_t out_len;
	psa_status_t status;

	if (session->cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) {
		status = psa_cipher_encrypt_setup(&psa_op, session->key_id, PSA_ALG_CBC_NO_PADDING);
	} else {
		status = psa_cipher_decrypt_setup(&psa_op, session->key_id, PSA_ALG_CBC_NO_PADDING);
	}
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_[en|de]crypt_setup() failed (%d)", status);
		return -EINVAL;
	}

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		iv_bytes = 16;
		if (session->cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) {
			/* Prefix IV to ciphertext, which is default behavior of Zephyr
			 * crypto API, unless CAP_NO_IV_PREFIX is requested.
			 */
			memcpy(pkt->out_buf, iv, iv_bytes);
			out_buf_ptr += iv_bytes;
			out_buf_size -= iv_bytes;
		} else {
			in_buf_ptr += iv_bytes;
			in_buf_size -= iv_bytes;
		}
	}

	status = psa_cipher_set_iv(&psa_op, iv, 16);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_set_iv() failed (%d)", status);
		return -EINVAL;
	}

	status = psa_cipher_update(&psa_op, in_buf_ptr, in_buf_size,
				   out_buf_ptr, out_buf_size, &out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_update() failed (%d)", status);
		return -EINVAL;
	}
	out_buf_ptr += out_len;
	out_buf_size -= out_len;
	pkt->out_len = out_len;

	status = psa_cipher_finish(&psa_op, out_buf_ptr, out_buf_size, &out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_finish() failed (%d)", status);
		return -EINVAL;
	}

	pkt->out_len += out_len;

	return 0;
}
#endif /* CONFIG_PSA_WANT_ALG_CBC_NO_PADDING */

#if CONFIG_PSA_WANT_ALG_CCM || CONFIG_PSA_WANT_ALG_GCM
static int mbedtls_aead(struct cipher_ctx *ctx, struct cipher_aead_pkt *apkt, uint8_t *nonce)
{
	struct mbedtls_shim_session *session = ctx->drv_sessn_state;
	psa_aead_operation_t psa_op = PSA_AEAD_OPERATION_INIT;
	psa_algorithm_t psa_alg;
	psa_status_t status;
	uint8_t *out_buf_ptr = apkt->pkt->out_buf;
	size_t out_buf_size = apkt->pkt->out_buf_max;
	size_t out_len, tag_size, tag_len, nonce_len;

	if (session->psa_alg == PSA_ALG_GCM) {
		tag_size = ctx->mode_params.gcm_info.tag_len;
		nonce_len = ctx->mode_params.gcm_info.nonce_len;
		psa_alg = PSA_ALG_GCM;
	} else {
		tag_size = ctx->mode_params.ccm_info.tag_len;
		nonce_len = ctx->mode_params.ccm_info.nonce_len;
		psa_alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_size);
	}

	if (session->cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) {
		status = psa_aead_encrypt_setup(&psa_op, session->key_id, psa_alg);
	} else {
		status = psa_aead_decrypt_setup(&psa_op, session->key_id, psa_alg);
	}
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_aead_[en|de]crypt_setup() failed (%d)", status);
		return -EIO;
	}

	status = psa_aead_set_nonce(&psa_op, nonce, nonce_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_aead_set_nonce() failed (%d)", status);
		return -EIO;
	}

	status = psa_aead_set_lengths(&psa_op, apkt->ad_len, apkt->pkt->in_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_aead_set_lengths() failed (%d)", status);
		return -EIO;
	}

	status = psa_aead_update_ad(&psa_op, apkt->ad, apkt->ad_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_aead_update_ad() failed (%d)", status);
		return -EIO;
	}

	apkt->pkt->out_len = 0;

	status = psa_aead_update(&psa_op, apkt->pkt->in_buf, apkt->pkt->in_len,
				 out_buf_ptr, out_buf_size, &out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_aead_update() failed (%d)", status);
		return -EIO;
	}

	out_buf_ptr += out_len;
	out_buf_size -= out_len;
	apkt->pkt->out_len += out_len;

	if (session->cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) {
		status = psa_aead_finish(&psa_op, out_buf_ptr, out_buf_size, &out_len,
					 apkt->tag, tag_size, &tag_len);
		apkt->pkt->out_len += out_len + tag_len;
	} else {
		status = psa_aead_verify(&psa_op, out_buf_ptr, out_buf_size, &out_len,
					 apkt->tag, tag_size);
		apkt->pkt->out_len += out_len;
	}
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_aead_[finish|verify]() failed (%d)", status);
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_PSA_WANT_ALG_CCM || CONFIG_PSA_WANT_ALG_GCM */
#endif /* CONFIG_PSA_WANT_KEY_TYPE_AES */

static int mbedtls_cipher_session_setup(const struct device *dev,
					struct cipher_ctx *ctx,
					enum cipher_algo algo, enum cipher_mode mode,
					enum cipher_op op_type)
{
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	struct mbedtls_shim_session *session;

	if (ctx->flags & ~(MBEDTLS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	if (ctx->keylen != 16U) {
		LOG_ERR("%u key size is not supported", ctx->keylen);
		return -EINVAL;
	}

	session = mbedtls_get_unused_session();
	if (session == NULL) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	switch (mode) {
#if CONFIG_PSA_WANT_KEY_TYPE_AES
#if CONFIG_PSA_WANT_ALG_ECB_NO_PADDING
	case CRYPTO_CIPHER_MODE_ECB:
		session->psa_alg = PSA_ALG_ECB_NO_PADDING;
		ctx->ops.block_crypt_hndlr = mbedtls_ecb;
		break;
#endif /* CONFIG_PSA_WANT_ALG_ECB_NO_PADDING */
#if CONFIG_PSA_WANT_ALG_CBC_NO_PADDING
	case CRYPTO_CIPHER_MODE_CBC:
		session->psa_alg = PSA_ALG_CBC_NO_PADDING;
		ctx->ops.cbc_crypt_hndlr = mbedtls_cbc;
		break;
#endif /* CONFIG_PSA_WANT_ALG_CBC_NO_PADDING */
#if CONFIG_PSA_WANT_ALG_CCM
	case CRYPTO_CIPHER_MODE_CCM: {
		uint16_t tag_len = ctx->mode_params.ccm_info.tag_len;

		if (tag_len > PSA_AEAD_TAG_MAX_SIZE) {
			return -EINVAL;
		}
		session->psa_alg = PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(PSA_ALG_CCM, tag_len);
		ctx->ops.ccm_crypt_hndlr = mbedtls_aead;
		break;
	}
#endif /* CONFIG_PSA_WANT_ALG_CCM */
#if CONFIG_PSA_WANT_ALG_GCM
	case CRYPTO_CIPHER_MODE_GCM:
		session->psa_alg = PSA_ALG_GCM;
		ctx->ops.gcm_crypt_hndlr = mbedtls_aead;
		break;
#endif /* CONFIG_PSA_WANT_ALG_GCM */
#endif /* CONFIG_PSA_WANT_KEY_TYPE_AES*/
	default:
		LOG_ERR("Unsupported mode");
		mbedtls_free_session(session);
		return -ENOTSUP;
	}

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_algorithm(&key_attr, session->psa_alg);
	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT);
	} else {
		psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_DECRYPT);
	}

	status = psa_import_key(&key_attr, ctx->key.bit_stream, ctx->keylen, &session->key_id);
	psa_reset_key_attributes(&key_attr);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key() failed (%d)", status);
		mbedtls_free_session(session);
		return -EIO;
	}

	session->cipher_op = op_type;
	ctx->drv_sessn_state = session;

	return 0;
}

static int mbedtls_cipher_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	struct mbedtls_shim_session *session = ctx->drv_sessn_state;

	psa_destroy_key(session->key_id);
	mbedtls_free_session(session);

	return 0;
}

static int mbedtls_hash_compute(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct mbedtls_shim_session *session = ctx->drv_sessn_state;
	size_t hash_out_len;
	psa_status_t status;

	if (!ctx->started) {
		status = psa_hash_setup(&session->hash_op, session->psa_alg);
		if (status != PSA_SUCCESS) {
			LOG_ERR("PSA hash operation setup failed");
			return -EIO;
		}
		ctx->started = true;
	}

	status = psa_hash_update(&session->hash_op, pkt->in_buf, pkt->in_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Could not update the hash");
		ctx->started = false;
		return -EINVAL;
	}

	if (finish) {
		ctx->started = false;
		/* "strct hash_pkt" has no information about the size of the
		 * output buffer, so we assume that it's at least large enough
		 * to contain the hashing result, but this is not safe.
		 */
		status = psa_hash_finish(&session->hash_op, pkt->out_buf,
					 PSA_HASH_LENGTH(session->psa_alg), &hash_out_len);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Could not compute the hash");
			return -EINVAL;
		}
	}

	return 0;
}

static int mbedtls_hash_session_setup(const struct device *dev,
				   struct hash_ctx *ctx,
				   enum hash_algo algo)
{
	struct mbedtls_shim_session *session;

	if (ctx->flags & ~(MBEDTLS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	session = mbedtls_get_unused_session();
	if (session == NULL) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	session->hash_op = psa_hash_operation_init();
	switch (algo) {
#if CONFIG_PSA_WANT_ALG_SHA_224
	case CRYPTO_HASH_ALGO_SHA224:
		session->psa_alg = PSA_ALG_SHA_224;
		break;
#endif
#if CONFIG_PSA_WANT_ALG_SHA_256
	case CRYPTO_HASH_ALGO_SHA256:
		session->psa_alg = PSA_ALG_SHA_256;
		break;
#endif
#if CONFIG_PSA_WANT_ALG_SHA_384
	case CRYPTO_HASH_ALGO_SHA384:
		session->psa_alg = PSA_ALG_SHA_384;
		break;
#endif
#if CONFIG_PSA_WANT_ALG_SHA_512
	case CRYPTO_HASH_ALGO_SHA512:
		session->psa_alg = PSA_ALG_SHA_512;
		break;
#endif
	default:
		LOG_ERR("Unsupported algo: %d", algo);
		mbedtls_free_session(session);
		return -EINVAL;
	}

	ctx->hash_hndlr = mbedtls_hash_compute;
	ctx->drv_sessn_state = session;
	ctx->started = false;

	return 0;
}

static int mbedtls_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct mbedtls_shim_session *session = ctx->drv_sessn_state;

	if (psa_hash_abort(&session->hash_op) != PSA_SUCCESS) {
		LOG_ERR("PSA hash abort failed");
		return -EIO;
	}

	mbedtls_free_session(session);

	return 0;
}

static int mbedtls_query_caps(const struct device *dev)
{
	return MBEDTLS_SUPPORT;
}

static DEVICE_API(crypto, mbedtls_crypto_funcs) = {
	.cipher_begin_session = mbedtls_cipher_session_setup,
	.cipher_free_session = mbedtls_cipher_session_free,
	.cipher_async_callback_set = NULL,
	.hash_begin_session = mbedtls_hash_session_setup,
	.hash_free_session = mbedtls_hash_session_free,
	.query_hw_caps = mbedtls_query_caps,
};

DEVICE_DEFINE(crypto_mbedtls, CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME,
	      NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
	      (void *)&mbedtls_crypto_funcs);
