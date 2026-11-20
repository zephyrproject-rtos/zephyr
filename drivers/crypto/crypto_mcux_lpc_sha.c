/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_sha

#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <fsl_sha.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_mcux_lpc_sha, CONFIG_CRYPTO_LOG_LEVEL);

#define CRYPTO_MCUX_LPC_SHA256_DIGEST_SIZE 32U
#define CRYPTO_MCUX_LPC_SHA_BLOCK_SIZE     64U

/*
 * SHA_Update() lets the engine fetch suitably placed input itself and programs
 * the block count into the 11 bit MEMCTRL COUNT field without a bound, so it
 * is fed at most this much per call.
 */
#define CRYPTO_MCUX_LPC_SHA_MAX_UPDATE (1024U * 64U)

struct crypto_mcux_lpc_sha_config {
	SHA_Type *base;
};

/*
 * The engine keeps the running digest in hardware rather than in the context
 * that fsl_sha.c hands around, so two interleaved hashes would corrupt each
 * other. Only one session is handed out at a time.
 */
struct crypto_mcux_lpc_sha_data {
	struct k_mutex lock;
	sha_ctx_t sha_ctx;
	/* Bytes fsl_sha.c holds in its partial-block buffer, 0 to 64. */
	size_t buffered;
	bool in_use;
};

static status_t crypto_mcux_lpc_sha_restart(const struct crypto_mcux_lpc_sha_config *config,
					    struct crypto_mcux_lpc_sha_data *data)
{
	data->buffered = 0U;

	return SHA_Init(config->base, &data->sha_ctx, kSHA_Sha256);
}

/*
 * When fsl_sha.c holds a partial block, an update that lets the engine fetch
 * the input itself first tops the block up from the input and then programs
 * the engine with the input address moved by that amount, which is not word
 * aligned unless the partial block is. Topping the block up to exactly one
 * full block in a call of its own keeps every address the engine fetches
 * from at the alignment of the caller's buffer.
 */
static size_t crypto_mcux_lpc_sha_next_chunk(const struct crypto_mcux_lpc_sha_data *data,
					     size_t remaining)
{
	if ((data->buffered != 0U) && (data->buffered != CRYPTO_MCUX_LPC_SHA_BLOCK_SIZE)) {
		return MIN(remaining, CRYPTO_MCUX_LPC_SHA_BLOCK_SIZE - data->buffered);
	}

	return MIN(remaining, CRYPTO_MCUX_LPC_SHA_MAX_UPDATE);
}

/* Mirror of the fill level fsl_sha.c keeps after SHA_Update() consumed @p len bytes. */
static void crypto_mcux_lpc_sha_account(struct crypto_mcux_lpc_sha_data *data, size_t len)
{
	if ((data->buffered + len) <= CRYPTO_MCUX_LPC_SHA_BLOCK_SIZE) {
		data->buffered += len;
	} else {
		data->buffered = (data->buffered + len) % CRYPTO_MCUX_LPC_SHA_BLOCK_SIZE;
	}
}

static int crypto_mcux_lpc_sha_compute(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct device *dev = ctx->device;
	const struct crypto_mcux_lpc_sha_config *config = dev->config;
	struct crypto_mcux_lpc_sha_data *data = dev->data;
	size_t digest_size = CRYPTO_MCUX_LPC_SHA256_DIGEST_SIZE;
	status_t status;
	const uint8_t *in = pkt->in_buf;
	size_t remaining = pkt->in_len;
	int ret = 0;

	if (finish && (pkt->out_buf == NULL)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->in_use || (ctx->drv_sessn_state != data)) {
		ret = -EINVAL;
		goto out;
	}

	while (remaining > 0U) {
		size_t chunk = crypto_mcux_lpc_sha_next_chunk(data, remaining);

		status = SHA_Update(config->base, &data->sha_ctx, in, chunk);
		if (status != kStatus_Success) {
			LOG_ERR("SHA_Update failed: %d", (int)status);
			ret = -EIO;
			goto out;
		}
		crypto_mcux_lpc_sha_account(data, chunk);
		in += chunk;
		remaining -= chunk;
	}

	if (!finish) {
		ctx->started = true;
		goto out;
	}

	status = SHA_Finish(config->base, &data->sha_ctx, pkt->out_buf, &digest_size);
	if (status != kStatus_Success) {
		LOG_ERR("SHA_Finish failed: %d", (int)status);
		ret = -EIO;
	}

	/* Start a fresh context for reuse, also after a failed finish. */
	ctx->started = false;
	status = crypto_mcux_lpc_sha_restart(config, data);
	if (status != kStatus_Success) {
		LOG_ERR("SHA_Init failed: %d", (int)status);
		ret = -EIO;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int crypto_mcux_lpc_sha_begin_session(const struct device *dev, struct hash_ctx *ctx,
					     enum hash_algo algo)
{
	const struct crypto_mcux_lpc_sha_config *config = dev->config;
	struct crypto_mcux_lpc_sha_data *data = dev->data;
	status_t status;
	int ret = 0;

	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("unsupported hash algorithm: %d", algo);
		return -ENOTSUP;
	}

	if ((ctx->flags & ~(CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS)) != 0U) {
		LOG_ERR("unsupported session flags: 0x%x", ctx->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->in_use) {
		ret = -EBUSY;
		goto out;
	}

	status = crypto_mcux_lpc_sha_restart(config, data);
	if (status != kStatus_Success) {
		LOG_ERR("SHA_Init failed: %d", (int)status);
		ret = -EIO;
		goto out;
	}

	data->in_use = true;
	ctx->drv_sessn_state = data;
	ctx->hash_hndlr = crypto_mcux_lpc_sha_compute;
	ctx->started = false;

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int crypto_mcux_lpc_sha_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_mcux_lpc_sha_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->in_use || (ctx->drv_sessn_state != data)) {
		ret = -EINVAL;
	} else {
		data->in_use = false;
		ctx->drv_sessn_state = NULL;
		ctx->started = false;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int crypto_mcux_lpc_sha_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS;
}

static int crypto_mcux_lpc_sha_init(const struct device *dev)
{
	const struct crypto_mcux_lpc_sha_config *config = dev->config;
	struct crypto_mcux_lpc_sha_data *data = dev->data;

	k_mutex_init(&data->lock);
	SHA_ClkInit(config->base);

	return 0;
}

static DEVICE_API(crypto, crypto_mcux_lpc_sha_api) = {
	.hash_begin_session = crypto_mcux_lpc_sha_begin_session,
	.hash_free_session = crypto_mcux_lpc_sha_free_session,
	.query_hw_caps = crypto_mcux_lpc_sha_query_caps,
};

static struct crypto_mcux_lpc_sha_data crypto_mcux_lpc_sha_data_0;

static const struct crypto_mcux_lpc_sha_config crypto_mcux_lpc_sha_config_0 = {
	.base = (SHA_Type *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, crypto_mcux_lpc_sha_init, NULL, &crypto_mcux_lpc_sha_data_0,
		      &crypto_mcux_lpc_sha_config_0, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &crypto_mcux_lpc_sha_api);
