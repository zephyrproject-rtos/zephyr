/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/cipher.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <crypto_engine_nsc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <hw_aes.h>
#endif

LOG_MODULE_REGISTER(crypto_bee_aes, CONFIG_CRYPTO_LOG_LEVEL);

#define DT_DRV_COMPAT realtek_bee_aes

#define BEE_AES_BLOCK_SIZE 16U

struct bee_aes_sessn_state {
	bool in_use;
	uint8_t key[32];
	size_t key_len;
	enum cipher_mode mode;
	enum cipher_op op;
};

static K_MUTEX_DEFINE(bee_aes_lock);

static struct bee_aes_sessn_state bee_aes_sessions[CONFIG_CRYPTO_BEE_AES_SESSIONS_MAX];

static void bee_aes_copy_to_hw(const uint8_t *src, uint32_t *dst, size_t len)
{
#if defined(CONFIG_CRYPTO_BEE_AES_SWAP_BUF)
	swap_buf(src, (uint8_t *)dst, len);
#else
	memcpy(dst, src, len);
#endif
}

static void bee_aes_copy_from_hw(const uint32_t *src, uint8_t *dst, size_t len)
{
#if defined(CONFIG_CRYPTO_BEE_AES_SWAP_BUF)
	swap_buf((const uint8_t *)src, dst, len);
#else
	memcpy(dst, src, len);
#endif
}

static struct bee_aes_sessn_state *bee_aes_sessn_alloc(void)
{
	struct bee_aes_sessn_state *ret = NULL;

	k_mutex_lock(&bee_aes_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(bee_aes_sessions); i++) {
		if (!bee_aes_sessions[i].in_use) {
			memset(&bee_aes_sessions[i], 0, sizeof(bee_aes_sessions[i]));
			bee_aes_sessions[i].in_use = true;
			ret = &bee_aes_sessions[i];
			break;
		}
	}

	k_mutex_unlock(&bee_aes_lock);

	if (!ret) {
		LOG_WRN("No free AES sessions (max %d)", CONFIG_CRYPTO_BEE_AES_SESSIONS_MAX);
	}

	return ret;
}

static void bee_aes_sessn_free(struct bee_aes_sessn_state *s)
{
	if (!s) {
		return;
	}

	k_mutex_lock(&bee_aes_lock, K_FOREVER);
	memset(s, 0, sizeof(*s));
	k_mutex_unlock(&bee_aes_lock);
}

static T_HW_AES_MODE bee_aes_hw_mode(enum cipher_mode mode)
{
	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		return AES_MODE_ECB;
	case CRYPTO_CIPHER_MODE_CBC:
		return AES_MODE_CBC;
	case CRYPTO_CIPHER_MODE_CFB:
		return AES_MODE_CFB;
	case CRYPTO_CIPHER_MODE_OFB:
		return AES_MODE_OFB;
	case CRYPTO_CIPHER_MODE_CTR:
		return AES_MODE_CTR;
	default:
		return AES_MODE_NONE;
	}
}

static int bee_aes_hw_block(const struct bee_aes_sessn_state *s, T_HW_AES_MODE mode,
			    const uint8_t *in, uint8_t *out, const uint8_t *iv, bool decrypt)
{
	uint32_t in_hw[BEE_AES_BLOCK_SIZE / sizeof(uint32_t)];
	uint32_t out_hw[BEE_AES_BLOCK_SIZE / sizeof(uint32_t)];
	uint32_t key_hw[32U / sizeof(uint32_t)];
	uint32_t iv_hw[BEE_AES_BLOCK_SIZE / sizeof(uint32_t)];
	uint32_t *iv_arg = NULL;
	bool ret;

	bee_aes_copy_to_hw(in, in_hw, BEE_AES_BLOCK_SIZE);
	bee_aes_copy_to_hw(s->key, key_hw, s->key_len);

	if (iv) {
		bee_aes_copy_to_hw(iv, iv_hw, BEE_AES_BLOCK_SIZE);
		iv_arg = iv_hw;
	}

	if (s->key_len == 16U) {
		if (decrypt) {
			ret = hw_aes_decrypt128(in_hw, out_hw, 4, key_hw, iv_arg, mode);
		} else {
			ret = hw_aes_encrypt128(in_hw, out_hw, 4, key_hw, iv_arg, mode);
		}
	} else if (s->key_len == 32U) {
		if (decrypt) {
			ret = hw_aes_decrypt256(in_hw, out_hw, 4, key_hw, iv_arg, mode);
		} else {
			ret = hw_aes_encrypt256(in_hw, out_hw, 4, key_hw, iv_arg, mode);
		}
	} else {
		return -EINVAL;
	}

	if (!ret) {
		return -EIO;
	}

	bee_aes_copy_from_hw(out_hw, out, BEE_AES_BLOCK_SIZE);
	return 0;
}

