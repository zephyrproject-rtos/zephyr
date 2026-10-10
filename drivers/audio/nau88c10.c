/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nau88c10

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nau88c10, CONFIG_AUDIO_CODEC_LOG_LEVEL);

/*
 * Register map and power-up order follow the NAU88C10 datasheet, Rev 1.9:
 * https://www.nuvoton.com/export/resource-files/en-us--DS_NAU88C10_DataSheet_EN_Rev1.9.pdf
 *
 * Only the audio interface target configuration is supported, and the volumes
 * audio_codec_set_property() takes are in dB.
 */
#define NAU88C10_REG_RESET       0x00
#define NAU88C10_REG_POWER1      0x01
#define NAU88C10_REG_POWER2      0x02
#define NAU88C10_REG_POWER3      0x03
#define NAU88C10_REG_AUDIO_IF    0x04
#define NAU88C10_REG_CLOCK1      0x06
#define NAU88C10_REG_CLOCK2      0x07
#define NAU88C10_REG_DAC_CTRL    0x0a
#define NAU88C10_REG_ADC_CTRL    0x0e
#define NAU88C10_REG_INPUT_CTRL  0x2c
#define NAU88C10_REG_PGA_GAIN    0x2d
#define NAU88C10_REG_ADC_BOOST   0x2f
#define NAU88C10_REG_SPK_MIXER   0x32
#define NAU88C10_REG_SPK_VOLUME  0x36
#define NAU88C10_REG_MONO_MIXER  0x38
#define NAU88C10_REG_DEVICE_ID   0x3f

/* Power Management 1 (0x01) */
#define NAU88C10_POWER1_REFIMP_3K  0x3
#define NAU88C10_POWER1_REFIMP_80K 0x1
#define NAU88C10_POWER1_IOBUFEN    BIT(2)
#define NAU88C10_POWER1_ABIASEN    BIT(3)
#define NAU88C10_POWER1_MICBIASEN  BIT(4)

/* Power Management 2 (0x02) */
#define NAU88C10_POWER2_ADCEN BIT(0)
#define NAU88C10_POWER2_PGAEN BIT(2)
#define NAU88C10_POWER2_BSTEN BIT(4)

/* Power Management 3 (0x03) */
#define NAU88C10_POWER3_DACEN    BIT(0)
#define NAU88C10_POWER3_SPKMXEN  BIT(2)
#define NAU88C10_POWER3_MOUTMXEN BIT(3)
#define NAU88C10_POWER3_PSPKEN   BIT(5)
#define NAU88C10_POWER3_NSPKEN   BIT(6)
#define NAU88C10_POWER3_MOUTEN   BIT(7)

/* Audio Interface (0x04) */
#define NAU88C10_AUDIO_IF_AIFMT_MASK            GENMASK(4, 3)
#define NAU88C10_AUDIO_IF_AIFMT_RIGHT_JUSTIFIED 0x0
#define NAU88C10_AUDIO_IF_AIFMT_LEFT_JUSTIFIED  0x1
#define NAU88C10_AUDIO_IF_AIFMT_I2S             0x2
#define NAU88C10_AUDIO_IF_WLEN_MASK             GENMASK(6, 5)
#define NAU88C10_AUDIO_IF_WLEN_16               0x0
#define NAU88C10_AUDIO_IF_WLEN_20               0x1
#define NAU88C10_AUDIO_IF_WLEN_24               0x2
#define NAU88C10_AUDIO_IF_WLEN_32               0x3
#define NAU88C10_AUDIO_IF_FSP                   BIT(7)
#define NAU88C10_AUDIO_IF_BCLKP                 BIT(8)

/* Clock Control 1 (0x06): MCLKSEL divides MCLK down to the 256 x Fs core clock */
#define NAU88C10_CLOCK1_MCLKSEL_MASK GENMASK(7, 5)

/* Clock Control 2 (0x07) */
#define NAU88C10_CLOCK2_SMPLR_MASK GENMASK(3, 1)

/* DAC Control (0x0a) */
#define NAU88C10_DAC_CTRL_DACMT BIT(6)

/* ADC Control (0x0e) */
#define NAU88C10_ADC_CTRL_HPFEN BIT(8)

/* Input Control (0x2c) */
#define NAU88C10_INPUT_CTRL_PMICPGA BIT(0)
#define NAU88C10_INPUT_CTRL_NMICPGA BIT(1)

/* PGA Gain (0x2d) */
#define NAU88C10_PGA_GAIN_MASK  GENMASK(5, 0)
#define NAU88C10_PGA_GAIN_PGAMT BIT(6)

