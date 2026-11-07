/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT everest_es8311

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(es8311, CONFIG_AUDIO_CODEC_LOG_LEVEL);

enum es8311_register {
	ES8311_RESET = 0x00,
	ES8311_CLOCK_ENABLE = 0x01,
	ES8311_CLOCK_PRE = 0x02,
	ES8311_DAC_OSR = 0x04,
	ES8311_CLOCK_DIV = 0x05,
	ES8311_LRCK_H = 0x07,
	ES8311_LRCK_L = 0x08,
	ES8311_SDP_IN = 0x09,
	ES8311_SDP_OUT = 0x0a,
	ES8311_ANALOG_POWER = 0x0d,
	ES8311_ADC_POWER = 0x0e,
	ES8311_DAC_POWER = 0x12,
	ES8311_OUTPUT = 0x13,
	ES8311_DAC_MUTE = 0x31,
	ES8311_DAC_VOLUME = 0x32,
	ES8311_DAC_EQ = 0x37,
	ES8311_CHIP_ID_H = 0xfd,
	ES8311_CHIP_ID_L = 0xfe,
};

#define ES8311_MUTE (BIT(6) | BIT(5))
#define ES8311_VOLUME_0DB 0xbf

struct es8311_config {
	struct i2c_dt_spec i2c;
	const struct device *supply;
	struct gpio_dt_spec pa;
};

struct es8311_data {
	struct k_mutex lock;
	int32_t volume;
	uint8_t clock_pre;
	uint8_t sdp_in;
	bool mute;
	bool configured;
	bool started;
	bool suspended;
};

struct es8311_reg_value {
	uint8_t reg;
	uint8_t value;
};

static int es8311_write(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct es8311_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, value);
}

