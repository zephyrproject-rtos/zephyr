/*
 * Copyright (c) 2026 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 * Copyright (c) 2026 Mario Paja
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cs42l51

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cirrus_cs42l51, 4);

/*
 * See datasheet: https://www.cirrus.com/products/cs42l51/
 */

/* Register map */
#define REG_CHIP_ID          0x01
#define REG_POWER_CTRL1      0x02
#define REG_MIC_POWER_CTRL   0x03
#define REG_IFACE_CTRL       0x04
#define REG_MIC_CTRL         0x05
#define REG_ADC_CTRL         0x06
#define REG_ADC_INPUT        0x07
#define REG_DAC_OUT_CTRL     0x08
#define REG_DAC_CTRL         0x09
#define REG_ALC_PGA_CTRL     0x0a
#define REG_ALC_PGB_CTRL     0x0b
#define REG_ADCA_ATT         0x0c
#define REG_ADCB_ATT         0x0d
#define REG_ADCA_VOL         0x0e
#define REG_ADCB_VOL         0x0f
#define REG_PCMA_VOL         0x10
#define REG_PCMB_VOL         0x11
#define REG_BEEP_FREQ        0x12
#define REG_BEEP_VOL         0x13
#define REG_BEEP_CONF        0x14
#define REG_TONE_CTRL        0x15
#define REG_AOUTA_VOL        0x16
#define REG_AOUTB_VOL        0x17
#define REG_PCM_MIXER        0x18
#define REG_LIMIT_THRES_DIS  0x19
#define REG_LIMIT_REL        0x1a
#define REG_LIMIT_ATT        0x1b
#define REG_ALC_EN           0x1c
#define REG_ALC_REL          0x1d
#define REG_ALC_THRES        0x1e
#define REG_NOISE_CONF       0x1f
#define REG_STATUS           0x20
#define REG_CHARGE_FREQ      0x21

/* Chip revision register (mirrors REG_CHIP_ID at address 0x01) */
#define CHIP_ID              0x1b
#define CHIP_ID_SHIFT        3
#define CHIP_REV_MASK        0x07

/* Power Control 1 (0x02) */
#define POWER_CTRL1_PDN       BIT(0)
#define POWER_CTRL1_PDN_ADCA  BIT(1)
#define POWER_CTRL1_PDN_ADCB  BIT(2)
#define POWER_CTRL1_PDN_PGAA  BIT(3)
#define POWER_CTRL1_PDN_PGAB  BIT(4)
#define POWER_CTRL1_PDN_DACA  BIT(5)
#define POWER_CTRL1_PDN_DACB  BIT(6)

/* Mic Power Control / speed mode (0x03) */
#define MIC_POWER_CTRL_MCLK_DIV2   BIT(0)
#define MIC_POWER_CTRL_PDN_BIAS    BIT(1)
#define MIC_POWER_CTRL_PDN_MICA    BIT(2)
#define MIC_POWER_CTRL_PDN_MICB    BIT(3)
#define MIC_POWER_CTRL_3ST_SP      BIT(4)
#define MIC_POWER_CTRL_SPEED_SHIFT 5
#define MIC_POWER_CTRL_SPEED_MASK  (0x3 << MIC_POWER_CTRL_SPEED_SHIFT)
#define MIC_POWER_CTRL_SPEED(x)    (((x) << MIC_POWER_CTRL_SPEED_SHIFT) & MIC_POWER_CTRL_SPEED_MASK)
#define MIC_POWER_CTRL_AUTO        BIT(7)

#define SPEED_MODE_DSM        0
#define SPEED_MODE_SSM        1
#define SPEED_MODE_HSM        2
#define SPEED_MODE_QSM        3

/* Interface Control (0x04) */
#define IFACE_CTRL_MICMIX           BIT(0)
#define IFACE_CTRL_DIGMIX           BIT(1)
#define IFACE_CTRL_ADC_I2S          BIT(2)
#define IFACE_CTRL_DAC_FORMAT_SHIFT 3
#define IFACE_CTRL_DAC_FORMAT_MASK  (0x7 << IFACE_CTRL_DAC_FORMAT_SHIFT)
#define IFACE_CTRL_DAC_FORMAT(x)                                              \
	(((x) << IFACE_CTRL_DAC_FORMAT_SHIFT) & IFACE_CTRL_DAC_FORMAT_MASK)
#define IFACE_CTRL_MASTER           BIT(6)
#define IFACE_CTRL_LOOPBACK         BIT(7)

