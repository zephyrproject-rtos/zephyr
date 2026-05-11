/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT      st_stm32_crc

#include <soc.h>
#include <stm32_ll_crc.h>
#include <stm32_ll_dma.h>

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_stm32, CONFIG_CRC_LOG_LEVEL);

#ifdef CONFIG_STM32_HAL2
#define STM32_CRC_POLY_SIZE_32B		LL_CRC_POLY_SIZE_32B
#define STM32_CRC_POLY_SIZE_16B		LL_CRC_POLY_SIZE_16B
#define STM32_CRC_POLY_SIZE_8B		LL_CRC_POLY_SIZE_8B
#define STM32_CRC_POLY_SIZE_7B		LL_CRC_POLY_SIZE_7B
#else /* CONFIG_STM32_HAL2 */
#define STM32_CRC_POLY_SIZE_32B		LL_CRC_POLYLENGTH_32B
#define STM32_CRC_POLY_SIZE_16B		LL_CRC_POLYLENGTH_16B
#define STM32_CRC_POLY_SIZE_8B		LL_CRC_POLYLENGTH_8B
#define STM32_CRC_POLY_SIZE_7B		LL_CRC_POLYLENGTH_7B
#endif /* CONFIG_STM32_HAL2 */

struct crc_stm32_config {
	CRC_TypeDef *base;
	struct stm32_pclken pclken[1];
#if defined(CONFIG_CRC_STM32_DMA)
	const struct device *dmac;
	uint32_t dma_channel;
	uint32_t dma_priority;
#endif /* CONFIG_CRC_STM32_DMA */
};

struct crc_stm32_data {
	struct k_sem dev_lock;
#if defined(CONFIG_CRC_STM32_DMA)
	struct k_sem dma_sync;
	int dma_transfer_status;
	struct dma_config dma_config;
	struct dma_block_config dma_block;
#endif /* CONFIG_CRC_STM32_DMA */
};

#if defined(CONFIG_CRC_STM32_DMA)
static void crc_stm32_dma_callback(const struct device *dma, void *user_data,
				   uint32_t channel, int status)
{
	ARG_UNUSED(dma);
	ARG_UNUSED(channel);
	struct crc_stm32_data *data = user_data;

	data->dma_transfer_status = status;
	k_sem_give(&data->dma_sync);
}
#endif /* CONFIG_CRC_STM32_DMA */

static int crc_stm32_get_poly_size(struct crc_ctx *ctx, uint32_t *size)
{
	int res = 0;

	switch (ctx->type) {
	case CRC7_BE:
		*size = STM32_CRC_POLY_SIZE_7B;
		break;
	case CRC8:
		*size = STM32_CRC_POLY_SIZE_8B;
		break;
	case CRC16:
	case CRC16_CCITT:
		*size = STM32_CRC_POLY_SIZE_16B;
		break;
	case CRC32_C:
	case CRC32_IEEE:
		*size = STM32_CRC_POLY_SIZE_32B;
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	return res;
}

static void crc_stm32_release(const struct device *dev, struct crc_ctx *ctx)
{
	struct crc_stm32_data *data = dev->data;

	ctx->state = CRC_STATE_IDLE;
	k_sem_give(&data->dev_lock);
}

/*
 * CRC API implementation
 */
static int crc_stm32_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_stm32_config *config = dev->config;
	struct crc_stm32_data *data = dev->data;
	uint32_t poly_size;
	int ret;

	const uint32_t in_rev = (ctx->reversed & CRC_FLAG_REVERSE_INPUT)
		? LL_CRC_INDATA_REVERSE_BYTE
		: LL_CRC_INDATA_REVERSE_NONE;
	const uint32_t out_rev = (ctx->reversed & CRC_FLAG_REVERSE_OUTPUT)
		? LL_CRC_OUTDATA_REVERSE_BIT
		: LL_CRC_OUTDATA_REVERSE_NONE;

	/*
	 * Parse configuration provided in the context. This does not depend
	 * on hardware so it can be done before acquiring CRC peripheral lock.
	 */
	if ((ctx->polynomial & 0x1u) == 0u) {
		/* Even polynomials are not supported by HW */
		return -ENOTSUP;
	}

	ret = crc_stm32_get_poly_size(ctx, &poly_size);
	if (ret != 0) {
		return ret;
	}

	/* Take ownership of CRC peripheral */
	k_sem_take(&data->dev_lock, K_FOREVER);

	LL_CRC_SetInitialData(config->base, ctx->seed);
	LL_CRC_SetPolynomialSize(config->base, poly_size);
	LL_CRC_SetPolynomialCoef(config->base, ctx->polynomial);
	LL_CRC_SetInputDataReverseMode(config->base, in_rev);
	LL_CRC_SetOutputDataReverseMode(config->base, out_rev);

	/* Reset CRC unit and load seed as initial value */
	LL_CRC_ResetCRCCalculationUnit(config->base);

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_stm32_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
				 size_t bufsize)
{
	const struct crc_stm32_config *config = dev->config;

	/* Ensure CRC calculation has been initialized by crc_begin() */
	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

#if defined(CONFIG_CRC_STM32_DMA)
	/*
	 * Perform a DMA transfer if the buffer size is
	 * as large as threshold specified in Kconfig.
	 */
	if (bufsize >= CONFIG_CRC_STM32_DMA_THRESHOLD) {
		struct crc_stm32_data *data = dev->data;
		int ret;

		/* Ensure buffer contents are visible by DMA */
		sys_cache_data_flush_range((void *)buffer, bufsize);

		k_sem_reset(&data->dma_sync);

		ret = dma_reload(config->dmac, config->dma_channel,
			(uintptr_t)buffer, (uintptr_t)&config->base->DR, bufsize);
		if (ret != 0) {
			LOG_ERR("dma_reload() failed: %d", ret);
			return ret;
		}

		/* Wait until DMA transfer is complete */
		k_sem_take(&data->dma_sync, K_FOREVER);

		if (data->dma_transfer_status != DMA_STATUS_COMPLETE) {
			LOG_ERR("DMA transfer failure (status=%d)", data->dma_transfer_status);

			/* Abort CRC computation and release device in case of failure */
			crc_stm32_release(dev, ctx);

			return -EIO;
		}

		return 0;
	} /* else: perform a busy CPU copy */
#endif /* CONFIG_CRC_STM32_DMA */
	const uint8_t *buf = buffer;

	for (size_t i = 0; i < bufsize; i++) {
		LL_CRC_FeedData8(config->base, buf[i]);
	}

	return 0;
}

