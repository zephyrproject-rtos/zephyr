/*
 * Copyright (c) 2020 Markus Fuchs <markus.fuchs@de.sauter-bc.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_cryp

#include <init.h>
#include <kernel.h>
#include <device.h>
#include <sys/__assert.h>
#include <crypto/cipher.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>
#include <sys/byteorder.h>

#include "crypto_stm32_priv.h"

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(crypto_stm32);

#define CRYP_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
		      CAP_NO_IV_PREFIX)
#define BLOCK_LEN_BYTES 16
#define BLOCK_LEN_WORDS (BLOCK_LEN_BYTES / sizeof(uint32_t))
#define CRYPTO_MAX_SESSION CONFIG_CRYPTO_STM32_MAX_SESSION

struct crypto_stm32_session crypto_stm32_sessions[CRYPTO_MAX_SESSION];

static void copy_reverse_words(uint8_t *dst_buf, int dst_len,
			       uint8_t *src_buf, int src_len)
{
	int i;

	__ASSERT_NO_MSG(dst_len >= src_len);
	__ASSERT_NO_MSG((dst_len % 4) == 0);

	memcpy(dst_buf, src_buf, src_len);
	for (i = 0; i < dst_len; i += sizeof(uint32_t)) {
		sys_mem_swap(&dst_buf[i], sizeof(uint32_t));
	}
}

static int do_encrypt(struct cipher_ctx *ctx, uint8_t *in_buf, int in_len,
		      uint8_t *out_buf)
{
	HAL_StatusTypeDef status;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	k_sem_take(&data->device_sem, K_FOREVER);

	status = HAL_CRYP_SetConfig(&data->hcryp, &session->config);
	if (status != HAL_OK) {
		LOG_ERR("Configuration error");
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	status = HAL_CRYP_Encrypt(&data->hcryp, (uint32_t *)in_buf, in_len,
				  (uint32_t *)out_buf, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		LOG_ERR("Encryption error");
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	k_sem_give(&data->device_sem);

	return 0;
}

static int do_decrypt(struct cipher_ctx *ctx, uint8_t *in_buf, int in_len,
		      uint8_t *out_buf)
{
	HAL_StatusTypeDef status;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(ctx->device);
	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	k_sem_take(&data->device_sem, K_FOREVER);

	status = HAL_CRYP_SetConfig(&data->hcryp, &session->config);
	if (status != HAL_OK) {
		LOG_ERR("Configuration error");
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	status = HAL_CRYP_Decrypt(&data->hcryp, (uint32_t *)in_buf, in_len,
				  (uint32_t *)out_buf, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		LOG_ERR("Decryption error");
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	k_sem_give(&data->device_sem);

	return 0;
}

static int crypto_stm32_ecb_encrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt)
{
	int ret;

	/* For security reasons, ECB mode should not be used to encrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot encrypt more than 1 block");
		return -EINVAL;
	}

	ret = do_encrypt(ctx, pkt->in_buf, pkt->in_len, pkt->out_buf);
	if (ret == 0) {
		pkt->out_len = 16;
	}

	return ret;
}

static int crypto_stm32_ecb_decrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt)
{
	int ret;

	/* For security reasons, ECB mode should not be used to encrypt
	 * more than one block. Use CBC mode instead.
	 */
	if (pkt->in_len > 16) {
		LOG_ERR("Cannot encrypt more than 1 block");
		return -EINVAL;
	}

	ret = do_decrypt(ctx, pkt->in_buf, pkt->in_len, pkt->out_buf);
	if (ret == 0) {
		pkt->out_len = 16;
	}

	return ret;
}

static int crypto_stm32_cbc_encrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;
	uint32_t vec[BLOCK_LEN_WORDS];
	int out_offset = 0;

	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	copy_reverse_words((uint8_t *)vec, sizeof(vec), iv, BLOCK_LEN_BYTES);
	session->config.pInitVect = vec;

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		/* Prefix IV to ciphertext unless CAP_NO_IV_PREFIX is set. */
		memcpy(pkt->out_buf, iv, 16);
		out_offset = 16;
	}

	ret = do_encrypt(ctx, pkt->in_buf, pkt->in_len,
			 pkt->out_buf + out_offset);
	if (ret == 0) {
		pkt->out_len = pkt->in_len + out_offset;
	}

	return ret;
}

static int crypto_stm32_cbc_decrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;
	uint32_t vec[BLOCK_LEN_WORDS];
	int in_offset = 0;

	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	copy_reverse_words((uint8_t *)vec, sizeof(vec), iv, BLOCK_LEN_BYTES);
	session->config.pInitVect = vec;

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		in_offset = 16;
	}

	ret = do_decrypt(ctx, pkt->in_buf + in_offset, pkt->in_len,
			 pkt->out_buf);
	if (ret == 0) {
		pkt->out_len = pkt->in_len - in_offset;
	}

	return ret;
}

