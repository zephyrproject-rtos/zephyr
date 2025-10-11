/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw88298

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aw88298);

#define AW88298_REG_ID       0x00
#define AW88298_REG_SYSST    0x01
#define AW88298_REG_SYSINT   0x02
#define AW88298_REG_SYSINTM  0x03
#define AW88298_REG_SYSCTRL  0x04
#define AW88298_REG_SYSCTRL2 0x05
#define AW88298_REG_I2SCTRL  0x06
#define AW88298_REG_HAGCCFG1 0x09
#define AW88298_REG_HAGCCFG2 0x0A
#define AW88298_REG_HAGCCFG3 0x0B
#define AW88298_REG_HAGCCFG4 0x0C

#define AW88298_REG_SYSCTRL_PWDN   BIT(0)
#define AW88298_REG_SYSCTRL_AMPPD  BIT(1)
#define AW88298_REG_SYSCTRL_I2SEN  BIT(6)
#define AW88298_REG_SYSCTRL2_HMUTE BIT(4)
#define AW88298_REG_I2SCTRL_I2SSR  (BIT_MASK(4) << 0)
#define AW88298_REG_I2SCTRL_I2SBCK (BIT_MASK(2) << 4)
#define AW88298_REG_I2SCTRL_I2SFS  (BIT_MASK(2) << 6)
#define AW88298_REG_I2SCTRL_I2SMD  (BIT_MASK(3) << 8)
#define AW88298_REG_HAGCCFG4_VOL   (BIT_MASK(8) << 8)

#define AW88298_ID_SOFTRESET            0x55AA
#define AW88298_I2SCTRL_I2SSR_VAL(val)  (((val) << 0) & AW88298_REG_I2SCTRL_I2SSR)
#define AW88298_I2SCTRL_I2SBCK_VAL(val) (((val) << 4) & AW88298_REG_I2SCTRL_I2SBCK)
#define AW88298_I2SCTRL_I2SFS_VAL(val)  (((val) << 6) & AW88298_REG_I2SCTRL_I2SFS)
#define AW88298_I2SCTRL_I2SMD_VAL(val)  (((val) << 8) & AW88298_REG_I2SCTRL_I2SMD)
#define AW88298_HAGCCFG4_VOL_VAL(val)   (((val) << 8) & AW88298_REG_HAGCCFG4_VOL)

#define AW88298_RESET_DELAY_MS 50

#define AW88298_VOLUME_DB_MAX 0
#define AW88298_VOLUME_DB_MIN (-96)

enum {
	AW88298_I2SCTRL_MODE_I2S = 4,
	AW88298_I2SCTRL_MODE_LEFT_JUSTIFIED,
	AW88298_I2SCTRL_MODE_RIGHT_JUSTIFIED,
};

enum {
	AW88298_I2SCTRL_FS_32BIT,
	AW88298_I2SCTRL_FS_24BIT,
	AW88298_I2SCTRL_FS_20BIT,
	AW88298_I2SCTRL_FS_16BIT,
};

enum {
	AW88298_I2SCTRL_BCK_16BIT,
	AW88298_I2SCTRL_BCK_20BIT,
	AW88298_I2SCTRL_BCK_24BIT,
	AW88298_I2SCTRL_BCK_32BIT,
};

struct aw88298_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
};

struct aw88298_data {
	struct k_mutex lock;
	int volume;
	bool mute;
};

static uint8_t aw88298_db2vol(int db)
{
	uint8_t hi;
	uint8_t lo;

	if (db > 0) {
		return 0;
	}

	hi = (-db) / 6;
	hi = hi < 0xF ? hi : 0xF;
	lo = (uint8_t)(((-db) - (hi * 6)) / 0.5);

	return (hi << 4) | (lo & 0xF);
}

