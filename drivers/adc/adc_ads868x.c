/*
 * Copyright (c) 2026 Ryomei Osaki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TI ADS8684 / ADS8688: 4 / 8 channel, 16-bit SAR ADC with a per-channel
 * input range, SPI mode 1.
 *
 * A frame is 32 clocks: a 16-bit command followed by the 16-bit conversion
 * result of the channel selected in the previous frame. Channels are read in
 * manual mode, one MAN_Ch_n command per channel, with a trailing NO_OP frame
 * to collect the last result.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_ads868x, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADS868X_RESOLUTION      16
#define ADS868X_REF_INTERNAL_MV 4096
#define ADS868X_MAX_CHANNELS    8

/*
 * Command register, the first 16 bits of a frame:
 *   NO_OP    0000 0000 0000 0000  continue in the current mode
 *   RST      1000 0101 0000 0000  program registers back to defaults
 *   MAN_Ch_n 1100 nnn0 0000 0000  select channel n for the next frame
 */
#define ADS868X_CMD_NO_OP      0x0000
#define ADS868X_CMD_RST        0x8500
#define ADS868X_CMD_MAN_CH(ch) (0xC000 | ((ch) << 10))

/*
 * Program register write, a 24-clock frame: 7-bit address, write bit, 8-bit
 * data. The device echoes the written data in the last 8 bits of the frame.
 */
#define ADS868X_PROG_WRITE    BIT(0)
#define ADS868X_REG_RANGE(ch) (0x05 + (ch))

/* Range register values (Vref = 4.096 V); bit 2 set means unipolar */
#define ADS868X_RANGE_UNIPOLAR      BIT(2)
#define ADS868X_RANGE_BIPOLAR_2_5   0x00 /* +/-2.5 x Vref */
#define ADS868X_RANGE_BIPOLAR_1_25  0x01 /* +/-1.25 x Vref */
#define ADS868X_RANGE_UNIPOLAR_2_5  0x05 /* 0 .. 2.5 x Vref */
#define ADS868X_RANGE_UNIPOLAR_1_25 0x06 /* 0 .. 1.25 x Vref */

struct ads868x_config {
	struct spi_dt_spec spi;
	uint8_t nchannels;
};

struct ads868x_data {
	struct adc_context ctx;

	uint16_t *buffer;
	uint16_t *repeat_buffer;
	/*
	 * Range register value programmed into each channel. Zero is the
	 * device default after reset (+/-2.5 x Vref).
	 */
	uint8_t range[ADS868X_MAX_CHANNELS];

	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_ADS868X_ACQUISITION_THREAD_STACK_SIZE);
};

static int ads868x_transceive(const struct device *dev, uint8_t *tx, uint8_t *rx, size_t len)
{
	const struct ads868x_config *config = dev->config;
	const struct spi_buf tx_buf = {.buf = tx, .len = len};
	const struct spi_buf rx_buf = {.buf = rx, .len = len};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	return spi_transceive_dt(&config->spi, &tx_set, &rx_set);
}

/* One frame: send a command, get the result of the previous frame's command */
static int ads868x_command(const struct device *dev, uint16_t cmd, uint16_t *result)
{
	uint8_t tx[4] = {0};
	uint8_t rx[4];
	int ret;

	sys_put_be16(cmd, tx);

	ret = ads868x_transceive(dev, tx, rx, sizeof(tx));
	if (ret == 0 && result != NULL) {
		*result = sys_get_be16(&rx[2]);
	}

	return ret;
}

static int ads868x_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	uint8_t tx[3] = {(reg << 1) | ADS868X_PROG_WRITE, val, 0};
	uint8_t rx[3];
	int ret;

	ret = ads868x_transceive(dev, tx, rx, sizeof(tx));
	if (ret) {
		return ret;
	}

	if (rx[2] != val) {
		LOG_ERR("register 0x%02x: wrote 0x%02x, device echoed 0x%02x", reg, val, rx[2]);
		return -EIO;
	}

	return 0;
}

