/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Everest ES8311 register definitions follow the user guide, revision 1.11. */

#define DT_DRV_COMPAT everest_es8311

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(es8311);

/* Register map (the subset this driver touches). */
#define ES8311_REG_RESET        0x00U /* reset / clock state machine */
#define ES8311_REG_CLK_MANAGER  0x01U /* master clock source + clock enables */
#define ES8311_REG_CLK_PRE      0x02U /* DIV_PRE, MULT_PRE */
#define ES8311_REG_ADC_OSR      0x03U /* ADC_FSMODE, ADC_OSR */
#define ES8311_REG_DAC_OSR      0x04U /* DAC_OSR */
#define ES8311_REG_CLK_DIV      0x05U /* DIV_CLKADC, DIV_CLKDAC */
#define ES8311_REG_CLK_BCLK     0x06U /* BCLK_CON, BCLK_INV, DIV_BCLK */
#define ES8311_REG_CLK_LRCK_H   0x07U /* tri-state controls, DIV_LRCK[11:8] */
#define ES8311_REG_CLK_LRCK_L   0x08U /* DIV_LRCK[7:0] */
#define ES8311_REG_SDP_IN       0x09U /* serial data port, DAC path (SDIN) */
#define ES8311_REG_SDP_OUT      0x0AU /* serial data port, ADC path (ASDOUT) */
#define ES8311_REG_PWRUP_AB     0x0BU /* PWRUP_A [7:3], PWRUP_B [3:1] */
#define ES8311_REG_PWRUP_C      0x0CU /* PWRUP_B [0], PWRUP_C [6:0] */
#define ES8311_REG_SYSTEM_0D    0x0DU /* analog power: bias, VREF, VMIDSEL */
#define ES8311_REG_SYSTEM_0E    0x0EU /* ADC power */
#define ES8311_REG_LOW_POWER    0x0FU /* LPDAC, LPPGA, LPPGAOUT, LPVCMMOD, LPADCVRP ... */
#define ES8311_REG_ANALOG_10    0x10U /* SYNCMODE, VMIDLOW, DAC_IBIAS_SW, IBIAS_SW, VX2OFF */
#define ES8311_REG_ANALOG_11    0x11U /* VSEL */
#define ES8311_REG_SYSTEM_12    0x12U /* DAC power */
#define ES8311_REG_SYSTEM_13    0x13U /* HPSW (bit 4): line-out or headphone drive */
#define ES8311_REG_ADC_PGA      0x14U /* LINSEL microphone mux, PGA gain */
#define ES8311_REG_ADC_RAMP     0x15U /* ADC volume ramp rate */
#define ES8311_REG_ADC_SCALE    0x16U /* ADC polarity, ADC_SCALE */
#define ES8311_REG_ADC_VOLUME   0x17U /* ADC digital volume */
#define ES8311_REG_ADC_ALC      0x18U /* ALC_EN, ADC_AUTOMUTE_EN, ALC_WINSIZE */
#define ES8311_REG_ADC_ALC_LVL  0x19U /* ALC maximum and minimum gain */
#define ES8311_REG_ADC_AUTOMUTE 0x1AU /* automute noise gate */
#define ES8311_REG_ADC_HPF1     0x1BU /* ADC high-pass filter, stage 1 */
#define ES8311_REG_ADC_HPF2     0x1CU /* EQ bypass + ADC high-pass filter stage 2 */
#define ES8311_REG_DAC_MUTE     0x31U /* DAC mute */
#define ES8311_REG_DAC_VOLUME   0x32U /* DAC digital volume */
#define ES8311_REG_DAC_OFFSET   0x33U /* DAC DC offset */
#define ES8311_REG_DAC_DRC      0x34U /* DRC_EN, DRC_WINSIZE */
#define ES8311_REG_DAC_DRC_LVL  0x35U /* DRC maximum and minimum level */
#define ES8311_REG_DAC_RAMP_EQ  0x37U /* DAC_RAMPRATE [7:4], DAC equaliser bypass [3] */
#define ES8311_REG_GPIO         0x44U /* ADC2DAC_SEL (bit 7), ADCDAT_SEL [6:4] */
#define ES8311_REG_ADC_GP45     0x45U /* GP control */
#define ES8311_REG_INI          0xFAU /* I2C_RETIME, INI_REG (bit 0) */
#define ES8311_REG_CHIP_ID1     0xFDU /* chip id, high byte */
#define ES8311_REG_CHIP_ID2     0xFEU /* chip id, low byte */

#define ES8311_CHIP_ID1 0x83U
#define ES8311_CHIP_ID2 0x11U

/* Enable CSM without asserting the digital reset bits. */
#define ES8311_RESET_CSM_ON 0x80U

