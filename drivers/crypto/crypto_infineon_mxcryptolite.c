/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_mxcryptolite_crypto

#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/infineon_mxcryptolite.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_infineon_mxcryptolite, CONFIG_CRYPTO_LOG_LEVEL);

#include "cy_cryptolite_aes.h"
#include "cy_cryptolite_aes_ccm.h"
#include "cy_cryptolite_sha256.h"

#define MXCRYPTOLITE_CAPS (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

/* AES block size is 16 bytes */
#define MXCRYPTOLITE_AES_BLOCK_SIZE 16U

/* SHA-256 digest size is 32 bytes */
#define MXCRYPTOLITE_SHA256_DIGEST_SIZE 32U

struct mxcryptolite_session {
	bool in_use;
	bool is_hash;

	/* Cipher fields */
	enum cipher_algo algo;
	enum cipher_mode mode;
	enum cipher_op dir;
	cy_stc_cryptolite_aes_state_t aes_state;
	cy_stc_cryptolite_aes_buffers_t aes_buffers;
	cy_stc_cryptolite_aes_ccm_state_t ccm_state;
	cy_stc_cryptolite_aes_ccm_buffers_t ccm_buffers;
	uint8_t key_sram[CY_CRYPTOLITE_AES_128_KEY_SIZE] __aligned(4);

	/* CTR mode state */
	uint32_t ctr_len;

	/* CCM mode parameters */
	uint16_t ccm_tag_len;
	uint16_t ccm_nonce_len;

	/* Hash fields */
	cy_stc_cryptolite_context_sha256_t sha256_ctx;
};

struct mxcryptolite_config {
	const struct device *mfd;
};

struct mxcryptolite_data {
	struct mxcryptolite_session sessions[CONFIG_CRYPTO_INFINEON_MXCRYPTOLITE_MAX_SESSION];
};

static struct mxcryptolite_session *mxcryptolite_session_alloc(const struct device *dev)
{
	const struct mxcryptolite_config *cfg = dev->config;
	struct mxcryptolite_data *data = dev->data;
	struct mxcryptolite_session *s = NULL;

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);
	for (int i = 0; i < CONFIG_CRYPTO_INFINEON_MXCRYPTOLITE_MAX_SESSION; i++) {
		if (!data->sessions[i].in_use) {
			data->sessions[i].in_use = true;
			s = &data->sessions[i];
			break;
		}
	}
	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	return s;
}

static void mxcryptolite_session_free(const struct device *dev,
				      struct mxcryptolite_session *session)
{
	const struct mxcryptolite_config *cfg = dev->config;

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);
	memset(session, 0, sizeof(*session));
	mfd_infineon_mxcryptolite_unlock(cfg->mfd);
}

/* Cipher handlers — ECB */

static int mxcryptolite_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct mxcryptolite_config *cfg = ctx->device->config;
	struct mxcryptolite_session *s = ctx->drv_sessn_state;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	cy_en_cryptolite_status_t rc;

	if ((pkt->in_len % MXCRYPTOLITE_AES_BLOCK_SIZE) != 0U) {
		return -EINVAL;
	}

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Cryptolite_Aes_Init(base, s->key_sram, &s->aes_state, &s->aes_buffers);
	if (rc != CY_CRYPTOLITE_SUCCESS) {
		mfd_infineon_mxcryptolite_unlock(cfg->mfd);
		LOG_ERR("AES init failed: %d", (int)rc);
		return -EIO;
	}

	for (uint32_t off = 0U; off < pkt->in_len; off += MXCRYPTOLITE_AES_BLOCK_SIZE) {
		rc = Cy_Cryptolite_Aes_Ecb(base, pkt->out_buf + off, pkt->in_buf + off,
					   &s->aes_state);
		if (rc != CY_CRYPTOLITE_SUCCESS) {
			Cy_Cryptolite_Aes_Free(base, &s->aes_state);
			mfd_infineon_mxcryptolite_unlock(cfg->mfd);
			LOG_ERR("AES ECB failed: %d", (int)rc);
			return -EIO;
		}
	}

	Cy_Cryptolite_Aes_Free(base, &s->aes_state);

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	pkt->out_len = pkt->in_len;

	return 0;
}

