/*
 * Copyright (c) 2020 Markus Fuchs <markus.fuchs@de.sauter-bc.com>
 * Copyright (c) 2025 Georgij Cernysiov <geo.cgv@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "crypto_stm32_priv.h"

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_stm32);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define DT_DRV_COMPAT st_stm32_cryp
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define DT_DRV_COMPAT st_stm32_aes
#else
#error No STM32 HW Crypto Accelerator in device tree
#endif

#define CRYP_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
		      CAP_NO_IV_PREFIX)
#define BLOCK_LEN_BYTES 16
#define BLOCK_LEN_WORDS (BLOCK_LEN_BYTES / sizeof(uint32_t))
#define CRYPTO_MAX_SESSION CONFIG_CRYPTO_STM32_MAX_SESSION

#if defined(CRYP_KEYSIZE_192B)
#define STM32_CRYPTO_KEYSIZE_192B_SUPPORT
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define STM32_CRYPTO_TYPEDEF            CRYP_TypeDef
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define STM32_CRYPTO_TYPEDEF            AES_TypeDef
#endif

#define STM32_CRYPTO_GCM_CCM_SUPPORT DT_INST_PROP(0, gcm_ccm_supported)

#if (K_HEAP_MEM_POOL_SIZE > 0)
#define STM32_CRYPTO_HEAP 1
#endif

#if IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT) && !IS_ENABLED(STM32_CRYPTO_HEAP)
#warning "CCM AD support requires CONFIG_HEAP_MEM_POOL_SIZE > 0"
#endif

#if IS_ENABLED(CONFIG_CRYPTO_STM32_USE_MBEDTLS_CT_MEMCMP)
#include <mbedtls/constant_time.h>
#define STM32_CRYPTO_MEMCMP(a, b, n) mbedtls_ct_memcmp((a), (b), (n))
#else
/* memcmp is vulnerable to timing attacks */
#define STM32_CRYPTO_MEMCMP(a, b, n) memcmp((a), (b), (n))
#endif

struct crypto_stm32_session crypto_stm32_sessions[CRYPTO_MAX_SESSION];

typedef HAL_StatusTypeDef status_t;

/**
 * @brief Function pointer type for AES encryption/decryption operations.
 *
 * This type defines a function pointer for generic AES operations.
 *
 * @param hcryp       Pointer to a CRYP_HandleTypeDef structure that contains
 *                    the configuration information for the CRYP module.
 * @param in_data     Pointer to input data (plaintext for encryption or ciphertext for decryption).
 * @param size        Length of the input data in bytes.
 * @param out_data    Pointer to output data (ciphertext for encryption or plaintext for
 * decryption).
 * @param timeout     Timeout duration in milliseconds.
 *
 * @retval status_t  HAL status of the operation.
 */
typedef status_t (*hal_cryp_aes_op_func_t)(CRYP_HandleTypeDef *hcryp, uint8_t *in_data,
					   uint16_t size, uint8_t *out_data, uint32_t timeout);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
#define hal_ecb_encrypt_op HAL_CRYP_AESECB_Encrypt
#define hal_ecb_decrypt_op HAL_CRYP_AESECB_Decrypt
#define hal_cbc_encrypt_op HAL_CRYP_AESCBC_Encrypt
#define hal_cbc_decrypt_op HAL_CRYP_AESCBC_Decrypt
#define hal_ctr_encrypt_op HAL_CRYP_AESCTR_Encrypt
#define hal_ctr_decrypt_op HAL_CRYP_AESCTR_Decrypt
#else
#define hal_ecb_encrypt_op hal_encrypt
#define hal_ecb_decrypt_op hal_decrypt
#define hal_cbc_encrypt_op hal_encrypt
#define hal_cbc_decrypt_op hal_decrypt
#define hal_ctr_encrypt_op hal_encrypt
#define hal_ctr_decrypt_op hal_decrypt
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes) */