static void bee_aes_ctr_inc(uint8_t *counter, size_t ctr_offset)
{
	for (int i = BEE_AES_BLOCK_SIZE - 1; i >= (int)ctr_offset; i--) {
		if (++counter[i] != 0) {
			break;
		}
	}
}

static int bee_aes_block_mode_crypt(const struct bee_aes_sessn_state *s, const uint8_t *in,
				    uint8_t *out, size_t len, const uint8_t *iv)
{
	T_HW_AES_MODE mode = bee_aes_hw_mode(s->mode);
	bool decrypt = (s->op == CRYPTO_CIPHER_OP_DECRYPT);
	uint8_t iv_state[BEE_AES_BLOCK_SIZE];
	int ret;

	if ((len % BEE_AES_BLOCK_SIZE) != 0U) {
		return -EINVAL;
	}

	if (iv) {
		memcpy(iv_state, iv, BEE_AES_BLOCK_SIZE);
	}

	while (len > 0U) {
		ret = bee_aes_hw_block(s, mode, in, out, iv ? iv_state : NULL, decrypt);
		if (ret != 0) {
			return ret;
		}

		if (s->mode == CRYPTO_CIPHER_MODE_CBC) {
			if (decrypt) {
				memcpy(iv_state, in, BEE_AES_BLOCK_SIZE);
			} else {
				memcpy(iv_state, out, BEE_AES_BLOCK_SIZE);
			}
		}

		in += BEE_AES_BLOCK_SIZE;
		out += BEE_AES_BLOCK_SIZE;
		len -= BEE_AES_BLOCK_SIZE;
	}

	return 0;
}

static int bee_aes_cfb_crypt(const struct bee_aes_sessn_state *s, const uint8_t *in, uint8_t *out,
			     size_t len, const uint8_t *iv)
{
	bool decrypt = (s->op == CRYPTO_CIPHER_OP_DECRYPT);
	uint8_t iv_state[BEE_AES_BLOCK_SIZE];
	uint8_t zero[BEE_AES_BLOCK_SIZE] = {0};
	int ret;

	memcpy(iv_state, iv, BEE_AES_BLOCK_SIZE);

	while (len > 0U) {
		uint8_t stream[BEE_AES_BLOCK_SIZE];
		size_t chunk = MIN(len, BEE_AES_BLOCK_SIZE);

		ret = bee_aes_hw_block(s, AES_MODE_CFB, zero, stream, iv_state, decrypt);
		if (ret != 0) {
			return ret;
		}

		for (size_t i = 0; i < chunk; i++) {
			out[i] = in[i] ^ stream[i];
		}

		if (chunk == BEE_AES_BLOCK_SIZE) {
			if (decrypt) {
				memcpy(iv_state, in, BEE_AES_BLOCK_SIZE);
			} else {
				memcpy(iv_state, out, BEE_AES_BLOCK_SIZE);
			}
		}

		in += chunk;
		out += chunk;
		len -= chunk;
	}

	return 0;
}

static int bee_aes_ofb_crypt(const struct bee_aes_sessn_state *s, const uint8_t *in, uint8_t *out,
			     size_t len, const uint8_t *iv)
{
	bool decrypt = (s->op == CRYPTO_CIPHER_OP_DECRYPT);
	uint8_t iv_state[BEE_AES_BLOCK_SIZE];
	uint8_t zero[BEE_AES_BLOCK_SIZE] = {0};
	int ret;

	memcpy(iv_state, iv, BEE_AES_BLOCK_SIZE);

	while (len > 0U) {
		uint8_t stream[BEE_AES_BLOCK_SIZE];
		size_t chunk = MIN(len, BEE_AES_BLOCK_SIZE);

		ret = bee_aes_hw_block(s, AES_MODE_OFB, zero, stream, iv_state, decrypt);
		if (ret != 0) {
			return ret;
		}

		for (size_t i = 0; i < chunk; i++) {
			out[i] = in[i] ^ stream[i];
		}

		memcpy(iv_state, stream, BEE_AES_BLOCK_SIZE);
		in += chunk;
		out += chunk;
		len -= chunk;
	}

	return 0;
}