/* Cipher handlers — CBC */

static int mxcryptolite_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct mxcryptolite_config *cfg = ctx->device->config;
	struct mxcryptolite_session *s = ctx->drv_sessn_state;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	bool no_iv_prefix = (ctx->flags & CAP_NO_IV_PREFIX) != 0U;
	cy_en_cryptolite_status_t rc;
	uint8_t *src;
	uint8_t *dst;
	uint32_t len;

	src = pkt->in_buf;
	len = pkt->in_len;

	if ((len % MXCRYPTOLITE_AES_BLOCK_SIZE) != 0U) {
		return -EINVAL;
	}

	if (!no_iv_prefix) {
		memcpy(pkt->out_buf, iv, MXCRYPTOLITE_AES_BLOCK_SIZE);
		dst = pkt->out_buf + MXCRYPTOLITE_AES_BLOCK_SIZE;
	} else {
		dst = pkt->out_buf;
	}

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Cryptolite_Aes_Init(base, s->key_sram, &s->aes_state, &s->aes_buffers);
	if (rc != CY_CRYPTOLITE_SUCCESS) {
		mfd_infineon_mxcryptolite_unlock(cfg->mfd);
		LOG_ERR("AES init failed: %d", (int)rc);
		return -EIO;
	}

	rc = Cy_Cryptolite_Aes_Cbc(base, len, iv, dst, src, &s->aes_state);

	Cy_Cryptolite_Aes_Free(base, &s->aes_state);

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	if (rc != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("AES CBC failed: %d", (int)rc);
		return -EIO;
	}

	pkt->out_len = no_iv_prefix ? len : len + MXCRYPTOLITE_AES_BLOCK_SIZE;

	return 0;
}

/* Cipher handlers — CTR */