/* L4 HAL driver uses uint8_t pointers for input/output data while the generic HAL driver uses
 * uint32_t pointers.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
#define CAST_VEC(x) (uint8_t *)(x)
#else
#define CAST_VEC(x) (uint32_t *)(x)
#endif

static int copy_words_adjust_endianness(uint8_t *dst_buf, int dst_len, const uint8_t *src_buf,
					int src_len)
{
	if ((dst_len < src_len) || ((dst_len % 4) != 0)) {
		LOG_ERR("Buffer length error");
		return -EINVAL;
	}

	memcpy(dst_buf, src_buf, src_len);

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
	for (int i = 0; i < dst_len; i += sizeof(uint32_t)) {
		sys_mem_swap(&dst_buf[i], sizeof(uint32_t));
	}
#endif

	return 0;
}

static int do_aes(struct cipher_ctx *ctx, hal_cryp_aes_op_func_t fn, uint8_t *in_buf, int in_len,
		      uint8_t *out_buf)
{
	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
	/* Device is initialized from the configuration in the encryption/decryption function
	 * called below.
	 */
	memcpy(&data->hcryp.Init, &session->config, sizeof(session->config));
#else
	if (HAL_CRYP_SetConfig(&data->hcryp, &session->config) != HAL_OK) {
		LOG_ERR("Configuration error");
		return -EIO;
	}
#endif

	if (fn(&data->hcryp, in_buf, in_len, out_buf, HAL_MAX_DELAY) != HAL_OK) {
		LOG_ERR("Encryption/decryption error");
		return -EIO;
	}

	return 0;
}

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
static status_t hal_encrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pPlainData, uint16_t Size,
			    uint8_t *pCypherData, uint32_t Timeout)
{
	return HAL_CRYP_Encrypt(hcryp, (uint32_t *)pPlainData, Size, (uint32_t *)pCypherData,
				Timeout);
}

static status_t hal_decrypt(CRYP_HandleTypeDef *hcryp, uint8_t *pCypherData, uint16_t Size,
			    uint8_t *pPlainData, uint32_t Timeout)
{
	return HAL_CRYP_Decrypt(hcryp, (uint32_t *)pCypherData, Size, (uint32_t *)pPlainData,
				Timeout);
}
#endif

static int crypto_stm32_ecb_encrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	int ret;

	/* For security reasons, ECB mode should not be used to encrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot encrypt more than 1 block");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = do_aes(ctx, hal_ecb_encrypt_op, pkt->in_buf, pkt->in_len, pkt->out_buf);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		pkt->out_len = 16;
	}

	return ret;
}

static int crypto_stm32_ecb_decrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	int ret;

	/* For security reasons, ECB mode should not be used to encrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot encrypt more than 1 block");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = do_aes(ctx, hal_ecb_decrypt_op, pkt->in_buf, pkt->in_len, pkt->out_buf);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		pkt->out_len = 16;
	}

	return ret;
}

static int crypto_stm32_cbc_encrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *const session = CRYPTO_STM32_SESSN(ctx);
	int ret;
	uint32_t vec[BLOCK_LEN_WORDS];
	int out_offset = 0;

	(void)copy_words_adjust_endianness((uint8_t *)vec, sizeof(vec), iv, BLOCK_LEN_BYTES);

	session->config.pInitVect = CAST_VEC(vec);

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		/* Prefix IV to ciphertext unless CAP_NO_IV_PREFIX is set. */
		memcpy(pkt->out_buf, iv, 16);
		out_offset = 16;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = do_aes(ctx, hal_cbc_encrypt_op, pkt->in_buf, pkt->in_len, pkt->out_buf + out_offset);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		pkt->out_len = pkt->in_len + out_offset;
	}

	return ret;
}

static int crypto_stm32_cbc_decrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *const session = CRYPTO_STM32_SESSN(ctx);
	int ret;
	uint32_t vec[BLOCK_LEN_WORDS];
	int in_offset = 0;

	(void)copy_words_adjust_endianness((uint8_t *)vec, sizeof(vec), iv, BLOCK_LEN_BYTES);

	session->config.pInitVect = CAST_VEC(vec);

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		in_offset = 16;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = do_aes(ctx, hal_cbc_decrypt_op, pkt->in_buf + in_offset, pkt->in_len, pkt->out_buf);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		pkt->out_len = pkt->in_len - in_offset;
	}

	return ret;
}

