/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_pcm1681

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "pcm1681.h"

LOG_MODULE_REGISTER(pcm1681);

union pcm1681_bus_spec {
	struct i2c_dt_spec i2c;
	struct spi_dt_spec spi;
};

typedef bool (*pcm1681_bus_is_ready)(union pcm1681_bus_spec const *bus_spec);
typedef int (*pcm1681_bus_reg_write)(union pcm1681_bus_spec const *bus_spec, uint8_t reg_addr,
				     uint8_t value);

struct pcm1681_bus_io {
	pcm1681_bus_is_ready is_ready;
	pcm1681_bus_reg_write reg_write;
};

struct pcm1681_config {
	union pcm1681_bus_spec bus_spec;
	struct pcm1681_bus_io bus_io;
};

struct pcm1681_data {
	uint8_t reg_map[PCM1681_N_REGISTERS];
};

static inline bool pcm1681_reg_is_accessible(uint8_t reg)
{
	return !((reg == PCM1681_REG_0) || (reg == PCM1681_REG_15));
}

static inline bool pcm1681_reg_is_writeable(uint8_t reg)
{
	return pcm1681_reg_is_accessible(reg) && reg != PCM1681_REG_14;
}

static inline void pcm1681_reg_update_masked(uint8_t *reg, uint8_t value, uint8_t pos, uint8_t mask)
{
	*reg = (*reg & ~mask) | (value << pos);
}

static inline void pcm1681_reg_read_masked(uint8_t reg, uint8_t *value, uint8_t pos, uint8_t mask)
{
	*value = (reg & mask) >> pos;
}

#ifdef CONFIG_AUDIO_CODEC_PCM1681_I2C
static bool pcm1681_i2c_is_ready(union pcm1681_bus_spec const *bus_spec)
{
	return i2c_is_ready_dt(&bus_spec->i2c);
}

static int pcm1681_i2c_reg_write(union pcm1681_bus_spec const *bus_spec, uint8_t reg_addr,
				 uint8_t value)
{
	return i2c_reg_write_byte_dt(&bus_spec->i2c, reg_addr, value);
}
#endif /* CONFIG_AUDIO_CODEC_PCM1681_I2C */

#ifdef CONFIG_AUDIO_CODEC_PCM1681_SPI
static bool pcm1681_spi_is_ready(union pcm1681_bus_spec const *bus_spec)
{
	return spi_is_ready_dt(&bus_spec->spi);
}

static int pcm1681_spi_reg_write(union pcm1681_bus_spec const *bus_spec, uint8_t reg_addr,
				 uint8_t value)
{
	const struct spi_buf buf[2] = {
		{
			.buf = &reg_addr,
			.len = 1,
		},
		{
			.buf = &value,
			.len = 1,
		},
	};
	const struct spi_buf_set set = {
		.buffers = buf,
		.count = 2,
	};

	return spi_write_dt(&bus_spec->spi, &set);
}
#endif /* CONFIG_AUDIO_CODEC_PCM1681_SPI */

static int pcm1681_set_volume(const struct device *dev, uint8_t channel, uint8_t volume)
{
	struct pcm1681_data *data = dev->data;
	uint8_t attenuation;
	uint8_t dams;
	uint8_t reg;

	if (channel == 0 || channel > 8) {
		LOG_ERR("Invalid channel (%d)", channel);
		return -EINVAL;
	}

	if (volume > 100) {
		LOG_ERR("Volume > 100 (%d)", volume);
		return -EINVAL;
	}

	/* Get DAMS */
	pcm1681_reg_read_masked(data->reg_map[PCM1681_DAMS_REG], &dams, PCM1681_DAMS_POS,
				PCM1681_DAMS_MASK);
	attenuation = (dams) ? VOL2ATT_WIDE(volume) : VOL2ATT_FINE(volume);

	switch (channel) {
	case 7:
		reg = PCM1681_AT7x_REG;
		break;
	case 8:
		reg = PCM1681_AT8x_REG;
		break;
	default:
		reg = channel;
		break;
	}

	pcm1681_reg_update_masked(&data->reg_map[reg], attenuation, PCM1681_ATxx_POS,
				  PCM1681_ATxx_MASK);

	return 0;
}