static int mxcryptolite_aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *nonce)
{
	const struct mxcryptolite_config *cfg = ctx->device->config;
	struct mxcryptolite_session *s = ctx->drv_sessn_state;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	cy_en_cryptolite_status_t rc;
	uint32_t ctr_len_bytes = s->ctr_len / BITS_PER_BYTE;
	uint8_t counter_blk[MXCRYPTOLITE_AES_BLOCK_SIZE];

	if (ctr_len_bytes == 0U || ctr_len_bytes > MXCRYPTOLITE_AES_BLOCK_SIZE) {
		return -EINVAL;
	}

	/* Build 16-byte counter block: nonce || zero-initialised counter */
	memset(counter_blk, 0, sizeof(counter_blk));
	memcpy(counter_blk, nonce, MXCRYPTOLITE_AES_BLOCK_SIZE - ctr_len_bytes);

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Cryptolite_Aes_Init(base, s->key_sram, &s->aes_state, &s->aes_buffers);
	if (rc != CY_CRYPTOLITE_SUCCESS) {
		mfd_infineon_mxcryptolite_unlock(cfg->mfd);
		LOG_ERR("AES init failed: %d", (int)rc);
		return -EIO;
	}

	/*
	 * Use the multistage CTR API rather than Cy_Cryptolite_Aes_Ctr(): the
	 * one-shot variant only processes whole 16-byte blocks and reports the
	 * trailing byte count via its srcOffset argument, leaving any final
	 * partial block untouched (clear/unencrypted) in the output buffer.
	 * The Update path XORs the trailing bytes against the final keystream
	 * block, so non-block-aligned lengths are handled correctly.
	 */
	rc = Cy_Cryptolite_Aes_Ctr_Setup(base, &s->aes_state);
	if (rc == CY_CRYPTOLITE_SUCCESS) {
		rc = Cy_Cryptolite_Aes_Ctr_Set_IV(base, counter_blk, &s->aes_state);
	}
	if (rc == CY_CRYPTOLITE_SUCCESS) {
		rc = Cy_Cryptolite_Aes_Ctr_Update(base, (uint32_t)pkt->in_len, pkt->out_buf,
						  pkt->in_buf, &s->aes_state);
	}
	if (rc == CY_CRYPTOLITE_SUCCESS) {
		rc = Cy_Cryptolite_Aes_Ctr_Finish(base, &s->aes_state);
	}

	Cy_Cryptolite_Aes_Free(base, &s->aes_state);

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	if (rc != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("AES CTR failed: %d", (int)rc);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	return 0;
}

/* Cipher handlers — CCM */

static int mxcryptolite_aes_ccm_op(struct cipher_ctx *ctx, struct cipher_aead_pkt *aead_pkt,
				   uint8_t *nonce)
{
	const struct mxcryptolite_config *cfg = ctx->device->config;
	struct mxcryptolite_session *s = ctx->drv_sessn_state;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	cy_en_cryptolite_status_t rc;
	struct cipher_pkt *pkt = aead_pkt->pkt;

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Cryptolite_Aes_Ccm_Init(base, &s->ccm_buffers, &s->ccm_state);
	if (rc != CY_CRYPTOLITE_SUCCESS) {
		mfd_infineon_mxcryptolite_unlock(cfg->mfd);
		LOG_ERR("CCM init failed: %d", (int)rc);
		return -EIO;
	}

	rc = Cy_Cryptolite_Aes_Ccm_SetKey(base, s->key_sram, &s->ccm_state);
	if (rc != CY_CRYPTOLITE_SUCCESS) {
		Cy_Cryptolite_Aes_Ccm_Free(base, &s->ccm_state);
		mfd_infineon_mxcryptolite_unlock(cfg->mfd);
		LOG_ERR("CCM set key failed: %d", (int)rc);
		return -EIO;
	}

	if (s->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
		rc = Cy_Cryptolite_Aes_Ccm_Encrypt_Tag(
			base, (uint32_t)s->ccm_nonce_len, nonce, aead_pkt->ad_len, aead_pkt->ad,
			(uint32_t)pkt->in_len, pkt->out_buf, pkt->in_buf, (uint32_t)s->ccm_tag_len,
			aead_pkt->tag, &s->ccm_state);
	} else {
		cy_en_cryptolite_ccm_auth_result_t auth_result;

		rc = Cy_Cryptolite_Aes_Ccm_Decrypt(
			base, (uint32_t)s->ccm_nonce_len, nonce, aead_pkt->ad_len, aead_pkt->ad,
			(uint32_t)pkt->in_len, pkt->out_buf, pkt->in_buf, (uint32_t)s->ccm_tag_len,
			aead_pkt->tag, &auth_result, &s->ccm_state);

		if (rc == CY_CRYPTOLITE_SUCCESS && auth_result != CY_CRYPTOLITE_TAG_VALID) {
			Cy_Cryptolite_Aes_Ccm_Free(base, &s->ccm_state);
			mfd_infineon_mxcryptolite_unlock(cfg->mfd);
			memset(pkt->out_buf, 0, pkt->in_len);
			LOG_ERR("CCM tag verification failed");
			return -EBADMSG;
		}
	}

	Cy_Cryptolite_Aes_Ccm_Free(base, &s->ccm_state);

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	if (rc != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("AES CCM failed: %d", (int)rc);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	return 0;
}

/* Session management */

static int mxcryptolite_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
					     enum cipher_algo algo, enum cipher_mode mode,
					     enum cipher_op op_type)
{
	struct mxcryptolite_session *s;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported cipher algorithm: %d", (int)algo);
		return -ENOTSUP;
	}

	if (mode != CRYPTO_CIPHER_MODE_ECB && mode != CRYPTO_CIPHER_MODE_CBC &&
	    mode != CRYPTO_CIPHER_MODE_CTR && mode != CRYPTO_CIPHER_MODE_CCM) {
		LOG_ERR("Unsupported cipher mode: %d", (int)mode);
		return -ENOTSUP;
	}

	/* CRYPTOLITE only has the forward AES cipher; ECB/CBC decrypt
	 * requires the inverse cipher which is not available.
	 */
	if (op_type == CRYPTO_CIPHER_OP_DECRYPT &&
	    (mode == CRYPTO_CIPHER_MODE_ECB || mode == CRYPTO_CIPHER_MODE_CBC)) {
		LOG_WRN("CRYPTOLITE does not support ECB/CBC decrypt");
		return -ENOTSUP;
	}

	if ((ctx->flags & ~MXCRYPTOLITE_CAPS) != 0U) {
		LOG_ERR("Unsupported flag combination: 0x%x", ctx->flags);
		return -ENOTSUP;
	}

	/* CRYPTOLITE only supports AES-128 */
	if (ctx->keylen != CY_CRYPTOLITE_AES_128_KEY_SIZE) {
		LOG_ERR("CRYPTOLITE only supports AES-128 (16-byte key), got: %u", ctx->keylen);
		return -EINVAL;
	}

	s = mxcryptolite_session_alloc(dev);
	if (s == NULL) {
		return -ENOSPC;
	}

	s->is_hash = false;
	s->algo = algo;
	s->mode = mode;
	s->dir = op_type;

	memcpy(s->key_sram, ctx->key.bit_stream, CY_CRYPTOLITE_AES_128_KEY_SIZE);

	ctx->drv_sessn_state = s;
	ctx->device = dev;

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		ctx->ops.block_crypt_hndlr = mxcryptolite_aes_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		ctx->ops.cbc_crypt_hndlr = mxcryptolite_aes_cbc_op;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		s->ctr_len = ctx->mode_params.ctr_info.ctr_len;
		ctx->ops.ctr_crypt_hndlr = mxcryptolite_aes_ctr_op;
		break;
	case CRYPTO_CIPHER_MODE_CCM: {
		uint16_t tag_len = ctx->mode_params.ccm_info.tag_len;
		uint16_t nonce_len = ctx->mode_params.ccm_info.nonce_len;

		/* NIST SP 800-38C: nonce length 7..13 */
		if (nonce_len < 7U || nonce_len > 13U) {
			LOG_ERR("CCM nonce_len %u out of range [7,13]", nonce_len);
			mxcryptolite_session_free(dev, s);
			return -EINVAL;
		}

		/* NIST SP 800-38C: tag length must be even, 4..16 */
		if (tag_len < 4U || tag_len > 16U || (tag_len & 1U) != 0U) {
			LOG_ERR("CCM tag_len %u invalid (must be even, 4..16)", tag_len);
			mxcryptolite_session_free(dev, s);
			return -EINVAL;
		}

		s->ccm_tag_len = tag_len;
		s->ccm_nonce_len = nonce_len;
		ctx->ops.ccm_crypt_hndlr = mxcryptolite_aes_ccm_op;
		break;
	}
	default:
		mxcryptolite_session_free(dev, s);
		return -ENOTSUP;
	}

	return 0;
}