/* Disable inherited ALC/DRC settings so the volume registers retain their dB meaning. */
#define ES8311_PWRUP_MIN     0x00U /* 0x0B: PWRUP_A = 0, PWRUP_B[3:1] = 0 */
#define ES8311_PWRUP_C_MIN   0x00U /* 0x0C: PWRUP_C = 0. Power-on default is 32. */
#define ES8311_LOW_POWER_OFF 0x00U /* 0x0F: every low-power-mode bit clear */
#define ES8311_ANALOG_10_VAL 0x1FU /* 0x10: differs from the 0x13 default in IBIAS_SW */
#define ES8311_ANALOG_11_VAL 0x7FU /* 0x11: VSEL. The datasheet says "Internal use". */
#define ES8311_ALC_OFF       0x00U /* 0x18: ALC_EN and ADC_AUTOMUTE_EN clear */
#define ES8311_ALC_LVL_DEF   0x00U /* 0x19 */
#define ES8311_AUTOMUTE_OFF  0x00U /* 0x1A */
#define ES8311_DAC_OFFSET_0  0x00U /* 0x33 */
#define ES8311_DRC_OFF       0x00U /* 0x34: DRC_EN clear */
#define ES8311_DRC_LVL_DEF   0x00U /* 0x35 */

/* Release INI_REG only after confirming the chip identity. */
#define ES8311_INI_RELEASE 0x00U

/* Keep the input clocks enabled and gate only the unused converter clocks. */
#define ES8311_CLK_SRC_BCLK          BIT(7) /* 0x01 bit 7: master clock from BCLK */
#define ES8311_CLK_MGR_BOTH_BASE     0x3FU  /* both converters clocked */
#define ES8311_CLK_MGR_PLAYBACK_BASE 0x35U  /* the ADC clocks gated off */
#define ES8311_CLK_MGR_CAPTURE_BASE  0x3AU  /* the DAC clocks gated off */

/* A 32-BCLK frame needs x8 multiplication for 256fs; DAC oversampling also depends on rate. */
#define ES8311_CLK_PRE_DIV1_MULT1 0x00U /* 0x02: DIV_PRE = 1, MULT_PRE = x1 */
#define ES8311_CLK_PRE_DIV1_MULT8 0x18U /* 0x02: DIV_PRE = 1, MULT_PRE = x8 */
#define ES8311_ADC_OSR_SINGLE_16  0x10U /* 0x03: single speed, ADC_OSR = 64 * fs */
#define ES8311_DAC_OSR_64FS       0x10U /* 0x04: DAC_OSR = 64 * fs */
#define ES8311_DAC_OSR_128FS      0x20U /* 0x04: DAC_OSR = 128 * fs, <= 16 kHz */
#define ES8311_CLK_DIV_ADC1_DAC1  0x00U /* 0x05: DIV_CLKADC = 1, DIV_CLKDAC = 1 */

/* Above this rate the vendor table uses 64 * fs; at or below it, 128 * fs. */
#define ES8311_DAC_OSR_128FS_MAX_RATE 16000U

/* Clock ratios for a two-slot, 16-bit I2S frame. */
#define ES8311_MCLK_FS_RATIO 256U
#define ES8311_BCLK_FS_RATIO 32U

/* Use the 3.3 V multiplier-input limit because the supply voltage is unknown. */
#define ES8311_MULT_INPUT_MIN_HZ 1000000U

/* Keep BCLK_CON and ADCDAT tri-state clear while programming the target-mode dividers. */
#define ES8311_BCLK_SLAVE_DIV4 0x03U /* 0x06: BCLK_CON clear, DIV_BCLK = 4 */
#define ES8311_LRCK_DIV_H      0x00U /* 0x07: no tri-state, DIV_LRCK[11:8] = 0 */
#define ES8311_LRCK_DIV_L      0xFFU /* 0x08: DIV_LRCK[7:0], so LRCK = MCLK / 256 */

/* Only SDP_IN has bit 7: it selects the mono DAC slot. */
#define ES8311_SDP_I2S_16BIT    0x0CU
#define ES8311_SDP_MUTE         0x40U
#define ES8311_SDP_IN_SEL_RIGHT 0x80U /* 0x09 bit 7: mono DAC takes the RIGHT I2S slot */