/* ADC Boost (0x2f) */
#define NAU88C10_ADC_BOOST_PGABST BIT(8)

/* Speaker Mixer (0x32) */
#define NAU88C10_SPK_MIXER_DACSPK BIT(0)

/* Speaker Volume (0x36) */
#define NAU88C10_SPK_VOLUME_MASK  GENMASK(5, 0)
#define NAU88C10_SPK_VOLUME_SPKMT BIT(6)
#define NAU88C10_SPK_VOLUME_SPKZC BIT(7)

/* Mono Mixer (0x38) */
#define NAU88C10_MONO_MIXER_DACMOUT  BIT(0)
#define NAU88C10_MONO_MIXER_MOUTMXMT BIT(6)

/*
 * A write packs a 7-bit register address and a 9-bit value into two bytes; a
 * read sends the address alone in a control byte, then reads the value back.
 */
#define NAU88C10_FRAME_ADDR_MASK  GENMASK(15, 9)
#define NAU88C10_FRAME_VALUE_MASK GENMASK(8, 0)
#define NAU88C10_CTRL_ADDR_MASK   GENMASK(7, 1)

/* 2-Wire ID (0x3f), which reads back the codec I2C address */
#define NAU88C10_DEVICE_ID 0x01a

/* Register values left untouched by a configure() call */
#define NAU88C10_SPK_VOLUME_RESET 0x039
#define NAU88C10_PGA_GAIN_RESET   0x010

/* Speaker amplifier gain: -57 dB to 6 dB in 1 dB steps */
#define NAU88C10_SPK_VOLUME_MIN_DB (-57)
#define NAU88C10_SPK_VOLUME_MAX_DB 6

/* Input PGA gain: -12 dB to 35.25 dB in 0.75 dB steps */
#define NAU88C10_PGA_GAIN_MIN_DB (-12)
#define NAU88C10_PGA_GAIN_MAX_DB 35

/* Core clock the codec runs on, as a multiple of the sample rate */
#define NAU88C10_CORE_CLOCK_RATIO 256

/* Time given to VREF to charge through the 3 kOhm reference impedance */
#define NAU88C10_VREF_RAMP_MS 50

struct nau88c10_config {
	struct i2c_dt_spec i2c;
	bool mic_single_ended;
};

struct nau88c10_data {
	/* Shadows of the registers holding both a routing/gain and a mute bit */
	uint16_t spk_volume;
	uint16_t pga_gain;
	uint16_t dac_ctrl;
	uint16_t mono_mixer;
};

static int nau88c10_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct nau88c10_config *cfg = dev->config;
	uint8_t buf[2];

	sys_put_be16(FIELD_PREP(NAU88C10_FRAME_ADDR_MASK, reg) |
		     FIELD_PREP(NAU88C10_FRAME_VALUE_MASK, val), buf);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int nau88c10_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct nau88c10_config *cfg = dev->config;
	uint8_t addr = FIELD_PREP(NAU88C10_CTRL_ADDR_MASK, reg);
	uint8_t buf[2];
	int ret;

	ret = i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*val = FIELD_GET(NAU88C10_FRAME_VALUE_MASK, sys_get_be16(buf));

	return 0;
}

/* MCLKSEL divider values, in halves so that the 1.5 ratio stays an integer */
static const uint8_t nau88c10_mclk_div_x2[] = {2, 3, 4, 6, 8, 12, 16, 24};

static int nau88c10_mclk_sel(uint32_t mclk_freq, uint32_t frame_clk_freq)
{
	uint32_t div_x2 = DIV_ROUND_CLOSEST(2U * mclk_freq,
					    NAU88C10_CORE_CLOCK_RATIO * frame_clk_freq);

	for (size_t i = 0; i < ARRAY_SIZE(nau88c10_mclk_div_x2); i++) {
		if (nau88c10_mclk_div_x2[i] == div_x2) {
			return (int)i;
		}
	}

	return -EINVAL;
}

/* SMPLR only selects the digital filter coefficients, it does not set the rate */
static int nau88c10_sample_rate_sel(uint32_t frame_clk_freq)
{
	switch (frame_clk_freq) {
	case 48000:
		return 0;
	case 32000:
		return 1;
	case 24000:
		return 2;
	case 16000:
		return 3;
	case 12000:
		return 4;
	case 8000:
		return 5;
	default:
		return -EINVAL;
	}
}