static int mxcryptolite_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	mxcryptolite_session_free(dev, ctx->drv_sessn_state);
	return 0;
}

/* Hash handlers */

static int mxcryptolite_hash_op(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct mxcryptolite_config *cfg = ctx->device->config;
	struct mxcryptolite_session *s = ctx->drv_sessn_state;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	cy_en_cryptolite_status_t rc;

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Cryptolite_Sha256_Update(base, pkt->in_buf, pkt->in_len, &s->sha256_ctx);
	if (rc != CY_CRYPTOLITE_SUCCESS) {
		mfd_infineon_mxcryptolite_unlock(cfg->mfd);
		LOG_ERR("SHA-256 update failed: %d", (int)rc);
		return -EIO;
	}

	if (finish) {
		rc = Cy_Cryptolite_Sha256_Finish(base, pkt->out_buf, &s->sha256_ctx);
		(void)Cy_Cryptolite_Sha256_Free(base, &s->sha256_ctx);

		/* Re-init context so the session can be reused for another
		 * hash_compute() call without requiring a new session.
		 */
		if (rc == CY_CRYPTOLITE_SUCCESS) {
			rc = Cy_Cryptolite_Sha256_Init(base, &s->sha256_ctx);
			if (rc == CY_CRYPTOLITE_SUCCESS) {
				rc = Cy_Cryptolite_Sha256_Start(base, &s->sha256_ctx);
			}
		}
	}

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	if (rc != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("SHA-256 failed: %d", (int)rc);
		return -EIO;
	}

	return 0;
}