/* Keep shared references active when powering down either converter. */
#define ES8311_ANALOG_BOTH     0x01U /* 0x0D */
#define ES8311_ANALOG_PLAYBACK 0x31U /* 0x0D: the ADC's bias and reference down */
#define ES8311_ANALOG_CAPTURE  0x09U /* 0x0D: the DAC's reference down */
#define ES8311_ADC_PWR_ON      0x02U /* 0x0E: PGA and modulator powered */
#define ES8311_ADC_PWR_DOWN    0x62U /* 0x0E: PDN_PGA | PDN_MOD, shared refs kept */
#define ES8311_DAC_PWR_ON      0x00U /* 0x12 */
#define ES8311_DAC_PWR_DOWN    0x02U /* 0x12: PDN_DAC */
/* Select headphone drive and clear the reserved bits, matching the reference configuration. */
#define ES8311_OUT_HEADPHONE   0x10U
/* Use a 0.25 dB / 32-LRCK volume ramp with the equalizer bypassed. */
#define ES8311_DAC_RAMP_EQ     0x48U /* 0x37 */

/* Assert both DSMMUTE and DEMMUTE. */
#define ES8311_DAC_MUTE_ON  0x60U
#define ES8311_DAC_MUTE_OFF 0x00U

/* Power-up margins; these delays do not establish audio-clock or analog settling. */
#define ES8311_CSM_SETTLE_MS   10
#define ES8311_PWR_UP_DELAY_MS 10

/* Digital volume: 0xBF is 0 dB; each step is 0.5 dB. The API accepts whole dB. */
#define ES8311_VOL_DB_MAX   32
#define ES8311_VOL_DB_MIN   (-95)
#define ES8311_VOL_0DB_CODE 0xBFU

/* Select differential MIC1; the binding defaults its PGA gain to 0 dB. */
#define ES8311_ADC_PGA_MIC1_0DB     0x10U /* 0x14: LINSEL = 1 (MIC1 diff), PGA gain code 0 */
#define ES8311_ADC_PGA_GAIN_STEP_DB 3U    /* 0x14 bits [3:0]: 3 dB per code, 0..30 dB */
#define ES8311_ADC_MIC_OFF          0x00U /* 0x14: no input selected */
#define ES8311_ADC_RAMP_RATE        0x40U /* 0x15: volume ramp rate */
#define ES8311_ADC_HPF1_VAL         0x0AU /* 0x1B */
#define ES8311_ADC_HPF2_DCBLOCK     0x6AU /* 0x1C: EQ bypass, cancels the DC offset */
#define ES8311_ADC_GP45_DEFAULT     0x00U /* 0x45 */

/* Enable ADC_SYNC and retain the +24 dB digital scale. */
#define ES8311_ADC_SCALE_24DB 0x24U

/* Select ADC data and retain the vendor I2C-noise setting in bit 3. */
#define ES8311_GPIO_ADCDAT_ADC 0x08U

/* Single-speed rates supported by the 256fs clock configuration. */
static const uint32_t es8311_rates[] = {
	8000U, 11025U, 12000U, 16000U, 22050U, 24000U, 32000U, 44100U, 48000U,
};

struct es8311_config {
	struct i2c_dt_spec bus;
	uint8_t sdp_in_sel; /* 0x09 bit 7: mono DAC source slot (0 = left, 0x80 = right) */
	uint8_t pga_reg;    /* 0x14: MIC1 differential plus the configured PGA gain */
};

struct es8311_data {
	struct k_mutex lock;
	uint8_t dac_volume_code; /* cached 0x32 */
	uint8_t adc_volume_code; /* cached 0x17 */
	/* Either the mute property or the stopped flag keeps a direction muted. */
	bool output_mute;
	bool adc_mute;
	bool output_stopped;
	bool input_stopped;
	/* Only a successfully configured route may be started. */
	bool playback;
	bool capture;
};

static int es8311_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct es8311_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->bus, reg, val);
}

static int es8311_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct es8311_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus, reg, val);
}

/* Program power-up timers before enabling analog power. */
static const struct es8311_reg_val {
	uint8_t reg;
	uint8_t val;
} es8311_known_state[] = {
	{ES8311_REG_PWRUP_AB, ES8311_PWRUP_MIN},
	{ES8311_REG_PWRUP_C, ES8311_PWRUP_C_MIN},
	{ES8311_REG_LOW_POWER, ES8311_LOW_POWER_OFF},
	{ES8311_REG_ANALOG_10, ES8311_ANALOG_10_VAL},
	{ES8311_REG_ANALOG_11, ES8311_ANALOG_11_VAL},
	{ES8311_REG_ADC_ALC, ES8311_ALC_OFF},
	{ES8311_REG_ADC_ALC_LVL, ES8311_ALC_LVL_DEF},
	{ES8311_REG_ADC_AUTOMUTE, ES8311_AUTOMUTE_OFF},
	{ES8311_REG_DAC_OFFSET, ES8311_DAC_OFFSET_0},
	{ES8311_REG_DAC_DRC, ES8311_DRC_OFF},
	{ES8311_REG_DAC_DRC_LVL, ES8311_DRC_LVL_DEF},
};

