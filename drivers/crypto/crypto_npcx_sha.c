/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_sha

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha_npcx, CONFIG_CRYPTO_LOG_LEVEL);

#define NPCX_HASH_CAPS_SUPPORT	(CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)
#define NPCX_SHA256_HANDLE_SIZE 212
#define NPCX_SHA_MAX_SESSION	1

/* The status code returns from Nuvoton Cryptographic Library ROM APIs */
enum ncl_status {
	NCL_STATUS_OK = 0xA5A5,
	NCL_STATUS_FAIL = 0x5A5A,
	NCL_STATUS_INVALID_PARAM = 0x02,
	NCL_STATUS_PARAM_NOT_SUPPORTED,
	NCL_STATUS_SYSTEM_BUSY,
	NCL_STATUS_AUTHENTICATION_FAIL,
	NCL_STATUS_NO_RESPONSE,
	NCL_STATUS_HARDWARE_ERROR,
};
enum ncl_sha_type {
	NCL_SHA_TYPE_2_256 = 0,
	NCL_SHA_TYPE_2_384 = 1,
	NCL_SHA_TYPE_2_512 = 2,
	NCL_SHA_TYPE_NUM
};

/* The following table holds the function pointer for each SHA API in NPCX ROM. */
struct npcx_ncl_sha {
	/* Get the SHA context size required by SHA APIs. */
	uint32_t (*get_context_size)(void);
	/* Initial SHA context. */
	enum ncl_status (*init_context)(void *ctx);
	/* Finalize SHA context. */
	enum ncl_status (*finalize_context)(void *ctx);
	/* Initiate the SHA hardware module and setups needed parameters. */
	enum ncl_status (*init)(void *ctx);
	/*
	 * Prepare the context buffer for a SHA calculation -  by loading the
	 * initial SHA-256/384/512 parameters.
	 */
	enum ncl_status (*start)(void *ctx, enum ncl_sha_type type);
	/*
	 * Updates the SHA calculation with the additional data. When the
	 * function returns, the hardware and memory buffer shall be ready to
	 * accept new data * buffers for SHA calculation and changes to the data
	 * in data buffer should no longer effect the SHA calculation.
	 */
	enum ncl_status (*update)(void *ctx, const uint8_t *data, uint32_t Len);
	/* Return the SHA result (digest.) */
	enum ncl_status (*finish)(void *ctx, uint8_t *hashDigest);
	/* Perform a complete SHA calculation */
	enum ncl_status (*calc)(void *ctx, enum ncl_sha_type type, const uint8_t *data,
				uint32_t Len, uint8_t *hashDigest);
	/* Power on/off the SHA module. */
	enum ncl_status (*power)(void *ctx, uint8_t enable);
	/* Reset the SHA hardware and terminate any in-progress operations. */
	enum ncl_status (*reset)(void *ctx);
};

/* The start address of the SHA API table. */
#define NPCX_NCL_SHA ((const struct npcx_ncl_sha *)DT_INST_REG_ADDR(0))

struct npcx_sha_context {
	uint8_t handle[NPCX_SHA256_HANDLE_SIZE];
} __aligned(4);

struct npcx_sha_session {
	struct npcx_sha_context npcx_sha_ctx;
	enum hash_algo algo;
	bool in_use;
};

struct npcx_sha_session npcx_sessions[NPCX_SHA_MAX_SESSION];

static int npcx_get_unused_session_index(void)
{
	int i;

	for (i = 0; i < NPCX_SHA_MAX_SESSION; i++) {
		if (!npcx_sessions[i].in_use) {
			npcx_sessions[i].in_use = true;
			return i;
		}
	}

	return -1;
}
static int npcx_sha_compute(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	enum ncl_status ret;
	struct npcx_sha_session *npcx_session = ctx->drv_sessn_state;
	struct npcx_sha_context *npcx_ctx = &npcx_session->npcx_sha_ctx;
	enum ncl_sha_type sha_type;

	switch (npcx_session->algo) {
	case CRYPTO_HASH_ALGO_SHA256:
		sha_type = NCL_SHA_TYPE_2_256;
		break;
	case CRYPTO_HASH_ALGO_SHA384:
		sha_type = NCL_SHA_TYPE_2_384;
		break;
	case CRYPTO_HASH_ALGO_SHA512:
		sha_type = NCL_SHA_TYPE_2_512;
		break;
	default:
		LOG_ERR("Unexpected algo: %d", npcx_session->algo);
		return -EINVAL;
	}

	if (!ctx->started) {
		ret = NPCX_NCL_SHA->start(npcx_ctx->handle, sha_type);
		if (ret != NCL_STATUS_OK) {
			LOG_ERR("Could not compute the hash, err:%d", ret);
			return -EINVAL;
		}
		ctx->started = true;
	}

	if (pkt->in_len != 0) {
		ret = NPCX_NCL_SHA->update(npcx_ctx->handle, pkt->in_buf, pkt->in_len);
		if (ret != NCL_STATUS_OK) {
			LOG_ERR("Could not update the hash, err:%d", ret);
			ctx->started = false;
			return -EINVAL;
		}
	}

	if (finish) {
		ctx->started = false;
		ret = NPCX_NCL_SHA->finish(npcx_ctx->handle, pkt->out_buf);
		if (ret != NCL_STATUS_OK) {
			LOG_ERR("Could not compute the hash, err:%d", ret);
			return -EINVAL;
		}
	}

	return 0;
}

static int npcx_hash_session_setup(const struct device *dev, struct hash_ctx *ctx,
				   enum hash_algo algo)
{
	int ctx_idx;
	struct npcx_sha_context *npcx_ctx;

	if (ctx->flags & ~(NPCX_HASH_CAPS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if ((algo != CRYPTO_HASH_ALGO_SHA256) && (algo != CRYPTO_HASH_ALGO_SHA384) &&
	    (algo != CRYPTO_HASH_ALGO_SHA512)) {
		LOG_ERR("Unsupported algo: %d", algo);
		return -EINVAL;
	}

	ctx_idx = npcx_get_unused_session_index();
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	npcx_sessions[ctx_idx].algo = algo;

	ctx->drv_sessn_state = &npcx_sessions[ctx_idx];
	ctx->started = false;
	ctx->hash_hndlr = npcx_sha_compute;

	npcx_ctx = &npcx_sessions[ctx_idx].npcx_sha_ctx;
	NPCX_NCL_SHA->init_context(npcx_ctx->handle);
	NPCX_NCL_SHA->power(npcx_ctx->handle, 1);
	NPCX_NCL_SHA->init(npcx_ctx->handle);
	NPCX_NCL_SHA->reset(npcx_ctx->handle);

	return 0;
}

static int npcx_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct npcx_sha_session *npcx_session = ctx->drv_sessn_state;
	struct npcx_sha_context *npcx_ctx = &npcx_session->npcx_sha_ctx;

	NPCX_NCL_SHA->reset(npcx_ctx->handle);
	NPCX_NCL_SHA->power(npcx_ctx->handle, 0);
	NPCX_NCL_SHA->finalize_context(npcx_ctx->handle);
	npcx_session->in_use = false;

	return 0;
}

static int npcx_query_caps(const struct device *dev)
{
	return NPCX_HASH_CAPS_SUPPORT;
}

static struct crypto_driver_api npcx_crypto_api = {
	.hash_begin_session = npcx_hash_session_setup,
	.hash_free_session = npcx_hash_session_free,
	.query_hw_caps = npcx_query_caps,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &npcx_crypto_api);
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one 'nuvoton,npcx-sha' compatible node can be supported");