static int crypto_stm32_ctr_encrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *const session = CRYPTO_STM32_SESSN(ctx);
	int ret;
	uint32_t ctr[BLOCK_LEN_WORDS] = {0};
	int ivlen = BLOCK_LEN_BYTES - (ctx->mode_params.ctr_info.ctr_len >> 3);

	if (copy_words_adjust_endianness((uint8_t *)ctr, sizeof(ctr), iv, ivlen) != 0) {
		return -EIO;
	}

	session->config.pInitVect = CAST_VEC(ctr);

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = do_aes(ctx, hal_ctr_encrypt_op, pkt->in_buf, pkt->in_len, pkt->out_buf);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int crypto_stm32_ctr_decrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *const session = CRYPTO_STM32_SESSN(ctx);
	int ret;
	uint32_t ctr[BLOCK_LEN_WORDS] = {0};
	int ivlen = BLOCK_LEN_BYTES - (ctx->mode_params.ctr_info.ctr_len >> 3);

	if (copy_words_adjust_endianness((uint8_t *)ctr, sizeof(ctr), iv, ivlen) != 0) {
		return -EIO;
	}

	session->config.pInitVect = CAST_VEC(ctr);

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = do_aes(ctx, hal_ctr_decrypt_op, pkt->in_buf, pkt->in_len, pkt->out_buf);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

#if IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT)
static int crypto_stm32_gcm(struct cipher_ctx *ctx, hal_cryp_aes_op_func_t fn,
			    struct cipher_aead_pkt *apkt, uint8_t *nonce)
{
	struct crypto_stm32_session *const session = CRYPTO_STM32_SESSN(ctx);
	uint32_t iv[BLOCK_LEN_WORDS] = {0};

	if (ctx->mode_params.gcm_info.nonce_len != 12U) {
		return -EINVAL;
	}

	if (ctx->mode_params.gcm_info.tag_len != BLOCK_LEN_BYTES) {
		return -EINVAL;
	}

	if (copy_words_adjust_endianness((uint8_t *)iv, sizeof(iv), nonce,
					 ctx->mode_params.gcm_info.nonce_len) != 0) {
		return -EIO;
	}

	iv[3] = 2U;

	session->config.pInitVect = CAST_VEC(iv);

	if (apkt->ad_len == 0U) {
		session->config.Header = NULL;
		session->config.HeaderSize = 0U;
	} else {
		session->config.Header = CAST_VEC(apkt->ad);
		session->config.HeaderSize = apkt->ad_len;
		session->config.HeaderWidthUnit = CRYP_HEADERWIDTHUNIT_BYTE;
	}

	return do_aes(ctx, fn, apkt->pkt->in_buf, apkt->pkt->in_len, apkt->pkt->out_buf);
}

static int crypto_stm32_gcm_encrypt(struct cipher_ctx *ctx, struct cipher_aead_pkt *apkt,
				    uint8_t *nonce)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	uint8_t tag[BLOCK_LEN_BYTES] = {0};
	int ret;

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_stm32_gcm(ctx, hal_encrypt, apkt, nonce);
	if (ret < 0) {
		goto out;
	}

	if (HAL_CRYPEx_AESGCM_GenerateAuthTAG(&data->hcryp, CAST_VEC(tag),
					      HAL_MAX_DELAY) != HAL_OK) {
		ret = -EIO;
	}

out:
	k_sem_give(&data->device_sem);

	if (ret == 0) {
		memcpy(apkt->tag, tag, ctx->mode_params.gcm_info.tag_len);
		apkt->pkt->out_len = apkt->pkt->in_len;
	}

	return ret;
}

static int crypto_stm32_gcm_decrypt(struct cipher_ctx *ctx, struct cipher_aead_pkt *apkt,
				    uint8_t *nonce)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	uint8_t tag[BLOCK_LEN_BYTES] = {0};
	int ret;

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_stm32_gcm(ctx, hal_decrypt, apkt, nonce);
	if (ret < 0) {
		goto out;
	}

	if (HAL_CRYPEx_AESGCM_GenerateAuthTAG(&data->hcryp, CAST_VEC(tag),
					      HAL_MAX_DELAY) != HAL_OK) {
		ret = -EIO;
	}

out:
	k_sem_give(&data->device_sem);

	if (ret < 0) {
		return ret;
	}

	if (STM32_CRYPTO_MEMCMP(tag, apkt->tag, ctx->mode_params.gcm_info.tag_len) != 0) {
		/* auth/tag verification fails */
		return -EFAULT;
	}

	apkt->pkt->out_len = apkt->pkt->in_len;
	return ret;
}