static int pcm1681_set_mute(const struct device *dev, uint8_t channel, bool mute)
{
	struct pcm1681_data *data = dev->data;
	uint8_t pos;
	uint8_t reg;

	if (channel == 0 || channel > 8) {
		LOG_ERR("Invalid channel (%d)", channel);
		return -EINVAL;
	}

	if (channel < 7) {
		reg = PCM1681_MUTx_REG;
		pos = channel - 1;
	} else {
		/* For channels 7 and 8, we can't write to the traditional register (??) */
		reg = PCM1681_MUT_OR_REG;
		pos = channel - 7;
	}
	pcm1681_reg_update_masked(&data->reg_map[reg], mute, pos, BIT(pos));

	return 0;
}

static int pcm1681_set_dac(const struct device *dev, uint8_t channel, bool enable)
{
	struct pcm1681_data *data = dev->data;
	uint8_t pos;
	uint8_t reg;

	if (channel == 0 || channel > 8) {
		LOG_ERR("Invalid channel (%d)", channel);
		return -EINVAL;
	}

	if (channel < 7) {
		reg = PCM1681_DACx_REG;
		pos = channel - 1;
	} else {
		/* For channels 7 and 8, we can't write to the traditional register (??) */
		reg = PCM1681_DAC_OR_REG;
		pos = channel - 7;
	}
	pcm1681_reg_update_masked(&data->reg_map[reg], !enable, pos, BIT(pos));

	return 0;
}

static int pcm1681_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct pcm1681_data *data = dev->data;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("Only AUDIO_DAI_TYPE_I2S supported");
		return -EINVAL;
	}

	switch (cfg->dai_cfg.i2s.format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		if ((cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_16_BITS) &
		    (cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_24_BITS)) {
			LOG_ERR("Word size %d not supported for i2s", cfg->dai_cfg.i2s.word_size);
		}
		pcm1681_reg_update_masked(&data->reg_map[PCM1681_FMTx_REG], PCM1681_FMT_I2S_16_24,
					  PCM1681_FMTx_POS, PCM1681_FMTx_MASK);
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		if ((cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_16_BITS) &
		    (cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_24_BITS)) {
			LOG_ERR("Word size %d not supported for left justified",
				cfg->dai_cfg.i2s.word_size);
		}
		pcm1681_reg_update_masked(&data->reg_map[PCM1681_FMTx_REG],
					  PCM1681_FMT_LEFT_JUSTIFIED_16_24, PCM1681_FMTx_POS,
					  PCM1681_FMTx_MASK);
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		if (cfg->dai_cfg.i2s.word_size == AUDIO_PCM_WIDTH_16_BITS) {
			pcm1681_reg_update_masked(&data->reg_map[PCM1681_FMTx_REG],
						  PCM1681_FMT_RIGHT_JUSTIFIED_16, PCM1681_FMTx_POS,
						  PCM1681_FMTx_MASK);
		} else if (cfg->dai_cfg.i2s.word_size == AUDIO_PCM_WIDTH_24_BITS) {
			pcm1681_reg_update_masked(&data->reg_map[PCM1681_FMTx_REG],
						  PCM1681_FMT_RIGHT_JUSTIFIED_24, PCM1681_FMTx_POS,
						  PCM1681_FMTx_MASK);
		} else {
			LOG_ERR("Word size %d not supported for right justified",
				cfg->dai_cfg.i2s.word_size);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("I2S format not supported");
		return -EINVAL;
	}

	return audio_codec_apply_properties(dev);
}

static void pcm1681_start_output(const struct device *dev)
{
	int ret;

	for (size_t i = 1; i <= PCM1681_N_CHANNELS; i++) {
		ret = pcm1681_set_dac(dev, i, true);
		if (ret < 0) {
			LOG_ERR("Failed to enable channel %d (%d)", i, ret);
			return;
		}
	}

	(void)audio_codec_apply_properties(dev);
}

static void pcm1681_stop_output(const struct device *dev)
{
	int ret;

	for (size_t i = 1; i <= PCM1681_N_CHANNELS; i++) {
		ret = pcm1681_set_dac(dev, i, false);
		if (ret < 0) {
			LOG_ERR("Failed to disable channel %d (%d)", i, ret);
			return;
		}
	}

	(void)audio_codec_apply_properties(dev);
}