static int es8311_write_known_state(const struct device *dev)
{
	for (size_t i = 0U; i < ARRAY_SIZE(es8311_known_state); i++) {
		int ret =
			es8311_reg_write(dev, es8311_known_state[i].reg, es8311_known_state[i].val);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/* Attempt every mute and power-down write, retaining the first error. */
static int es8311_quiesce(const struct device *dev)
{
	static const struct es8311_reg_val quiesce[] = {
		/* Speaker, microphone, the DAC's own data port, then the power-downs. */
		{ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON},
		{ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE},
		{ES8311_REG_SDP_IN, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE},
		{ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_DOWN},
		{ES8311_REG_SYSTEM_0E, ES8311_ADC_PWR_DOWN},
		{ES8311_REG_ADC_PGA, ES8311_ADC_MIC_OFF},
	};
	int first_err = 0;

	for (size_t i = 0U; i < ARRAY_SIZE(quiesce); i++) {
		int ret = es8311_reg_write(dev, quiesce[i].reg, quiesce[i].val);

		if (ret < 0 && first_err == 0) {
			first_err = ret;
		}
	}

	return first_err;
}

static bool es8311_rate_supported(uint32_t rate)
{
	for (size_t i = 0; i < ARRAY_SIZE(es8311_rates); i++) {
		if (es8311_rates[i] == rate) {
			return true;
		}
	}

	return false;
}

/* Signed arithmetic keeps the lower-bound check from wrapping to an unsigned value. */
BUILD_ASSERT((int)ES8311_VOL_0DB_CODE + (ES8311_VOL_DB_MAX * 2) <= 0xFF,
	     "the maximum dB level must not overflow the volume register");
BUILD_ASSERT((int)ES8311_VOL_0DB_CODE + (ES8311_VOL_DB_MIN * 2) >= 0,
	     "the minimum dB level must not underflow the volume register");

/* Convert a dB volume level to a digital volume register code. */
static uint8_t es8311_db_to_code(int db)
{
	if (db > ES8311_VOL_DB_MAX) {
		db = ES8311_VOL_DB_MAX;
	} else if (db < ES8311_VOL_DB_MIN) {
		db = ES8311_VOL_DB_MIN;
	}

	return (uint8_t)((int)ES8311_VOL_0DB_CODE + (db * 2));
}

static int es8311_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct es8311_data *data = dev->data;
	const struct es8311_config *dcfg = dev->config;
	uint32_t rate;
	uint8_t clk_mgr;
	uint8_t analog;
	uint8_t dac_osr;
	uint8_t sdp_in;
	uint8_t clk_pre;
	bool mclk_from_bclk;
	bool playback = false;
	bool capture = false;
	int ret = 0;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_INF("Unsupported DAI type %d", cfg->dai_type);
		return -ENOTSUP;
	}

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		playback = true;
		break;
	case AUDIO_ROUTE_CAPTURE:
		capture = true;
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		playback = true;
		capture = true;
		break;
	default:
		LOG_INF("Unsupported route %u (playback/capture only)", cfg->dai_route);
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.i2s.format & I2S_FMT_DATA_FORMAT_MASK) != I2S_FMT_DATA_FORMAT_I2S) {
		LOG_INF("Unsupported I2S data format 0x%x (only standard I2S)",
			cfg->dai_cfg.i2s.format & I2S_FMT_DATA_FORMAT_MASK);
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.i2s.format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		LOG_INF("LSB-first data ordering not supported");
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.i2s.format & I2S_FMT_CLK_FORMAT_MASK) != I2S_FMT_CLK_NF_NB) {
		LOG_INF("Unsupported I2S clock format 0x%x (only NF_NB)",
			cfg->dai_cfg.i2s.format & I2S_FMT_CLK_FORMAT_MASK);
		return -ENOTSUP;
	}

	/* The clock dividers require 32 BCLK periods per stereo frame. */
	if (cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_16_BITS) {
		LOG_INF("Unsupported word size %u: this driver supports 16-bit I2S only",
			cfg->dai_cfg.i2s.word_size);
		return -ENOTSUP;
	}

	/* Some controllers use channels to size the frame even in I2S mode. */
	if (cfg->dai_cfg.i2s.channels != 2U) {
		LOG_INF("Unsupported channel count %u: the clock tree here assumes a two-slot "
			"frame",
			cfg->dai_cfg.i2s.channels);
		return -ENOTSUP;
	}

	rate = cfg->dai_cfg.i2s.frame_clk_freq;
	if (!es8311_rate_supported(rate)) {
		LOG_INF("Unsupported sample rate %u", rate);
		return -ENOTSUP;
	}

	/* A gated BCLK can stop the DAC clock before the output is muted. */
	if ((cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_GATED) != 0U) {
		LOG_INF("I2S_OPT_BIT_CLK_GATED is not supported: a gated bit clock stops the "
			"serial port, and stops the whole codec when the master clock is "
			"derived from BCLK");
		return -ENOTSUP;
	}

	/* Clock-role flags describe this codec, not the I2S controller. */
	if ((cfg->dai_cfg.i2s.options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) !=
	    (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) {
		LOG_INF("this codec is always the clock target: the bit- and frame-clock roles "
			"must "
			"both be TARGET (the I2S controller drives BCLK and LRCK)");
		return -ENOTSUP;
	}

	/* Zero selects BCLK multiplication; otherwise MCLK must provide 256fs. */
	if (cfg->mclk_freq == 0U) {
		uint32_t mult_in = rate * ES8311_BCLK_FS_RATIO;

		if (mult_in <= ES8311_MULT_INPUT_MIN_HZ) {
			LOG_INF("a BCLK-derived clock at %u Hz gives a %u Hz multiplier input, "
				"under the published minimum; drive MCLK at %u Hz instead",
				rate, mult_in, rate * ES8311_MCLK_FS_RATIO);
			return -ENOTSUP;
		}
		mclk_from_bclk = true;
		clk_pre = ES8311_CLK_PRE_DIV1_MULT8;
	} else if (cfg->mclk_freq == rate * ES8311_MCLK_FS_RATIO) {
		mclk_from_bclk = false;
		clk_pre = ES8311_CLK_PRE_DIV1_MULT1;
	} else {
		LOG_INF("mclk_freq %u is neither 0 (derive from BCLK) nor %u (256fs on the "
			"MCLK pin)",
			cfg->mclk_freq, rate * ES8311_MCLK_FS_RATIO);
		return -ENOTSUP;
	}

	LOG_DBG("Configure: rate=%u", rate);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Invalidate the old route before the first write can partially reconfigure it. */
	data->playback = false;
	data->capture = false;
	/* Reconfiguration leaves both directions stopped until an explicit start. */
	data->output_stopped = true;
	data->input_stopped = true;

	/* Program both directions so a route change cannot retain an unused converter. */
	if (playback && capture) {
		clk_mgr = ES8311_CLK_MGR_BOTH_BASE;
		analog = ES8311_ANALOG_BOTH;
	} else if (playback) {
		clk_mgr = ES8311_CLK_MGR_PLAYBACK_BASE;
		analog = ES8311_ANALOG_PLAYBACK;
	} else {
		clk_mgr = ES8311_CLK_MGR_CAPTURE_BASE;
		analog = ES8311_ANALOG_CAPTURE;
	}

	if (mclk_from_bclk) {
		clk_mgr |= ES8311_CLK_SRC_BCLK;
	}

	/* Use the reference table's higher DAC oversampling ratio at 8-16 kHz. */
	dac_osr = (rate <= ES8311_DAC_OSR_128FS_MAX_RATE) ? ES8311_DAC_OSR_128FS
							  : ES8311_DAC_OSR_64FS;

	sdp_in = ES8311_SDP_I2S_16BIT | dcfg->sdp_in_sel | (playback ? 0U : ES8311_SDP_MUTE);

	/* Mute both serial ports and power down the DAC before changing its clocks. */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_IN, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_DOWN);
	if (ret < 0) {
		goto end;
	}

	/* Keep an active ADC powered to avoid restarting analog settling. */

	/* Power the clock state machine up. This resets no register: see 0x00 above. */
	ret = es8311_reg_write(dev, ES8311_REG_RESET, ES8311_RESET_CSM_ON);
	if (ret < 0) {
		goto end;
	}
	k_msleep(ES8311_CSM_SETTLE_MS);

	/* Restore the analog profile before enabling power; no register-file reset is performed. */
	ret = es8311_write_known_state(dev);
	if (ret < 0) {
		goto end;
	}

	ret = es8311_reg_write(dev, ES8311_REG_CLK_MANAGER, clk_mgr);
	if (ret < 0) {
		goto end;
	}

	ret = es8311_reg_write(dev, ES8311_REG_CLK_PRE, clk_pre);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_ADC_OSR, ES8311_ADC_OSR_SINGLE_16);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_OSR, dac_osr);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_DIV, ES8311_CLK_DIV_ADC1_DAC1);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_BCLK, ES8311_BCLK_SLAVE_DIV4);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_LRCK_H, ES8311_LRCK_DIV_H);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_LRCK_L, ES8311_LRCK_DIV_L);
	if (ret < 0) {
		goto end;
	}

	/* Keep both serial ports muted until the configuration writes complete. */

	/* Retain shared bias and references for the active converter. */
	ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_0D, analog);
	if (ret < 0) {
		goto end;
	}
	k_msleep(ES8311_PWR_UP_DELAY_MS);

	if (playback) {
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_ON);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_13, ES8311_OUT_HEADPHONE);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_DAC_VOLUME, data->dac_volume_code);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_DAC_RAMP_EQ, ES8311_DAC_RAMP_EQ);
		if (ret < 0) {
			goto end;
		}
	} else {
		/* Power the DAC down rather than leaving a previous route's DAC live. */
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_DOWN);
		if (ret < 0) {
			goto end;
		}
	}

	if (capture) {
		/* Power the capture path; its serial output remains muted until start(RX). */
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_0E, ES8311_ADC_PWR_ON);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_PGA, dcfg->pga_reg);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_RAMP, ES8311_ADC_RAMP_RATE);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_SCALE, ES8311_ADC_SCALE_24DB);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_VOLUME, data->adc_volume_code);
		if (ret < 0) {
			goto end;
		}

		/* The high-pass filter cancels the ADC's digital DC offset. */
		ret = es8311_reg_write(dev, ES8311_REG_ADC_HPF1, ES8311_ADC_HPF1_VAL);
		if (ret < 0) {
			goto end;
		}
		ret = es8311_reg_write(dev, ES8311_REG_ADC_HPF2, ES8311_ADC_HPF2_DCBLOCK);
		if (ret < 0) {
			goto end;
		}
	} else {
		/* Disconnect MIC1 as well as powering down the ADC. */
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_0E, ES8311_ADC_PWR_DOWN);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_PGA, ES8311_ADC_MIC_OFF);
		if (ret < 0) {
			goto end;
		}
	}

	/* Clear inherited ADC-to-DAC routing even for playback-only configurations. */
	ret = es8311_reg_write(dev, ES8311_REG_GPIO, ES8311_GPIO_ADCDAT_ADC);
	if (ret < 0) {
		goto end;
	}

	ret = es8311_reg_write(dev, ES8311_REG_ADC_GP45, ES8311_ADC_GP45_DEFAULT);
	if (ret < 0) {
		goto end;
	}

	/* Restore the DAC serial format while the DAC mute remains asserted. */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_IN, sdp_in);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	if (ret < 0) {
		goto end;
	}
	/* Keep capture muted; start(RX) controls the first unmute. */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	if (ret < 0) {
		goto end;
	}

	data->playback = playback;
	data->capture = capture;