static int crypto_stm32_ccm(struct cipher_ctx *ctx, hal_cryp_aes_op_func_t fn,
			    struct cipher_aead_pkt *apkt, uint8_t *nonce, uint8_t *tag)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *const session = CRYPTO_STM32_SESSN(ctx);
	/* B1 - Associated Data (AD) */
	uint8_t *b1 = NULL;
	/* B0 - Authentication block */
	uint8_t b0[BLOCK_LEN_BYTES] __aligned(4) = {0};
	unsigned int idx;
	int ret;
	uint8_t q;

	/* tag length: 4, 6, 8, 10, 12, 14, 16 */
	if ((ctx->mode_params.ccm_info.tag_len < 4U) ||
	    (ctx->mode_params.ccm_info.tag_len > 16U) ||
	    (ctx->mode_params.ccm_info.tag_len % 2U) != 0U) {
		return -EINVAL;
	}

	/* nonce length [7, 13] */
	if ((ctx->mode_params.ccm_info.nonce_len < 7U) ||
	    (ctx->mode_params.ccm_info.nonce_len > 13U)) {
		return -EINVAL;
	}

	/* bytes left for payload length */
	q = 15U - ctx->mode_params.ccm_info.nonce_len;

	/* check if payload length fits into q bytes */
	if (apkt->pkt->in_len > BIT_MASK(8U * q)) {
		return -EINVAL;
	}

	if (apkt->ad_len == 0U) {
		session->config.Header = NULL;
		session->config.HeaderSize = 0U;
	} else {
#if IS_ENABLED(STM32_CRYPTO_HEAP)
		size_t b1_padded_len;
		uint8_t header_len;

		if (apkt->ad_len < 0xFF00U) {
			header_len = 2U;
		} else {
			/* ad_len is uint32_t (support len < 0xFFFFFFFFU) */
			header_len = 6U;
		}

		b1_padded_len = ROUND_UP(apkt->ad_len + header_len, BLOCK_LEN_BYTES);
		if (b1_padded_len > UINT32_MAX) {
			/* HeaderSize is uint32_t */
			return -EINVAL;
		}

		b1 = k_calloc(1, b1_padded_len);
		if (b1 == NULL) {
			return -ENOMEM;
		}

		if (header_len == 2U) {
			sys_put_be16(apkt->ad_len, b1);
		} else {
			b1[0] = 0xFFU;
			b1[1] = 0xFEU;
			sys_put_be32(apkt->ad_len, b1 + 2U);
		}

		memcpy(b1 + header_len, apkt->ad, apkt->ad_len);

		session->config.Header = CAST_VEC(b1);
		session->config.HeaderSize = b1_padded_len / sizeof(uint32_t);
		session->config.HeaderWidthUnit = CRYP_HEADERWIDTHUNIT_WORD;

		/* set AD presence flag */
		b0[0] = BIT(6);
#else
		return -ENOMEM;
#endif /* IS_ENABLED(STM32_CRYPTO_HEAP) */
	}

	/* encode leftover flags */
	b0[0] |= ((ctx->mode_params.gcm_info.tag_len - 2U) / 2U) << 3U;
	b0[0] |= q - 1U;

	/* encode nonce */
	memcpy(&b0[1], nonce, ctx->mode_params.ccm_info.nonce_len);

	/* encode payload length */
	for (idx = 0U; idx < q; idx++) {
		b0[15U - idx] = (uint8_t)((apkt->pkt->in_len >> (8U * idx)) & 0xFFU);
	}

	/* set B0 */
	for (idx = 0U; idx < sizeof(b0); idx += sizeof(uint32_t)) {
		sys_cpu_to_be(&b0[idx], sizeof(uint32_t));
	}

	session->config.B0 = CAST_VEC(b0);

	ret = do_aes(ctx, fn, apkt->pkt->in_buf, apkt->pkt->in_len, apkt->pkt->out_buf);

	k_free(b1);

	if (ret < 0) {
		return ret;
	}

	/* compute auth tag, b0 shall be valid; hcryp holds a pointer to it */
	if (HAL_CRYPEx_AESCCM_GenerateAuthTAG(&data->hcryp, CAST_VEC(tag),
					      HAL_MAX_DELAY) != HAL_OK) {
		ret = -EIO;
	}

	return ret;
}

