/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_mxcrypto_crypto

#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/infineon_mxcrypto.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_infineon_mxcrypto, CONFIG_CRYPTO_LOG_LEVEL);

#include "cy_crypto_core_hw.h"
#include "cy_crypto_core_aes.h"
#include "cy_crypto_core_sha.h"

#define MXCRYPTO_CAPS (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | \
		       CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

/* AES block size is 16 bytes */
#define MXCRYPTO_AES_BLOCK_SIZE 16U

/* Maximum SHA digest is 64 bytes (SHA-512) */
#define MXCRYPTO_SHA_MAX_DIGEST_SIZE 64U

/*
 * Maximum AES key size in bytes (AES-256).
 * Used for the SRAM key staging buffer.
 */
#define MXCRYPTO_AES_MAX_KEY_SIZE 32U

/* Maximum AEAD tag size (GCM) and full 16-byte counter block for CTR. */
#define MXCRYPTO_AES_TAG_MAX_SIZE 16U

/*
 * Caller buffer DMA-safety contract.
 *
 * The PDL issues SCB_InvalidateDCache_by_Addr() on caller-supplied
 * buffers.  When the D-Cache is enabled, that invalidation only
 * affects whole cache lines, so the buffer must:
 *   1. start at a cache-line aligned address, and
 *   2. occupy its trailing cache line exclusively, i.e. the caller's
 *      underlying allocation must be padded up to a multiple of the
 *      cache-line size.
 *
 * Otherwise, invalidation spills into adjacent cache lines and
 * corrupts unrelated data.
 *
 * Rather than silently bounce caller data through driver-owned SRAM
 * buffers (which adds a memcpy cost to every operation), the driver
 * checks the start-address alignment and returns -EINVAL if it is
 * not satisfied.  The trailing-padding requirement cannot be checked
 * at runtime and is the caller's responsibility.
 *
 * Small driver-internal scratch (key, IV, tag, CTR counter/stream,
 * SHA initial-hash staging) lives in the session struct, which is
 * placed in the __nocache section, so PDL cache maintenance on those
 * pointers is a harmless no-op.
 */
#ifdef CONFIG_DCACHE
#define MXCRYPTO_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define MXCRYPTO_ALIGN 1U
#endif

static inline bool mxcrypto_buf_align_safe(const void *buf, size_t len)
{
	if (len == 0U) {
		return true;
	}
	if (buf == NULL) {
		return false;
	}
	if (((uintptr_t)buf % MXCRYPTO_ALIGN) != 0U) {
		return false;
	}
	return true;
}

struct mxcrypto_session {
	bool in_use;
	bool is_hash;

	/* Cipher fields */
	enum cipher_algo algo;
	enum cipher_mode mode;
	enum cipher_op dir;
	cy_en_crypto_aes_key_length_t key_len;

	/*
	 * The PDL issues cache-maintenance calls on the buffers it
	 * receives.  All session storage below is placed in the nocache
	 * section (mxcrypto_data __nocache) so those calls on
	 * driver-internal scratch are harmless.
	 *
	 * Caller-supplied data buffers (in_buf, out_buf, ad) are passed
	 * directly to the PDL; the caller is responsible for honoring
	 * the alignment/size contract checked by mxcrypto_buf_align_safe().
	 */
	cy_stc_crypto_aes_state_t aes_state;
	cy_stc_crypto_aes_buffers_t aes_buffers;
	cy_stc_crypto_aes_ccm_state_t ccm_state;
	cy_stc_crypto_aes_ccm_buffers_t ccm_buffers;
	uint8_t key_sram[MXCRYPTO_AES_MAX_KEY_SIZE];
	/*
	 * Small driver-owned scratch.  Used either to stage tiny
	 * caller-owned items (IV, tag) where requiring cache-line
	 * alignment would force the caller to over-allocate, or to hold
	 * driver-built data that the caller does not own (CTR full
	 * counter block, key-stream block).
	 */
	uint8_t aes_iv_sram[MXCRYPTO_AES_BLOCK_SIZE];
	uint8_t aes_tag_sram[MXCRYPTO_AES_TAG_MAX_SIZE];
	uint8_t aes_stream_sram[MXCRYPTO_AES_BLOCK_SIZE];
	uint16_t key_len_bytes;
	uint16_t nonce_len;
	uint16_t tag_len;