#define DAC_DIF_LJ    0x00
#define DAC_DIF_I2S   0x01
#define DAC_DIF_RJ24  0x02
#define DAC_DIF_RJ16  0x05

/* ADC Input (0x07) */
#define ADC_INPUT_ADCA_MUTE      BIT(0)
#define ADC_INPUT_ADCB_MUTE      BIT(1)
#define ADC_INPUT_AINA_MUX_SHIFT 4
#define ADC_INPUT_AINA_MUX_MASK  (0x3 << ADC_INPUT_AINA_MUX_SHIFT)
#define ADC_INPUT_AINB_MUX_SHIFT 6
#define ADC_INPUT_AINB_MUX_MASK  (0x3 << ADC_INPUT_AINB_MUX_SHIFT)

/* ADC input mux selection, shared encoding for AINA_MUX and AINB_MUX. */
#define ADC_INPUT_MUX_AIN1       0
#define ADC_INPUT_MUX_AIN2       1
#define ADC_INPUT_MUX_MIC        2
#define ADC_INPUT_MUX_MIC_PREAMP 3

/* Maps the devicetree "input-channel" property (1, 2 or 3) to the ADC input
 * mux encoding above; channel 3 is the MICIN1/MICIN2 pins wired to MIC.
 */
#define CS42L51_INPUT_MUX(channel)                                           \
	((channel) == 1 ? ADC_INPUT_MUX_AIN1 :                                \
	 (channel) == 2 ? ADC_INPUT_MUX_AIN2 : ADC_INPUT_MUX_MIC)

/* DAC Output Control (0x08) */
#define DAC_OUT_CTRL_DACA_MUTE BIT(0)
#define DAC_OUT_CTRL_DACB_MUTE BIT(1)

/* ADCx/PCMx mixer volume registers (0x0e-0x11): 7-bit two's complement gain,
 * bit 7 mutes the mixer output.
 */
#define MIX_VOL_MASK  0x7f
#define MIX_VOL_MUTE  BIT(7)

/* Above this MCLK frequency the internal MCLK/2 pre-divider must be used. */
#define MCLK_MAX_UNDIVIDED 27000000

struct cs42l51_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	uint8_t input_mic;
};

static inline int cs42l51_update(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t mask,
				 uint8_t value)
{
	return i2c_reg_update_byte_dt(i2c, reg, mask, value);
}

static int cs42l51_configure_playback(const struct i2c_dt_spec *i2c)
{
	int ret;

	ret = cs42l51_update(i2c, REG_POWER_CTRL1,
			      POWER_CTRL1_PDN_DACA | POWER_CTRL1_PDN_DACB, 0);
	if (ret < 0) {
		return ret;
	}

	return cs42l51_update(i2c, REG_DAC_OUT_CTRL,
			       DAC_OUT_CTRL_DACA_MUTE | DAC_OUT_CTRL_DACB_MUTE, 0);
}

