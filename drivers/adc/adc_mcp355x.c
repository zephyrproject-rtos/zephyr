/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_mcp355x, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include "adc_common.h"

#define MCP355X_RESOLUTION 22U

#define MCP355X_OVL  BIT(23)
#define MCP355X_OVH  BIT(22)
#define MCP355X_SIGN BIT(21)

/* Datasheet fSCK maximum */
#define MCP355X_SCK_MAX_HZ MHZ(5)

/* 6 bytes = 48 SCK cycles = 9.6 us at 5 MHz, above tCSL (8 us) */
#define MCP355X_XFER_SIZE 6U

/* tCSHSD is 10 us typical, without a maximum */
#define MCP355X_CS_SHUTDOWN_US 50U

/* From shutdown: 32 fOSC periods of start-up and 144 extra ones, plus 2% tolerance */
#define MCP355X_WAKEUP_PERIODS     (32U + 144U)
#define MCP355X_WAKEUP_US(fosc_hz) DIV_ROUND_UP((MCP355X_WAKEUP_PERIODS * USEC_PER_SEC), (fosc_hz))
#define MCP355X_CONV_WAIT_US(tconv_us, fosc_hz)                                                    \
	(((tconv_us) + MCP355X_WAKEUP_US(fosc_hz)) * 102U / 100U)

struct mcp355x_config {
	struct spi_dt_spec bus;
	uint32_t conv_wait_us;
};

struct mcp355x_data {
	struct adc_context ctx;
	const struct device *dev;
	int32_t *buffer;
	int32_t *repeat_buffer;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MCP355X_ACQUISITION_THREAD_STACK_SIZE);
};

static int mcp355x_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id != 0U) {
		LOG_ERR("unsupported channel id '%u'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("unsupported channel reference '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%u'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	/* Two's complement results need the differential scale */
	if (channel_cfg->differential == 0U) {
		LOG_ERR("channel must be differential");
		return -ENOTSUP;
	}

	return 0;
}

static int mcp355x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct mcp355x_data *data = dev->data;
	int ret;

	if (sequence->channels != BIT(0)) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	if (sequence->resolution != MCP355X_RESOLUTION) {
		LOG_ERR("unsupported resolution %u", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("oversampling is not supported");
		return -ENOTSUP;
	}

	ret = adc_sequence_validate_buffer(sequence, 1U, sizeof(int32_t));
	if (ret < 0) {
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int mcp355x_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct mcp355x_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async != NULL, async);
	ret = mcp355x_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int mcp355x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return mcp355x_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcp355x_data *data = CONTAINER_OF(ctx, struct mcp355x_data, ctx);

	data->repeat_buffer = data->buffer;
	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct mcp355x_data *data = CONTAINER_OF(ctx, struct mcp355x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int mcp355x_start_conversion(const struct device *dev)
{
	const struct mcp355x_config *config = dev->config;
	uint8_t dummy[MCP355X_XFER_SIZE];
	const struct spi_buf rx_buf = {.buf = dummy, .len = sizeof(dummy)};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1U};
	int ret;

	/* A pulse that finds a finished conversion only enters shutdown: pulse twice */
	for (uint8_t i = 0U; i < 2U; i++) {
		k_busy_wait(MCP355X_CS_SHUTDOWN_US);

		ret = spi_read_dt(&config->bus, &rx);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int mcp355x_read_sample(const struct device *dev, int32_t *result)
{
	const struct mcp355x_config *config = dev->config;
	uint8_t rx_data[MCP355X_XFER_SIZE];
	const struct spi_buf rx_buf = {.buf = rx_data, .len = sizeof(rx_data)};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1U};
	uint32_t raw;
	int ret;

	ret = mcp355x_start_conversion(dev);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_USEC(config->conv_wait_us));

	ret = spi_read_dt(&config->bus, &rx);
	if (ret < 0) {
		return ret;
	}

	raw = sys_get_be24(rx_data);

	/* Both overflow bits set: device busy or communication error */
	if ((raw & (MCP355X_OVL | MCP355X_OVH)) == (MCP355X_OVL | MCP355X_OVH)) {
		return -EIO;
	}

	/* Out of range, OVL is the sign of a 23-bit result */
	if ((raw & MCP355X_OVH) != 0U) {
		raw &= ~MCP355X_OVH;
	} else if ((raw & (MCP355X_OVL | MCP355X_SIGN)) != 0U) {
		raw |= GENMASK(31, 22);
	}

	*result = (int32_t)raw;

	return 0;
}

static void mcp355x_acquisition_thread(void *p1, void *p2, void *p3)
{
	struct mcp355x_data *data = p1;
	int32_t result;
	int ret;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		ret = mcp355x_read_sample(data->dev, &result);
		if (ret < 0) {
			LOG_ERR("failed to read sample (err %d)", ret);
			/* The timer only stops on its own when a sequence completes */
			adc_context_disable_timer(&data->ctx);
			adc_context_complete(&data->ctx, ret);
			continue;
		}

		*data->buffer++ = result;
		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int mcp355x_init(const struct device *dev)
{
	const struct mcp355x_config *config = dev->config;
	struct mcp355x_data *data = dev->data;
	k_tid_t tid;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->bus.bus);
		return -ENODEV;
	}

	data->dev = dev;
	k_sem_init(&data->sem, 0, 1);

	tid = k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
			      mcp355x_acquisition_thread, data, NULL, NULL,
			      CONFIG_ADC_MCP355X_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
	(void)k_thread_name_set(tid, dev->name);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, mcp355x_adc_api) = {
	.channel_setup = mcp355x_channel_setup,
	.read = mcp355x_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcp355x_read_async,
#endif
};

#define MCP355X_DEFINE(node_id, tconv_us, fosc_hz)                                                 \
	BUILD_ASSERT(DT_PROP(node_id, spi_max_frequency) <= MCP355X_SCK_MAX_HZ,                    \
		     "MCP355x spi-max-frequency exceeds the 5 MHz fSCK maximum");                  \
                                                                                                   \
	static struct mcp355x_data DT_CAT(mcp355x_data_, node_id) = {                              \
		ADC_CONTEXT_INIT_TIMER(DT_CAT(mcp355x_data_, node_id), ctx),                       \
		ADC_CONTEXT_INIT_LOCK(DT_CAT(mcp355x_data_, node_id), ctx),                        \
		ADC_CONTEXT_INIT_SYNC(DT_CAT(mcp355x_data_, node_id), ctx),                        \
	};                                                                                         \
                                                                                                   \
	static const struct mcp355x_config DT_CAT(mcp355x_config_, node_id) = {                    \
		.bus = SPI_DT_SPEC_GET(node_id, SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL |           \
							SPI_MODE_CPHA | SPI_TRANSFER_MSB |         \
							SPI_WORD_SET(8)),                          \
		.conv_wait_us = MCP355X_CONV_WAIT_US(tconv_us, fosc_hz),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, mcp355x_init, NULL, &DT_CAT(mcp355x_data_, node_id),             \
			 &DT_CAT(mcp355x_config_, node_id), POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			 &mcp355x_adc_api);

/* tCONV (us) and fOSC (Hz) of each part */
DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3550_50, MCP355X_DEFINE, 80000U, 102400U)
DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3550_60, MCP355X_DEFINE, 66670U, 122880U)
DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3551, MCP355X_DEFINE, 73100U, 112640U)
DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3553, MCP355X_DEFINE, 16670U, 122880U)
