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
#include <zephyr/irq.h>
#include <soc.h>

#include "crypto_stm32_hash_priv.h"

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_stm32_hash);

#define DT_DRV_COMPAT st_stm32_hash

#if !defined(CONFIG_STM32_HAL2) && DT_INST_IRQ_HAS_IDX(0, 0)
#define STM32_HASH_USE_IT
static void stm32_hash_isr(const struct device *dev);
#endif

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
#if DT_INST_PROP(0, st_has_sha384_algorithm)
	case CRYPTO_HASH_ALGO_SHA384:
		hash_cfg.algorithm = HAL_HASH_ALGO_SHA384;
		break;
#endif /* DT_INST_PROP(0, st_has_sha384_algorithm) */
#if DT_INST_PROP(0, st_has_sha512_algorithm)
	case CRYPTO_HASH_ALGO_SHA512:
		hash_cfg.algorithm = HAL_HASH_ALGO_SHA512;
		break;
#endif /* DT_INST_PROP(0, st_has_sha512_algorithm) */
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
#elif defined(HAL_HASH_VERSION) && HAL_HASH_VERSION == 200
	HASH_ConfigTypeDef hash_cfg = {
		.DataType = HASH_BYTE_SWAP,
	};

	switch (session->algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		hash_cfg.Algorithm = HASH_ALGOSELECTION_SHA224;
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		hash_cfg.Algorithm = HASH_ALGOSELECTION_SHA256;
		break;
#if DT_INST_PROP(0, st_has_sha384_algorithm)
	case CRYPTO_HASH_ALGO_SHA384:
		hash_cfg.Algorithm = HASH_ALGOSELECTION_SHA384;
		break;
#endif /* DT_INST_PROP(0, st_has_sha384_algorithm) */
#if DT_INST_PROP(0, st_has_sha512_algorithm)
	case CRYPTO_HASH_ALGO_SHA512:
		hash_cfg.Algorithm = HASH_ALGOSELECTION_SHA512;
		break;
#endif /* DT_INST_PROP(0, st_has_sha512_algorithm) */
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

	status = HAL_HASH_Start(&data->hhash, pkt->in_buf, pkt->in_len, pkt->out_buf,
				  HAL_MAX_DELAY);
#else /* CONFIG_STM32_HAL2 */
#if defined(STM32_HASH_USE_IT)
	if ((session->algo == CRYPTO_HASH_ALGO_SHA224) ||
	    (session->algo == CRYPTO_HASH_ALGO_SHA256)) {
		k_sem_reset(&data->complete_sem);

		if (session->algo == CRYPTO_HASH_ALGO_SHA224) {
			status = HAL_HASHEx_SHA224_Start_IT(&data->hhash, pkt->in_buf, pkt->in_len,
							    pkt->out_buf);
		} else {
			status = HAL_HASHEx_SHA256_Start_IT(&data->hhash, pkt->in_buf, pkt->in_len,
							    pkt->out_buf);
		}

		if (status == HAL_OK) {
			/*
			 * HAL_HASHEx_SHAxxx_Start_IT() arms the peripheral and returns
			 * immediately; completion (or error) is signalled asynchronously
			 * from the HASH ISR via HAL_HASH_DgstCpltCallback()/
			 * HAL_HASH_ErrorCallback(), both of which give complete_sem.
			 */
			if (k_sem_take(&data->complete_sem, K_MSEC(1000)) != 0) {
				status = HAL_TIMEOUT;
			} else if (data->hhash.State == HAL_HASH_STATE_READY) {
				/*
				 * hhash->Phase is left at HAL_HASH_PHASE_PROCESS after a
				 * completed computation. Since this driver only performs
				 * one-shot, non-multipart hashing, force it back to READY here
				 * so the next call re-triggers HASH_CR_INIT instead of silently
				 * continuing this message's internal digest state.
				 */
				data->hhash.Phase = HAL_HASH_PHASE_READY;
				k_sem_give(&data->device_sem);
				LOG_DBG("Hash computation successful (IT)");
				return 0;
			} else {
				status = HAL_ERROR;
			}
		}

		LOG_WRN("HASH IT path failed (status=%d), falling back to polling", status);
		data->hhash.State = HAL_HASH_STATE_READY;
		data->hhash.Phase = HAL_HASH_PHASE_READY;
	}
#endif /* DT_INST_IRQ_HAS_IDX(0, 0) */

	switch (session->algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		status = HAL_HASHEx_SHA224_Start(&data->hhash, pkt->in_buf, pkt->in_len,
						 pkt->out_buf, HAL_MAX_DELAY);
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		status = HAL_HASHEx_SHA256_Start(&data->hhash, pkt->in_buf, pkt->in_len,
						 pkt->out_buf, HAL_MAX_DELAY);
		break;
#if DT_INST_PROP(0, st_has_sha384_algorithm)
	case CRYPTO_HASH_ALGO_SHA384:
		status = HAL_HASHEx_SHA384_Start(&data->hhash, pkt->in_buf, pkt->in_len,
						 pkt->out_buf, HAL_MAX_DELAY);
		break;
#endif /* DT_INST_PROP(0, st_has_sha384_algorithm) */
#if DT_INST_PROP(0, st_has_sha512_algorithm)
	case CRYPTO_HASH_ALGO_SHA512:
		status = HAL_HASHEx_SHA512_Start(&data->hhash, pkt->in_buf, pkt->in_len,
						 pkt->out_buf, HAL_MAX_DELAY);
		break;
#endif /* DT_INST_PROP(0, st_has_sha512_algorithm) */
	default:
		k_sem_give(&data->device_sem);
		LOG_ERR("Unsupported algorithm in handler: %d", session->algo);
		return -ENOTSUP;
	}

	/*
	 * The HAL leaves hhash->Phase at HAL_HASH_PHASE_PROCESS after a
	 * completed computation (it only resets to READY for a genuinely new
	 * message/session). Since this driver only performs one-shot,
	 * non-multipart hashing, force it back to READY here so the next
	 * hash_compute() call on this (or another) session re-triggers
	 * HASH_CR_INIT instead of silently continuing this message's
	 * internal digest state - which would produce a wrong digest with
	 * no error reported.
	 */
	data->hhash.Phase = HAL_HASH_PHASE_READY;
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
#if DT_INST_PROP(0, st_has_sha384_algorithm)
	case CRYPTO_HASH_ALGO_SHA384:
#endif /* DT_INST_PROP(0, st_has_sha384_algorithm) */
#if DT_INST_PROP(0, st_has_sha512_algorithm)
	case CRYPTO_HASH_ALGO_SHA512:
#endif /* DT_INST_PROP(0, st_has_sha512_algorithm) */
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

#if defined(STM32_HASH_USE_IT)
	k_sem_init(&data->complete_sem, 0, 1);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), stm32_hash_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