static int crypto_stm32_ccm_encrypt(struct cipher_ctx *ctx, struct cipher_aead_pkt *apkt,
				    uint8_t *nonce)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	uint8_t tag[BLOCK_LEN_BYTES] = {0};
	int ret;

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_stm32_ccm(ctx, hal_encrypt, apkt, nonce, &tag[0]);

	k_sem_give(&data->device_sem);

	if (ret == 0) {
		memcpy(apkt->tag, tag, ctx->mode_params.ccm_info.tag_len);
		apkt->pkt->out_len = apkt->pkt->in_len;
	}

	return ret;
}

static int crypto_stm32_ccm_decrypt(struct cipher_ctx *ctx, struct cipher_aead_pkt *apkt,
				    uint8_t *nonce)
{
	struct crypto_stm32_data *const data = CRYPTO_STM32_DATA(ctx->device);
	uint8_t tag[BLOCK_LEN_BYTES] = {0};
	int ret;

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_stm32_ccm(ctx, hal_decrypt, apkt, nonce, &tag[0]);

	k_sem_give(&data->device_sem);

	if (ret < 0) {
		return ret;
	}

	if (STM32_CRYPTO_MEMCMP(tag, apkt->tag, ctx->mode_params.ccm_info.tag_len) != 0) {
		/* auth/tag verification fails */
		return -EFAULT;
	}

	apkt->pkt->out_len = apkt->pkt->in_len;
	return ret;
}
#endif /* IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT) */

static int crypto_stm32_get_unused_session_index(const struct device *dev)
{
	int i;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);

	k_sem_take(&data->session_sem, K_FOREVER);

	for (i = 0; i < CRYPTO_MAX_SESSION; i++) {
		if (!crypto_stm32_sessions[i].in_use) {
			crypto_stm32_sessions[i].in_use = true;
			k_sem_give(&data->session_sem);
			return i;
		}
	}

	k_sem_give(&data->session_sem);

	return -1;
}

static int crypto_stm32_session_setup(const struct device *dev,
				      struct cipher_ctx *ctx,
				      enum cipher_algo algo,
				      enum cipher_mode mode,
				      enum cipher_op op_type)
{
	int ctx_idx, ret;
	struct crypto_stm32_session *session;

	if (ctx->flags & ~(CRYP_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algo");
		return -ENOTSUP;
	}

	if ((mode != CRYPTO_CIPHER_MODE_ECB) &&
	    (mode != CRYPTO_CIPHER_MODE_CBC) &&
	    (mode != CRYPTO_CIPHER_MODE_CTR) &&
	    (!IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT) ||
		((mode != CRYPTO_CIPHER_MODE_CCM) &&
		 (mode != CRYPTO_CIPHER_MODE_GCM)))) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	/* The STM32F4 CRYP peripheral supports key sizes of 128, 192 and 256
	 * bits.
	 */
	if ((ctx->keylen != 16U) &&
#if defined(STM32_CRYPTO_KEYSIZE_192B_SUPPORT)
	    (ctx->keylen != 24U) &&
#endif
	    (ctx->keylen != 32U)) {
		LOG_ERR("%u key size is not supported", ctx->keylen);
		return -ENOTSUP;
	}

	ctx_idx = crypto_stm32_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}
	session = &crypto_stm32_sessions[ctx_idx];
	memset(&session->config, 0, sizeof(session->config));

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);

	if (data->hcryp.State == HAL_CRYP_STATE_RESET) {
		if (HAL_CRYP_Init(&data->hcryp) != HAL_OK) {
			LOG_ERR("Initialization error");
			session->in_use = false;
			return -EIO;
		}
	}