static int ads868x_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads868x_config *config = dev->config;
	struct ads868x_data *data = dev->data;
	uint8_t range;
	int ret;

	if (channel_cfg->channel_id >= config->nchannels) {
		LOG_ERR("unsupported channel id %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL &&
	    channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("unsupported reference %d", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition time %d", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	/*
	 * The gain selects the span of the range relative to Vref and
	 * "differential" its bipolar variant (the inputs are single-ended).
	 */
	switch (channel_cfg->gain) {
	case ADC_GAIN_2_5:
		range = channel_cfg->differential ? ADS868X_RANGE_BIPOLAR_2_5
						  : ADS868X_RANGE_UNIPOLAR_2_5;
		break;
	case ADC_GAIN_4_5:
		range = channel_cfg->differential ? ADS868X_RANGE_BIPOLAR_1_25
						  : ADS868X_RANGE_UNIPOLAR_1_25;
		break;
	default:
		LOG_ERR("unsupported gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	/* Keep the register write out of an ongoing read */
	adc_context_lock(&data->ctx, false, NULL);

	ret = ads868x_write_reg(dev, ADS868X_REG_RANGE(channel_cfg->channel_id), range);
	if (ret == 0) {
		data->range[channel_cfg->channel_id] = range;
	}

	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ads868x_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = POPCOUNT(sequence->channels) * sizeof(uint16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads868x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads868x_config *config = dev->config;
	struct ads868x_data *data = dev->data;
	int ret;

	if (sequence->resolution != ADS868X_RESOLUTION) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->channels == 0 || (sequence->channels & ~BIT_MASK(config->nchannels)) != 0) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("calibration is not supported");
		return -ENOTSUP;
	}

	ret = ads868x_validate_buffer_size(sequence);
	if (ret) {
		LOG_ERR("buffer too small");
		return ret;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ads868x_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct ads868x_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async != NULL, async);
	ret = ads868x_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ads868x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ads868x_read_async(dev, sequence, NULL);
}

/*
 * Callbacks required by adc_context.h. Sampling runs on the acquisition
 * thread since it may be requested from the interval timer.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads868x_data *data = CONTAINER_OF(ctx, struct ads868x_data, ctx);

	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads868x_data *data = CONTAINER_OF(ctx, struct ads868x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Convert a conversion result into the value stored in the sequence buffer */
static uint16_t ads868x_to_sample(const struct ads868x_data *data, uint8_t channel, uint16_t raw)
{
	/* Bipolar ranges are output as offset binary; make it two's complement */
	if ((data->range[channel] & ADS868X_RANGE_UNIPOLAR) == 0) {
		raw ^= BIT(15);
	}

	return raw;
}

static int ads868x_read_channels(const struct device *dev)
{
	struct ads868x_data *data = dev->data;
	uint32_t channels = data->ctx.sequence.channels;
	uint8_t channel = find_lsb_set(channels) - 1;
	uint16_t raw;
	int ret;

	/*
	 * A frame returns the result of the channel selected in the previous
	 * frame: select the first channel, then select each further channel
	 * while collecting the previous one, and finish with a NO_OP.
	 */
	ret = ads868x_command(dev, ADS868X_CMD_MAN_CH(channel), NULL);
	if (ret) {
		return ret;
	}
	channels &= ~BIT(channel);

	while (channels != 0) {
		uint8_t next = find_lsb_set(channels) - 1;

		ret = ads868x_command(dev, ADS868X_CMD_MAN_CH(next), &raw);
		if (ret) {
			return ret;
		}
		*data->buffer++ = ads868x_to_sample(data, channel, raw);

		channels &= ~BIT(next);
		channel = next;
	}

	ret = ads868x_command(dev, ADS868X_CMD_NO_OP, &raw);
	if (ret) {
		return ret;
	}
	*data->buffer++ = ads868x_to_sample(data, channel, raw);

	return 0;
}

static void ads868x_acquisition_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct ads868x_data *data = dev->data;
	int ret;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		ret = ads868x_read_channels(dev);
		if (ret) {
			LOG_ERR("acquisition failed (err %d)", ret);
			adc_context_disable_timer(&data->ctx);
			adc_context_complete(&data->ctx, ret);
			continue;
		}

		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int ads868x_init(const struct device *dev)
{
	const struct ads868x_config *config = dev->config;
	struct ads868x_data *data = dev->data;
	k_tid_t tid;
	int ret;

	adc_context_init(&data->ctx);
	k_sem_init(&data->sem, 0, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	/* Software reset: program registers back to their defaults */
	ret = ads868x_command(dev, ADS868X_CMD_RST, NULL);
	if (ret) {
		LOG_ERR("reset failed (err %d)", ret);
		return ret;
	}

	tid = k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
			      ads868x_acquisition_thread, (void *)dev, NULL, NULL,
			      CONFIG_ADC_ADS868X_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, dev->name);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, ads868x_api) = {
	.channel_setup = ads868x_channel_setup,
	.read = ads868x_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads868x_read_async,
#endif
	.ref_internal = ADS868X_REF_INTERNAL_MV,
};

#define ADS868X_SPI_OP (SPI_OP_MODE_CONTROLLER | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

#define ADS868X_INIT(t, n, nchan)                                                                  \
	static struct ads868x_data ads##t##_data_##n;                                              \
	static const struct ads868x_config ads##t##_config_##n = {                                 \
		.spi = SPI_DT_SPEC_GET(DT_INST(n, ti_ads##t), ADS868X_SPI_OP),                     \
		.nchannels = nchan,                                                                \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST(n, ti_ads##t), ads868x_init, NULL, &ads##t##_data_##n,            \
			 &ads##t##_config_##n, POST_KERNEL, CONFIG_ADC_ADS868X_INIT_PRIORITY,      \
			 &ads868x_api);

#define DT_DRV_COMPAT   ti_ads8684
#define ADS8684_INIT(n) ADS868X_INIT(8684, n, 4)
DT_INST_FOREACH_STATUS_OKAY(ADS8684_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads8688
#define ADS8688_INIT(n) ADS868X_INIT(8688, n, 8)
DT_INST_FOREACH_STATUS_OKAY(ADS8688_INIT)
