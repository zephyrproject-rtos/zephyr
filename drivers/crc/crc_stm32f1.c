/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32f1_crc

#include <soc.h>
#include <stm32_ll_crc.h>

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/bit_rev.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_stm32f1, CONFIG_CRC_LOG_LEVEL);

/*
 * Early implementations of the CRC IP only implemented CRC-32 as defined by Ethernet, Wi-Fi, SATA,
 * MPEG-2, Gzip, HDMI and others.
 */
#define STM32_CRC_V1_POLYNOMIAL 0x04c11db7u

struct crc_stm32f1_config {
	CRC_TypeDef *base;
	struct stm32_pclken pclken[1];
};

struct crc_stm32f1_data {
	struct k_sem dev_lock;
	uint8_t nb_pending;
	uint8_t pending_bytes[3];
};

static void crc_stm32f1_release(const struct device *dev, struct crc_ctx *ctx)
{
	struct crc_stm32f1_data *data = dev->data;

	ctx->state = CRC_STATE_IDLE;
	k_sem_give(&data->dev_lock);
}

/*
 * CRC API implementation
 */
static int crc_stm32f1_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_stm32f1_config *config = dev->config;
	struct crc_stm32f1_data *data = dev->data;

	if (ctx->polynomial != STM32_CRC_V1_POLYNOMIAL) {
		return -ENOTSUP;
	}

	switch (ctx->type) {
	case CRC32_IEEE:
	case CRC32_MPEG2:
		break;
	default:
		return -ENOTSUP;
	}

	if (ctx->seed != 0xffffffffu) {
		return -ENOTSUP;
	}

	/* Take ownership of CRC peripheral */
	k_sem_take(&data->dev_lock, K_FOREVER);

	/* Reset CRC unit */
	LL_CRC_ResetCRCCalculationUnit(config->base);

	data->nb_pending = 0U;

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static void crc_stm32f1_feed_word(struct crc_ctx *ctx, CRC_TypeDef *base, uint32_t word)
{
	if ((ctx->reversed & CRC_FLAG_REVERSE_INPUT) != 0U) {
		/*
		 * STM32 is little endian anyway, but this makes both cases consistent and doesn't
		 * cost anything.
		 */
		word = sys_bit_rev32(sys_cpu_to_le32(word));
	} else {
		word = sys_cpu_to_be32(word);
	}

	LL_CRC_FeedData32(base, word);
}

static int crc_stm32f1_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			      size_t bufsize)
{
	const struct crc_stm32f1_config *config = dev->config;
	struct crc_stm32f1_data *data = dev->data;
	/* We may need an intermediate 32-bit buffer to realign data on the fly. */
	uint32_t word;
	/*
	 * The original IP can only be fed a word at a time. This is to be pointed at the remainder
	 * of the buffer.
	 */
	const void *tail;

	/* Ensure CRC calculation has been initialized by crc_begin() */
	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	if ((data->nb_pending > 0U) || !IS_ALIGNED(buffer, sizeof(uint32_t))) {
		uint8_t *buf8 = (uint8_t *)&word;
		const size_t nb_words = (bufsize + data->nb_pending) / sizeof(word);

		memcpy(buf8, data->pending_bytes, data->nb_pending);
		memcpy(buf8 + data->nb_pending, buffer,
		       MIN(bufsize, sizeof(word) - data->nb_pending));

		if (nb_words > 0U) {
			crc_stm32f1_feed_word(ctx, config->base, word);

			const uint8_t *src =
				(const uint8_t *)buffer + sizeof(word) - data->nb_pending;

			for (size_t i = 1; i < nb_words; i++) {
				memcpy(buf8, src, sizeof(word));
				crc_stm32f1_feed_word(ctx, config->base, word);
				src += sizeof(word);
			}

			tail = src;
			ptrdiff_t remaining = (ptrdiff_t)((const uint8_t *)buffer + bufsize -
							  (const uint8_t *)tail);
			data->nb_pending = (uint8_t)remaining;
		} else {
			tail = buf8;
			data->nb_pending = (uint8_t)(data->nb_pending + bufsize);
		}
	} else {
		/* Data can be fed directly from the input buffer. */
		const uint32_t *buf32 = buffer;
		const size_t nb_words = bufsize / sizeof(uint32_t);

		for (size_t i = 0U; i < nb_words; i++) {
			crc_stm32f1_feed_word(ctx, config->base, buf32[i]);
		}

		data->nb_pending = (uint8_t)(bufsize % sizeof(uint32_t));
		tail = buf32 + nb_words;
	}

	memcpy(data->pending_bytes, tail, data->nb_pending);

	return 0;
}

static int crc_stm32f1_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_stm32f1_config *config = dev->config;
	struct crc_stm32f1_data *data = dev->data;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	/*
	 * Check there are no pending bytes at this stage, as the original IP can only
	 * process full words.
	 */
	if (data->nb_pending > 0U) {
		crc_stm32f1_release(dev, ctx);
		return -ENOTSUP;
	}

	ctx->result = LL_CRC_ReadData32(config->base);

	if ((ctx->reversed & CRC_FLAG_REVERSE_OUTPUT) != 0U) {
		ctx->result = sys_bit_rev32(ctx->result);
	}

	/* Post-process result as required by some algorithms */
	if (ctx->type == CRC32_IEEE) {
		ctx->result ^= 0xFFFFFFFFu;
	}

	crc_stm32f1_release(dev, ctx);

	return 0;
}

static DEVICE_API(crc, crc_stm32f1_driver_api) = {
	.begin = crc_stm32f1_begin,
	.update = crc_stm32f1_update,
	.finish = crc_stm32f1_finish,
};

static int crc_stm32f1_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct crc_stm32f1_config *config = dev->config;
	struct crc_stm32f1_data *data = dev->data;
	int ret;

	ret = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
	if (ret != 0) {
		LOG_ERR("Failed to enable CRC clock: %d", ret);
		return ret;
	}

	k_sem_init(&data->dev_lock, 1, 1);

	return 0;
}

#define CRC_STM32_INIT(inst)                                                                       \
	static const struct crc_stm32f1_config crc_stm32f1_config_##inst = {                       \
		.base = (void *)DT_INST_REG_ADDR(inst),                                            \
		.pclken = STM32_DT_INST_CLOCKS(inst),                                              \
	};                                                                                         \
                                                                                                   \
	static struct crc_stm32f1_data crc_stm32f1_data_##inst;                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, crc_stm32f1_init, NULL, &crc_stm32f1_data_##inst,              \
			      &crc_stm32f1_config_##inst, POST_KERNEL,                             \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_stm32f1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_STM32_INIT)
