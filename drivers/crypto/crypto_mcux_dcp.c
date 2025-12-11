/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_dcp

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcux_dcp, CONFIG_CRYPTO_LOG_LEVEL);

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/util.h>

#include <fsl_dcp.h>

#define CRYPTO_DCP_CIPHER_CAPS		(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS |\
					CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#define CRYPTO_DCP_HASH_CAPS		(CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

struct crypto_dcp_session {
	dcp_handle_t handle;
	dcp_hash_ctx_t hash_ctx;
	bool in_use;
};

struct crypto_dcp_config {
	DCP_Type *base;
};

struct crypto_dcp_data {
	struct crypto_dcp_session sessions[CONFIG_CRYPTO_MCUX_DCP_MAX_SESSION];
};

/* Helper function to convert common FSL error status codes to errno codes */
static inline int fsl_to_errno(status_t status)
{
	switch (status) {
	case kStatus_Success:
		return 0;
	case kStatus_InvalidArgument:
		return -EINVAL;
	case kStatus_Timeout:
		return -EAGAIN;
	}

	return -1;
}

static struct crypto_dcp_session *get_session(const struct device *dev)
{
	struct crypto_dcp_data *data = dev->data;

	for (size_t i = 0; i < CONFIG_CRYPTO_MCUX_DCP_MAX_SESSION; ++i) {
		if (!data->sessions[i].in_use) {
			data->sessions[i].in_use = true;

			return &data->sessions[i];
		}
	}

	return NULL;
}

static inline void free_session(struct crypto_dcp_session *session)
{
	session->in_use = false;
}

static int crypto_dcp_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CRYPTO_DCP_CIPHER_CAPS | CRYPTO_DCP_HASH_CAPS;
}

static int crypto_dcp_aes_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct crypto_dcp_config *cfg = ctx->device->config;
	struct crypto_dcp_session *session = ctx->drv_sessn_state;
	status_t status;
	size_t iv_bytes;
	uint8_t *p_iv, iv_loc[16];

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		/* Prefix IV to ciphertext, which is default behavior of Zephyr
		 * crypto API, unless CAP_NO_IV_PREFIX is requested.
		 */
		iv_bytes = 16U;
		memcpy(pkt->out_buf, iv, 16U);
		p_iv = iv;
	} else {
		iv_bytes = 0U;
		memcpy(iv_loc, iv, 16U);
		p_iv = iv_loc;
	}

	sys_cache_data_disable();
	status = DCP_AES_EncryptCbc(cfg->base, &session->handle, pkt->in_buf,
				    pkt->out_buf + iv_bytes, pkt->in_len, p_iv);
	sys_cache_data_enable();

	if (status != kStatus_Success) {
		return fsl_to_errno(status);
	}

	pkt->out_len = pkt->in_len + iv_bytes;

	return 0;
}

static int crypto_dcp_aes_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct crypto_dcp_config *cfg = ctx->device->config;
	struct crypto_dcp_session *session = ctx->drv_sessn_state;
	status_t status;
	size_t iv_bytes;
	uint8_t *p_iv, iv_loc[16];

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		iv_bytes = 16U;
		p_iv = iv;
	} else {
		iv_bytes = 0U;
		memcpy(iv_loc, iv, 16U);
		p_iv = iv_loc;
	}

	sys_cache_data_disable();
	status = DCP_AES_DecryptCbc(cfg->base, &session->handle, pkt->in_buf + iv_bytes,
				    pkt->out_buf, pkt->in_len, p_iv);
	sys_cache_data_enable();

	if (status != kStatus_Success) {
		return fsl_to_errno(status);
	}

	pkt->out_len = pkt->in_len - iv_bytes;

	return 0;
}

static int crypto_dcp_aes_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct crypto_dcp_config *cfg = ctx->device->config;
	struct crypto_dcp_session *session = ctx->drv_sessn_state;
	status_t status;

	sys_cache_data_disable();
	status = DCP_AES_EncryptEcb(cfg->base, &session->handle, pkt->in_buf, pkt->out_buf,
				    pkt->in_len);
	sys_cache_data_enable();

	if (status != kStatus_Success) {
		return fsl_to_errno(status);
	}

	pkt->out_len = pkt->in_len;

	return 0;
}

static int crypto_dcp_aes_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct crypto_dcp_config *cfg = ctx->device->config;
	struct crypto_dcp_session *session = ctx->drv_sessn_state;
	status_t status;

	sys_cache_data_disable();
	status = DCP_AES_DecryptEcb(cfg->base, &session->handle, pkt->in_buf, pkt->out_buf,
				    pkt->in_len);
	sys_cache_data_enable();

	if (status != kStatus_Success) {
		return fsl_to_errno(status);
	}

	pkt->out_len = pkt->in_len;

	return 0;
}

static int crypto_dcp_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
					   enum cipher_algo algo, enum cipher_mode mode,
					   enum cipher_op op_type)
{
	const struct crypto_dcp_config *cfg = dev->config;
	struct crypto_dcp_session *session;
	status_t status;