static int crc_stm32_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_stm32_config *config = dev->config;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	ctx->result = LL_CRC_ReadData32(config->base);

	/* Post-process result as required by some algorithms */
	if (ctx->type == CRC32_IEEE) {
		ctx->result ^= 0xFFFFFFFFu;
	}

	crc_stm32_release(dev, ctx);

	return 0;
}

static DEVICE_API(crc, crc_stm32_driver_api) = {
	.begin = crc_stm32_begin,
	.update = crc_stm32_update,
	.finish = crc_stm32_finish,
};

static int crc_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct crc_stm32_config *config = dev->config;
	struct crc_stm32_data *data = dev->data;
	int ret;

	ret = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
	if (ret != 0) {
		LOG_ERR("Failed to enable CRC clock: %d", ret);
		return ret;
	}

	k_sem_init(&data->dev_lock, 1, 1);

#if defined(CONFIG_CRC_STM32_DMA)
	k_sem_init(&data->dma_sync, 0, 1);

	/*
	 * source/destination address parameters are
	 * irrelevant as they will be overwritten when
	 * we call dma_reload() to start the transfer
	 *
	 * On the other hand, adjustment options are
	 * loaded upon config() and must be valid.
	 */
	data->dma_block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->dma_block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	data->dma_config = (struct dma_config) {
		.channel_priority = config->dma_priority,

		/*
		 * As far as the DMA is concerned, this is a M2M transfer
		 * because there is no HW handshake between CRC and DMA.
		 */
		.channel_direction = MEMORY_TO_MEMORY,

		/*
		 * Perform byte-sized transfers as the CRC unit can only consume
		 * 8 bits per cycle (the DMA can do up to one transfer per cycle).
		 * This also avoids the need to handle non-aligned buffers.
		 */
		.source_data_size = 1,
		.source_burst_length = 1,
		.dest_data_size = 1,
		.dest_burst_length = 1,

		/*
		 * Initial block configuration is irrelevant
		 * as we will override it anyways (but the
		 * pointer must be non-NULL?!)
		 */
		.block_count = 1,
		.head_block = &data->dma_block,

		.user_data = data,
		.dma_callback = crc_stm32_dma_callback,
	};

	ret = dma_config(config->dmac, config->dma_channel, &data->dma_config);
	if (ret != 0) {
		LOG_ERR("dma_config() failed: %d", ret);
		return ret;
	}
#endif /* CONFIG_CRC_STM32_DMA */

	return 0;
}

#define CRC_STM32_INIT(inst)								\
	static const struct crc_stm32_config crc_stm32_config_##inst = {		\
		.base = (void *)DT_INST_REG_ADDR(inst),					\
		.pclken = STM32_DT_INST_CLOCKS(inst),					\
		IF_ENABLED(CONFIG_CRC_STM32_DMA, (					\
			.dmac = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(inst, 0)),	\
			.dma_channel = DT_INST_DMAS_CELL_BY_IDX(inst, 0, channel),	\
			.dma_priority = STM32_DMA_CONFIG_PRIORITY(			\
				STM32_DMA_CHANNEL_CONFIG_BY_IDX(inst, 0)),		\
		))									\
	};										\
											\
	static struct crc_stm32_data crc_stm32_data_##inst;				\
											\
	DEVICE_DT_INST_DEFINE(inst, crc_stm32_init, NULL,				\
			      &crc_stm32_data_##inst, &crc_stm32_config_##inst,		\
			      POST_KERNEL, CONFIG_CRC_DRIVER_INIT_PRIORITY,		\
			      &crc_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_STM32_INIT)