	/* Hash fields */
	cy_en_crypto_sha_mode_t sha_mode;
	cy_stc_crypto_sha_state_t sha_state;
	cy_stc_crypto_v2_sha_buffers_t sha_buffers;
#ifdef CONFIG_DCACHE
	/*
	 * Cy_Crypto_Core_Sha_Init points sha_state.initialHash at a
	 * static const table in flash.  The PDL DMA path needs to read
	 * it via a cache-line-aligned SRAM address, so the driver
	 * stages it through this nocache scratch.
	 */
	uint8_t sha_init_hash_sram[MXCRYPTO_SHA_MAX_DIGEST_SIZE];
#endif /* CONFIG_DCACHE */
	uint8_t sha_digest_size;
};

struct mxcrypto_config {
	const struct device *mfd;
};

struct mxcrypto_data {
	struct mxcrypto_session sessions[CONFIG_CRYPTO_INFINEON_MXCRYPTO_MAX_SESSION];
};

static struct mxcrypto_session *mxcrypto_session_alloc(const struct device *dev)
{
	const struct mxcrypto_config *cfg = dev->config;
	struct mxcrypto_data *data = dev->data;
	struct mxcrypto_session *session = NULL;

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);
	for (int i = 0; i < CONFIG_CRYPTO_INFINEON_MXCRYPTO_MAX_SESSION; i++) {
		if (!data->sessions[i].in_use) {
			data->sessions[i].in_use = true;
			session = &data->sessions[i];
			break;
		}
	}
	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	return session;
}

static void mxcrypto_session_free(const struct device *dev,
				  struct mxcrypto_session *session)
{
	const struct mxcrypto_config *cfg = dev->config;

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);
	memset(session, 0, sizeof(*session));
	mfd_infineon_mxcrypto_unlock(cfg->mfd);
}

/* Cipher handlers — ECB */