static int pcm1681_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	int ret;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		if (channel != AUDIO_CHANNEL_ALL) {
			LOG_ERR("Volume only supported for all channels");
			return -EINVAL;
		}
		for (size_t i = 1; i <= PCM1681_N_CHANNELS; i++) {
			ret = pcm1681_set_volume(dev, i, val.vol);
			if (ret < 0) {
				LOG_ERR("Failed to set volume for channel %d (%d)", i, ret);
				return ret;
			}
		}
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (channel != AUDIO_CHANNEL_ALL) {
			LOG_ERR("Mute only supported for all channels");
			return -EINVAL;
		}
		for (size_t i = 1; i <= PCM1681_N_CHANNELS; i++) {
			ret = pcm1681_set_mute(dev, i, val.mute);
			if (ret < 0) {
				LOG_ERR("Failed to set mute for channel %d (%d)", i, ret);
				return ret;
			}
		}
		break;
	default:
		LOG_ERR("Property %d not supported", property);
		return -EINVAL;
	}

	return 0;
}

static int pcm1681_apply_properties(const struct device *dev)
{
	const struct pcm1681_config *config = dev->config;
	struct pcm1681_data *data = dev->data;
	int ret;

	if (!config->bus_io.is_ready(&config->bus_spec)) {
		LOG_ERR("Bus not ready");
		return -ENODEV;
	}

	ARRAY_FOR_EACH(data->reg_map, reg) {
		if (!pcm1681_reg_is_writeable(reg)) {
			continue;
		}

		ret = config->bus_io.reg_write(&config->bus_spec, reg, data->reg_map[reg]);
		if (ret < 0) {
			LOG_ERR("Failed to write register %d", ret);
			return ret;
		}
	}

	return 0;
}

static const struct audio_codec_api pcm1681_api = {
	.configure = pcm1681_configure,
	.start_output = pcm1681_start_output,
	.stop_output = pcm1681_stop_output,
	.set_property = pcm1681_set_property,
	.apply_properties = pcm1681_apply_properties,
};

static int pcm1681_init(const struct device *dev)
{
	int ret;

	/* Reset all registers to their default value */
	ret = pcm1681_apply_properties(dev);
	if (ret < 0) {
		LOG_ERR("Failed to apply default properties (%d)", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_AUDIO_CODEC_PCM1681_I2C
#define PCM1681_CONFIG_I2C(inst)                                                                   \
	.bus_spec =                                                                                \
		{                                                                                  \
			.i2c = I2C_DT_SPEC_INST_GET(inst),                                         \
	},                                                                                         \
	.bus_io = {                                                                                \
		.is_ready = pcm1681_i2c_is_ready,                                                  \
		.reg_write = pcm1681_i2c_reg_write,                                                \
	},
#else
#define PCM1681_CONFIG_I2C(inst)
#endif /* CONFIG_AUDIO_CODEC_PCM1681_I2C */

#ifdef CONFIG_AUDIO_CODEC_PCM1681_SPI
#define PCM1681_CONFIG_SPI(inst)                                                                   \
	.bus_spec =                                                                                \
		{                                                                                  \
			.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_SLAVE | SPI_WORD_SET(8)),    \
	},                                                                                         \
	.bus_io = {                                                                                \
		.is_ready = pcm1681_spi_is_ready,                                                  \
		.reg_write = pcm1681_spi_reg_write,                                                \
	},
#else
#define PCM1681_CONFIG_SPI(inst)
#endif /* CONFIG_AUDIO_CODEC_PCM1681_SPI */

#define PCM1681_CONFIG(inst)                                                                       \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (PCM1681_CONFIG_I2C(inst)),                         \
	(PCM1681_CONFIG_SPI(inst)))

#define PCM1681_DEFINE(inst)                                                                       \
	static const struct pcm1681_config pcm1681_##inst##_config = {PCM1681_CONFIG(inst)};       \
                                                                                                   \
	static struct pcm1681_data pcm1681_##inst##_data = {                                       \
		.reg_map = PCM1681_DEFAULT_REG_MAP,                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, pcm1681_init, NULL, &pcm1681_##inst##_data,                    \
			      &pcm1681_##inst##_config, POST_KERNEL,                               \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &pcm1681_api);

DT_INST_FOREACH_STATUS_OKAY(PCM1681_DEFINE)
