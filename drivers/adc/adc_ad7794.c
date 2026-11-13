/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad7794

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/adc/ad7794-adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_ad7794, CONFIG_ADC_LOG_LEVEL);

/* Communications register */
#define AD7794_COMM_READ   BIT(6)
#define AD7794_COMM_RS_MSK GENMASK(5, 3)

/* Register addresses */
#define AD7794_REG_STATUS 0U
#define AD7794_REG_MODE   1U
#define AD7794_REG_CONF   2U
#define AD7794_REG_DATA   3U
#define AD7794_REG_ID     4U
#define AD7794_REG_IO     5U

/* Status register */
#define AD7794_STATUS_NRDY BIT(7)
#define AD7794_STATUS_ERR  BIT(6)

/* Mode register */
#define AD7794_MODE_MD_MSK GENMASK(15, 13)
#define AD7794_MODE_FS_MSK GENMASK(3, 0)
#define AD7794_MD_SINGLE   1U
#define AD7794_MD_IDLE     2U

/* Configuration register */
#define AD7794_CONF_VBIAS_MSK  GENMASK(15, 14)
#define AD7794_CONF_UNIPOLAR   BIT(12)
#define AD7794_CONF_BOOST      BIT(11)
#define AD7794_CONF_GAIN_MSK   GENMASK(10, 8)
#define AD7794_CONF_REFSEL_MSK GENMASK(7, 6)
#define AD7794_CONF_BUF        BIT(4)
#define AD7794_CONF_CH_MSK     GENMASK(3, 0)

#define AD7794_REFSEL_REFIN1   0U
#define AD7794_REFSEL_REFIN2   1U
#define AD7794_REFSEL_INTERNAL 2U

/* IO register */
#define AD7794_IO_IEXCDIR_MSK GENMASK(3, 2)
#define AD7794_IO_IEXCEN_MSK  GENMASK(1, 0)

/* ID register */
#define AD7794_ID_MSK GENMASK(3, 0)
#define AD7794_ID     0xFU

#define AD7794_VBIAS_SEL_MSK GENMASK(1, 0)

#define AD7794_REF_INTERNAL_MV 1170U
#define AD7794_RESOLUTION      24U
#define AD7794_BIPOLAR_ZERO    BIT(23)
#define AD7794_MAX_CHANNELS    16U
#define AD7794_RESET_DELAY_US  500U

/*
 * Single conversion settling time in ms (2 / f_ADC, rounded up), indexed by
 * the filter update rate select bits FS[3:0].
 */
static const uint16_t ad7794_settling_ms[] = {
	[AD7794_DR_470_HZ] = 5U,    [AD7794_DR_242_HZ] = 9U,    [AD7794_DR_123_HZ] = 17U,
	[AD7794_DR_62_HZ] = 33U,    [AD7794_DR_50_HZ] = 40U,    [AD7794_DR_39_HZ] = 52U,
	[AD7794_DR_33_2_HZ] = 61U,  [AD7794_DR_19_6_HZ] = 103U, [AD7794_DR_16_7_HZ_50_HZ] = 120U,
	[AD7794_DR_16_7_HZ] = 120U, [AD7794_DR_12_5_HZ] = 160U, [AD7794_DR_10_HZ] = 200U,
	[AD7794_DR_8_33_HZ] = 241U, [AD7794_DR_6_25_HZ] = 320U, [AD7794_DR_4_17_HZ] = 480U,
};

struct ad7794_config {
	struct spi_dt_spec spi;
};

struct ad7794_channel {
	uint16_t conf;
	uint8_t io;
	uint8_t fs;
	bool bipolar;
};

struct ad7794_data {
	struct adc_context ctx;
	struct k_sem acq_sem;
	int32_t *buffer;
	int32_t *repeat_buffer;
	uint16_t mode_base;
	uint32_t channels_configured;
	struct ad7794_channel channels[AD7794_MAX_CHANNELS];
};