static int aw88298_update_reg(const struct device *dev, uint8_t reg, uint16_t mask, uint16_t value)
{
	const struct aw88298_config *cfg = dev->config;
	struct aw88298_data *data = dev->data;
	uint8_t buf[3] = {reg};
	int ret;
	uint16_t regval;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_write_read_dt(&cfg->bus, &reg, 1, &buf[1], 2);
	if (ret < 0) {
		LOG_ERR("write_read reg 0x%02x failed: %d", reg, ret);
		goto end;
	}

	regval = sys_get_be16(&buf[1]);
	regval = (regval & ~mask) | (mask & value);
	sys_put_be16(regval, &buf[1]);

	ret = i2c_write_dt(&cfg->bus, buf, ARRAY_SIZE(buf));
	if (ret < 0) {
		LOG_ERR("write reg 0x%02x failed: %d", reg, ret);
	}

end:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int aw88298_get_sample_rate_code(uint32_t sample_rate, uint16_t *code)
{
	switch (sample_rate) {
	case AUDIO_PCM_RATE_8K:
		*code = 0x0U;
		break;
	case AUDIO_PCM_RATE_11P025K:
		*code = 0x1U;
		break;
	case AUDIO_PCM_RATE_16K:
		*code = 0x3U;
		break;
	case AUDIO_PCM_RATE_22P05K:
		*code = 0x4U;
		break;
	case AUDIO_PCM_RATE_24K:
		*code = 0x5U;
		break;
	case AUDIO_PCM_RATE_32K:
		*code = 0x6U;
		break;
	case AUDIO_PCM_RATE_44P1K:
		*code = 0x7U;
		break;
	case AUDIO_PCM_RATE_48K:
		*code = 0x8U;
		break;
	case AUDIO_PCM_RATE_96K:
		*code = 0x9U;
		break;
	case AUDIO_PCM_RATE_192K:
		*code = 0xAU;
		break;
	default:
		LOG_INF("Unsupported sample rate %u", sample_rate);
		return -ENOTSUP;
	}

	return 0;
}

static int aw88298_get_word_size_codes(audio_pcm_width_t width, uint16_t *fs_code,
				       uint16_t *bck_code)
{
	switch (width) {
	case AUDIO_PCM_WIDTH_16_BITS:
		*fs_code = AW88298_I2SCTRL_FS_16BIT;
		*bck_code = AW88298_I2SCTRL_BCK_16BIT;
		break;
	case AUDIO_PCM_WIDTH_20_BITS:
		*fs_code = AW88298_I2SCTRL_FS_20BIT;
		*bck_code = AW88298_I2SCTRL_BCK_20BIT;
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		*fs_code = AW88298_I2SCTRL_FS_24BIT;
		*bck_code = AW88298_I2SCTRL_BCK_24BIT;
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		*fs_code = AW88298_I2SCTRL_FS_32BIT;
		*bck_code = AW88298_I2SCTRL_BCK_32BIT;
		break;
	default:
		LOG_INF("Unsupported word size %u", width);
		return -ENOTSUP;
	}

	return 0;
}

static int aw88298_get_i2s_mode_code(audio_dai_type_t type, i2s_fmt_t format, uint16_t *code)
{
	const i2s_fmt_t fmt = format & I2S_FMT_DATA_FORMAT_MASK;

	switch (type) {
	case AUDIO_DAI_TYPE_I2S:
		if (fmt != I2S_FMT_DATA_FORMAT_I2S) {
			LOG_INF("I2S DAI requires standard I2S format, got 0x%x", fmt);
			return -ENOTSUP;
		}

		*code = AW88298_I2SCTRL_MODE_I2S;
		return 0;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		if (fmt != I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) {
			LOG_INF("Left-justified DAI requires matching data format, got 0x%x", fmt);
			return -ENOTSUP;
		}

		*code = AW88298_I2SCTRL_MODE_LEFT_JUSTIFIED;
		return 0;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		if (fmt != I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
			LOG_INF("Right-justified DAI requires matching data format, got 0x%x", fmt);
			return -ENOTSUP;
		}

		*code = AW88298_I2SCTRL_MODE_RIGHT_JUSTIFIED;
		return 0;
	default:
		LOG_INF("Unsupported DAI type %d", type);
		return -ENOTSUP;
	}
}

static int aw88298_get_sysctrl_cfg(uint16_t *mask, uint16_t *value)
{
	*mask = AW88298_REG_SYSCTRL_I2SEN | AW88298_REG_SYSCTRL_PWDN;
	*value = AW88298_REG_SYSCTRL_I2SEN;

	return 0;
}

static int aw88298_get_i2sctrl_cfg(const struct audio_codec_cfg *cfg, uint16_t *mask,
				   uint16_t *value)
{
	const i2s_opt_t options = cfg->dai_cfg.i2s.options;
	const i2s_fmt_t format = cfg->dai_cfg.i2s.format;
	uint16_t mode_code;
	uint16_t fs_code;
	uint16_t bck_code;
	uint16_t rate_code;
	int ret;

	if ((format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		LOG_INF("LSB-first data ordering not supported");
		return -ENOTSUP;
	}

	if ((format & I2S_FMT_CLK_FORMAT_MASK) != I2S_FMT_CLK_NF_NB) {
		LOG_INF("Unsupported I2S clock format 0x%x", format & I2S_FMT_CLK_FORMAT_MASK);
		return -ENOTSUP;
	}

	if ((options & I2S_OPT_BIT_CLK_SLAVE) == 0U) {
		LOG_INF("AW88298 requires external LRCLK/BCLK (slave mode)");
		return -ENOTSUP;
	}

	if (!!((options & I2S_OPT_BIT_CLK_SLAVE)) != !!((options & I2S_OPT_FRAME_CLK_SLAVE))) {
		LOG_INF("Inconsistent clock master/slave options 0x%x", options);
		return -ENOTSUP;
	}

	ret = aw88298_get_i2s_mode_code(cfg->dai_type, format, &mode_code);
	if (ret < 0) {
		return ret;
	}

	ret = aw88298_get_word_size_codes(cfg->dai_cfg.i2s.word_size, &fs_code, &bck_code);
	if (ret < 0) {
		return ret;
	}

	ret = aw88298_get_sample_rate_code(cfg->dai_cfg.i2s.frame_clk_freq, &rate_code);
	if (ret < 0) {
		return ret;
	}

	*mask = AW88298_REG_I2SCTRL_I2SMD | AW88298_REG_I2SCTRL_I2SFS | AW88298_REG_I2SCTRL_I2SBCK |
		AW88298_REG_I2SCTRL_I2SSR;
	*value = AW88298_I2SCTRL_I2SMD_VAL(mode_code) | AW88298_I2SCTRL_I2SFS_VAL(fs_code) |
		 AW88298_I2SCTRL_I2SBCK_VAL(bck_code) | AW88298_I2SCTRL_I2SSR_VAL(rate_code);

	return 0;
}

static int aw88298_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint16_t sysctrl_mask;
	uint16_t sysctrl_value;
	uint16_t i2sctrl_mask;
	uint16_t i2sctrl_value;
	int ret;

	LOG_DBG("Configure: rate=%u channels=%u options=0x%x", cfg->dai_cfg.i2s.frame_clk_freq,
		cfg->dai_cfg.i2s.channels, cfg->dai_cfg.i2s.options);

	if (cfg->dai_route != AUDIO_ROUTE_PLAYBACK) {
		LOG_INF("Unsupported route %u", cfg->dai_route);
		return -ENOTSUP;
	}

	ret = aw88298_get_sysctrl_cfg(&sysctrl_mask, &sysctrl_value);
	if (ret < 0) {
		return ret;
	}

	ret = aw88298_get_i2sctrl_cfg(cfg, &i2sctrl_mask, &i2sctrl_value);
	if (ret < 0) {
		return ret;
	}

	ret = aw88298_update_reg(dev, AW88298_REG_SYSCTRL, sysctrl_mask, sysctrl_value);
	if (ret < 0) {
		LOG_ERR("Failed to set SYSCTRL mask=%x val=%x", sysctrl_mask, sysctrl_value);
		return ret;
	}

	ret = aw88298_update_reg(dev, AW88298_REG_I2SCTRL, i2sctrl_mask, i2sctrl_value);
	if (ret < 0) {
		LOG_ERR("Failed to set I2SCTRL mask=%x val=%x", i2sctrl_mask, i2sctrl_value);
		return ret;
	}

	return 0;
}

static void aw88298_start_output(const struct device *dev)
{
	int ret;

	ret = aw88298_update_reg(dev, AW88298_REG_SYSCTRL, AW88298_REG_SYSCTRL_AMPPD, 0);
	if (ret < 0) {
		LOG_ERR("Failed to unset SYSCTRL(AMPPD)");
		return;
	}
}

static void aw88298_stop_output(const struct device *dev)
{
	int ret;

	ret = aw88298_update_reg(dev, AW88298_REG_SYSCTRL, AW88298_REG_SYSCTRL_AMPPD, 0xFFFF);
	if (ret < 0) {
		LOG_ERR("Failed to set SYSCTRL(AMPPD)");
		return;
	}
}

static int aw88298_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	struct aw88298_data *data = dev->data;
	int ret = 0;

	if (channel != AUDIO_CHANNEL_ALL && channel != AUDIO_CHANNEL_FRONT_LEFT &&
	    channel != AUDIO_CHANNEL_FRONT_RIGHT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		if (val.vol < AW88298_VOLUME_DB_MIN || val.vol > AW88298_VOLUME_DB_MAX) {
			ret = -EINVAL;
			break;
		}

		data->volume = val.vol;
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		data->mute = val.mute;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int aw88298_apply_properties(const struct device *dev)
{
	struct aw88298_data *data = dev->data;
	uint16_t volume_field;
	uint16_t mute_field;
	int volume;
	bool mute;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	volume = data->volume;
	mute = data->mute;
	k_mutex_unlock(&data->lock);

	volume_field = AW88298_HAGCCFG4_VOL_VAL(aw88298_db2vol(volume));
	mute_field = mute ? AW88298_REG_SYSCTRL2_HMUTE : 0;

	ret = aw88298_update_reg(dev, AW88298_REG_HAGCCFG4, AW88298_REG_HAGCCFG4_VOL, volume_field);
	if (ret < 0) {
		LOG_ERR("Failed to set HAGCCFG4(VOL) %x", volume_field);
		return ret;
	}

	ret = aw88298_update_reg(dev, AW88298_REG_SYSCTRL2, AW88298_REG_SYSCTRL2_HMUTE, mute_field);
	if (ret < 0) {
		LOG_ERR("Failed to set SYSCTRL2(MUTE) %x", mute_field);
		return ret;
	}

	return 0;
}

static const struct audio_codec_api aw88298_api = {
	.configure = aw88298_configure,
	.start_output = aw88298_start_output,
	.stop_output = aw88298_stop_output,
	.set_property = aw88298_set_property,
	.apply_properties = aw88298_apply_properties,
};

static int aw88298_init(const struct device *dev)
{
	const struct aw88298_config *cfg = dev->config;
	struct aw88298_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C controller not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	if (cfg->reset_gpio.port) {
		if (!device_is_ready(cfg->reset_gpio.port)) {
			LOG_ERR("GPIO device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure enable GPIO (%d)", ret);
			return ret;
		}

		k_msleep(AW88298_RESET_DELAY_MS);

		ret = gpio_pin_set_dt(&cfg->reset_gpio, false);
		if (ret < 0) {
			LOG_ERR("Failed to deassert reset pin");
			return ret;
		}
	} else {
		ret = aw88298_update_reg(dev, AW88298_REG_ID, 0xFFFF, AW88298_ID_SOFTRESET);
		if (ret < 0) {
			LOG_ERR("Software-Reset failed.");
			return ret;
		}
	}

	ret = aw88298_update_reg(dev, AW88298_REG_SYSCTRL,
				 AW88298_REG_SYSCTRL_AMPPD | AW88298_REG_SYSCTRL_PWDN, 0);
	if (ret < 0) {
		LOG_ERR("Failed to unset SYSCTRL(AMPPD|PWDN)");
		return ret;
	}

	ret = aw88298_update_reg(dev, AW88298_REG_SYSCTRL2, AW88298_REG_SYSCTRL2_HMUTE, 0);
	if (ret < 0) {
		LOG_ERR("Failed to unset SYSCTRL2(HMUTE)");
		return ret;
	}

	return 0;
}

#define AW88298_INST(idx)                                                                          \
	static const struct aw88298_config aw88298_config_##idx = {                                \
		.bus = I2C_DT_SPEC_INST_GET(idx),                                                  \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(idx, reset_gpios, {0}),                     \
	};                                                                                         \
	static struct aw88298_data aw88298_data_##idx;                                             \
	DEVICE_DT_INST_DEFINE(idx, aw88298_init, NULL, &aw88298_data_##idx, &aw88298_config_##idx, \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &aw88298_api)

DT_INST_FOREACH_STATUS_OKAY(AW88298_INST)