static int nau88c10_audio_if(const struct audio_codec_cfg *codec_cfg, uint16_t *audio_if)
{
	const struct i2s_config *i2s_cfg = &codec_cfg->dai_cfg.i2s;
	uint16_t aifmt, wlen;

	switch (codec_cfg->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		aifmt = NAU88C10_AUDIO_IF_AIFMT_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		aifmt = NAU88C10_AUDIO_IF_AIFMT_LEFT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		aifmt = NAU88C10_AUDIO_IF_AIFMT_RIGHT_JUSTIFIED;
		break;
	default:
		LOG_ERR("Unsupported DAI type %d", codec_cfg->dai_type);
		return -ENOTSUP;
	}

	switch (i2s_cfg->word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		wlen = NAU88C10_AUDIO_IF_WLEN_16;
		break;
	case AUDIO_PCM_WIDTH_20_BITS:
		wlen = NAU88C10_AUDIO_IF_WLEN_20;
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		wlen = NAU88C10_AUDIO_IF_WLEN_24;
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		wlen = NAU88C10_AUDIO_IF_WLEN_32;
		break;
	default:
		LOG_ERR("Unsupported word size %d", i2s_cfg->word_size);
		return -ENOTSUP;
	}

	*audio_if = FIELD_PREP(NAU88C10_AUDIO_IF_AIFMT_MASK, aifmt) |
		    FIELD_PREP(NAU88C10_AUDIO_IF_WLEN_MASK, wlen);

	if ((i2s_cfg->format & I2S_FMT_BIT_CLK_INV) != 0) {
		*audio_if |= NAU88C10_AUDIO_IF_BCLKP;
	}

	if ((i2s_cfg->format & I2S_FMT_FRAME_CLK_INV) != 0) {
		*audio_if |= NAU88C10_AUDIO_IF_FSP;
	}

	return 0;
}