#endif

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
#elif defined(HAL_HASH_VERSION) && HAL_HASH_VERSION == 200
	data->hhash.Instance = cfg->base;
	if (HAL_HASH_Init(&data->hhash) != HAL_OK) {
		LOG_ERR("Peripheral init error");
		return -EIO;
	}
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

#if defined(STM32_HASH_USE_IT)
static void stm32_hash_isr(const struct device *dev)
{
	struct crypto_stm32_hash_data *data = CRYPTO_STM32_HASH_DATA(dev);

	HAL_HASH_IRQHandler(&data->hhash);
}

void HAL_HASH_DgstCpltCallback(HASH_HandleTypeDef *hhash)
{
	ARG_UNUSED(hhash);
	k_sem_give(&crypto_stm32_hash_dev_data.complete_sem);
}

void HAL_HASH_ErrorCallback(HASH_HandleTypeDef *hhash)
{
	ARG_UNUSED(hhash);
	k_sem_give(&crypto_stm32_hash_dev_data.complete_sem);
}
#endif /* DT_INST_IRQ_HAS_IDX(0, 0) */

static const struct crypto_stm32_hash_config crypto_stm32_hash_dev_config = {
	.base = (void *)DT_INST_REG_ADDR(0),
	.reset = RESET_DT_SPEC_INST_GET(0),
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
};

DEVICE_DT_INST_DEFINE(0, crypto_stm32_hash_init, NULL, &crypto_stm32_hash_dev_data,
		      &crypto_stm32_hash_dev_config, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		      &stm32_hash_funcs);