#endif

	switch (ctx->keylen) {
	case 16U:
		session->config.KeySize = CRYP_KEYSIZE_128B;
		break;
#if defined(STM32_CRYPTO_KEYSIZE_192B_SUPPORT)
	case 24U:
		session->config.KeySize = CRYP_KEYSIZE_192B;
		break;
#endif
	case 32U:
		session->config.KeySize = CRYP_KEYSIZE_256B;
		break;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
			session->config.Algorithm = CRYP_AES_ECB;
#endif
			ctx->ops.block_crypt_hndlr = crypto_stm32_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
			session->config.Algorithm = CRYP_AES_CBC;
#endif
			ctx->ops.cbc_crypt_hndlr = crypto_stm32_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
			session->config.Algorithm = CRYP_AES_CTR;
#endif
			ctx->ops.ctr_crypt_hndlr = crypto_stm32_ctr_encrypt;
			break;
#if IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT)
		case CRYPTO_CIPHER_MODE_GCM:
			session->config.Algorithm = CRYP_AES_GCM;
			ctx->ops.gcm_crypt_hndlr = crypto_stm32_gcm_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			session->config.Algorithm = CRYP_AES_CCM;
			ctx->ops.ccm_crypt_hndlr = crypto_stm32_ccm_encrypt;
			break;
#endif /* IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT) */
		default:
			CODE_UNREACHABLE;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
			session->config.Algorithm = CRYP_AES_ECB;
#endif
			ctx->ops.block_crypt_hndlr = crypto_stm32_ecb_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
			session->config.Algorithm = CRYP_AES_CBC;
#endif
			ctx->ops.cbc_crypt_hndlr = crypto_stm32_cbc_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
			session->config.Algorithm = CRYP_AES_CTR;
#endif
			ctx->ops.ctr_crypt_hndlr = crypto_stm32_ctr_decrypt;
			break;
#if IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT)
		case CRYPTO_CIPHER_MODE_GCM:
			session->config.Algorithm = CRYP_AES_GCM;
			ctx->ops.gcm_crypt_hndlr = crypto_stm32_gcm_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			session->config.Algorithm = CRYP_AES_CCM;
			ctx->ops.ccm_crypt_hndlr = crypto_stm32_ccm_decrypt;
			break;
#endif /* IS_ENABLED(STM32_CRYPTO_GCM_CCM_SUPPORT) */
		default:
			CODE_UNREACHABLE;
		}
	}

	ret = copy_words_adjust_endianness((uint8_t *)session->key, CRYPTO_STM32_AES_MAX_KEY_LEN,
				 ctx->key.bit_stream, ctx->keylen);
	if (ret != 0) {
		return -EIO;
	}

	session->config.pKey = CAST_VEC(session->key);
	session->config.DataType = CRYP_DATATYPE_8B;

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
	session->config.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;
#endif

	ctx->drv_sessn_state = session;
	ctx->device = dev;

	return 0;
}

static int crypto_stm32_session_free(const struct device *dev,
				     struct cipher_ctx *ctx)
{
	int i;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);
	const struct crypto_stm32_config *cfg = CRYPTO_STM32_CFG(dev);
	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	session->in_use = false;

	k_sem_take(&data->session_sem, K_FOREVER);

	/* Disable peripheral only if there are no more active sessions. */
	for (i = 0; i < CRYPTO_MAX_SESSION; i++) {
		if (crypto_stm32_sessions[i].in_use) {
			k_sem_give(&data->session_sem);
			return 0;
		}
	}

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
	/* Deinitialize and reset peripheral. */
	if (HAL_CRYP_DeInit(&data->hcryp) != HAL_OK) {
		LOG_ERR("Deinitialization error");
		k_sem_give(&data->session_sem);
		return -EIO;
	}
#endif

	(void)reset_line_toggle_dt(&cfg->reset);

	k_sem_give(&data->session_sem);

	return 0;
}

static int crypto_stm32_query_caps(const struct device *dev)
{
	return CRYP_SUPPORT;
}

static int crypto_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);
	const struct crypto_stm32_config *cfg = CRYPTO_STM32_CFG(dev);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	k_sem_init(&data->device_sem, 1, 1);
	k_sem_init(&data->session_sem, 1, 1);

	if (HAL_CRYP_DeInit(&data->hcryp) != HAL_OK) {
		LOG_ERR("Peripheral reset error");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(crypto, crypto_enc_funcs) = {
	.cipher_begin_session = crypto_stm32_session_setup,
	.cipher_free_session = crypto_stm32_session_free,
	.cipher_async_callback_set = NULL,
	.query_hw_caps = crypto_stm32_query_caps,
};

static struct crypto_stm32_data crypto_stm32_dev_data = {
	.hcryp = {
		.Instance = (STM32_CRYPTO_TYPEDEF *)DT_INST_REG_ADDR(0),
	}
};

static const struct crypto_stm32_config crypto_stm32_dev_config = {
	.reset = RESET_DT_SPEC_INST_GET(0),
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	}
};

DEVICE_DT_INST_DEFINE(0, crypto_stm32_init, NULL,
		    &crypto_stm32_dev_data,
		    &crypto_stm32_dev_config, POST_KERNEL,
		    CONFIG_CRYPTO_INIT_PRIORITY, (void *)&crypto_enc_funcs);
