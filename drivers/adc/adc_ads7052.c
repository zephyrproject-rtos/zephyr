/* TI ADS7052 ADC
 *
 * Copyright (c) 2023 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ads7052

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(adc_ads7052);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADS7052_RESOLUTION 14U

struct ads7052_config {
	struct spi_dt_spec bus;
	uint8_t channels;
};

struct ads7052_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t channels;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_ADS7052_ACQUISITION_THREAD_STACK_SIZE);
};

static int adc_ads7052_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct ads7052_config *config = dev->config;

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_VDD_1) {
		LOG_ERR("unsupported channel reference '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= config->channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}
	return 0;
}

static int ads7052_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)
{
	uint8_t channels = 0;
	size_t needed;

	channels = POPCOUNT(sequence->channels);

	needed = channels * sizeof(uint16_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief Send ADS7052 Offset calibration request
 *
 * On power-up, the host must provide 24 SCLKs in the first serial transfer to enter the OFFCAL
 * state. During normal operation, the host must provide 64 SCLKs in the serial transfer frame to
 * enter the OFFCAL state.
 */
static int ads7052_send_calibration(const struct device *dev, bool power_up)
{
	const struct ads7052_config *config = dev->config;
	int err;
	uint8_t sclks_needed = power_up ? 24 : 64;
	uint8_t num_bytes = sclks_needed / 8;
	uint8_t tx_bytes[8] = {0};

	const struct spi_buf tx_buf = {.buf = tx_bytes, .len = num_bytes};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	err = spi_write_dt(&config->bus, &tx);

	return err;
}

static int ads7052_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads7052_config *config = dev->config;
	struct ads7052_data *data = dev->data;
	int err;

	if (sequence->resolution != ADS7052_RESOLUTION) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > config->channels) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		ads7052_send_calibration(dev, false);
	}

	err = ads7052_validate_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_ads7052_read_async(const struct device *dev, const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	struct ads7052_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = ads7052_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_ads7052_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_ads7052_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads7052_data *data = CONTAINER_OF(ctx, struct ads7052_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads7052_data *data = CONTAINER_OF(ctx, struct ads7052_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/**
 *  @brief Get a 14-bit integer from raw ADC data.
 *
 *  @param src Location of the big-endian 14-bit integer to get.
 *
 *  @return 14-bit integer in host endianness.
 */
static inline int ads7052_get_be14(const uint8_t src[2])
{
	return ((src[0] & 0x7F) << 7) | (src[1] >> 1);
}

/**
 * @brief Read ADS7052 over SPI interface
 *
 * A leading 0 is output on the SDO pin on the CS falling edge.
 * The most significant bit (MSB) of the output data is launched on the SDO pin on the rising edge
 * after the first SCLK falling edge. Subsequent output bits are launched on the subsequent rising
 * edges provided on SCLK. When all 14 output bits are shifted out, the device outputs 0's on the
 * subsequent SCLK rising edges. The device enters the ACQ state after 18 clocks and a minimum time
 * of tACQ must be provided for acquiring the next sample. If the device is provided with less than
 * 18 SCLK falling edges in the present serial transfer frame, the device provides an invalid
 * conversion result in the next serial transfer frame
 */
static int ads7052_read_channel(const struct device *dev, uint8_t channel, uint16_t *result)
{
	const struct ads7052_config *config = dev->config;
	int err;
	uint8_t rx_bytes[3];
	const struct spi_buf rx_buf[1] = {{.buf = rx_bytes, .len = sizeof(rx_bytes)}};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	err = spi_read_dt(&config->bus, &rx);
	if (err) {
		return err;
	}

	*result = ads7052_get_be14(rx_bytes);
	*result &= BIT_MASK(ADS7052_RESOLUTION);

	return 0;
}

static void ads7052_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct ads7052_data *data = p1;
	uint16_t result = 0;
	uint8_t channel;
	int err = 0;

	err = ads7052_send_calibration(data->dev, true);
	if (err) {
		LOG_ERR("failed to send powerup sequence (err %d)", err);
	}

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		while (data->channels != 0) {
			channel = find_lsb_set(data->channels) - 1;

			LOG_DBG("reading channel %d", channel);

			err = ads7052_read_channel(data->dev, channel, &result);
			if (err) {
				LOG_ERR("failed to read channel %d (err %d)", channel, err);
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("read channel %d, result = %d", channel, result);

			*data->buffer++ = result;
			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int adc_ads7052_init(const struct device *dev)
{
	const struct ads7052_config *config = dev->config;
	struct ads7052_data *data = dev->data;

	data->dev = dev;

	adc_context_init(&data->ctx);
	k_sem_init(&data->sem, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	k_thread_create(&data->thread, data->stack,
			K_KERNEL_STACK_SIZEOF(data->stack),
			ads7052_acquisition_thread, data, NULL, NULL,
			CONFIG_ADC_ADS7052_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api ads7052_api = {
	.channel_setup = adc_ads7052_channel_setup,
	.read = adc_ads7052_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_ads7052_read_async,
#endif
};

#define ADC_ADS7052_SPI_CFG \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ADC_ADS7052_INIT(n)                                                                        \
                                                                                                   \
	static const struct ads7052_config ads7052_cfg_##n = {                                     \
		.bus = SPI_DT_SPEC_INST_GET(n, ADC_ADS7052_SPI_CFG, 1U),                           \
		.channels = 1,                                                                     \
	};                                                                                         \
                                                                                                   \
	static struct ads7052_data ads7052_data_##n = {                                            \
		ADC_CONTEXT_INIT_TIMER(ads7052_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_LOCK(ads7052_data_##n, ctx),                                      \
		ADC_CONTEXT_INIT_SYNC(ads7052_data_##n, ctx),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_ads7052_init, NULL, &ads7052_data_##n, &ads7052_cfg_##n,      \
			      POST_KERNEL, CONFIG_ADC_ADS7052_INIT_PRIORITY, &ads7052_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS7052_INIT)