static int mxcrypto_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct mxcrypto_config *cfg = ctx->device->config;
	struct mxcrypto_session *session = ctx->drv_sessn_state;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	cy_en_crypto_dir_mode_t dir;
	cy_en_crypto_status_t rc;

	if ((pkt->in_len % MXCRYPTO_AES_BLOCK_SIZE) != 0U) {
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("AES ECB: out_buf_max %d < required %d",
			pkt->out_buf_max, pkt->in_len);
		return -EINVAL;
	}

	if (!mxcrypto_buf_align_safe(pkt->in_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(pkt->out_buf, pkt->in_len)) {
		LOG_ERR("AES ECB: caller buffers must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}

	dir = (session->dir == CRYPTO_CIPHER_OP_ENCRYPT) ? CY_CRYPTO_ENCRYPT : CY_CRYPTO_DECRYPT;

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Crypto_Core_Aes_Init(base, session->key_sram,
				     session->key_len, &session->aes_state);
	if (rc != CY_CRYPTO_SUCCESS) {
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		LOG_ERR("AES init failed: %d", (int)rc);
		return -EIO;
	}

	for (uint32_t off = 0U; off < pkt->in_len; off += MXCRYPTO_AES_BLOCK_SIZE) {
		rc = Cy_Crypto_Core_Aes_Ecb(base, dir,
					    pkt->out_buf + off,
					    pkt->in_buf + off,
					    &session->aes_state);
		if (rc != CY_CRYPTO_SUCCESS) {
			break;
		}
	}

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES ECB failed: %d", (int)rc);
		(void)Cy_Crypto_Core_Aes_Free(base, &session->aes_state);
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		return -EIO;
	}

	rc = Cy_Crypto_Core_Aes_Free(base, &session->aes_state);
	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES Free failed: %d", (int)rc);
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		return -EIO;
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	pkt->out_len = pkt->in_len;

	return 0;
}

/* Cipher handlers — CBC */

static int mxcrypto_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			       uint8_t *iv)
{
	const struct mxcrypto_config *cfg = ctx->device->config;
	struct mxcrypto_session *session = ctx->drv_sessn_state;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	bool no_iv_prefix = (ctx->flags & CAP_NO_IV_PREFIX) != 0U;
	cy_en_crypto_dir_mode_t dir;
	cy_en_crypto_status_t rc;
	uint8_t *src;
	uint8_t *dst;
	uint32_t len;
	uint32_t needed;

	dir = (session->dir == CRYPTO_CIPHER_OP_ENCRYPT) ? CY_CRYPTO_ENCRYPT : CY_CRYPTO_DECRYPT;

	if (no_iv_prefix || session->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
		/*
		 * Encrypt: in_buf = plaintext, in_len = plaintext length.
		 * If IV prefix: prepend IV to out_buf, write ciphertext after.
		 * If no IV prefix: write ciphertext directly to out_buf.
		 */
		src = pkt->in_buf;
		len = pkt->in_len;
		if (!no_iv_prefix) {
			dst = pkt->out_buf + MXCRYPTO_AES_BLOCK_SIZE;
		} else {
			dst = pkt->out_buf;
		}
	} else {
		/*
		 * Decrypt with IV prefix: in_buf = IV || ciphertext.
		 * Skip the IV prefix, decrypt the remaining ciphertext.
		 * Guard against underflow when in_len < IV size.
		 */
		if (pkt->in_len < MXCRYPTO_AES_BLOCK_SIZE) {
			return -EINVAL;
		}
		src = pkt->in_buf + MXCRYPTO_AES_BLOCK_SIZE;
		len = pkt->in_len - MXCRYPTO_AES_BLOCK_SIZE;
		dst = pkt->out_buf;
	}

	if ((len % MXCRYPTO_AES_BLOCK_SIZE) != 0U) {
		return -EINVAL;
	}

	needed = (session->dir == CRYPTO_CIPHER_OP_ENCRYPT && !no_iv_prefix)
			? (len + MXCRYPTO_AES_BLOCK_SIZE) : len;
	if ((uint32_t)pkt->out_buf_max < needed) {
		LOG_ERR("AES CBC: out_buf_max %d < required %u",
			pkt->out_buf_max, needed);
		return -EINVAL;
	}

	/*
	 * Validate the caller-visible buffer pointers, not the
	 * internally-shifted (src/dst) ones: the 16-byte IV prefix
	 * deliberately moves the working pointer off the cache-line
	 * boundary by one AES block.  The PDL's cache maintenance still
	 * operates correctly on the surrounding allocation as long as
	 * the underlying buffer (pkt->in_buf / pkt->out_buf) is itself
	 * cache-line aligned and padded.
	 */
	if (!mxcrypto_buf_align_safe(pkt->in_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(pkt->out_buf, pkt->out_buf_max)) {
		LOG_ERR("AES CBC: caller buffers must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);

	/*
	 * Stage the 16-byte IV through the session's nocache scratch so
	 * we don't impose a cache-line allocation requirement on the
	 * caller for a 16-byte value.
	 */
	memcpy(session->aes_iv_sram, iv, MXCRYPTO_AES_BLOCK_SIZE);

	rc = Cy_Crypto_Core_Aes_Init(base, session->key_sram,
				     session->key_len, &session->aes_state);
	if (rc != CY_CRYPTO_SUCCESS) {
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		LOG_ERR("AES init failed: %d", (int)rc);
		return -EIO;
	}

	rc = Cy_Crypto_Core_Aes_Cbc(base, dir, len, session->aes_iv_sram,
				    dst, src, &session->aes_state);

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES CBC failed: %d", (int)rc);
		(void)Cy_Crypto_Core_Aes_Free(base, &session->aes_state);
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		return -EIO;
	}

	rc = Cy_Crypto_Core_Aes_Free(base, &session->aes_state);
	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES Free failed: %d", (int)rc);
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		return -EIO;
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	if (session->dir == CRYPTO_CIPHER_OP_ENCRYPT && !no_iv_prefix) {
		memcpy(pkt->out_buf, iv, MXCRYPTO_AES_BLOCK_SIZE);
		pkt->out_len = len + MXCRYPTO_AES_BLOCK_SIZE;
	} else {
		pkt->out_len = len;
	}

	return 0;
}

/* Cipher handlers — CTR */

static int mxcrypto_aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			       uint8_t *iv)
{
	const struct mxcrypto_config *cfg = ctx->device->config;
	struct mxcrypto_session *session = ctx->drv_sessn_state;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	cy_en_crypto_status_t rc;
	uint32_t src_offset = 0U;
	uint16_t ctr_len_bytes = (uint16_t)(ctx->mode_params.ctr_info.ctr_len / 8U);
	uint16_t iv_len;

	if (ctr_len_bytes == 0U || ctr_len_bytes > MXCRYPTO_AES_BLOCK_SIZE) {
		return -EINVAL;
	}
	iv_len = MXCRYPTO_AES_BLOCK_SIZE - ctr_len_bytes;

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("AES CTR: out_buf_max %d < required %d",
			pkt->out_buf_max, pkt->in_len);
		return -EINVAL;
	}

	if (!mxcrypto_buf_align_safe(pkt->in_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(pkt->out_buf, pkt->in_len)) {
		LOG_ERR("AES CTR: caller buffers must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);

	/*
	 * Compose the 16-byte initial counter block (nonce || zero
	 * counter) and zero the key-stream block in driver-owned nocache
	 * scratch.  These are driver-built; they are not caller buffers.
	 */
	memcpy(session->aes_iv_sram, iv, iv_len);
	memset(session->aes_iv_sram + iv_len, 0, ctr_len_bytes);
	memset(session->aes_stream_sram, 0, MXCRYPTO_AES_BLOCK_SIZE);

	rc = Cy_Crypto_Core_Aes_Init(base, session->key_sram,
				     session->key_len, &session->aes_state);
	if (rc != CY_CRYPTO_SUCCESS) {
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		LOG_ERR("AES init failed: %d", (int)rc);
		return -EIO;
	}

	rc = Cy_Crypto_Core_Aes_Ctr(base, pkt->in_len, &src_offset,
				    session->aes_iv_sram, session->aes_stream_sram,
				    pkt->out_buf, pkt->in_buf, &session->aes_state);

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES CTR failed: %d", (int)rc);
		(void)Cy_Crypto_Core_Aes_Free(base, &session->aes_state);
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		return -EIO;
	}

	rc = Cy_Crypto_Core_Aes_Free(base, &session->aes_state);
	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES Free failed: %d", (int)rc);
		mfd_infineon_mxcrypto_unlock(cfg->mfd);
		return -EIO;
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	pkt->out_len = pkt->in_len;

	return 0;
}

/* Cipher handlers — CCM */

static int mxcrypto_aes_ccm_op(struct cipher_ctx *ctx,
			       struct cipher_aead_pkt *aead, uint8_t *nonce)
{
	const struct mxcrypto_config *cfg = ctx->device->config;
	struct mxcrypto_session *session = ctx->drv_sessn_state;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	struct cipher_pkt *pkt = aead->pkt;
	cy_en_crypto_status_t rc;
	int ret = 0;

	if (session->tag_len > MXCRYPTO_AES_TAG_MAX_SIZE) {
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("AES CCM: out_buf_max %d < required %d",
			pkt->out_buf_max, pkt->in_len);
		return -EINVAL;
	}

	if (!mxcrypto_buf_align_safe(pkt->in_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(pkt->out_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(aead->ad, aead->ad_len)) {
		LOG_ERR("AES CCM: caller buffers must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);

	/*
	 * The tag and nonce are small.  Stage them through driver-owned
	 * nocache scratch so we don't impose cache-line allocation
	 * requirements on the caller for short fixed-size items.
	 */
	memcpy(session->aes_iv_sram, nonce, session->nonce_len);
	if (session->dir == CRYPTO_CIPHER_OP_DECRYPT) {
		memcpy(session->aes_tag_sram, aead->tag, session->tag_len);
	}

	rc = Cy_Crypto_Core_Aes_Ccm_Init(base, &session->ccm_buffers, &session->ccm_state);
	if (rc == CY_CRYPTO_SUCCESS) {
		rc = Cy_Crypto_Core_Aes_Ccm_SetKey(base, session->key_sram,
						   session->key_len, &session->ccm_state);
	}
	if (rc == CY_CRYPTO_SUCCESS) {
		if (session->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
			rc = Cy_Crypto_Core_Aes_Ccm_Encrypt_Tag(base,
								session->nonce_len,
								session->aes_iv_sram,
								aead->ad_len, aead->ad,
								pkt->in_len, pkt->out_buf,
								pkt->in_buf,
								session->tag_len,
								session->aes_tag_sram,
								&session->ccm_state);
		} else {
			cy_en_crypto_aesccm_tag_verify_result_t verified =
				CY_CRYPTO_CCM_TAG_INVALID;

			rc = Cy_Crypto_Core_Aes_Ccm_Decrypt(base,
							    session->nonce_len,
							    session->aes_iv_sram,
							    aead->ad_len, aead->ad,
							    pkt->in_len, pkt->out_buf,
							    pkt->in_buf,
							    session->tag_len,
							    session->aes_tag_sram, &verified,
							    &session->ccm_state);
			if (rc == CY_CRYPTO_SUCCESS &&
			    verified != CY_CRYPTO_CCM_TAG_VALID) {
				ret = -EBADMSG;
			}
		}
	}

	if (rc == CY_CRYPTO_SUCCESS && session->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
		memcpy(aead->tag, session->aes_tag_sram, session->tag_len);
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES CCM failed: %d", (int)rc);
		return -EIO;
	}

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}
	return ret;
}

/* Cipher handlers — GCM */

static int mxcrypto_aes_gcm_op(struct cipher_ctx *ctx,
			       struct cipher_aead_pkt *aead, uint8_t *nonce)
{
	const struct mxcrypto_config *cfg = ctx->device->config;
	struct mxcrypto_session *session = ctx->drv_sessn_state;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	struct cipher_pkt *pkt = aead->pkt;
	cy_en_crypto_status_t rc;
	int ret = 0;

	if (session->tag_len > MXCRYPTO_AES_TAG_MAX_SIZE) {
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("AES GCM: out_buf_max %d < required %d",
			pkt->out_buf_max, pkt->in_len);
		return -EINVAL;
	}

	if (!mxcrypto_buf_align_safe(pkt->in_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(pkt->out_buf, pkt->in_len) ||
	    !mxcrypto_buf_align_safe(aead->ad, aead->ad_len)) {
		LOG_ERR("AES GCM: caller buffers must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);

	/*
	 * The tag and nonce are small.  Stage them through driver-owned
	 * nocache scratch so we don't impose cache-line allocation
	 * requirements on the caller for short fixed-size items.
	 */
	memcpy(session->aes_iv_sram, nonce, session->nonce_len);
	if (session->dir == CRYPTO_CIPHER_OP_DECRYPT) {
		memcpy(session->aes_tag_sram, aead->tag, session->tag_len);
	}

	if (session->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
		rc = Cy_Crypto_Core_Aes_GCM_Encrypt_Tag(base,
							session->key_sram, session->key_len,
							session->aes_iv_sram,
							session->nonce_len,
							aead->ad, aead->ad_len,
							pkt->in_buf, pkt->in_len,
							pkt->out_buf,
							session->aes_tag_sram,
							session->tag_len);
	} else {
		cy_en_crypto_aesgcm_tag_verify_result_t verified =
			CY_CRYPTO_TAG_INVALID;

		rc = Cy_Crypto_Core_Aes_GCM_Decrypt_Tag(base,
							session->key_sram, session->key_len,
							session->aes_iv_sram,
							session->nonce_len,
							aead->ad, aead->ad_len,
							pkt->in_buf, pkt->in_len,
							session->aes_tag_sram,
							session->tag_len,
							pkt->out_buf, &verified);
		if (rc == CY_CRYPTO_SUCCESS && verified != CY_CRYPTO_TAG_VALID) {
			ret = -EBADMSG;
		}
	}

	if (rc == CY_CRYPTO_SUCCESS && session->dir == CRYPTO_CIPHER_OP_ENCRYPT) {
		memcpy(aead->tag, session->aes_tag_sram, session->tag_len);
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("AES GCM failed: %d", (int)rc);
		return -EIO;
	}

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}
	return ret;
}

/* Session management */

static int mxcrypto_cipher_begin_session(const struct device *dev,
					 struct cipher_ctx *ctx,
					 enum cipher_algo algo,
					 enum cipher_mode mode,
					 enum cipher_op op_type)
{
	struct mxcrypto_session *session;
	uint16_t key_bits;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported cipher algorithm: %d", (int)algo);
		return -ENOTSUP;
	}

	if (mode != CRYPTO_CIPHER_MODE_ECB && mode != CRYPTO_CIPHER_MODE_CBC &&
	    mode != CRYPTO_CIPHER_MODE_CTR && mode != CRYPTO_CIPHER_MODE_CCM &&
	    mode != CRYPTO_CIPHER_MODE_GCM) {
		LOG_ERR("Unsupported cipher mode: %d", (int)mode);
		return -ENOTSUP;
	}

	if ((ctx->flags & ~MXCRYPTO_CAPS) != 0U) {
		LOG_ERR("Unsupported flag combination: 0x%x", ctx->flags);
		return -ENOTSUP;
	}

	key_bits = ctx->keylen * 8U;
	if (key_bits != 128U && key_bits != 192U && key_bits != 256U) {
		LOG_ERR("Unsupported AES key length: %u bits", key_bits);
		return -EINVAL;
	}

	session = mxcrypto_session_alloc(dev);
	if (session == NULL) {
		return -ENOSPC;
	}

	session->is_hash = false;
	session->algo = algo;
	session->mode = mode;
	session->dir = op_type;
	session->key_len_bytes = (uint16_t)(ctx->keylen);

	switch (key_bits) {
	case 128U:
		session->key_len = CY_CRYPTO_KEY_AES_128;
		break;
	case 192U:
		session->key_len = CY_CRYPTO_KEY_AES_192;
		break;
	default:
		session->key_len = CY_CRYPTO_KEY_AES_256;
		break;
	}

	/*
	 * Copy the key to the session buffer.  On D-Cache targets this
	 * ensures a cache-line-aligned SRAM pointer is passed to the PDL.
	 */
	memcpy(session->key_sram, ctx->key.bit_stream, session->key_len_bytes);

	ctx->drv_sessn_state = session;
	ctx->device = dev;

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		ctx->ops.block_crypt_hndlr = mxcrypto_aes_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		ctx->ops.cbc_crypt_hndlr = mxcrypto_aes_cbc_op;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		ctx->ops.ctr_crypt_hndlr = mxcrypto_aes_ctr_op;
		break;
	case CRYPTO_CIPHER_MODE_CCM:
		session->nonce_len = ctx->mode_params.ccm_info.nonce_len;
		session->tag_len = ctx->mode_params.ccm_info.tag_len;
		/* NIST SP 800-38C: nonce 7..13 bytes, tag even in [4,16]. */
		if (session->nonce_len < 7U || session->nonce_len > 13U) {
			LOG_ERR("CCM nonce_len %u out of range [7,13]",
				session->nonce_len);
			mxcrypto_session_free(dev, session);
			return -EINVAL;
		}
		if (session->tag_len < 4U || session->tag_len > 16U ||
		    (session->tag_len & 1U) != 0U) {
			LOG_ERR("CCM tag_len %u invalid (must be even, 4..16)",
				session->tag_len);
			mxcrypto_session_free(dev, session);
			return -EINVAL;
		}
		ctx->ops.ccm_crypt_hndlr = mxcrypto_aes_ccm_op;
		break;
	case CRYPTO_CIPHER_MODE_GCM:
		session->nonce_len = ctx->mode_params.gcm_info.nonce_len;
		session->tag_len = ctx->mode_params.gcm_info.tag_len;
		/*
		 * NIST SP 800-38D: nonce must be > 0 bits; 96-bit (12 B) is
		 * the recommended default.  The IV staging buffer bounds the
		 * upper limit to MXCRYPTO_AES_BLOCK_SIZE.  Tag 4..16 bytes.
		 */
		if (session->nonce_len == 0U ||
		    session->nonce_len > MXCRYPTO_AES_BLOCK_SIZE) {
			LOG_ERR("GCM nonce_len %u out of range [1,%u]",
				session->nonce_len, MXCRYPTO_AES_BLOCK_SIZE);
			mxcrypto_session_free(dev, session);
			return -EINVAL;
		}
		if (session->tag_len < 4U || session->tag_len > 16U) {
			LOG_ERR("GCM tag_len %u invalid (must be 4..16)",
				session->tag_len);
			mxcrypto_session_free(dev, session);
			return -EINVAL;
		}
		ctx->ops.gcm_crypt_hndlr = mxcrypto_aes_gcm_op;
		break;
	default:
		mxcrypto_session_free(dev, session);
		return -ENOTSUP;
	}

	return 0;
}

static int mxcrypto_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	mxcrypto_session_free(dev, ctx->drv_sessn_state);
	return 0;
}

/* Hash handlers */

/*
 * Initialize + Start the PDL SHA state for this session.  Must be
 * called with the MFD lock held.  On D-Cache targets, stages the
 * PDL's static-const initialHash table through the session's nocache
 * scratch so the PDL DMA MemCpy can read it via a cache-line-aligned
 * SRAM address.
 */
static cy_en_crypto_status_t mxcrypto_sha_init_start(CRYPTO_Type *base,
						     struct mxcrypto_session *session)
{
	cy_en_crypto_status_t rc;

	rc = Cy_Crypto_Core_Sha_Init(base, &session->sha_state, session->sha_mode,
				     &session->sha_buffers);
	if (rc != CY_CRYPTO_SUCCESS) {
		return rc;
	}

#ifdef CONFIG_DCACHE
	memcpy(session->sha_init_hash_sram, session->sha_state.initialHash,
	       session->sha_state.hashSize);
	session->sha_state.initialHash = session->sha_init_hash_sram;
#endif

	return Cy_Crypto_Core_Sha_Start(base, &session->sha_state);
}

static int mxcrypto_hash_op(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct mxcrypto_config *cfg = ctx->device->config;
	struct mxcrypto_session *session = ctx->drv_sessn_state;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	cy_en_crypto_status_t rc;

	if (!mxcrypto_buf_align_safe(pkt->in_buf, pkt->in_len)) {
		LOG_ERR("SHA: caller in_buf must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}
	if (finish &&
	    !mxcrypto_buf_align_safe(pkt->out_buf, session->sha_digest_size)) {
		LOG_ERR("SHA: caller out_buf must be %u-byte aligned",
			MXCRYPTO_ALIGN);
		return -EINVAL;
	}

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);

	rc = Cy_Crypto_Core_Sha_Update(base, &session->sha_state,
				       pkt->in_buf, pkt->in_len);

	if (rc == CY_CRYPTO_SUCCESS && finish) {
		rc = Cy_Crypto_Core_Sha_Finish(base, &session->sha_state,
					       pkt->out_buf);
		(void)Cy_Crypto_Core_Sha_Free(base, &session->sha_state);

		/*
		 * Re-arm the SHA context so the session can be reused for
		 * another hash_compute() call without a new begin_session.
		 */
		if (rc == CY_CRYPTO_SUCCESS) {
			rc = mxcrypto_sha_init_start(base, session);
		}
	}

	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("SHA failed: %d", (int)rc);
		return -EIO;
	}

	return 0;
}

static int mxcrypto_hash_begin_session(const struct device *dev,
				       struct hash_ctx *ctx,
				       enum hash_algo algo)
{
	const struct mxcrypto_config *cfg = dev->config;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	struct mxcrypto_session *session;
	cy_en_crypto_sha_mode_t mode;
	cy_en_crypto_status_t rc;
	uint8_t digest_size;

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		mode = CY_CRYPTO_MODE_SHA224;
		digest_size = 28U;
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		mode = CY_CRYPTO_MODE_SHA256;
		digest_size = 32U;
		break;
	case CRYPTO_HASH_ALGO_SHA384:
		mode = CY_CRYPTO_MODE_SHA384;
		digest_size = 48U;
		break;
	case CRYPTO_HASH_ALGO_SHA512:
		mode = CY_CRYPTO_MODE_SHA512;
		digest_size = 64U;
		break;
	default:
		LOG_ERR("Unsupported hash algorithm: %d", (int)algo);
		return -ENOTSUP;
	}

	if ((ctx->flags & ~MXCRYPTO_CAPS) != 0U) {
		LOG_ERR("Unsupported flag combination: 0x%x", ctx->flags);
		return -ENOTSUP;
	}

	session = mxcrypto_session_alloc(dev);
	if (session == NULL) {
		return -ENOSPC;
	}

	session->is_hash = true;
	session->sha_mode = mode;
	session->sha_digest_size = digest_size;

	/*
	 * Initialize + start the SHA context up front so hash_op can
	 * stream Update calls and only Finish when the caller flags it.
	 */
	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);
	rc = mxcrypto_sha_init_start(base, session);
	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("SHA session init failed: %d", (int)rc);
		mxcrypto_session_free(dev, session);
		return -EIO;
	}

	ctx->drv_sessn_state = session;
	ctx->device = dev;
	ctx->hash_hndlr = mxcrypto_hash_op;

	return 0;
}

static int mxcrypto_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	const struct mxcrypto_config *cfg = dev->config;
	CRYPTO_Type *base = mfd_infineon_mxcrypto_get_base(cfg->mfd);
	struct mxcrypto_session *session = ctx->drv_sessn_state;

	mfd_infineon_mxcrypto_lock(cfg->mfd, K_FOREVER);
	(void)Cy_Crypto_Core_Sha_Free(base, &session->sha_state);
	mfd_infineon_mxcrypto_unlock(cfg->mfd);

	mxcrypto_session_free(dev, session);
	return 0;
}