static int nau88c10_configure(const struct device *dev, struct audio_codec_cfg *codec_cfg)
{
	const struct nau88c10_config *cfg = dev->config;
	struct nau88c10_data *data = dev->data;
	const struct i2s_config *i2s_cfg = &codec_cfg->dai_cfg.i2s;
	uint16_t power1, power2 = 0, power3 = 0;
	uint16_t audio_if;
	bool playback, capture;
	int mclk_sel, rate_sel;
	int ret;

	switch (codec_cfg->dai_route) {
	case AUDIO_ROUTE_BYPASS:
		playback = false;
		capture = false;
		break;
	case AUDIO_ROUTE_PLAYBACK:
		playback = true;
		capture = false;
		break;
	case AUDIO_ROUTE_CAPTURE:
		playback = false;
		capture = true;
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		playback = true;
		capture = true;
		break;
	default:
		return -EINVAL;
	}

	ret = nau88c10_audio_if(codec_cfg, &audio_if);
	if (ret < 0) {
		return ret;
	}

	if ((i2s_cfg->options & (I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET)) != 0) {
		LOG_ERR("Codec can only be driven as an audio interface target");
		return -ENOTSUP;
	}

	rate_sel = nau88c10_sample_rate_sel(i2s_cfg->frame_clk_freq);
	if (rate_sel < 0) {
		LOG_ERR("Unsupported sample rate %u", i2s_cfg->frame_clk_freq);
		return rate_sel;
	}

	mclk_sel = nau88c10_mclk_sel(codec_cfg->mclk_freq, i2s_cfg->frame_clk_freq);
	if (mclk_sel < 0) {
		LOG_ERR("MCLK %u Hz is not a supported multiple of the %u Hz core clock",
			codec_cfg->mclk_freq,
			NAU88C10_CORE_CLOCK_RATIO * i2s_cfg->frame_clk_freq);
		return mclk_sel;
	}

	/*
	 * Nothing else works until VREF has ramped up, so charge it through the
	 * 3 kOhm reference before settling on the 80 kOhm one used while running.
	 */
	power1 = NAU88C10_POWER1_ABIASEN | NAU88C10_POWER1_IOBUFEN;
	if (capture) {
		power1 |= NAU88C10_POWER1_MICBIASEN;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_POWER1, power1 | NAU88C10_POWER1_REFIMP_3K);
	if (ret < 0) {
		return ret;
	}

	k_msleep(NAU88C10_VREF_RAMP_MS);

	ret = nau88c10_write(dev, NAU88C10_REG_POWER1, power1 | NAU88C10_POWER1_REFIMP_80K);
	if (ret < 0) {
		return ret;
	}

	/* CLKM and CLKIOEN left clear: MCLK drives the codec, which is a clock target */
	ret = nau88c10_write(dev, NAU88C10_REG_CLOCK1,
			     FIELD_PREP(NAU88C10_CLOCK1_MCLKSEL_MASK, mclk_sel));
	if (ret < 0) {
		return ret;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_CLOCK2,
			     FIELD_PREP(NAU88C10_CLOCK2_SMPLR_MASK, rate_sel));
	if (ret < 0) {
		return ret;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_AUDIO_IF, audio_if);
	if (ret < 0) {
		return ret;
	}

	if (capture) {
		uint16_t input_ctrl = NAU88C10_INPUT_CTRL_PMICPGA;

		if (!cfg->mic_single_ended) {
			input_ctrl |= NAU88C10_INPUT_CTRL_NMICPGA;
		}

		ret = nau88c10_write(dev, NAU88C10_REG_INPUT_CTRL, input_ctrl);
		if (ret < 0) {
			return ret;
		}

		/*
		 * 20 dB boost between the PGA and the ADC, as in the microphone
		 * application of the datasheet
		 */
		ret = nau88c10_write(dev, NAU88C10_REG_ADC_BOOST, NAU88C10_ADC_BOOST_PGABST);
		if (ret < 0) {
			return ret;
		}

		/* Filter out the microphone DC offset */
		ret = nau88c10_write(dev, NAU88C10_REG_ADC_CTRL, NAU88C10_ADC_CTRL_HPFEN);
		if (ret < 0) {
			return ret;
		}

		power2 = NAU88C10_POWER2_BSTEN | NAU88C10_POWER2_PGAEN | NAU88C10_POWER2_ADCEN;
	}

	/* Keep the DAC muted until start_output() */
	data->dac_ctrl = NAU88C10_DAC_CTRL_DACMT;
	ret = nau88c10_write(dev, NAU88C10_REG_DAC_CTRL, data->dac_ctrl);
	if (ret < 0) {
		return ret;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_SPK_MIXER,
			     playback ? NAU88C10_SPK_MIXER_DACSPK : 0);
	if (ret < 0) {
		return ret;
	}

	data->mono_mixer = playback ? NAU88C10_MONO_MIXER_DACMOUT : 0;
	ret = nau88c10_write(dev, NAU88C10_REG_MONO_MIXER, data->mono_mixer);
	if (ret < 0) {
		return ret;
	}

	if (playback) {
		power3 = NAU88C10_POWER3_DACEN | NAU88C10_POWER3_SPKMXEN |
			 NAU88C10_POWER3_MOUTMXEN;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_POWER2, power2);
	if (ret < 0) {
		return ret;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_POWER3, power3);
	if (ret < 0) {
		return ret;
	}

	/* The datasheet powers the output stages up last */
	if (playback) {
		power3 |= NAU88C10_POWER3_PSPKEN | NAU88C10_POWER3_NSPKEN |
			  NAU88C10_POWER3_MOUTEN;

		ret = nau88c10_write(dev, NAU88C10_REG_POWER3, power3);
		if (ret < 0) {
			return ret;
		}
	}

	/*
	 * Apply speaker gain changes right away rather than on a zero crossing,
	 * which a silent output never produces
	 */
	data->spk_volume |= NAU88C10_SPK_VOLUME_SPKZC;

	return nau88c10_write(dev, NAU88C10_REG_SPK_VOLUME, data->spk_volume);
}

static void nau88c10_start_output(const struct device *dev)
{
	struct nau88c10_data *data = dev->data;
	int ret;

	data->dac_ctrl &= ~NAU88C10_DAC_CTRL_DACMT;

	ret = nau88c10_write(dev, NAU88C10_REG_DAC_CTRL, data->dac_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to unmute the DAC: %d", ret);
	}
}

static void nau88c10_stop_output(const struct device *dev)
{
	struct nau88c10_data *data = dev->data;
	int ret;

	data->dac_ctrl |= NAU88C10_DAC_CTRL_DACMT;

	ret = nau88c10_write(dev, NAU88C10_REG_DAC_CTRL, data->dac_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to mute the DAC: %d", ret);
	}
}

static int nau88c10_set_output_volume(const struct device *dev, int vol)
{
	struct nau88c10_data *data = dev->data;

	if (!IN_RANGE(vol, NAU88C10_SPK_VOLUME_MIN_DB, NAU88C10_SPK_VOLUME_MAX_DB)) {
		LOG_ERR("Output volume %d dB out of range", vol);
		return -EINVAL;
	}

	data->spk_volume &= ~NAU88C10_SPK_VOLUME_MASK;
	data->spk_volume |= FIELD_PREP(NAU88C10_SPK_VOLUME_MASK,
				       vol - NAU88C10_SPK_VOLUME_MIN_DB);

	return nau88c10_write(dev, NAU88C10_REG_SPK_VOLUME, data->spk_volume);
}