end:
	/* A partial configuration invalidates the route and requires best-effort quiescing. */
	if (ret < 0) {
		/* Preserve the original error even if cleanup fails too. */
		(void)es8311_quiesce(dev);
		LOG_ERR("configure() I2C error: %d. A best-effort quiesce was attempted.", ret);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/* The route, mute property and stopped flag must all permit an unmute. */
static int es8311_start_rx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	uint8_t sdp_out = ES8311_SDP_I2S_16BIT;
	int ret;

	if (!data->capture) {
		LOG_WRN("start: no capture route configured");
		return -EIO;
	}

	/* Already started: do not re-issue the unmute. The output side guards the same way. */
	if (!data->input_stopped) {
		return 0;
	}

	if (data->adc_mute) {
		sdp_out |= ES8311_SDP_MUTE;
	}

	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, sdp_out);
	if (ret < 0) {
		/* May have landed. A mute only ever leaves the path same-or-safer. */
		(void)es8311_reg_write(dev, ES8311_REG_SDP_OUT,
				       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		data->input_stopped = true;
		return ret;
	}

	data->input_stopped = false;

	return 0;
}

static int es8311_stop_rx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;

	data->input_stopped = true;

	return es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
}

static int es8311_start_tx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	if (!data->playback) {

		LOG_WRN("start: no playback route configured");
		return -EIO;
	}

	if (!data->output_stopped) {
		/* A repeated start must not issue another unmute. */
		return 0;
	}

	/* Write the requested state before marking the direction started. */
	if (data->output_mute) {
		/* Started, but the caller's OUTPUT_MUTE is authoritative: establish MUTED. */
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	} else {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	}
	if (ret < 0) {
		(void)es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
		data->output_stopped = true;
	} else {
		data->output_stopped = false;
	}

	return ret;
}

