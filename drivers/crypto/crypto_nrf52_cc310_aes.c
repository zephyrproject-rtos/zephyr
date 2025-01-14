/*
 * Copyright (c) 2025 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <errno.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT nordic_nrf52_cc310_aes

LOG_MODULE_REGISTER(nrf52_cc310_aes, CONFIG_CRYPTO_LOG_LEVEL);

#define AES_KEY_SIZE   16
#define AES_BLOCK_SIZE 16

struct crypto_session {
	uint32_t current_ctr; /* only used for AES-CTR sessions */
	bool in_use;
};

struct crypto_data {
	struct crypto_session sessions[CONFIG_CRYPTO_NRF52_CC310_AES_MAX_SESSION];
};

K_MUTEX_DEFINE(crypto_in_use);
K_SEM_DEFINE(crypto_work_done, 0, 1);

static struct crypto_data crypto_cc310_data;

static int crypto_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (CAP_RAW_KEY | CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS |
		CAP_NO_IV_PREFIX | CAP_AES_CTR_CUSTOM_COUNTER_INIT);
}

static int crypto_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static bool crypto_sessions_in_use(void)
{
	bool sessions_in_use = false;

	for (unsigned int i = 0; i < ARRAY_SIZE(crypto_cc310_data.sessions); i++) {
		sessions_in_use |= crypto_cc310_data.sessions[i].in_use;
	}

	return sessions_in_use;
}

static void crypto_enable_cryptocell(void)
{
	/* Enable CRYPTOCELL subsystem */
	NRF_CRYPTOCELL->ENABLE = CRYPTOCELL_ENABLE_ENABLE_Enabled;

	/* Enable engine and DMA clock.
	 * Try to enable as long as they are not ready, otherwise
	 * potential risk of getting stuck if one of the clocks
	 * is not started properly
	 */
	while (!(NRF_CC_MISC->CLK_STATUS & CC_MISC_CLK_STATUS_AES_CLK_Msk)) {
		NRF_CC_MISC->AES_CLK = CC_MISC_AES_CLK_ENABLE_Enable;
		k_yield();
	}

	while (!(NRF_CC_MISC->CLK_STATUS & CC_MISC_CLK_STATUS_DMA_CLK_Msk)) {
		NRF_CC_MISC->DMA_CLK = CC_MISC_DMA_CLK_ENABLE_Enable;
		k_yield();
	}

	/* Wait until crypto engine is Idle */
	while (NRF_CC_CTL->CRYPTO_BUSY == CC_CTL_CRYPTO_BUSY_STATUS_Busy) {
		k_yield();
	}

	/* Configure AES as cryptographic flow */
	NRF_CC_CTL->CRYPTO_CTL = CC_CTL_CRYPTO_CTL_MODE_AESActive;
}

static inline void crypto_disable_cryptocell(void)
{
	NRF_CRYPTOCELL->ENABLE = CRYPTOCELL_ENABLE_ENABLE_Disabled;
}

static inline void crypto_aes_set_key(const uint32_t *key)
{
	NRF_CC_AES->AES_KEY_0[0] = key[0];
	NRF_CC_AES->AES_KEY_0[1] = key[1];
	NRF_CC_AES->AES_KEY_0[2] = key[2];
	NRF_CC_AES->AES_KEY_0[3] = key[3];
}

static int crypto_aes_run(struct cipher_ctx *ctx, const uint8_t *key, const uint8_t *in_buf,
			  const uint8_t *out_buf, int len)
{
	crypto_aes_set_key((uint32_t *)key);

	/* Configure DMA output destination address */
	NRF_CC_DOUT->DST_MEM_ADDR =
		ctx->flags & CAP_INPLACE_OPS ? (uint32_t)in_buf : (uint32_t)out_buf;
	NRF_CC_DOUT->DST_MEM_SIZE = (uint32_t)len;

	/* Configure DMA input source address to start the cryptographic operation */
	NRF_CC_DIN->SRC_MEM_ADDR = (uint32_t)in_buf;
	NRF_CC_DIN->SRC_MEM_SIZE = (uint32_t)len;

	/* Wait on DOUT DMA interrupt */
	while (!(NRF_CC_HOST_RGF->IRR & CC_HOST_RGF_IRR_DOUT_TO_MEM_INT_Msk)) {
		if (NRF_CC_HOST_RGF->IRR & CC_HOST_RGF_IRR_AHB_ERR_INT_Msk) {
			LOG_ERR("AHB error! Both input and output buffer have to be located in "
				"RAM!");
			return -EINVAL;
		}
		k_yield();
	}

	LOG_DBG("IRR register: %x", NRF_CC_HOST_RGF->IRR);
	return 0;
}