static int nau88c10_set_output_mute(const struct device *dev, bool mute)
{
	struct nau88c10_data *data = dev->data;
	int ret;

	if (mute) {
		data->spk_volume |= NAU88C10_SPK_VOLUME_SPKMT;
		data->mono_mixer |= NAU88C10_MONO_MIXER_MOUTMXMT;
	} else {
		data->spk_volume &= ~NAU88C10_SPK_VOLUME_SPKMT;
		data->mono_mixer &= ~NAU88C10_MONO_MIXER_MOUTMXMT;
	}

	ret = nau88c10_write(dev, NAU88C10_REG_SPK_VOLUME, data->spk_volume);
	if (ret < 0) {
		return ret;
	}

	return nau88c10_write(dev, NAU88C10_REG_MONO_MIXER, data->mono_mixer);
}

static int nau88c10_set_input_volume(const struct device *dev, int vol)
{
	struct nau88c10_data *data = dev->data;

	if (!IN_RANGE(vol, NAU88C10_PGA_GAIN_MIN_DB, NAU88C10_PGA_GAIN_MAX_DB)) {
		LOG_ERR("Input volume %d dB out of range", vol);
		return -EINVAL;
	}

	data->pga_gain &= ~NAU88C10_PGA_GAIN_MASK;
	data->pga_gain |= FIELD_PREP(NAU88C10_PGA_GAIN_MASK,
				     ((vol - NAU88C10_PGA_GAIN_MIN_DB) * 4) / 3);

	return nau88c10_write(dev, NAU88C10_REG_PGA_GAIN, data->pga_gain);
}

static int nau88c10_set_input_mute(const struct device *dev, bool mute)
{
	struct nau88c10_data *data = dev->data;

	if (mute) {
		data->pga_gain |= NAU88C10_PGA_GAIN_PGAMT;
	} else {
		data->pga_gain &= ~NAU88C10_PGA_GAIN_PGAMT;
	}

	return nau88c10_write(dev, NAU88C10_REG_PGA_GAIN, data->pga_gain);
}

static int nau88c10_set_property(const struct device *dev, audio_property_t property,
				 audio_channel_t channel, audio_property_value_t val)
{
	/* Mono codec: every property applies to the single ADC/DAC channel */
	if (channel != AUDIO_CHANNEL_ALL && channel != AUDIO_CHANNEL_FRONT_LEFT) {
		return -ENOTSUP;
	}

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return nau88c10_set_output_volume(dev, val.vol);
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return nau88c10_set_output_mute(dev, val.mute);
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return nau88c10_set_input_volume(dev, val.vol);
	case AUDIO_PROPERTY_INPUT_MUTE:
		return nau88c10_set_input_mute(dev, val.mute);
	default:
		return -ENOTSUP;
	}
}

static int nau88c10_apply_properties(const struct device *dev)
{
	/* Properties are written to the codec as they are set */
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(audio_codec, nau88c10_api) = {
	.configure = nau88c10_configure,
	.start_output = nau88c10_start_output,
	.stop_output = nau88c10_stop_output,
	.set_property = nau88c10_set_property,
	.apply_properties = nau88c10_apply_properties,
};

static int nau88c10_init(const struct device *dev)
{
	const struct nau88c10_config *cfg = dev->config;
	struct nau88c10_data *data = dev->data;
	uint16_t id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = nau88c10_read(dev, NAU88C10_REG_DEVICE_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read the device ID: %d", ret);
		return ret;
	}

	if (id != NAU88C10_DEVICE_ID) {
		LOG_ERR("Unexpected device ID 0x%03x", id);
		return -ENODEV;
	}

	/* Any write to the reset register restores the whole register map */
	ret = nau88c10_write(dev, NAU88C10_REG_RESET, 0);
	if (ret < 0) {
		LOG_ERR("Failed to reset the codec: %d", ret);
		return ret;
	}

	data->spk_volume = NAU88C10_SPK_VOLUME_RESET;
	data->pga_gain = NAU88C10_PGA_GAIN_RESET;
	data->dac_ctrl = 0;
	data->mono_mixer = 0;

	return 0;
}

#define NAU88C10_INIT(inst)                                                                        \
	static const struct nau88c10_config nau88c10_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.mic_single_ended = DT_INST_PROP(inst, mic_single_ended),                          \
	};                                                                                         \
                                                                                                   \
	static struct nau88c10_data nau88c10_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nau88c10_init, NULL, &nau88c10_data_##inst,                    \
			      &nau88c10_config_##inst, POST_KERNEL,                                \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &nau88c10_api);

DT_INST_FOREACH_STATUS_OKAY(NAU88C10_INIT)