/* Write the mute directly so a failed read cannot prevent the attempt. */
static int es8311_stop_tx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;

	data->output_stopped = true;

	/* The lifecycle stops; the OUTPUT_MUTE property is left exactly as the caller set it. */
	return es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
}

/* Start RX before TX; if TX fails, undo only the RX start performed by this call. */
static int es8311_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct es8311_data *data = dev->data;
	bool rx_was_stopped;
	int ret = 0;

	if (dir == 0U || (dir & ~(audio_dai_dir_t)AUDIO_DAI_DIR_TXRX) != 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	rx_was_stopped = data->input_stopped;

	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = es8311_start_rx_locked(dev);
	}

	if (ret == 0 && (dir & AUDIO_DAI_DIR_TX) != 0U) {
		ret = es8311_start_tx_locked(dev);
		if (ret < 0 && (dir & AUDIO_DAI_DIR_RX) != 0U && rx_was_stopped) {
			/* Undo the opener this call made, not one it inherited. */
			(void)es8311_stop_rx_locked(dev);
		}
	}

	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("start(0x%x) failed (%d)", (unsigned int)dir, ret);
	}

	return ret;
}

static int es8311_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct es8311_data *data = dev->data;
	int first_err = 0;
	int ret;

	if (dir == 0U || (dir & ~(audio_dai_dir_t)AUDIO_DAI_DIR_TXRX) != 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((dir & AUDIO_DAI_DIR_TX) != 0U) {
		ret = es8311_stop_tx_locked(dev);
		if (ret < 0) {
			first_err = ret;
		}
	}

	/* A failed configure clears the route cache, so stopping must not depend on it. */
	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = es8311_stop_rx_locked(dev);
		if (ret < 0 && first_err == 0) {
			first_err = ret;
		}
	}

	k_mutex_unlock(&data->lock);

	if (first_err < 0) {
		LOG_ERR("stop(0x%x) failed (%d); the caller must not stop BCLK after this",
			(unsigned int)dir, first_err);
	}

	return first_err;
}