static int crypto_aes(struct cipher_ctx *ctx, enum cipher_op op, uint8_t mode,
		      struct cipher_pkt *pkt, unsigned int offset)
{
	NRF_CC_AES->AES_CONTROL = mode << CC_AES_AES_CONTROL_MODE_KEY0_Pos;

	switch (op) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		/* Configure AES engine control for decryption using ECB mode (default) */
		NRF_CC_AES->AES_CONTROL |= CC_AES_AES_CONTROL_DEC_KEY0_Encrypt;
		return crypto_aes_run(ctx, ctx->key.bit_stream, pkt->in_buf, pkt->out_buf + offset,
				      pkt->in_len);
	case CRYPTO_CIPHER_OP_DECRYPT:
		/* Configure AES engine control for decryption using ECB mode (default) */
		NRF_CC_AES->AES_CONTROL |= CC_AES_AES_CONTROL_DEC_KEY0_Decrypt;
		return crypto_aes_run(ctx, ctx->key.bit_stream, pkt->in_buf + offset, pkt->out_buf,
				      pkt->in_len);
	default:
		LOG_ERR("Unsupported cipher_op: %d", op);
		return -ENOSYS;
	}
}

static int crypto_aes_entry_guard(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	if (!ctx) {
		LOG_ERR("Missing context");
		return -EINVAL;
	}

	if (!pkt) {
		LOG_ERR("Missing packet");
		return -EINVAL;
	}

	if (pkt->in_len % AES_BLOCK_SIZE) {
		LOG_ERR("Can't work on partial blocks");
		return -EINVAL;
	}

	if (pkt->in_len == 0) {
		LOG_WRN("Zero-sized packet");
		return 0;
	}

	if (ctx->keylen != AES_KEY_SIZE) {
		LOG_ERR("Invalid key len: %" PRIu16, ctx->keylen);
		return -EINVAL;
	}

	return 1;
}

static int crypto_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			     const enum cipher_op op)
{
	int ret = crypto_aes_entry_guard(ctx, pkt);

	if (ret < 1) {
		return ret;
	}

	if (pkt->in_len > AES_BLOCK_SIZE) {
		LOG_ERR("Refusing to work on multiple ECB blocks");
		return -EINVAL;
	}

	if ((ctx->flags & CAP_INPLACE_OPS) && (pkt->out_buf != NULL)) {
		LOG_ERR("In-place must not have an out_buf");
		return -EINVAL;
	}

	k_mutex_lock(&crypto_in_use, K_FOREVER);

	ret = crypto_aes(ctx, op, CC_AES_AES_CONTROL_MODE_KEY0_ECB, pkt, 0);
	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	k_mutex_unlock(&crypto_in_use);
	return ret;
}