	if (algo != CRYPTO_CIPHER_ALGO_AES ||
	    (mode != CRYPTO_CIPHER_MODE_CBC && mode != CRYPTO_CIPHER_MODE_ECB)) {
		return -ENOTSUP;
	}

	if (ctx->flags & ~(CRYPTO_DCP_CIPHER_CAPS)) {
		return -ENOTSUP;
	}

	session = get_session(dev);
	if (session == NULL) {
		return -ENOSPC;
	}

	if (mode == CRYPTO_CIPHER_MODE_CBC) {
		if (op_type == CRYPTO_CIPHER_OP_DECRYPT) {
			ctx->ops.cbc_crypt_hndlr = crypto_dcp_aes_cbc_decrypt;
		} else {
			ctx->ops.cbc_crypt_hndlr = crypto_dcp_aes_cbc_encrypt;
		}
	} else {
		if (op_type == CRYPTO_CIPHER_OP_DECRYPT) {
			ctx->ops.block_crypt_hndlr = crypto_dcp_aes_ecb_decrypt;
		} else {
			ctx->ops.block_crypt_hndlr = crypto_dcp_aes_ecb_encrypt;
		}
	}

	ctx->drv_sessn_state = session;

	status = DCP_AES_SetKey(cfg->base, &session->handle, ctx->key.bit_stream, ctx->keylen);
	if (status != kStatus_Success) {
		free_session(session);
		return fsl_to_errno(status);
	}

	return 0;
}

static int crypto_dcp_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	struct crypto_dcp_session *session;

	ARG_UNUSED(dev);

	session = ctx->drv_sessn_state;
	free_session(session);

	return 0;
}

static int crypto_dcp_sha256(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct crypto_dcp_config *cfg = ctx->device->config;
	struct crypto_dcp_session *session = ctx->drv_sessn_state;
	status_t status;

	sys_cache_data_disable();
	status = DCP_HASH_Update(cfg->base, &session->hash_ctx, pkt->in_buf, pkt->in_len);
	sys_cache_data_enable();

	if (status != kStatus_Success) {
		return fsl_to_errno(status);
	}

	if (finish) {
		sys_cache_data_disable();
		status = DCP_HASH_Finish(cfg->base, &session->hash_ctx, pkt->out_buf, NULL);
		sys_cache_data_enable();
	}

	return fsl_to_errno(status);
}

static int crypto_dcp_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
					 enum hash_algo algo)
{
	const struct crypto_dcp_config *cfg = dev->config;
	struct crypto_dcp_session *session;
	status_t status;

	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		return -ENOTSUP;
	}

	if (ctx->flags & ~(CRYPTO_DCP_HASH_CAPS)) {
		return -ENOTSUP;
	}

	session = get_session(dev);
	if (session == NULL) {
		return -ENOSPC;
	}

	status = DCP_HASH_Init(cfg->base, &session->handle, &session->hash_ctx, kDCP_Sha256);
	if (status != kStatus_Success) {
		free_session(session);
		return fsl_to_errno(status);
	}

	ctx->drv_sessn_state = session;
	ctx->hash_hndlr = crypto_dcp_sha256;

	return 0;
}

static int crypto_dcp_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_dcp_session *session;

	ARG_UNUSED(dev);

	session = ctx->drv_sessn_state;
	free_session(session);

	return 0;
}

static int crypto_dcp_init(const struct device *dev)
{
	const struct crypto_dcp_config *cfg = dev->config;
	struct crypto_dcp_data *data = dev->data;
	dcp_config_t hal_cfg;

	DCP_GetDefaultConfig(&hal_cfg);

	DCP_Init(cfg->base, &hal_cfg);

	/* Assign unique channels/key slots to each session */
	for (size_t i = 0; i < CONFIG_CRYPTO_MCUX_DCP_MAX_SESSION; ++i) {
		data->sessions[i].in_use = false;
		data->sessions[i].handle.channel = kDCP_Channel0 << i;
		data->sessions[i].handle.keySlot = kDCP_KeySlot0 + i;
		data->sessions[i].handle.swapConfig = kDCP_NoSwap;
	}

	return 0;
}

static DEVICE_API(crypto, crypto_dcp_api) = {
	.query_hw_caps = crypto_dcp_query_hw_caps,
	.cipher_begin_session = crypto_dcp_cipher_begin_session,
	.cipher_free_session = crypto_dcp_cipher_free_session,
	.hash_begin_session = crypto_dcp_hash_begin_session,
	.hash_free_session = crypto_dcp_hash_free_session,
};

#define CRYPTO_DCP_DEFINE(inst)									\
	static const struct crypto_dcp_config crypto_dcp_config_##inst = {			\
		.base = (DCP_Type *)DT_INST_REG_ADDR(inst),					\
	};											\
	static struct crypto_dcp_data crypto_dcp_data_##inst;					\
	DEVICE_DT_INST_DEFINE(inst, crypto_dcp_init, NULL,					\
			      &crypto_dcp_data_##inst, &crypto_dcp_config_##inst,		\
			      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, &crypto_dcp_api);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_DCP_DEFINE)