static int bee_aes_ctr_crypt(const struct bee_aes_sessn_state *s, const uint8_t *in, uint8_t *out,
			     size_t len, const uint8_t *iv, uint32_t ctr_len_bits)
{
	size_t ctr_bytes;
	size_t iv_bytes;
	uint8_t counter[BEE_AES_BLOCK_SIZE];
	int ret;

	if (ctr_len_bits == 0U || (ctr_len_bits % 8U) != 0U || ctr_len_bits > 128U) {
		LOG_ERR("CTR: invalid counter length %u bits", ctr_len_bits);
		return -EINVAL;
	}

	ctr_bytes = ctr_len_bits / 8U;
	iv_bytes = BEE_AES_BLOCK_SIZE - ctr_bytes;

	memset(counter, 0, sizeof(counter));
	memcpy(counter, iv, iv_bytes);

	while (len > 0U) {
		uint8_t stream[BEE_AES_BLOCK_SIZE];
		size_t chunk = MIN(len, BEE_AES_BLOCK_SIZE);

		/* Match the Realtek SDK CTR wrapper: generate the CTR keystream by
		 * encrypting the counter block with AES-ECB, then XOR in software.
		 * Do not call the HW AES_MODE_CTR path here; its counter semantics do
		 * not match Zephyr's split [nonce || counter] CTR test vectors.
		 */
		ret = bee_aes_hw_block(s, AES_MODE_ECB, counter, stream, NULL, false);
		if (ret != 0) {
			return ret;
		}

		for (size_t i = 0; i < chunk; i++) {
			out[i] = in[i] ^ stream[i];
		}

		bee_aes_ctr_inc(counter, iv_bytes);
		in += chunk;
		out += chunk;
		len -= chunk;
	}

	return 0;
}

static int bee_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct bee_aes_sessn_state *s = ctx->drv_sessn_state;
	int ret;

	if (!s || pkt->in_len < 0 || !pkt->in_buf || !pkt->out_buf) {
		return -EINVAL;
	}

	k_mutex_lock(&bee_aes_lock, K_FOREVER);
	ret = bee_aes_block_mode_crypt(s, pkt->in_buf, pkt->out_buf, pkt->in_len, NULL);
	k_mutex_unlock(&bee_aes_lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int bee_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct bee_aes_sessn_state *s = ctx->drv_sessn_state;
	bool decrypt;
	bool prefix_iv;
	const uint8_t *in;
	uint8_t *out;
	size_t len;
	int ret;

	if (!s || pkt->in_len < 0) {
		return -EINVAL;
	}

	decrypt = (s->op == CRYPTO_CIPHER_OP_DECRYPT);
	prefix_iv = (ctx->flags & CAP_NO_IV_PREFIX) == 0U;
	in = pkt->in_buf;
	out = pkt->out_buf;
	len = pkt->in_len;

	if (!iv) {
		return -EINVAL;
	}

	if (!decrypt && prefix_iv) {
		memcpy(out, iv, BEE_AES_BLOCK_SIZE);
		out += BEE_AES_BLOCK_SIZE;
	} else if (decrypt && prefix_iv) {
		if (len < BEE_AES_BLOCK_SIZE) {
			return -EINVAL;
		}
		iv = (uint8_t *)in;
		in += BEE_AES_BLOCK_SIZE;
		len -= BEE_AES_BLOCK_SIZE;
	}

	k_mutex_lock(&bee_aes_lock, K_FOREVER);
	ret = bee_aes_block_mode_crypt(s, in, out, len, iv);
	k_mutex_unlock(&bee_aes_lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
		if (!decrypt && prefix_iv) {
			pkt->out_len += BEE_AES_BLOCK_SIZE;
		} else if (decrypt && prefix_iv) {
			pkt->out_len -= BEE_AES_BLOCK_SIZE;
		}
	}

	return ret;
}