/* Driver capabilities and init */

static int mxcrypto_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return MXCRYPTO_CAPS;
}

static int mxcrypto_init(const struct device *dev)
{
	const struct mxcrypto_config *cfg = dev->config;
	struct mxcrypto_data *data = dev->data;

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("MXCRYPTO MFD device not ready");
		return -ENODEV;
	}

	/*
	 * mxcrypto_data is placed in the nocache section (which is
	 * noinit), so explicitly zero session bookkeeping here.
	 */
	memset(data->sessions, 0, sizeof(data->sessions));

	return 0;
}

static DEVICE_API(crypto, mxcrypto_driver_api) = {
	.cipher_begin_session = mxcrypto_cipher_begin_session,
	.cipher_free_session = mxcrypto_cipher_free_session,
	.cipher_async_callback_set = NULL,
	.hash_begin_session = mxcrypto_hash_begin_session,
	.hash_free_session = mxcrypto_hash_free_session,
	.hash_async_callback_set = NULL,
	.query_hw_caps = mxcrypto_query_hw_caps,
};

#define MXCRYPTO_DEFINE(n)                                                                         \
	static struct mxcrypto_data mxcrypto_data_##n __nocache;                                   \
	static const struct mxcrypto_config mxcrypto_config_##n = {                                \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mxcrypto_init, NULL, &mxcrypto_data_##n,                          \
			      &mxcrypto_config_##n, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,      \
			      &mxcrypto_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MXCRYPTO_DEFINE)