static int crypto_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			     const enum cipher_op op, const uint8_t iv[AES_BLOCK_SIZE])
{
	int offset = 0;
	int ret = crypto_aes_entry_guard(ctx, pkt);

	if (ret < 1) {
		return ret;
	}

	k_mutex_lock(&crypto_in_use, K_FOREVER);

	NRF_CC_AES->AES_IV_0[0] = *((uint32_t *)iv);
	NRF_CC_AES->AES_IV_0[1] = *((uint32_t *)iv + 1);
	NRF_CC_AES->AES_IV_0[2] = *((uint32_t *)iv + 2);
	NRF_CC_AES->AES_IV_0[3] = *((uint32_t *)iv + 3);

	/* Prefix IV to/remove from ciphertext unless CAP_NO_IV_PREFIX is set. */
	switch (op) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
			if (pkt->out_buf_max < AES_BLOCK_SIZE) {
				LOG_ERR("Output buf too small");
				ret = -ENOMEM;
				goto out_unlock;
			}
			if (!pkt->out_buf) {
				LOG_ERR("Missing output buf");
				ret = -EINVAL;
				goto out_unlock;
			}
			offset = AES_BLOCK_SIZE;
			memcpy(pkt->out_buf, iv, AES_BLOCK_SIZE);
		}
		ret = crypto_aes(ctx, op, CC_AES_AES_CONTROL_MODE_KEY0_CBC, pkt, offset);
		if (ret != 0) {
			goto out_unlock;
		}
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
			offset = -AES_BLOCK_SIZE;
		}
		ret = crypto_aes(ctx, op, CC_AES_AES_CONTROL_MODE_KEY0_CBC, pkt, -offset);
		if (ret != 0) {
			goto out_unlock;
		}
		break;
	default:
		LOG_ERR("Unsupported cipher_op: %d", op);
		ret = -ENOSYS;
		goto out_unlock;
	}

	pkt->out_len = pkt->in_len + offset;

	/* Update passed IV buffer with new version */
	*((uint32_t *)iv) = NRF_CC_AES->AES_IV_0[0];
	*((uint32_t *)iv + 1) = NRF_CC_AES->AES_IV_0[1];
	*((uint32_t *)iv + 2) = NRF_CC_AES->AES_IV_0[2];
	*((uint32_t *)iv + 3) = NRF_CC_AES->AES_IV_0[3];

out_unlock:
	k_mutex_unlock(&crypto_in_use);
	return ret;
}

static int crypto_aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t iv[12])
{
	k_mutex_lock(&crypto_in_use, K_FOREVER);

	int ret = crypto_aes_entry_guard(ctx, pkt);

	if (ret < 1) {
		return ret;
	}

	if (ctx->mode_params.ctr_info.ctr_len != 32) {
		LOG_ERR("Unsupported counter length: %" PRIu16, ctx->mode_params.ctr_info.ctr_len);
		return -ENOSYS;
	}

	struct crypto_session *session = (struct crypto_session *)ctx->drv_sessn_state;

	/* Initialization Vector */
	uint32_t *ctr_val = (uint32_t *)iv;

	NRF_CC_AES->AES_CTR[0] = ctr_val[0];
	NRF_CC_AES->AES_CTR[1] = ctr_val[1];
	NRF_CC_AES->AES_CTR[2] = ctr_val[2];
	NRF_CC_AES->AES_CTR[3] = sys_cpu_to_be32(session->current_ctr);
	LOG_DBG("ctr_val: %x, %x, %x, %x", ctr_val[0], ctr_val[1], ctr_val[2],
		sys_cpu_to_be32(session->current_ctr));

	ret = crypto_aes(ctx, CRYPTO_CIPHER_OP_ENCRYPT, CC_AES_AES_CONTROL_MODE_KEY0_CTR, pkt, 0);
	if (ret == 0) {
		pkt->out_len = pkt->in_len;
		session->current_ctr = sys_be32_to_cpu(NRF_CC_AES->AES_CTR[3]);
	}

	k_mutex_unlock(&crypto_in_use);
	return ret;
}

static int crypto_aes_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	return crypto_aes_ecb_op(ctx, pkt, CRYPTO_CIPHER_OP_ENCRYPT);
}

static int crypto_aes_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	return crypto_aes_ecb_op(ctx, pkt, CRYPTO_CIPHER_OP_DECRYPT);
}

static int crypto_aes_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	return crypto_aes_cbc_op(ctx, pkt, CRYPTO_CIPHER_OP_ENCRYPT, iv);
}