static int bee_aes_cfb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct bee_aes_sessn_state *s = ctx->drv_sessn_state;
	int ret;

	if (!s || pkt->in_len < 0 || !pkt->in_buf || !pkt->out_buf || !iv) {
		return -EINVAL;
	}

	k_mutex_lock(&bee_aes_lock, K_FOREVER);
	ret = bee_aes_cfb_crypt(s, pkt->in_buf, pkt->out_buf, pkt->in_len, iv);
	k_mutex_unlock(&bee_aes_lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int bee_aes_ofb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct bee_aes_sessn_state *s = ctx->drv_sessn_state;
	int ret;

	if (!s || pkt->in_len < 0 || !pkt->in_buf || !pkt->out_buf || !iv) {
		return -EINVAL;
	}

	k_mutex_lock(&bee_aes_lock, K_FOREVER);
	ret = bee_aes_ofb_crypt(s, pkt->in_buf, pkt->out_buf, pkt->in_len, iv);
	k_mutex_unlock(&bee_aes_lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int bee_aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	struct bee_aes_sessn_state *s = ctx->drv_sessn_state;
	int ret;

	if (!s || pkt->in_len < 0 || !pkt->in_buf || !pkt->out_buf || !iv) {
		return -EINVAL;
	}

	k_mutex_lock(&bee_aes_lock, K_FOREVER);
	ret = bee_aes_ctr_crypt(s, pkt->in_buf, pkt->out_buf, pkt->in_len, iv,
				ctx->mode_params.ctr_info.ctr_len);
	k_mutex_unlock(&bee_aes_lock);

	if (ret == 0) {
		pkt->out_len = pkt->in_len;
	}

	return ret;
}

static int bee_aes_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX;
}

static int bee_aes_begin_session(const struct device *dev, struct cipher_ctx *ctx,
				 enum cipher_algo algo, enum cipher_mode mode,
				 enum cipher_op op_type)
{
	struct bee_aes_sessn_state *s;

	ARG_UNUSED(dev);

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algorithm %d", algo);
		return -ENOTSUP;
	}

	if (mode != CRYPTO_CIPHER_MODE_ECB && mode != CRYPTO_CIPHER_MODE_CBC &&
	    mode != CRYPTO_CIPHER_MODE_CFB && mode != CRYPTO_CIPHER_MODE_OFB &&
	    mode != CRYPTO_CIPHER_MODE_CTR) {
		LOG_ERR("Unsupported mode %d", mode);
		return -ENOTSUP;
	}

	if (ctx->keylen != 16U && ctx->keylen != 32U) {
		LOG_ERR("Unsupported key length %zu (must be 16 or 32)", ctx->keylen);
		return -EINVAL;
	}

	s = bee_aes_sessn_alloc();
	if (!s) {
		return -ENOMEM;
	}

	s->key_len = ctx->keylen;
	s->mode = mode;
	s->op = op_type;
	memcpy(s->key, ctx->key.bit_stream, ctx->keylen);

	ctx->drv_sessn_state = s;
	ctx->ops.cipher_mode = mode;

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		ctx->ops.block_crypt_hndlr = bee_aes_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		ctx->ops.cbc_crypt_hndlr = bee_aes_cbc_op;
		break;
	case CRYPTO_CIPHER_MODE_CFB:
		ctx->ops.cfb_crypt_hndlr = bee_aes_cfb_op;
		break;
	case CRYPTO_CIPHER_MODE_OFB:
		ctx->ops.ofb_crypt_hndlr = bee_aes_ofb_op;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		ctx->ops.ctr_crypt_hndlr = bee_aes_ctr_op;
		break;
	default:
		bee_aes_sessn_free(s);
		return -ENOTSUP;
	}

	return 0;
}

static int bee_aes_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);

	bee_aes_sessn_free(ctx->drv_sessn_state);
	ctx->drv_sessn_state = NULL;

	return 0;
}

static DEVICE_API(crypto, bee_aes_crypto_api) = {
	.query_hw_caps = bee_aes_query_caps,
	.cipher_begin_session = bee_aes_begin_session,
	.cipher_free_session = bee_aes_free_session,
	.cipher_async_callback_set = NULL,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &bee_aes_crypto_api);