static int mxcryptolite_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
					   enum hash_algo algo)
{
	struct mxcryptolite_session *s;

	/* CRYPTOLITE only supports SHA-256 */
	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("CRYPTOLITE only supports SHA-256, got: %d", (int)algo);
		return -ENOTSUP;
	}

	if ((ctx->flags & ~MXCRYPTOLITE_CAPS) != 0U) {
		LOG_ERR("Unsupported flag combination: 0x%x", ctx->flags);
		return -ENOTSUP;
	}

	s = mxcryptolite_session_alloc(dev);
	if (s == NULL) {
		return -ENOSPC;
	}

	s->is_hash = true;

	/* Initialize and start the SHA-256 context at session creation
	 * so that hash_op can stream Update calls.
	 */
	const struct mxcryptolite_config *cfg = dev->config;
	CRYPTOLITE_Type *base = mfd_infineon_mxcryptolite_get_base(cfg->mfd);
	cy_en_cryptolite_status_t rc;

	mfd_infineon_mxcryptolite_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Cryptolite_Sha256_Init(base, &s->sha256_ctx);
	if (rc == CY_CRYPTOLITE_SUCCESS) {
		rc = Cy_Cryptolite_Sha256_Start(base, &s->sha256_ctx);
	}

	mfd_infineon_mxcryptolite_unlock(cfg->mfd);

	if (rc != CY_CRYPTOLITE_SUCCESS) {
		LOG_ERR("SHA-256 session init failed: %d", (int)rc);
		mxcryptolite_session_free(dev, s);
		return -EIO;
	}

	ctx->drv_sessn_state = s;
	ctx->device = dev;
	ctx->hash_hndlr = mxcryptolite_hash_op;

	return 0;
}

static int mxcryptolite_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	mxcryptolite_session_free(dev, ctx->drv_sessn_state);
	return 0;
}

/* Driver capabilities and init */

static int mxcryptolite_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return MXCRYPTOLITE_CAPS;
}

static int mxcryptolite_init(const struct device *dev)
{
	const struct mxcryptolite_config *cfg = dev->config;

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("MXCRYPTOLITE MFD device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(crypto, mxcryptolite_driver_api) = {
	.cipher_begin_session = mxcryptolite_cipher_begin_session,
	.cipher_free_session = mxcryptolite_cipher_free_session,
	.cipher_async_callback_set = NULL,
	.hash_begin_session = mxcryptolite_hash_begin_session,
	.hash_free_session = mxcryptolite_hash_free_session,
	.hash_async_callback_set = NULL,
	.query_hw_caps = mxcryptolite_query_hw_caps,
};

#define MXCRYPTOLITE_DEFINE(n)                                                                     \
	static struct mxcryptolite_data mxcryptolite_data_##n;                                     \
	static const struct mxcryptolite_config mxcryptolite_config_##n = {                        \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mxcryptolite_init, NULL, &mxcryptolite_data_##n,                  \
			      &mxcryptolite_config_##n, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,  \
			      &mxcryptolite_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MXCRYPTOLITE_DEFINE)