/* The mandatory legacy wrappers cannot return I2C errors. */
static void es8311_start_output(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es8311_start_tx_locked(dev);
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("start_output: failed to establish the output state (%d)", ret);
	}
}

static void es8311_stop_output(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es8311_stop_tx_locked(dev);
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("stop_output: failed to mute (%d)", ret);
	}
}

static int es8311_set_property(const struct device *dev, audio_property_t property,
			       audio_channel_t channel, audio_property_value_t val)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	if (channel != AUDIO_CHANNEL_ALL && channel != AUDIO_CHANNEL_FRONT_LEFT &&
	    channel != AUDIO_CHANNEL_FRONT_RIGHT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		data->dac_volume_code = es8311_db_to_code(val.vol);
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		data->output_mute = val.mute;
		break;
	case AUDIO_PROPERTY_INPUT_VOLUME:
		data->adc_volume_code = es8311_db_to_code(val.vol);
		break;
	case AUDIO_PROPERTY_INPUT_MUTE:
		data->adc_mute = val.mute;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/* Hold the lock across mute, volume and unmute writes so stop cannot interleave. */
static int es8311_apply_properties(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int first_err = 0;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Attempt every requested mute, including retries for stopped directions. */
	if (data->playback && (data->output_mute || data->output_stopped)) {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
		if (ret < 0) {
			LOG_ERR("Failed to mute the DAC (%d)", ret);
			first_err = (first_err == 0) ? ret : first_err;
		}
	}

	/* The ADC mutes at its serial data port, not through its volume. */
	if (data->capture && (data->adc_mute || data->input_stopped)) {
		ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT,
				       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		if (ret < 0) {
			LOG_ERR("Failed to mute the microphone (%d)", ret);
			first_err = (first_err == 0) ? ret : first_err;
		}
	}

	/* Do not increase either volume after a mute failure. */
	if (first_err == 0 && data->playback) {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_VOLUME, data->dac_volume_code);
		if (ret < 0) {
			LOG_ERR("Failed to set DAC volume 0x%02x (%d)", data->dac_volume_code, ret);
			first_err = ret;
		}
	}

	if (first_err == 0 && data->capture) {
		ret = es8311_reg_write(dev, ES8311_REG_ADC_VOLUME, data->adc_volume_code);
		if (ret < 0) {
			LOG_ERR("Failed to set ADC volume 0x%02x (%d)", data->adc_volume_code, ret);
			first_err = ret;
		}
	}

	/* Unmute only after prior writes succeed; a failed RX unmute must also block TX. */
	bool mic_unmute_sent = false;

	if (first_err == 0 && data->capture && !data->adc_mute && !data->input_stopped) {
		ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT);
		if (ret < 0) {
			LOG_ERR("Failed to unmute the microphone (%d)", ret);
			first_err = ret;
			/* The write may have landed; attempt to mute again. */
			(void)es8311_reg_write(dev, ES8311_REG_SDP_OUT,
					       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		} else {
			mic_unmute_sent = true;
		}
	}

	/* Unmute the DAC last, only if both its property and lifecycle permit it. */
	if (first_err == 0 && data->playback && !data->output_mute && !data->output_stopped) {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
		if (ret < 0) {
			LOG_ERR("Failed to unmute the DAC (%d)", ret);
			first_err = ret;
			/* Retry the DAC mute and undo any RX unmute sent by this call. */
			(void)es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
			if (mic_unmute_sent) {
				(void)es8311_reg_write(dev, ES8311_REG_SDP_OUT,
						       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return first_err;
}

/* The codec has one analog input and one output; no routing callbacks are needed. */
static DEVICE_API(audio_codec, es8311_api) = {
	.configure = es8311_configure,
	.start_output = es8311_start_output,
	.stop_output = es8311_stop_output,
	.set_property = es8311_set_property,
	.apply_properties = es8311_apply_properties,
	.start = es8311_start,
	.stop = es8311_stop,
};

static int es8311_read_id(const struct device *dev, uint8_t *id1, uint8_t *id2)
{
	int ret;

	ret = es8311_reg_read(dev, ES8311_REG_CHIP_ID1, id1);
	if (ret < 0) {
		LOG_ERR("Failed to read chip id1 (%d)", ret);
		return ret;
	}

	ret = es8311_reg_read(dev, ES8311_REG_CHIP_ID2, id2);
	if (ret < 0) {
		LOG_ERR("Failed to read chip id2 (%d)", ret);
		return ret;
	}

	return 0;
}

/* Identify the part. Reads only: nothing is written to a device not yet known to be one. */
static int es8311_check_id(const struct device *dev)
{
	uint8_t id1 = 0U;
	uint8_t id2 = 0U;
	int ret;

	ret = es8311_read_id(dev, &id1, &id2);
	if (ret < 0) {
		return ret;
	}

	/* Do not write ES8311 registers to a device with an unknown identity. */
	if (id1 != ES8311_CHIP_ID1 || id2 != ES8311_CHIP_ID2) {
		LOG_ERR("Not an ES8311: chip id 0x%02x%02x (expected 0x%02x%02x)", id1, id2,
			ES8311_CHIP_ID1, ES8311_CHIP_ID2);
		return -ENODEV;
	}

	return 0;
}

static int es8311_init(const struct device *dev)
{
	const struct es8311_config *cfg = dev->config;
	struct es8311_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C controller not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	/* Both directions default to 0 dB. */
	data->dac_volume_code = ES8311_VOL_0DB_CODE;
	data->adc_volume_code = ES8311_VOL_0DB_CODE;
	data->output_mute = false;
	data->adc_mute = false;
	/* Only start() may unmute a configured direction. */
	data->output_stopped = true;
	data->input_stopped = true;
	/* Nothing is routed until configure() says so. */
	data->playback = false;
	data->capture = false;

	ret = es8311_check_id(dev);
	if (ret < 0) {
		return ret;
	}

	/* Release INI_REG before attempting the mute and power-down writes. */
	int normalize_err = es8311_reg_write(dev, ES8311_REG_INI, ES8311_INI_RELEASE);
	int quiesce_err;

	if (normalize_err < 0) {
		LOG_ERR("Failed to release INI_REG (%d). Quiescing anyway: the safety writes are "
			"worth attempting either way.",
			normalize_err);
	}

	/* Attempt quiescing even if the INI_REG write failed. */
	quiesce_err = es8311_quiesce(dev);
	if (quiesce_err < 0) {
		LOG_ERR("Failed to fully quiesce the codec (%d). Every safety write was still "
			"attempted.",
			quiesce_err);
	}

	/* Report a quiesce failure ahead of the INI_REG error. */
	if (quiesce_err < 0) {
		return quiesce_err;
	}

	return normalize_err;
}

#define ES8311_INST(idx)                                                                           \
	static const struct es8311_config es8311_config_##idx = {                                  \
		.bus = I2C_DT_SPEC_INST_GET(idx),                                                  \
		.sdp_in_sel = (DT_INST_ENUM_IDX(idx, everest_mono_dac_source) == 1)                \
				      ? ES8311_SDP_IN_SEL_RIGHT                                    \
				      : 0U,                                                        \
		.pga_reg = ES8311_ADC_PGA_MIC1_0DB | (DT_INST_PROP(idx, everest_mic_pga_gain_db) / \
						      ES8311_ADC_PGA_GAIN_STEP_DB),                \
	};                                                                                         \
	static struct es8311_data es8311_data_##idx;                                               \
	DEVICE_DT_INST_DEFINE(idx, es8311_init, NULL, &es8311_data_##idx, &es8311_config_##idx,    \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &es8311_api)

DT_INST_FOREACH_STATUS_OKAY(ES8311_INST)
