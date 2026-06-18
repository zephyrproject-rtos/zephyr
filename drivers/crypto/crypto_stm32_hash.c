/*
 * Copyright (c) 2025 Bayrem Gharsellaoui
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <soc.h>

#include "crypto_stm32_hash_priv.h"

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_stm32_hash);

#define DT_DRV_COMPAT st_stm32_hash

static struct crypto_stm32_hash_session stm32_hash_sessions[CONFIG_CRYPTO_STM32_HASH_MAX_SESSIONS];

static int crypto_stm32_hash_get_unused_session_index(const struct device *dev)
{
	struct crypto_stm32_hash_data *data = CRYPTO_STM32_HASH_DATA(dev);

	k_sem_take(&data->session_sem, K_FOREVER);

	for (int i = 0; i < CONFIG_CRYPTO_STM32_HASH_MAX_SESSIONS; i++) {
		if (!stm32_hash_sessions[i].in_use) {
			stm32_hash_sessions[i].in_use = true;
			k_sem_give(&data->session_sem);
			return i;
		}
	}

	k_sem_give(&data->session_sem);
	return -1;
}

static int stm32_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct device *dev = ctx->device;
	struct crypto_stm32_hash_data *data = CRYPTO_STM32_HASH_DATA(dev);
	struct crypto_stm32_hash_session *session = CRYPTO_STM32_HASH_SESSN(ctx);
	stm32_status_t status;

	if (!pkt || !pkt->in_buf || !pkt->out_buf) {
		LOG_ERR("Invalid packet buffers");
		return -EINVAL;
	}

	if (!finish) {
		LOG_ERR("Multipart hashing not supported yet");
		return -ENOTSUP;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

#if defined(CONFIG_STM32_HAL2)
	hal_hash_config_t hash_cfg = {
		.data_swapping = HAL_HASH_DATA_SWAP_BYTE,
	};
	uint32_t digest_size;

	switch (session->algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		hash_cfg.algorithm = HAL_HASH_ALGO_SHA224;
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		hash_cfg.algorithm = HAL_HASH_ALGO_SHA256;
		break;
	default:
		k_sem_give(&data->device_sem);
		LOG_ERR("Unsupported algorithm in handler: %d", session->algo);
		return -ENOTSUP;
	}

	status = HAL_HASH_SetConfig(&data->hhash, &hash_cfg);
	if (status != HAL_OK) {
		k_sem_give(&data->device_sem);
		LOG_ERR("HAL_HASH_SetConfig failed: %d", status);
		return -EIO;
	}

	/*
	 * In Zephyr, the output buffer is assumed to be large enough to receive
	 * the digest. The HAL parameter output_buffer_size_byte is used only for
	 * parameter validation; specify UINT32_MAX instead of the "proper value"
	 * (= per-algorithm digest size) as the end result should be the same.
	 */
	status = HAL_HASH_Compute(&data->hhash, pkt->in_buf, pkt->in_len, pkt->out_buf,
				  UINT32_MAX, &digest_size, HAL_MAX_DELAY);
	UNUSED(digest_size);
#else /* CONFIG_STM32_HAL2 */
	switch (session->algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		status = HAL_HASHEx_SHA224_Start(&data->hhash, pkt->in_buf, pkt->in_len,
						 pkt->out_buf, HAL_MAX_DELAY);
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		status = HAL_HASHEx_SHA256_Start(&data->hhash, pkt->in_buf, pkt->in_len,
						 pkt->out_buf, HAL_MAX_DELAY);
		break;
	default:
		k_sem_give(&data->device_sem);
		LOG_ERR("Unsupported algorithm in handler: %d", session->algo);
		return -ENOTSUP;
	}
#endif /* CONFIG_STM32_HAL2 */

	k_sem_give(&data->device_sem);

	if (status != HAL_OK) {
		LOG_ERR("HAL HASH computation failed (status=%d)", status);
		return -EIO;
	}

	LOG_DBG("Hash computation successful");
	return 0;
}

static int stm32_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
				    enum hash_algo algo)
{
	int ctx_idx;
	struct crypto_stm32_hash_session *session;

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
	case CRYPTO_HASH_ALGO_SHA256:
		break;
	default:
		LOG_ERR("Unsupported hash algorithm: %d", algo);
		return -ENOTSUP;
	}

	ctx_idx = crypto_stm32_hash_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	session = &stm32_hash_sessions[ctx_idx];
	memset(session->digest, 0, sizeof(session->digest));
	session->in_use = true;
	session->algo = algo;

	ctx->drv_sessn_state = session;
	ctx->hash_hndlr = stm32_hash_handler;
	ctx->started = false;

	LOG_DBG("begin_session (algo=%d)", algo);
	return 0;
}

static int stm32_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_stm32_hash_session *session = CRYPTO_STM32_HASH_SESSN(ctx);

	if (!session) {
		LOG_ERR("Tried to free a NULL session");
		return -EINVAL;
	}

	memset(session, 0, sizeof(*session));

	LOG_DBG("Session freed");
	return 0;
}

static int stm32_hash_query_caps(const struct device *dev)
{
	return (CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS);
}

static int crypto_stm32_hash_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct crypto_stm32_hash_config *cfg = CRYPTO_STM32_HASH_CFG(dev);
	struct crypto_stm32_hash_data *data = CRYPTO_STM32_HASH_DATA(dev);

	if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken) != 0) {
		LOG_ERR("Clock op failed\n");
		return -EIO;
	}

	k_sem_init(&data->device_sem, 1, 1);
	k_sem_init(&data->session_sem, 1, 1);

#if defined(CONFIG_STM32_HAL2)
	if (HAL_HASH_Init(&data->hhash, HAL_HASH) != HAL_OK) {
		LOG_ERR("Peripheral init error");
		return -EIO;
	}
	/*
	 * NOTE: we need to SetConfig before each operation,
	 * so don't bother configuring "DataType" here since
	 * it will be overwritten later.
	 */
#else /* CONFIG_STM32_HAL2 */
	data->hhash.Init.DataType = HASH_DATATYPE_8B;
	if (HAL_HASH_Init(&data->hhash) != HAL_OK) {
		LOG_ERR("Peripheral init error");
		return -EIO;
	}
#endif /* CONFIG_STM32_HAL2 */

	return 0;
}

static DEVICE_API(crypto, stm32_hash_funcs) = {
	.hash_begin_session = stm32_hash_begin_session,
	.hash_free_session = stm32_hash_free_session,
	.query_hw_caps = stm32_hash_query_caps,
};

static struct crypto_stm32_hash_data crypto_stm32_hash_dev_data = {0};

static const struct crypto_stm32_hash_config crypto_stm32_hash_dev_config = {
	.reset = RESET_DT_SPEC_INST_GET(0),
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
};

DEVICE_DT_INST_DEFINE(0, crypto_stm32_hash_init, NULL, &crypto_stm32_hash_dev_data,
		      &crypto_stm32_hash_dev_config, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &stm32_hash_funcs);