static int cs42l51_configure_capture(const struct i2c_dt_spec *i2c, uint8_t input_mic)
{
	uint8_t mic_pdn = MIC_POWER_CTRL_PDN_MICA | MIC_POWER_CTRL_PDN_MICB |
			  MIC_POWER_CTRL_PDN_BIAS;
	int ret;

	/* Capture needs the mic bias/inputs powered, unlike the default. */
	ret = cs42l51_update(i2c, REG_MIC_POWER_CTRL, mic_pdn, 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The devicetree "input-channel" property selects AIN1, AIN2 or the
	 * MICIN1/MICIN2 pins on both ADC channels. None of those options use
	 * the mic preamp (PGA), so it stays powered down.
	 */
	ret = cs42l51_update(i2c, REG_POWER_CTRL1,
			      POWER_CTRL1_PDN_ADCA | POWER_CTRL1_PDN_ADCB, 0);
	if (ret < 0) {
		return ret;
	}

	return cs42l51_update(i2c, REG_ADC_INPUT,
			       ADC_INPUT_AINA_MUX_MASK | ADC_INPUT_AINB_MUX_MASK |
			       ADC_INPUT_ADCA_MUTE | ADC_INPUT_ADCB_MUTE,
			       (input_mic << ADC_INPUT_AINA_MUX_SHIFT) |
			       (input_mic << ADC_INPUT_AINB_MUX_SHIFT));
}

static int cs42l51_configure(const struct device *dev, struct audio_codec_cfg *audiocfg)
{
	const struct cs42l51_config *cfg = dev->config;
	const struct i2s_config *i2s = &audiocfg->dai_cfg.i2s;
	uint8_t iface_ctrl = 0;
	uint8_t mic_power_ctrl;
	uint8_t format;
	int ret;

	if (audiocfg->dai_route == AUDIO_ROUTE_BYPASS) {
		return 0;
	}

	/* Mic bias/inputs default powered down; cs42l51_configure_capture() powers them up. */
	mic_power_ctrl = MIC_POWER_CTRL_PDN_MICA | MIC_POWER_CTRL_PDN_MICB |
			 MIC_POWER_CTRL_PDN_BIAS;

	/* The codec's DAC word length fields do not cover 32-bit words. */
	if (i2s->word_size == 32) {
		LOG_ERR("Unsupported word size %u", i2s->word_size);
		return -ENOTSUP;
	}

	switch (audiocfg->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		format = DAC_DIF_I2S;
		iface_ctrl |= IFACE_CTRL_ADC_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		format = DAC_DIF_LJ;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		switch (i2s->word_size) {
		case 16:
			format = DAC_DIF_RJ16;
			break;
		case 24:
			format = DAC_DIF_RJ24;
			break;
		default:
			LOG_ERR("Unsupported word size %u for right-justified format",
				i2s->word_size);
			return -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported DAI type %d", audiocfg->dai_type);
		return -ENOTSUP;
	}

	iface_ctrl |= IFACE_CTRL_DAC_FORMAT(format);

	if ((i2s->options & I2S_OPT_FRAME_CLK_TARGET) == 0) {
		uint32_t ratio;

		if (i2s->frame_clk_freq == 0) {
			return -ENOTSUP;
		}

		iface_ctrl |= IFACE_CTRL_MASTER;

		/* MCLK frequency is expected to be supplied by audiocfg->mclk_freq. */
		ratio = audiocfg->mclk_freq / i2s->frame_clk_freq;

		switch (ratio) {
		case 128:
			mic_power_ctrl |= MIC_POWER_CTRL_SPEED(SPEED_MODE_SSM);
			break;
		case 256:
			mic_power_ctrl |= MIC_POWER_CTRL_SPEED(SPEED_MODE_SSM) |
					 MIC_POWER_CTRL_MCLK_DIV2;
			break;
		default:
			LOG_ERR("Unsupported MCLK/LRCK ratio %u for master mode", ratio);
			return -ENOTSUP;
		}

		if (i2s->frame_clk_freq > 50000) {
			/* Double-speed rates require DSM, regardless of the MCLK divider. */
			mic_power_ctrl = (mic_power_ctrl & ~MIC_POWER_CTRL_SPEED_MASK) |
					 MIC_POWER_CTRL_SPEED(SPEED_MODE_DSM);
		}
	} else {
		mic_power_ctrl |= MIC_POWER_CTRL_AUTO;

		if (audiocfg->mclk_freq > MCLK_MAX_UNDIVIDED) {
			mic_power_ctrl |= MIC_POWER_CTRL_MCLK_DIV2;
		}
	}

	/* Power everything down while the interface is reconfigured. */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, REG_POWER_CTRL1,
				     POWER_CTRL1_PDN | POWER_CTRL1_PDN_DACA |
				     POWER_CTRL1_PDN_DACB | POWER_CTRL1_PDN_PGAA |
				     POWER_CTRL1_PDN_PGAB | POWER_CTRL1_PDN_ADCA |
				     POWER_CTRL1_PDN_ADCB);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, REG_IFACE_CTRL, iface_ctrl);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, REG_MIC_POWER_CTRL, mic_power_ctrl);
	if (ret < 0) {
		return ret;
	}

	switch (audiocfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		ret = cs42l51_configure_playback(&cfg->i2c);
		break;
	case AUDIO_ROUTE_CAPTURE:
		ret = cs42l51_configure_capture(&cfg->i2c, cfg->input_mic);
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		ret = cs42l51_configure_playback(&cfg->i2c);
		if (ret == 0) {
			ret = cs42l51_configure_capture(&cfg->i2c, cfg->input_mic);
		}
		break;
	default:
		return -ENOTSUP;
	}

	if (ret < 0) {
		LOG_ERR("Failed to configure DAI route %d (err %d)", audiocfg->dai_route, ret);
		return ret;
	}

	return cs42l51_update(&cfg->i2c, REG_POWER_CTRL1, POWER_CTRL1_PDN, 0);
}