static int es8311_write_regs(const struct device *dev, const struct es8311_reg_value *regs,
			     size_t count)
{
	int ret;

	for (size_t i = 0; i < count; i++) {
		ret = es8311_write(dev, regs[i].reg, regs[i].value);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int es8311_pa_set(const struct device *dev, bool enable)
{
	const struct es8311_config *config = dev->config;

	if (config->pa.port == NULL) {
		return 0;
	}

	return gpio_pin_set_dt(&config->pa, enable);
}

static int es8311_reset(const struct device *dev)
{
	int ret;

	ret = es8311_write(dev, ES8311_RESET, 0x1f);
	if (ret != 0) {
		return ret;
	}

	k_msleep(20);
	ret = es8311_write(dev, ES8311_RESET, 0x00);
	if (ret != 0) {
		return ret;
	}

	/* Start the state machine with the serial port in clock target mode. */
	ret = es8311_write(dev, ES8311_RESET, 0x80);
	if (ret != 0) {
		return ret;
	}

	return es8311_write(dev, ES8311_DAC_MUTE, ES8311_MUTE);
}

static int es8311_power_on(const struct device *dev)
{
	const struct es8311_config *config = dev->config;
	uint8_t id_h;
	uint8_t id_l;
	int ret;

	if (IS_ENABLED(CONFIG_REGULATOR) && config->supply != NULL) {
		ret = regulator_enable(config->supply);
		if (ret != 0) {
			return ret;
		}
	}

	/* Allow the codec's power-on reset to complete before accessing I2C. */
	k_msleep(20);
	ret = i2c_reg_read_byte_dt(&config->i2c, ES8311_CHIP_ID_H, &id_h);
	if (ret == 0) {
		ret = i2c_reg_read_byte_dt(&config->i2c, ES8311_CHIP_ID_L, &id_l);
	}
	if (ret == 0 && (id_h != 0x83 || id_l != 0x11)) {
		LOG_ERR("Unexpected chip ID %02x%02x", id_h, id_l);
		ret = -ENODEV;
	}
	if (ret == 0) {
		ret = es8311_reset(dev);
	}
	if (ret != 0 && IS_ENABLED(CONFIG_REGULATOR) && config->supply != NULL) {
		int err = regulator_disable(config->supply);

		if (err != 0) {
			LOG_ERR("Failed to disable supply (%d)", err);
		}
	}

	return ret;
}

static int es8311_update_properties(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	/* The DAC volume register has 0.5 dB steps, with 0 dB at 0xbf. */
	ret = es8311_write(dev, ES8311_DAC_VOLUME, ES8311_VOLUME_0DB + 2 * data->volume);
	if (ret != 0) {
		return ret;
	}

	return es8311_write(dev, ES8311_DAC_MUTE,
			    data->mute || !data->started ? ES8311_MUTE : 0);
}

static int es8311_setup(const struct device *dev, uint8_t clock_pre, uint8_t sdp_in)
{
	const struct es8311_reg_value regs[] = {
		{ES8311_DAC_MUTE, ES8311_MUTE},
		/* MCLK input, DAC clock and serial input clock enabled. */
		{ES8311_CLOCK_ENABLE, 0x35},
		{ES8311_CLOCK_PRE, clock_pre},
		{ES8311_DAC_OSR, 0x10},
		{ES8311_CLOCK_DIV, 0x00},
		{ES8311_LRCK_H, 0x00},
		{ES8311_LRCK_L, 0xff},
		{ES8311_SDP_IN, sdp_in},
		/* Disable the unused serial output and analog capture path. */
		{ES8311_SDP_OUT, 0x40},
		{ES8311_ANALOG_POWER, 0x31},
		{ES8311_ADC_POWER, 0x72},
		{ES8311_DAC_POWER, 0x00},
		{ES8311_OUTPUT, 0x10},
		{ES8311_DAC_EQ, 0x08},
	};
	int ret;

	ret = es8311_write_regs(dev, regs, ARRAY_SIZE(regs));
	if (ret != 0) {
		return ret;
	}

	return es8311_update_properties(dev);
}

static int es8311_clock_config(uint32_t mclk, uint32_t rate, uint8_t *clock_pre)
{
	if (rate < 8000U || rate > 48000U || mclk == 0U) {
		return -EINVAL;
	}

	/* Use a 256 Fs DAC clock. Keep the pre-divider output below 4.5 MHz
	 * to support the full supply voltage range, including 1.8 V.
	 */
	for (uint32_t mult = 0; mult < 4U; mult++) {
		for (uint32_t div = 1; div <= 8U; div++) {
			if ((uint64_t)mclk >= 4500000ULL * div) {
				continue;
			}
			if (((uint64_t)mclk << mult) == (uint64_t)rate * 256U * div) {
				*clock_pre = ((div - 1U) << 5) | (mult << 3);
				return 0;
			}
		}
	}

	return -ENOTSUP;
}

static int es8311_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct es8311_data *data = dev->data;
	const struct i2s_config *i2s = &cfg->dai_cfg.i2s;
	const i2s_opt_t target = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	uint8_t clock_pre;
	uint8_t sdp_in;
	int ret;

	if (cfg->dai_route != AUDIO_ROUTE_PLAYBACK || cfg->dai_type != AUDIO_DAI_TYPE_I2S ||
	    i2s->format != I2S_FMT_DATA_FORMAT_I2S || (i2s->options & target) != target ||
	    i2s->channels != 2U) {
		return -ENOTSUP;
	}

	switch (i2s->word_size) {
	case 16:
		sdp_in = 3U << 2;
		break;
	case 18:
		sdp_in = 2U << 2;
		break;
	case 20:
		sdp_in = 1U << 2;
		break;
	case 24:
		sdp_in = 0;
		break;
	case 32:
		sdp_in = 4U << 2;
		break;
	default:
		return -ENOTSUP;
	}

	ret = es8311_clock_config(cfg->mclk_freq, i2s->frame_clk_freq, &clock_pre);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->suspended) {
		ret = -EACCES;
	} else if (data->started) {
		ret = -EBUSY;
	} else {
		data->configured = false;
		ret = es8311_setup(dev, clock_pre, sdp_in);
		if (ret == 0) {
			data->clock_pre = clock_pre;
			data->sdp_in = sdp_in;
			data->configured = true;
		}
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int es8311_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	if (dir != AUDIO_DAI_DIR_TX) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->suspended) {
		ret = -EACCES;
	} else if (!data->configured) {
		ret = -EIO;
	} else if (!data->started) {
		ret = es8311_write(dev, ES8311_DAC_MUTE, data->mute ? ES8311_MUTE : 0);
		if (ret == 0) {
			ret = es8311_pa_set(dev, true);
		}
		if (ret == 0) {
			data->started = true;
		} else {
			int err = es8311_write(dev, ES8311_DAC_MUTE, ES8311_MUTE);

			if (err != 0) {
				LOG_ERR("Failed to mute output (%d)", err);
			}
		}
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int es8311_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct es8311_data *data = dev->data;
	int ret = 0;
	int err;

	if (dir != AUDIO_DAI_DIR_TX) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->suspended) {
		ret = es8311_write(dev, ES8311_DAC_MUTE, ES8311_MUTE);
		/* Disable the amplifier even if the codec cannot be reached. */
		err = es8311_pa_set(dev, false);
		if (ret == 0) {
			ret = err;
		}
		if (ret == 0) {
			data->started = false;
		}
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static void es8311_start_output(const struct device *dev)
{
	int ret = es8311_start(dev, AUDIO_DAI_DIR_TX);

	if (ret != 0) {
		LOG_ERR("Failed to start output (%d)", ret);
	}
}

static void es8311_stop_output(const struct device *dev)
{
	int ret = es8311_stop(dev, AUDIO_DAI_DIR_TX);

	if (ret != 0) {
		LOG_ERR("Failed to stop output (%d)", ret);
	}
}

static int es8311_set_property(const struct device *dev, audio_property_t property,
			       audio_channel_t channel, audio_property_value_t value)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	if (channel != AUDIO_CHANNEL_ALL && channel != AUDIO_CHANNEL_FRONT_LEFT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		if (value.vol < -95 || value.vol > 32) {
			ret = -EINVAL;
		} else {
			data->volume = value.vol;
		}
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		data->mute = value.mute;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int es8311_apply_properties(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = data->suspended ? -EACCES : es8311_update_properties(dev);
	k_mutex_unlock(&data->lock);

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int es8311_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct es8311_config *config = dev->config;
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND: {
		static const struct es8311_reg_value standby[] = {
			{ES8311_DAC_MUTE, ES8311_MUTE},
			{ES8311_DAC_POWER, 0x02},
			{ES8311_OUTPUT, 0x00},
			{ES8311_ADC_POWER, 0x7a},
			{ES8311_ANALOG_POWER, 0xfc},
			{ES8311_CLOCK_ENABLE, 0x00},
		};

		if (data->started) {
			ret = -EBUSY;
			break;
		}
		ret = es8311_write_regs(dev, standby, ARRAY_SIZE(standby));
		if (ret == 0 && IS_ENABLED(CONFIG_REGULATOR) && config->supply != NULL) {
			ret = regulator_disable(config->supply);
		}
		if (ret == 0) {
			data->suspended = true;
		} else {
			/* Require reconfiguration after an incomplete shutdown. */
			data->configured = false;
		}
		break;
	}
	case PM_DEVICE_ACTION_RESUME:
		ret = es8311_power_on(dev);
		if (ret == 0 && data->configured) {
			ret = es8311_setup(dev, data->clock_pre, data->sdp_in);
			if (ret != 0 && IS_ENABLED(CONFIG_REGULATOR) && config->supply != NULL) {
				int err = regulator_disable(config->supply);

				if (err != 0) {
					LOG_ERR("Failed to disable supply (%d)", err);
				}
			}
		}
		if (ret == 0) {
			data->suspended = false;
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}
#endif

static int es8311_init(const struct device *dev)
{
	const struct es8311_config *config = dev->config;
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C controller not ready");
		return -ENODEV;
	}
	if (IS_ENABLED(CONFIG_REGULATOR) && config->supply != NULL &&
	    !device_is_ready(config->supply)) {
		LOG_ERR("Codec supply not ready");
		return -ENODEV;
	}
	if (config->pa.port != NULL) {
		if (!gpio_is_ready_dt(&config->pa)) {
			LOG_ERR("Amplifier GPIO controller not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pa, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			return ret;
		}
	}

	ret = es8311_power_on(dev);
	if (ret != 0) {
		LOG_ERR("Failed to power on codec (%d)", ret);
	}

	return ret;
}

static DEVICE_API(audio_codec, es8311_api) = {
	.configure = es8311_configure,
	.start_output = es8311_start_output,
	.stop_output = es8311_stop_output,
	.set_property = es8311_set_property,
	.apply_properties = es8311_apply_properties,
	.start = es8311_start,
	.stop = es8311_stop,
};

#define ES8311_DEFINE(inst)                                                                       \
	static const struct es8311_config es8311_config_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                \
		.supply = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vin_supply)),               \
		.pa = GPIO_DT_SPEC_INST_GET_OR(inst, pa_gpios, {0}),                              \
	};                                                                                        \
	static struct es8311_data es8311_data_##inst;                                             \
	PM_DEVICE_DT_INST_DEFINE(inst, es8311_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(inst, es8311_init, PM_DEVICE_DT_INST_GET(inst), &es8311_data_##inst,\
			      &es8311_config_##inst, POST_KERNEL,                                 \
			      CONFIG_AUDIO_CODEC_ES8311_INIT_PRIORITY, &es8311_api);

DT_INST_FOREACH_STATUS_OKAY(ES8311_DEFINE)