static int ad7794_reg_read(const struct device *dev, uint8_t reg, uint8_t *val, size_t len)
{
	const struct ad7794_config *cfg = dev->config;
	uint8_t cmd = AD7794_COMM_READ | FIELD_PREP(AD7794_COMM_RS_MSK, reg);
	const struct spi_buf tx_buf = {.buf = &cmd, .len = sizeof(cmd)};
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(cmd)},
		{.buf = val, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1U};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int ad7794_reg_write(const struct device *dev, uint8_t reg, uint16_t val, size_t len)
{
	const struct ad7794_config *cfg = dev->config;
	uint8_t buf[3];
	const struct spi_buf tx_buf = {.buf = buf, .len = len + 1U};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1U};

	buf[0] = FIELD_PREP(AD7794_COMM_RS_MSK, reg);
	if (len == 2U) {
		sys_put_be16(val, &buf[1]);
	} else {
		buf[1] = (uint8_t)val;
	}

	return spi_write_dt(&cfg->spi, &tx);
}

static int ad7794_reset(const struct device *dev)
{
	const struct ad7794_config *cfg = dev->config;
	uint8_t buf[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
	const struct spi_buf tx_buf = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1U};
	int ret;

	ret = spi_write_dt(&cfg->spi, &tx);
	if (ret != 0) {
		return ret;
	}

	k_usleep(AD7794_RESET_DELAY_US);

	return 0;
}