static void cs42l51_start_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void cs42l51_stop_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int cs42l51_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static inline int cs42l51_set_mute(const struct i2c_dt_spec *i2c, uint8_t reg,
				   audio_channel_t channel, bool mute)
{
	uint8_t mask;

	switch (channel) {
	case AUDIO_CHANNEL_ALL:
		mask = BIT(0) | BIT(1);
		break;
	case AUDIO_CHANNEL_FRONT_LEFT:
		mask = BIT(0);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		mask = BIT(1);
		break;
	default:
		return -ENOTSUP;
	}

	return cs42l51_update(i2c, reg, mask, mute ? mask : 0);
}

/*
 * Linear volume scale over the register's signed range (-64..+63 in
 * hardware-specific 0.5 dB steps), so the mapping is monotonic across the
 * whole two's complement field without relying on datasheet dB endpoints.
 */
static inline int cs42l51_set_volume(const struct i2c_dt_spec *i2c, uint8_t reg_a, uint8_t reg_b,
				     audio_channel_t channel, int vol)
{
	int32_t signed_vol = -64 + (127 * CLAMP(vol, 0, 100)) / 100;
	uint8_t raw = (uint8_t)signed_vol & MIX_VOL_MASK;
	int ret;

	switch (channel) {
	case AUDIO_CHANNEL_ALL:
		ret = cs42l51_update(i2c, reg_a, MIX_VOL_MASK, raw);
		if (ret < 0) {
			return ret;
		}
		return cs42l51_update(i2c, reg_b, MIX_VOL_MASK, raw);
	case AUDIO_CHANNEL_FRONT_LEFT:
		return cs42l51_update(i2c, reg_a, MIX_VOL_MASK, raw);
	case AUDIO_CHANNEL_FRONT_RIGHT:
		return cs42l51_update(i2c, reg_b, MIX_VOL_MASK, raw);
	default:
		return -ENOTSUP;
	}
}

static int cs42l51_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	const struct cs42l51_config *cfg = dev->config;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return cs42l51_set_mute(&cfg->i2c, REG_DAC_OUT_CTRL, channel, val.mute);
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return cs42l51_set_volume(&cfg->i2c, REG_PCMA_VOL, REG_PCMB_VOL, channel, val.vol);
	case AUDIO_PROPERTY_INPUT_MUTE:
		return cs42l51_set_mute(&cfg->i2c, REG_ADC_INPUT, channel, val.mute);
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return cs42l51_set_volume(&cfg->i2c, REG_ADCA_VOL, REG_ADCB_VOL, channel, val.vol);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(audio_codec, cs42l51_api) = {
	.configure = cs42l51_configure,
	.start_output = cs42l51_start_output,
	.stop_output = cs42l51_stop_output,
	.set_property = cs42l51_set_property,
	.apply_properties = cs42l51_apply_properties,
};

static int cs42l51_init(const struct device *dev)
{
	const struct cs42l51_config *cfg = dev->config;
	uint8_t regval;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Unable to configure reset GPIO");
		return ret;
	}

	/* Hold RESET asserted while the supplies settle. */
	k_sleep(K_MSEC(1));

	ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
	if (ret < 0) {
		return ret;
	}

	/* Allow the internal power-on-reset sequence to complete. */
	k_sleep(K_MSEC(2));

	ret = i2c_reg_read_byte_dt(&cfg->i2c, REG_CHIP_ID, &regval);
	if (ret < 0) {
		LOG_ERR("Unable to read device ID");
		return -ENODEV;
	}

	if ((regval >> CHIP_ID_SHIFT) != CHIP_ID) {
		LOG_ERR("Wrong Chip ID (got %02x)", regval);
		return -ENODEV;
	}

	LOG_INF("Found CS42L51 (chip=%02x, rev=%d)", regval >> CHIP_ID_SHIFT,
		regval & CHIP_REV_MASK);

	return 0;
}

#define CS42L51_INIT(inst)                                                   \
	static const struct cs42l51_config cs42l51_config_##inst = {         \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                            \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),       \
		.input_mic = CS42L51_INPUT_MUX(DT_INST_PROP_OR(inst, input_channel, 1)), \
	};                                                                    \
	DEVICE_DT_INST_DEFINE(inst, cs42l51_init, NULL, NULL,                 \
			      &cs42l51_config_##inst, POST_KERNEL,            \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY,               \
			      &cs42l51_api);

DT_INST_FOREACH_STATUS_OKAY(CS42L51_INIT)