static int crypto_aes_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	return crypto_aes_cbc_op(ctx, pkt, CRYPTO_CIPHER_OP_DECRYPT, iv);
}

static int crypto_begin_session(const struct device *dev, struct cipher_ctx *ctx,
				const enum cipher_algo algo, const enum cipher_mode mode,
				const enum cipher_op op)
{
	int ret = 0;
	struct crypto_session *session = NULL;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("This driver supports only AES");
		return -ENOTSUP;
	}

	if (!(ctx->flags & CAP_SYNC_OPS)) {
		LOG_ERR("This driver supports only synchronous mode");
		return -ENOTSUP;
	}

	if (ctx->key.bit_stream == NULL) {
		LOG_ERR("No key provided");
		return -EINVAL;
	}

	if (ctx->keylen != AES_KEY_SIZE) {
		LOG_ERR("Only AES-128 supported");
		return -ENOSYS;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_CBC:
		if (ctx->flags & CAP_INPLACE_OPS && (ctx->flags & CAP_NO_IV_PREFIX) == 0) {
			LOG_ERR("In-place requires no IV prefix");
			return -EINVAL;
		}
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		if (ctx->mode_params.ctr_info.ctr_len != 32U) {
			LOG_ERR("Only 32 bit counter implemented");
			return -ENOSYS;
		}
		break;
	case CRYPTO_CIPHER_MODE_ECB:
	case CRYPTO_CIPHER_MODE_CCM:
	case CRYPTO_CIPHER_MODE_GCM:
	default:
		break;
	}

	k_mutex_lock(&crypto_in_use, K_FOREVER);

	for (unsigned int i = 0; i < ARRAY_SIZE(crypto_cc310_data.sessions); i++) {
		if (crypto_cc310_data.sessions[i].in_use) {
			continue;
		}

		LOG_DBG("Claiming session %u", i);
		session = &crypto_cc310_data.sessions[i];
		break;
	}

	if (session == NULL) {
		LOG_ERR("All %d session(s) in use", CONFIG_CRYPTO_NRF52_CC310_AES_MAX_SESSION);
		ret = -ENOSPC;
		goto out;
	}

	switch (op) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_aes_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_aes_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_aes_ctr_op;
			session->current_ctr = ctx->mode_params.ctr_info.ctr_initial_value;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
		case CRYPTO_CIPHER_MODE_GCM:
		default:
			LOG_ERR("Unsupported encryption mode: %d", mode);
			ret = -ENOSYS;
			goto out;
		}
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_aes_ecb_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_aes_cbc_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_aes_ctr_op;
			session->current_ctr = ctx->mode_params.ctr_info.ctr_initial_value;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
		case CRYPTO_CIPHER_MODE_GCM:
		default:
			LOG_ERR("Unsupported decryption mode: %d", mode);
			ret = -ENOSYS;
			goto out;
		}
		break;
	default:
		LOG_ERR("Unsupported cipher_op: %d", op);
		ret = -ENOSYS;
		goto out;
	}

	if (!crypto_sessions_in_use()) {
		crypto_enable_cryptocell();
	}

	session->in_use = true;
	ctx->drv_sessn_state = session;

out:
	k_mutex_unlock(&crypto_in_use);

	return ret;
}

static int crypto_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);

	if (!ctx) {
		LOG_ERR("Missing context");
		return -EINVAL;
	}

	struct crypto_session *session = (struct crypto_session *)ctx->drv_sessn_state;

	k_mutex_lock(&crypto_in_use, K_FOREVER);
	session->in_use = false;
	if (!crypto_sessions_in_use()) {
		crypto_disable_cryptocell();
	}
	k_mutex_unlock(&crypto_in_use);

	return 0;
}

/* AES only, no support for hashing */
static const struct crypto_driver_api crypto_api = {
	.query_hw_caps = crypto_query_hw_caps,
	.cipher_begin_session = crypto_begin_session,
	.cipher_free_session = crypto_free_session,
};

DEVICE_DT_INST_DEFINE(0, crypto_init, NULL, NULL, NULL, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &crypto_api);