static int ad7794_gain_to_code(enum adc_gain gain, uint8_t *code)
{
	switch (gain) {
	case ADC_GAIN_1:
		*code = 0U;
		break;
	case ADC_GAIN_2:
		*code = 1U;
		break;
	case ADC_GAIN_4:
		*code = 2U;
		break;
	case ADC_GAIN_8:
		*code = 3U;
		break;
	case ADC_GAIN_16:
		*code = 4U;
		break;
	case ADC_GAIN_32:
		*code = 5U;
		break;
	case ADC_GAIN_64:
		*code = 6U;
		break;
	case ADC_GAIN_128:
		*code = 7U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ad7794_ref_to_code(enum adc_reference ref, uint8_t *code)
{
	switch (ref) {
	case ADC_REF_INTERNAL:
		*code = AD7794_REFSEL_INTERNAL;
		break;
	case ADC_REF_EXTERNAL0:
		*code = AD7794_REFSEL_REFIN1;
		break;
	case ADC_REF_EXTERNAL1:
		*code = AD7794_REFSEL_REFIN2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ad7794_acq_time_to_fs(uint16_t acq_time, uint8_t *fs)
{
	uint16_t value = ADC_ACQ_TIME_VALUE(acq_time);

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		*fs = AD7794_DR_16_7_HZ;
		return 0;
	}

	if ((ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) || (value < AD7794_DR_470_HZ) ||
	    (value > AD7794_DR_4_17_HZ)) {
		return -EINVAL;
	}

	*fs = (uint8_t)value;

	return 0;
}

static int ad7794_current_source_to_io(const uint8_t pin[2], uint8_t *io)
{
	uint8_t dir = pin[0];
	uint8_t en = pin[1];

	if ((dir > AD7794_IEXCDIR_BOTH_IOUT2) || (en > AD7794_IEXCEN_1_MA)) {
		return -EINVAL;
	}

	/* Both sources on one pin is only permitted for 10 uA and 210 uA */
	if ((dir >= AD7794_IEXCDIR_BOTH_IOUT1) && (en == AD7794_IEXCEN_1_MA)) {
		return -EINVAL;
	}

	*io = FIELD_PREP(AD7794_IO_IEXCDIR_MSK, dir) | FIELD_PREP(AD7794_IO_IEXCEN_MSK, en);

	return 0;
}

static int ad7794_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg)
{
	struct ad7794_data *data = dev->data;
	struct ad7794_channel *ch;
	uint8_t gain;
	uint8_t refsel;
	uint8_t fs;
	uint8_t io = 0U;
	uint16_t conf;
	int ret;

	if (channel_cfg->channel_id >= AD7794_MAX_CHANNELS) {
		LOG_ERR("Invalid channel %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->input_positive > AD7794_ADC_AIN1M_AIN1M) {
		LOG_ERR("Invalid input %u", channel_cfg->input_positive);
		return -EINVAL;
	}

	ret = ad7794_gain_to_code(channel_cfg->gain, &gain);
	if (ret != 0) {
		LOG_ERR("Unsupported gain %d", channel_cfg->gain);
		return ret;
	}

	ret = ad7794_ref_to_code(channel_cfg->reference, &refsel);
	if (ret != 0) {
		LOG_ERR("Unsupported reference %d", channel_cfg->reference);
		return ret;
	}

	ret = ad7794_acq_time_to_fs(channel_cfg->acquisition_time, &fs);
	if (ret != 0) {
		LOG_ERR("Unsupported acquisition time 0x%x", channel_cfg->acquisition_time);
		return ret;
	}

	if ((channel_cfg->vbias_pins & ~(AD7794_VBIAS_SEL_MSK | AD7794_VBIAS_BOOST)) != 0U) {
		LOG_ERR("Invalid bias voltage configuration 0x%x", channel_cfg->vbias_pins);
		return -EINVAL;
	}

	if (channel_cfg->current_source_pin_set) {
		ret = ad7794_current_source_to_io(channel_cfg->current_source_pin, &io);
		if (ret != 0) {
			LOG_ERR("Invalid current source configuration");
			return ret;
		}
	}

	conf = FIELD_PREP(AD7794_CONF_VBIAS_MSK, channel_cfg->vbias_pins & AD7794_VBIAS_SEL_MSK) |
	       FIELD_PREP(AD7794_CONF_GAIN_MSK, gain) | FIELD_PREP(AD7794_CONF_REFSEL_MSK, refsel) |
	       FIELD_PREP(AD7794_CONF_CH_MSK, channel_cfg->input_positive) | AD7794_CONF_BUF;

	if ((channel_cfg->vbias_pins & AD7794_VBIAS_BOOST) != 0U) {
		conf |= AD7794_CONF_BOOST;
	}

	if (channel_cfg->differential == 0U) {
		conf |= AD7794_CONF_UNIPOLAR;
	}

	ch = &data->channels[channel_cfg->channel_id];
	ch->conf = conf;
	ch->io = io;
	ch->fs = fs;
	ch->bipolar = (channel_cfg->differential != 0U);

	data->channels_configured |= BIT(channel_cfg->channel_id);

	return 0;
}

static int ad7794_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ad7794_data *data = dev->data;
	size_t needed;

	if (sequence->resolution != AD7794_RESOLUTION) {
		LOG_ERR("Unsupported resolution %u", sequence->resolution);
		return -EINVAL;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("Oversampling not supported");
		return -EINVAL;
	}

	if ((sequence->channels == 0U) ||
	    ((sequence->channels & ~data->channels_configured) != 0U)) {
		LOG_ERR("Invalid or unconfigured channels 0x%x", sequence->channels);
		return -EINVAL;
	}

	needed = POPCOUNT(sequence->channels) * sizeof(int32_t);
	if (sequence->options != NULL) {
		needed *= 1U + sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ad7794_data *data = CONTAINER_OF(ctx, struct ad7794_data, ctx);

	data->repeat_buffer = data->buffer;
	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ad7794_data *data = CONTAINER_OF(ctx, struct ad7794_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int ad7794_wait_for_conversion(const struct device *dev, uint8_t fs)
{
	uint16_t settling_ms = ad7794_settling_ms[fs];
	int64_t deadline;
	uint8_t status;
	int ret;

	k_msleep(settling_ms);

	/* Allow the internal clock tolerance before giving up */
	deadline = k_uptime_get() + settling_ms;

	do {
		ret = ad7794_reg_read(dev, AD7794_REG_STATUS, &status, sizeof(status));
		if (ret != 0) {
			return ret;
		}

		if ((status & AD7794_STATUS_NRDY) == 0U) {
			if ((status & AD7794_STATUS_ERR) != 0U) {
				LOG_DBG("Conversion result clamped or reference missing");
			}
			return 0;
		}

		k_msleep(1);
	} while (k_uptime_get() < deadline);

	return -ETIMEDOUT;
}

static int ad7794_read_channel(const struct device *dev, const struct ad7794_channel *ch,
			       int32_t *result)
{
	struct ad7794_data *data = dev->data;
	uint8_t raw[3];
	uint32_t code;
	int ret;

	ret = ad7794_reg_write(dev, AD7794_REG_CONF, ch->conf, 2U);
	if (ret != 0) {
		return ret;
	}

	ret = ad7794_reg_write(dev, AD7794_REG_IO, ch->io, 1U);
	if (ret != 0) {
		return ret;
	}

	ret = ad7794_reg_write(dev, AD7794_REG_MODE,
			       data->mode_base | FIELD_PREP(AD7794_MODE_MD_MSK, AD7794_MD_SINGLE) |
				       FIELD_PREP(AD7794_MODE_FS_MSK, ch->fs),
			       2U);
	if (ret != 0) {
		return ret;
	}

	ret = ad7794_wait_for_conversion(dev, ch->fs);
	if (ret != 0) {
		return ret;
	}

	ret = ad7794_reg_read(dev, AD7794_REG_DATA, raw, sizeof(raw));
	if (ret != 0) {
		return ret;
	}

	code = sys_get_be24(raw);
	if (ch->bipolar) {
		/* Offset binary to two's complement */
		*result = (int32_t)code - (int32_t)AD7794_BIPOLAR_ZERO;
	} else {
		*result = (int32_t)code;
	}

	return 0;
}

static void ad7794_perform_read(const struct device *dev)
{
	struct ad7794_data *data = dev->data;
	uint32_t channels = data->ctx.sequence.channels;
	uint8_t channel_id;
	int ret;

	k_sem_take(&data->acq_sem, K_FOREVER);

	while (channels != 0U) {
		channel_id = (uint8_t)(find_lsb_set(channels) - 1);

		ret = ad7794_read_channel(dev, &data->channels[channel_id], data->buffer);
		if (ret != 0) {
			LOG_ERR("Reading channel %u failed (%d)", channel_id, ret);
			adc_context_disable_timer(&data->ctx);
			adc_context_complete(&data->ctx, ret);
			return;
		}

		data->buffer++;
		channels &= ~BIT(channel_id);
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static int ad7794_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ad7794_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);

	ret = ad7794_validate_sequence(dev, sequence);
	if (ret == 0) {
		data->buffer = sequence->buffer;
		adc_context_start_read(&data->ctx, sequence);

		while (k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
			ad7794_perform_read(dev);
		}

		ret = data->ctx.status;
	}

	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ad7794_init(const struct device *dev)
{
	const struct ad7794_config *cfg = dev->config;
	struct ad7794_data *data = dev->data;
	uint8_t mode[2];
	uint8_t id;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus %s not ready", cfg->spi.bus->name);
		return -ENODEV;
	}

	adc_context_init(&data->ctx);
	k_sem_init(&data->acq_sem, 0, 1);

	ret = ad7794_reset(dev);
	if (ret != 0) {
		LOG_ERR("Reset failed (%d)", ret);
		return ret;
	}

	ret = ad7794_reg_read(dev, AD7794_REG_ID, &id, sizeof(id));
	if (ret != 0) {
		return ret;
	}

	if (FIELD_GET(AD7794_ID_MSK, id) != AD7794_ID) {
		LOG_ERR("Invalid device ID 0x%02x", id);
		return -ENODEV;
	}

	ret = ad7794_reg_read(dev, AD7794_REG_MODE, mode, sizeof(mode));
	if (ret != 0) {
		return ret;
	}

	data->mode_base =
		(uint16_t)(sys_get_be16(mode) & ~(AD7794_MODE_MD_MSK | AD7794_MODE_FS_MSK));

	/* Leave the continuous conversion mode entered after reset */
	ret = ad7794_reg_write(dev, AD7794_REG_MODE,
			       data->mode_base | FIELD_PREP(AD7794_MODE_MD_MSK, AD7794_MD_IDLE) |
				       FIELD_PREP(AD7794_MODE_FS_MSK, AD7794_DR_16_7_HZ),
			       2U);
	if (ret != 0) {
		return ret;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, ad7794_api) = {
	.channel_setup = ad7794_channel_setup,
	.read = ad7794_read,
	.ref_internal = AD7794_REF_INTERNAL_MV,
};

#define AD7794_INIT(n)                                                                             \
	static const struct ad7794_config ad7794_config_##n = {                                    \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL |            \
						       SPI_MODE_CPHA | SPI_WORD_SET(8) |           \
						       SPI_TRANSFER_MSB),                          \
	};                                                                                         \
	static struct ad7794_data ad7794_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, ad7794_init, NULL, &ad7794_data_##n, &ad7794_config_##n,          \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &ad7794_api);

DT_INST_FOREACH_STATUS_OKAY(AD7794_INIT)