static int crypto_stm32_ctr_encrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;
	uint32_t ctr[BLOCK_LEN_WORDS] = {0};
	int ivlen = ctx->keylen - (ctx->mode_params.ctr_info.ctr_len >> 3);

	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	copy_reverse_words((uint8_t *)ctr, sizeof(ctr), iv, ivlen);
	session->config.pInitVect = ctr;

	ret = do_encrypt(ctx, pkt->in_buf, pkt->in_len, pkt->out_buf);
	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int crypto_stm32_ctr_decrypt(struct cipher_ctx *ctx,
				    struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;
	uint32_t ctr[BLOCK_LEN_WORDS] = {0};
	int ivlen = ctx->keylen - (ctx->mode_params.ctr_info.ctr_len >> 3);

	struct crypto_stm32_session *session = CRYPTO_STM32_SESSN(ctx);

	copy_reverse_words((uint8_t *)ctr, sizeof(ctr), iv, ivlen);
	session->config.pInitVect = ctr;

	ret = do_decrypt(ctx, pkt->in_buf, pkt->in_len, pkt->out_buf);
	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

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
	int ctx_idx;
	struct crypto_stm32_session *session;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);

	if (ctx->flags & ~(CRYP_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	/* The CRYP peripheral supports the AES ECB, CBC, CTR, CCM and GCM
	 * modes of operation, of which ECB, CBC, CTR and CCM are supported
	 * through the crypto API. However, in CCM mode, although the STM32Cube
	 * HAL driver follows the documentation (cf. RM0090, par. 23.3) by
	 * padding incomplete input data blocks in software prior encryption,
	 * incorrect authentication tags are returned for input data which is
	 * not a multiple of 128 bits. Therefore, CCM mode is not supported by
	 * this driver.
	 */
	if ((mode != CRYPTO_CIPHER_MODE_ECB) &&
	    (mode != CRYPTO_CIPHER_MODE_CBC) &&
	    (mode != CRYPTO_CIPHER_MODE_CTR)) {
		LOG_ERR("Unsupported mode");
		return -EINVAL;
	}

	/* The STM32F4 CRYP peripheral supports key sizes of 128, 192 and 256
	 * bits.
	 */
	if ((ctx->keylen != 16U) &&
	    (ctx->keylen != 24U) &&
	    (ctx->keylen != 32U)) {
		LOG_ERR("%u key size is not supported", ctx->keylen);
		return -EINVAL;
	}

	ctx_idx = crypto_stm32_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}
	session = &crypto_stm32_sessions[ctx_idx];
	memset(&session->config, 0, sizeof(session->config));

	if (data->hcryp.State == HAL_CRYP_STATE_RESET) {
		if (HAL_CRYP_Init(&data->hcryp) != HAL_OK) {
			LOG_ERR("Initialization error");
			session->in_use = false;
			return -EIO;
		}
	}

	switch (ctx->keylen) {
	case 16U:
		session->config.KeySize = CRYP_KEYSIZE_128B;
		break;
	case 24U:
		session->config.KeySize = CRYP_KEYSIZE_192B;
		break;
	case 32U:
		session->config.KeySize = CRYP_KEYSIZE_256B;
		break;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			session->config.Algorithm = CRYP_AES_ECB;
			ctx->ops.block_crypt_hndlr = crypto_stm32_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			session->config.Algorithm = CRYP_AES_CBC;
			ctx->ops.cbc_crypt_hndlr = crypto_stm32_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			session->config.Algorithm = CRYP_AES_CTR;
			ctx->ops.ctr_crypt_hndlr = crypto_stm32_ctr_encrypt;
			break;
		default:
			break;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			session->config.Algorithm = CRYP_AES_ECB;
			ctx->ops.block_crypt_hndlr = crypto_stm32_ecb_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			session->config.Algorithm = CRYP_AES_CBC;
			ctx->ops.cbc_crypt_hndlr = crypto_stm32_cbc_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			session->config.Algorithm = CRYP_AES_CTR;
			ctx->ops.ctr_crypt_hndlr = crypto_stm32_ctr_decrypt;
			break;
		default:
			break;
		}
	}

	copy_reverse_words((uint8_t *)session->key, CRYPTO_STM32_AES_MAX_KEY_LEN,
			   ctx->key.bit_stream, ctx->keylen);

	session->config.pKey = session->key;
	session->config.DataType = CRYP_DATATYPE_8B;
	session->config.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;

	ctx->drv_sessn_state = session;
	ctx->device = dev;

	return 0;
}

static int crypto_stm32_session_free(const struct device *dev,
				     struct cipher_ctx *ctx)
{
	int i;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);
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

	/* Deinitialize and reset peripheral. */
	if (HAL_CRYP_DeInit(&data->hcryp) != HAL_OK) {
		LOG_ERR("Deinitialization error");
		k_sem_give(&data->session_sem);
		return -EIO;
	}
	__HAL_RCC_CRYP_FORCE_RESET();
	__HAL_RCC_CRYP_RELEASE_RESET();

	k_sem_give(&data->session_sem);

	return 0;
}

static int crypto_stm32_query_caps(const struct device *dev)
{
	return CRYP_SUPPORT;
}

static int crypto_stm32_init(const struct device *dev)
{
	const struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);
	const struct crypto_stm32_config *cfg = CRYPTO_STM32_CFG(dev);

	__ASSERT_NO_MSG(clk);

	clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken);

	k_sem_init(&data->device_sem, 1, 1);
	k_sem_init(&data->session_sem, 1, 1);

	if (HAL_CRYP_DeInit(&data->hcryp) != HAL_OK) {
		LOG_ERR("Peripheral reset error");
		return -EIO;
	}

	return 0;
}

static struct crypto_driver_api crypto_enc_funcs = {
	.begin_session = crypto_stm32_session_setup,
	.free_session = crypto_stm32_session_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = crypto_stm32_query_caps,
};

static struct crypto_stm32_data crypto_stm32_dev_data = {
	.hcryp = {
		.Instance = CRYP
	}
};

static struct crypto_stm32_config crypto_stm32_dev_config = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	}
};

DEVICE_DT_INST_DEFINE(0, crypto_stm32_init, device_pm_control_nop,
		    &crypto_stm32_dev_data,
		    &crypto_stm32_dev_config, POST_KERNEL,
		    CONFIG_CRYPTO_INIT_PRIORITY, (void *)&crypto_enc_funcs);
