/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlv320aic3104

#include <errno.h>
#include <stdbool.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "tlv320aic3104_priv.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlv320aic3104);

/*
 * Bus access
 */

bool tlv320aic3104_bus_is_ready(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;

	return i2c_is_ready_dt(&cfg->bus);
}

static int ensure_page(const struct device *dev, uint8_t page)
{
	struct tlv320aic3104_data *data = dev->data;
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	if (data->page_cache != page) {
		ret = i2c_reg_write_byte_dt(&cfg->bus, PAGE_CONTROL_ADDR, page);
		if (ret < 0) {
			LOG_ERR("Failed to set page %u: %d", page, ret);
			return ret;
		}
		data->page_cache = page;
	}

	return 0;
}

int tlv320aic3104_bus_write_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t val)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	ret = ensure_page(dev, page);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->bus, addr, val);
	if (ret < 0) {
		LOG_ERR("Failed to write reg 0x%02x: %d", addr, ret);
		return ret;
	}

	return 0;
}

int tlv320aic3104_bus_update_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t mask,
				 uint8_t val)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	ret = ensure_page(dev, page);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->bus, addr, mask, val);
	if (ret < 0) {
		LOG_ERR("Failed to update reg 0x%02x: %d", addr, ret);
		return ret;
	}

	return 0;
}

int tlv320aic3104_bus_read_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t *val)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	ret = ensure_page(dev, page);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&cfg->bus, addr, val);
	if (ret < 0) {
		LOG_ERR("Failed to read reg 0x%02x: %d", addr, ret);
		return ret;
	}

	return 0;
}

/*
 * Clock (PLL / divider) solver
 */

#define MCLK_MIN_HZ 512000U
#define MCLK_MAX_HZ 50000000U

#define FSREF_MAX_HZ 53000U

#define FSREF_FRACTIONAL_NCODEC_MIN_HZ 39000U

#define FSREF_44K1_HZ 44100U
#define FSREF_48K_HZ  48000U

#define NCODEC_X2_MIN 2U
#define NCODEC_X2_MAX 12U

#define NDAC_NADC_DAC_SHIFT 4U

#define CLKDIV_FSREF_RATIO 128U
#define PLL_FSREF_RATIO    2048U

#define DIVIDER_Q_MIN 2U
#define DIVIDER_Q_MAX 17U

#define PLL_P_MIN 1U
#define PLL_P_MAX 8U
#define PLL_R_MIN 1U
#define PLL_R_MAX 16U
#define PLL_J_MIN 1U
#define PLL_J_MAX 63U

#define PLL_Q_FIELD_MODULUS 16U
#define PLL_P_FIELD_MODULUS 8U
#define PLL_R_FIELD_MODULUS 16U
#define PLL_Q_SHIFT         3U
#define PLL_J_MASK          0x3FU
#define PLL_J_SHIFT         2U
#define PLL_D_MSB_SHIFT     6U
#define PLL_D_LSB_MASK      0x3FU
#define PLL_D_LSB_SHIFT     2U

#define PLL_J_PERF_MIN 4U
#define PLL_D_MAX      9999U
#define PLL_K_SCALE    10000U
#define PLL_K10000_MIN (PLL_J_MIN * PLL_K_SCALE)
#define PLL_K10000_MAX (PLL_J_MAX * PLL_K_SCALE + PLL_D_MAX)

#define PLL_INPUT_OVER_P_MIN_D0_HZ  512000U
#define PLL_INPUT_OVER_P_MIN_DNZ_HZ 10000000U
#define PLL_INPUT_OVER_P_MAX_HZ     20000000U
#define PLL_VCO_MIN_HZ              80000000U
#define PLL_VCO_MAX_HZ              110000000U
#define PLL_J_MAX_D0                55U
#define PLL_J_MAX_DNZ               11U
#define PLL_R_FRACTIONAL            1U

static uint8_t encode_ncodec(int ncodec_x2)
{
	return (uint8_t)(ncodec_x2 - (int)NCODEC_X2_MIN);
}

static uint8_t encode_q(int q)
{
	return (uint8_t)((uint32_t)q % PLL_Q_FIELD_MODULUS);
}

static uint8_t encode_r(int r)
{
	return (uint8_t)((uint32_t)r % PLL_R_FIELD_MODULUS);
}

static uint8_t encode_p(int p)
{
	return (uint8_t)((uint32_t)p % PLL_P_FIELD_MODULUS);
}

static void fill_ncodec_and_fsref_bit(struct tlv320aic3104_clock_solution *out, int ncodec_x2,
				      int64_t fsref_x2)
{
	const uint8_t code = encode_ncodec(ncodec_x2);
	int64_t d_44100 = fsref_x2 - (int64_t)FSREF_44K1_HZ * 2;
	int64_t d_48000 = fsref_x2 - (int64_t)FSREF_48K_HZ * 2;

	out->r2_ndac_nadc = (uint8_t)((code << NDAC_NADC_DAC_SHIFT) | code);

	if (d_44100 < 0) {
		d_44100 = -d_44100;
	}
	if (d_48000 < 0) {
		d_48000 = -d_48000;
	}
	out->r7_fsref_family = (d_44100 <= d_48000) ? CODEC_DATAPATH_FSREF_FAMILY_BIT : 0U;
}

static bool solve_divider(uint32_t mclk_hz, int ncodec_x2, int64_t fsref_x2,
			  struct tlv320aic3104_clock_solution *out)
{
	const int64_t denom = (int64_t)CLKDIV_FSREF_RATIO * fsref_x2;
	int64_t q;

	if (denom == 0 || ((int64_t)mclk_hz * 2) % denom != 0) {
		return false;
	}

	q = ((int64_t)mclk_hz * 2) / denom;

	if (q < DIVIDER_Q_MIN || q > DIVIDER_Q_MAX) {
		return false;
	}
	if ((ncodec_x2 % 2) != 0) {
		if ((q % 2) != 0) {
			return false;
		}
		if (fsref_x2 < (int64_t)FSREF_FRACTIONAL_NCODEC_MIN_HZ * 2) {
			return false;
		}
	}

	fill_ncodec_and_fsref_bit(out, ncodec_x2, fsref_x2);
	out->pll_enabled = false;
	out->r3_pll_enable_q_p = (uint8_t)(encode_q((int)q) << PLL_Q_SHIFT);
	out->r4_pll_j = 0U;
	out->r5_pll_d_msb = 0U;
	out->r6_pll_d_lsb = 0U;
	out->r11_pll_r = 0U;
	out->r101_codec_clkin_src = R101_CODEC_CLKIN_CLKDIV_OUT;
	return true;
}

static bool pll_limits_met(uint32_t mclk_hz, int p, int r, int64_t k10000)
{
	const int64_t j = k10000 / PLL_K_SCALE;
	const int64_t d = k10000 % PLL_K_SCALE;
	const int64_t vco_x10000_over_p = (int64_t)mclk_hz * k10000 * r;
	const int64_t vco_min = (int64_t)PLL_VCO_MIN_HZ * PLL_K_SCALE * p;
	const int64_t vco_max = (int64_t)PLL_VCO_MAX_HZ * PLL_K_SCALE * p;

	if (d == 0) {
		if (mclk_hz < PLL_INPUT_OVER_P_MIN_D0_HZ * (uint32_t)p ||
		    mclk_hz > PLL_INPUT_OVER_P_MAX_HZ * (uint32_t)p) {
			return false;
		}
		if (j < PLL_J_PERF_MIN || j > PLL_J_MAX_D0) {
			return false;
		}
	} else {
		if (mclk_hz < PLL_INPUT_OVER_P_MIN_DNZ_HZ * (uint32_t)p ||
		    mclk_hz > PLL_INPUT_OVER_P_MAX_HZ * (uint32_t)p) {
			return false;
		}
		if (j < PLL_J_PERF_MIN || j > PLL_J_MAX_DNZ || r != (int)PLL_R_FRACTIONAL) {
			return false;
		}
	}

	return vco_x10000_over_p >= vco_min && vco_x10000_over_p <= vco_max;
}

static bool solve_pll(uint32_t mclk_hz, int ncodec_x2, int64_t fsref_x2,
		      struct tlv320aic3104_clock_solution *out)
{
	for (int p = PLL_P_MIN; p <= (int)PLL_P_MAX; p++) {
		for (int r = PLL_R_MIN; r <= (int)PLL_R_MAX; r++) {
			const int64_t num = ((fsref_x2 * PLL_FSREF_RATIO) / 2) * p * PLL_K_SCALE;
			const int64_t den = (int64_t)mclk_hz * r;
			int64_t k10000;
			int64_t j;
			int64_t d;

			if (den == 0 || num % den != 0) {
				continue;
			}

			k10000 = num / den;

			if (k10000 < PLL_K10000_MIN || k10000 > PLL_K10000_MAX) {
				continue;
			}
			if (!pll_limits_met(mclk_hz, p, r, k10000)) {
				continue;
			}

			j = k10000 / PLL_K_SCALE;
			d = k10000 % PLL_K_SCALE;

			fill_ncodec_and_fsref_bit(out, ncodec_x2, fsref_x2);
			out->pll_enabled = true;
			out->r3_pll_enable_q_p = (uint8_t)(R3_PLL_ENABLE_BIT | encode_p(p));
			out->r4_pll_j = (uint8_t)(((uint32_t)j & PLL_J_MASK) << PLL_J_SHIFT);
			out->r5_pll_d_msb = (uint8_t)((uint32_t)d >> PLL_D_MSB_SHIFT);
			out->r6_pll_d_lsb =
				(uint8_t)(((uint32_t)d & PLL_D_LSB_MASK) << PLL_D_LSB_SHIFT);
			out->r11_pll_r = encode_r(r);
			out->r101_codec_clkin_src = R101_CODEC_CLKIN_PLLDIV_OUT;
			return true;
		}
	}
	return false;
}

int tlv320aic3104_clock_solve(uint32_t mclk_hz, uint32_t sample_rate_hz,
			      struct tlv320aic3104_clock_solution *out)
{
	if (mclk_hz == 0U || sample_rate_hz == 0U) {
		return -EINVAL;
	}
	if (mclk_hz < MCLK_MIN_HZ || mclk_hz > MCLK_MAX_HZ) {
		return -ENOTSUP;
	}

	for (int ncodec_x2 = NCODEC_X2_MIN; ncodec_x2 <= (int)NCODEC_X2_MAX; ncodec_x2++) {
		const int64_t fsref_x2 = (int64_t)sample_rate_hz * ncodec_x2;

		if (fsref_x2 > (int64_t)FSREF_MAX_HZ * 2) {
			continue;
		}
		if (solve_divider(mclk_hz, ncodec_x2, fsref_x2, out)) {
			return 0;
		}
	}

	for (int ncodec_x2 = NCODEC_X2_MIN; ncodec_x2 <= (int)NCODEC_X2_MAX; ncodec_x2++) {
		const int64_t fsref_x2 = (int64_t)sample_rate_hz * ncodec_x2;

		if (fsref_x2 > (int64_t)FSREF_MAX_HZ * 2) {
			continue;
		}
		if (solve_pll(mclk_hz, ncodec_x2, fsref_x2, out)) {
			return 0;
		}
	}

	return -ENOTSUP;
}

/*
 * DAI (ASI) solver
 */

int tlv320aic3104_dai_solve(audio_dai_type_t dai_type, uint8_t word_size, uint8_t i2s_options,
			    struct tlv320aic3104_dai_solution *out)
{
	uint8_t mode;
	uint8_t wordlen;
	uint8_t r8;

	if (out == NULL) {
		return -EINVAL;
	}

	switch (dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		mode = ASI_CTRL_B_MODE_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		mode = ASI_CTRL_B_MODE_LEFT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		mode = ASI_CTRL_B_MODE_RIGHT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_PCM:
	case AUDIO_DAI_TYPE_PCMA:
	case AUDIO_DAI_TYPE_PCMB:
	case AUDIO_DAI_TYPE_INVALID:
	default:
		return -ENOTSUP;
	}

	switch (word_size) {
	case 16:
		wordlen = ASI_CTRL_B_WORDLEN_16;
		break;
	case 20:
		wordlen = ASI_CTRL_B_WORDLEN_20;
		break;
	case 24:
		wordlen = ASI_CTRL_B_WORDLEN_24;
		break;
	case 32:
		wordlen = ASI_CTRL_B_WORDLEN_32;
		break;
	default:
		return -ENOTSUP;
	}

	r8 = ASI_CTRL_A_HIZ_DOUT;
	if ((i2s_options & I2S_OPT_BIT_CLK_TARGET) == 0) {
		r8 |= ASI_CTRL_A_BCLK_OUTPUT;
	}
	if ((i2s_options & I2S_OPT_FRAME_CLK_TARGET) == 0) {
		r8 |= ASI_CTRL_A_WCLK_OUTPUT;
	}

	out->r8_asi_ctrl_a = r8;
	out->r9_asi_ctrl_b = (uint8_t)(mode | wordlen);
	return 0;
}

/*
 * Fault reporting
 */

int tlv320aic3104_fault_init(const struct device *dev)
{
	return tlv320aic3104_bus_write_reg(dev, 0, HP_OUTPUT_DRIVER_CTRL,
					   HP_OUTPUT_DRIVER_SHORT_CCT_PROTECT_EN |
						   HP_OUTPUT_DRIVER_SHORT_CCT_AUTO_PWRDN);
}

int tlv320aic3104_fault_register_callback(const struct device *dev, audio_codec_error_callback_t cb)
{
	struct tlv320aic3104_data *data = dev->data;

	data->fault_cb = cb;
	return 0;
}

int tlv320aic3104_fault_clear(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;

	data->fault_sticky_errors = 0;
	return 0;
}

int tlv320aic3104_fault_check(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;
	uint8_t flags;
	int ret;

	ret = tlv320aic3104_bus_read_reg(dev, 0, STICKY_INTERRUPT_FLAGS, &flags);
	if (ret < 0) {
		return ret;
	}

	if ((flags & STICKY_INTERRUPT_FLAGS_SHORT_CCT_MASK) == 0) {
		return 0;
	}

	data->fault_sticky_errors |= AUDIO_CODEC_ERROR_OVERCURRENT;

	if (data->fault_cb != NULL) {
		data->fault_cb(dev, data->fault_sticky_errors);
	}

	return 0;
}

int tlv320aic3104_fault_get_errors(const struct device *dev, uint32_t *out_errors)
{
	const struct tlv320aic3104_data *data = dev->data;

	if (out_errors == NULL) {
		return -EINVAL;
	}

	*out_errors = data->fault_sticky_errors;
	return 0;
}

/*
 * Input path
 */

static uint8_t line2_adc_val(uint8_t input, bool is_mono, uint8_t stereo_val)
{
	if (input != TLV320AIC3104_INPUT_LINE2) {
		return LINE2_TO_ADC_DISCONNECT;
	}

	return is_mono ? LINE2_TO_ADC_MONO : stereo_val;
}

int tlv320aic3104_in_set_line2_routing(const struct device *dev, bool is_mono)
{
	const struct tlv320aic3104_data *data = dev->data;
	const uint8_t left = line2_adc_val(data->input_left, is_mono, LINE2_TO_ADC_STEREO_L);
	const uint8_t right = line2_adc_val(data->input_right, is_mono, LINE2_TO_ADC_STEREO_R);
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_LADC, left);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_RADC, right);
}

static int set_adc_power(const struct device *dev, uint8_t addr, bool on)
{
	return tlv320aic3104_bus_update_reg(dev, 0, addr, ADC_CHANNEL_POWER_BIT,
					    on ? ADC_CHANNEL_POWER_BIT : 0);
}

static int set_line1_field(const struct device *dev, uint8_t addr, bool connect)
{
	return tlv320aic3104_bus_update_reg(
		dev, 0, addr, (uint8_t)~LINE1_TO_ADC_FIELD_PRESERVE_MASK,
		connect ? LINE1_TO_ADC_CONNECT_0DB : LINE1_TO_ADC_DISCONNECT);
}

static int route_adc_side(const struct device *dev, uint32_t input, uint8_t line2_reg,
			  uint8_t line2_connect_val, uint8_t line1_reg)
{
	const bool line2 = (input == TLV320AIC3104_INPUT_LINE2);
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, line2_reg,
					  line2 ? line2_connect_val : LINE2_TO_ADC_DISCONNECT);
	if (ret < 0) {
		return ret;
	}

	return set_line1_field(dev, line1_reg, !line2);
}

int tlv320aic3104_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	struct tlv320aic3104_data *data = dev->data;
	bool do_left;
	bool do_right;
	int ret;

	if (input != TLV320AIC3104_INPUT_LINE1 && input != TLV320AIC3104_INPUT_LINE2) {
		return -ENOTSUP;
	}

	ret = tlv320aic3104_channel_to_lr(channel, &do_left, &do_right);
	if (ret < 0) {
		return ret;
	}

	if (do_left) {
		ret = route_adc_side(dev, input, MIC2_LINE2_TO_LADC, LINE2_TO_ADC_STEREO_L,
				     LINE1L_TO_LADC_CTRL);
		if (ret < 0) {
			return ret;
		}
		data->input_left = (uint8_t)input;
	}
	if (do_right) {
		ret = route_adc_side(dev, input, MIC2_LINE2_TO_RADC, LINE2_TO_ADC_STEREO_R,
				     LINE1R_TO_RADC_CTRL);
		if (ret < 0) {
			return ret;
		}
		data->input_right = (uint8_t)input;
	}

	return 0;
}

#define ADC_PGA_STEP_DIDB     5
#define ADC_PGA_GAIN_MAX_DIDB ((int)ADC_PGA_GAIN_MAX_CODE * ADC_PGA_STEP_DIDB)

static uint8_t pga_gain_code_from_didb(int gain_didb)
{
	const int clamped = CLAMP(gain_didb, 0, ADC_PGA_GAIN_MAX_DIDB);

	return (uint8_t)(clamped / ADC_PGA_STEP_DIDB);
}

int tlv320aic3104_in_set_pga_gain(const struct device *dev, audio_channel_t channel, int gain_didb)
{
	struct tlv320aic3104_data *data = dev->data;
	const uint8_t code = pga_gain_code_from_didb(gain_didb);
	int ret;

	bool do_left;
	bool do_right;

	ret = tlv320aic3104_channel_to_lr(channel, &do_left, &do_right);
	if (ret < 0) {
		return ret;
	}

	if (do_left) {
		data->last_left_adc_pga = code;
		ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, code);
		if (ret < 0) {
			return ret;
		}
	}
	if (do_right) {
		data->last_right_adc_pga = code;
		ret = tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, code);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define MIC_BIAS_LEVEL_2V   1U
#define MIC_BIAS_LEVEL_2V5  2U
#define MIC_BIAS_LEVEL_AVDD 3U

static uint8_t micbias_reg_value(uint8_t level)
{
	switch (level) {
	case MIC_BIAS_LEVEL_2V:
		return MICBIAS_2V;
	case MIC_BIAS_LEVEL_2V5:
		return MICBIAS_2V5;
	case MIC_BIAS_LEVEL_AVDD:
		return MICBIAS_AVDD;
	default:
		return MICBIAS_OFF;
	}
}

static uint8_t adc_hpf_reg_value(uint8_t code)
{
	const uint8_t corner = (uint8_t)(code & ADC_HPF_CORNER_MASK);

	return (uint8_t)((corner << ADC_HPF_LEFT_SHIFT) | (corner << ADC_HPF_RIGHT_SHIFT));
}

int tlv320aic3104_in_start(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	const struct tlv320aic3104_data *data = dev->data;
	const bool is_mono = (data->channel_mode == TLV320AIC3104_CHANNEL_MONO);
	int ret;

	ret = tlv320aic3104_in_set_line2_routing(dev, is_mono);
	if (ret < 0) {
		return ret;
	}

	ret = set_adc_power(dev, LINE1L_TO_LADC_CTRL, true);
	if (ret < 0) {
		return ret;
	}
	ret = set_adc_power(dev, LINE1R_TO_RADC_CTRL, true);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, MICBIAS_CTRL,
					  micbias_reg_value(cfg->mic_bias_level));
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, CODEC_FILTER, adc_hpf_reg_value(cfg->adc_hpf));
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, data->last_left_adc_pga);
	if (ret < 0) {
		return ret;
	}
	return tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, data->last_right_adc_pga);
}

int tlv320aic3104_in_stop(const struct device *dev)
{
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, ADC_PGA_MUTE);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, ADC_PGA_MUTE);
	if (ret < 0) {
		return ret;
	}

	ret = set_adc_power(dev, LINE1L_TO_LADC_CTRL, false);
	if (ret < 0) {
		return ret;
	}
	return set_adc_power(dev, LINE1R_TO_RADC_CTRL, false);
}

/*
 * Output path
 */

static int write_output_volume_regs(const struct device *dev, uint8_t dac_vol, uint8_t routing_vol);

static uint8_t left_mixer_reg(uint8_t output)
{
	return (output == TLV320AIC3104_OUTPUT_LOP) ? DAC_L1_TO_LEFT_LOP_VOL : DAC_L1_TO_HPLOUT_VOL;
}

static uint8_t right_mixer_reg(uint8_t output)
{
	return (output == TLV320AIC3104_OUTPUT_LOP) ? DAC_R1_TO_RIGHT_LOP_VOL
						    : DAC_R1_TO_HPROUT_VOL;
}

int tlv320aic3104_out_init_hprcom(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	if (!cfg->hprcom_vcm_output) {
		return 0;
	}

	ret = tlv320aic3104_bus_update_reg(dev, 0, HP_OUTPUT_DRIVER_CTRL,
					   HP_OUTPUT_DRIVER_HPRCOM_MODE_MASK,
					   HP_OUTPUT_DRIVER_HPRCOM_MODE_VCM);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, HPRCOM_LEVEL, HPRCOM_LEVEL_0DB_UNMUTED_POWERED);
}

static uint8_t output_routing_vol(const struct tlv320aic3104_data *data)
{
	return data->output_muted ? DAC_TO_OUT_ROUTED_MUTE : data->last_routing_vol;
}

static int write_output_level(const struct device *dev)
{
	const struct tlv320aic3104_data *data = dev->data;

	if (data->output_muted) {
		return write_output_volume_regs(dev, DAC_VOL_MUTE, DAC_TO_OUT_ROUTED_MUTE);
	}

	return write_output_volume_regs(dev, data->last_dac_vol, data->last_routing_vol);
}

int tlv320aic3104_out_start(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;
	int ret;

	ret = write_output_level(dev);
	if (ret < 0) {
		return ret;
	}
	data->output_running = true;

	return tlv320aic3104_fault_check(dev);
}

int tlv320aic3104_out_stop(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;

	data->output_running = false;

	return write_output_volume_regs(dev, DAC_VOL_MUTE, DAC_TO_OUT_ROUTED_MUTE);
}

static int write_output_volume_regs(const struct device *dev, uint8_t dac_vol, uint8_t routing_vol)
{
	const struct tlv320aic3104_data *data = dev->data;
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_DAC_VOL, dac_vol);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, RIGHT_DAC_VOL, dac_vol);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, left_mixer_reg(data->output_left), routing_vol);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, right_mixer_reg(data->output_right),
					   routing_vol);
}

static int apply_output_volume_regs(const struct device *dev, uint8_t dac_vol, uint8_t routing_vol)
{
	struct tlv320aic3104_data *data = dev->data;

	data->last_dac_vol = dac_vol;
	data->last_routing_vol = routing_vol;

	if (!data->output_running || data->output_muted) {
		return 0;
	}

	return write_output_volume_regs(dev, dac_vol, routing_vol);
}

int tlv320aic3104_out_set_volume(const struct device *dev, int vol)
{
	uint8_t dac_vol;
	uint8_t routing_vol;
	int db;

	if (vol > 0 || vol < -CODEC_VOLUME_ATTEN_MAX) {
		return -EINVAL;
	}

	db = CLAMP(-vol, 0, CODEC_VOLUME_ATTEN_MAX);

	dac_vol = (uint8_t)db;
	routing_vol = DAC_TO_OUT_ROUTED_0DB | (uint8_t)db;

	return apply_output_volume_regs(dev, dac_vol, routing_vol);
}

int tlv320aic3104_out_set_mute(const struct device *dev, bool mute)
{
	struct tlv320aic3104_data *data = dev->data;

	data->output_muted = mute;

	if (!data->output_running) {
		return 0;
	}

	return write_output_level(dev);
}

static int set_route_bit(const struct device *dev, uint8_t addr, bool routed)
{
	return tlv320aic3104_bus_update_reg(dev, 0, addr, DAC_TO_OUT_ROUTE_BIT,
					    routed ? DAC_TO_OUT_ROUTE_BIT : 0);
}

static int select_terminal(const struct device *dev, uint8_t selected, uint8_t other)
{
	const struct tlv320aic3104_data *data = dev->data;
	int ret = set_route_bit(dev, selected, true);

	if (ret < 0) {
		return ret;
	}
	ret = set_route_bit(dev, other, false);
	if (ret < 0) {
		return ret;
	}

	if (!data->output_running) {
		return 0;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, selected, output_routing_vol(data));
}

int tlv320aic3104_out_route_output(const struct device *dev, audio_channel_t channel,
				   uint32_t output)
{
	struct tlv320aic3104_data *data = dev->data;
	uint8_t selected_left;
	uint8_t other_left;
	uint8_t selected_right;
	uint8_t other_right;
	bool do_left;
	bool do_right;
	int ret;

	switch (output) {
	case TLV320AIC3104_OUTPUT_HP:
		selected_left = DAC_L1_TO_HPLOUT_VOL;
		other_left = DAC_L1_TO_LEFT_LOP_VOL;
		selected_right = DAC_R1_TO_HPROUT_VOL;
		other_right = DAC_R1_TO_RIGHT_LOP_VOL;
		break;
	case TLV320AIC3104_OUTPUT_LOP:
		selected_left = DAC_L1_TO_LEFT_LOP_VOL;
		other_left = DAC_L1_TO_HPLOUT_VOL;
		selected_right = DAC_R1_TO_RIGHT_LOP_VOL;
		other_right = DAC_R1_TO_HPROUT_VOL;
		break;
	default:
		return -ENOTSUP;
	}

	ret = tlv320aic3104_channel_to_lr(channel, &do_left, &do_right);
	if (ret < 0) {
		return ret;
	}

	if (do_left) {
		ret = select_terminal(dev, selected_left, other_left);
		if (ret < 0) {
			return ret;
		}
		data->output_left = (uint8_t)output;
	}
	if (do_right) {
		ret = select_terminal(dev, selected_right, other_right);
		if (ret < 0) {
			return ret;
		}
		data->output_right = (uint8_t)output;
	}

	return 0;
}

int tlv320aic3104_out_set_channel_mode(const struct device *dev, bool is_mono)
{
	return tlv320aic3104_bus_update_reg(dev, 0, CODEC_DATAPATH_SETUP,
					    CODEC_DATAPATH_MODE_FIELD_MASK,
					    is_mono ? CODEC_DATAPATH_DAC_MONO
						    : CODEC_DATAPATH_DAC_STEREO);
}

/*
 * Top-level codec driver
 */

#define CODEC_RESET_DELAY_MS 100

static int codec_hw_reset(const struct device *dev);
static int codec_soft_reset(const struct device *dev);
static int apply_channel_mode_regs(const struct device *dev, enum tlv320aic3104_channel_mode mode);
static int apply_clock_solution(const struct device *dev,
				const struct tlv320aic3104_clock_solution *sol);

static int codec_initialize(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	struct tlv320aic3104_data *data = dev->data;

	if (!tlv320aic3104_bus_is_ready(dev)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	data->input_left = TLV320AIC3104_INPUT_LINE2;
	data->input_right = TLV320AIC3104_INPUT_LINE2;

	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	static const struct {
		uint8_t addr;
		uint8_t val;
	} k_init_regs[] = {
		{ASI_CTRL_C, 0x00U},
		{CODEC_FILTER, 0x00U},
		{LINE1L_TO_LADC_CTRL, LINE1_TO_ADC_DISCONNECT},
		{LINE1R_TO_RADC_CTRL, LINE1_TO_ADC_DISCONNECT},
		{HEADSET_DETECT_B, HEADSET_DETECT_B_DIFF_AC},
		{DAC_POWER_DRV, DAC_POWER_ON},
		{HP_OUTPUT_STAGE, HP_OUTPUT_1P5V_SOFT},
		{OUTPUT_POP_REDUCTION, OUTPUT_POP_800MS_BG},
		{LEFT_DAC_VOL, DAC_VOL_MUTE},
		{RIGHT_DAC_VOL, DAC_VOL_MUTE},
		{DAC_L1_TO_HPLOUT_VOL, DAC_TO_OUT_ROUTED_MUTE},
		{HPLOUT_LEVEL, HP_LEVEL_0DB_UNMUTED_POWERED},
		{DAC_R1_TO_HPROUT_VOL, DAC_TO_OUT_ROUTED_MUTE},
		{HPROUT_LEVEL, HP_LEVEL_0DB_UNMUTED_POWERED},
		{DAC_L1_TO_LEFT_LOP_VOL, DAC_TO_OUT_ROUTED_MUTE},
		{LEFT_LOP_LEVEL, LOP_LEVEL_0DB_UNMUTED_POWERED},
		{DAC_R1_TO_RIGHT_LOP_VOL, DAC_TO_OUT_ROUTED_MUTE},
		{RIGHT_LOP_LEVEL, LOP_LEVEL_0DB_UNMUTED_POWERED},
		{DAC_QUIESCENT, 0x00U},
	};

	const struct tlv320aic3104_config *dev_cfg = dev->config;
	struct tlv320aic3104_data *data = dev->data;
	struct tlv320aic3104_clock_solution clock_sol;
	struct tlv320aic3104_dai_solution dai_sol;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	ret = tlv320aic3104_dai_solve(cfg->dai_type, cfg->dai_cfg.i2s.word_size,
				      cfg->dai_cfg.i2s.options, &dai_sol);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_clock_solve(cfg->mclk_freq, cfg->dai_cfg.i2s.frame_clk_freq,
					&clock_sol);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&dev_cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}
	ret = codec_hw_reset(dev);
	if (ret < 0) {
		return ret;
	}
	ret = codec_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	data->input_left = TLV320AIC3104_INPUT_LINE2;
	data->input_right = TLV320AIC3104_INPUT_LINE2;
	data->output_running = false;

	ret = apply_channel_mode_regs(dev, data->channel_mode);
	if (ret < 0) {
		return ret;
	}

	ret = apply_clock_solution(dev, &clock_sol);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, ASI_CTRL_A, dai_sol.r8_asi_ctrl_a);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, ASI_CTRL_B, dai_sol.r9_asi_ctrl_b);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(k_init_regs); i++) {
		ret = tlv320aic3104_bus_write_reg(dev, 0, k_init_regs[i].addr, k_init_regs[i].val);
		if (ret < 0) {
			return ret;
		}
	}

	ret = tlv320aic3104_fault_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_out_init_hprcom(dev);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_out_route_output(dev, AUDIO_CHANNEL_FRONT_LEFT, data->output_left);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_out_route_output(dev, AUDIO_CHANNEL_FRONT_RIGHT, data->output_right);
	if (ret < 0) {
		return ret;
	}

	data->last_dac_vol = DAC_VOL_0DB;
	data->last_routing_vol = DAC_TO_OUT_ROUTED_0DB;

	return 0;
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	int ret;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		if (channel != AUDIO_CHANNEL_ALL) {
			LOG_ERR("Only AUDIO_CHANNEL_ALL supported for output volume");
			return -EINVAL;
		}
		ret = tlv320aic3104_out_set_volume(dev, val.vol);
		if (ret < 0) {
			return ret;
		}
		return tlv320aic3104_fault_check(dev);
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (channel != AUDIO_CHANNEL_ALL) {
			LOG_ERR("Only AUDIO_CHANNEL_ALL supported for output mute");
			return -EINVAL;
		}
		ret = tlv320aic3104_out_set_mute(dev, val.mute);
		if (ret < 0) {
			return ret;
		}
		return tlv320aic3104_fault_check(dev);
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return tlv320aic3104_in_set_pga_gain(dev, channel, val.vol);
	default:
		break;
	}

	return -EINVAL;
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = tlv320aic3104_in_start(dev);
		if (ret < 0) {
			return ret;
		}
	}
	if ((dir & AUDIO_DAI_DIR_TX) != 0U) {
		ret = tlv320aic3104_out_start(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = tlv320aic3104_in_stop(dev);
		if (ret < 0) {
			return ret;
		}
	}
	if ((dir & AUDIO_DAI_DIR_TX) != 0U) {
		ret = tlv320aic3104_out_stop(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int codec_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int codec_hw_reset(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);
	if (ret < 0) {
		return ret;
	}
	(void)k_msleep(CODEC_RESET_DELAY_MS);
	ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
	if (ret < 0) {
		return ret;
	}
	(void)k_msleep(CODEC_RESET_DELAY_MS);

	return 0;
}

static int codec_soft_reset(const struct device *dev)
{
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, SOFT_RESET_ADDR, SOFT_RESET_ASSERT);
	if (ret < 0) {
		return ret;
	}
	(void)k_msleep(CODEC_RESET_DELAY_MS);

	return 0;
}

static int apply_channel_mode_regs(const struct device *dev, enum tlv320aic3104_channel_mode mode)
{
	const bool is_mono = (mode == TLV320AIC3104_CHANNEL_MONO);
	int ret;

	ret = tlv320aic3104_out_set_channel_mode(dev, is_mono);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_in_set_line2_routing(dev, is_mono);
}

static int apply_clock_solution(const struct device *dev,
				const struct tlv320aic3104_clock_solution *sol)
{
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, NDAC_NADC, sol->r2_ndac_nadc);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, PLL_PROG_A, sol->r3_pll_enable_q_p);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, PLL_PROG_B, sol->r4_pll_j);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, PLL_PROG_C, sol->r5_pll_d_msb);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, PLL_PROG_D, sol->r6_pll_d_lsb);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, PLL_PROG_E, sol->r11_pll_r);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, CODEC_CLKIN_SRC, sol->r101_codec_clkin_src);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_bus_update_reg(dev, 0, CODEC_DATAPATH_SETUP,
					    CODEC_DATAPATH_FSREF_FAMILY_BIT, sol->r7_fsref_family);
}

static int codec_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	struct tlv320aic3104_data *data = dev->data;
	int ret;

	if (channel == AUDIO_CHANNEL_ALL || channel == AUDIO_CHANNEL_FRONT_CENTER) {
		const enum tlv320aic3104_channel_mode mode = (channel == AUDIO_CHANNEL_FRONT_CENTER)
								     ? TLV320AIC3104_CHANNEL_MONO
								     : TLV320AIC3104_CHANNEL_STEREO;

		ret = apply_channel_mode_regs(dev, mode);
		if (ret < 0) {
			return ret;
		}
		data->channel_mode = mode;

		channel = AUDIO_CHANNEL_ALL;
	}

	return tlv320aic3104_out_route_output(dev, channel, output);
}

static void codec_start_output(const struct device *dev)
{
	int ret = tlv320aic3104_out_start(dev);

	if (ret < 0) {
		LOG_ERR("Failed to start output: %d", ret);
	}
}

static void codec_stop_output(const struct device *dev)
{
	int ret = tlv320aic3104_out_stop(dev);

	if (ret < 0) {
		LOG_ERR("Failed to stop output: %d", ret);
	}
}

static DEVICE_API(audio_codec, codec_driver_api) = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
	.route_input = tlv320aic3104_in_route_input,
	.route_output = codec_route_output,
	.start = codec_start,
	.stop = codec_stop,
	.clear_errors = tlv320aic3104_fault_clear,
	.register_error_callback = tlv320aic3104_fault_register_callback,
};

#define TLV320AIC3104_DEFINE(inst)                                                                 \
	static struct tlv320aic3104_data tlv320aic3104_data_##inst;                                \
	static const struct tlv320aic3104_config tlv320aic3104_config_##inst = {                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.mic_bias_level = (uint8_t)DT_INST_PROP(inst, mic_bias_level),                     \
		.adc_hpf = (uint8_t)DT_INST_PROP(inst, adc_high_pass_filter),                      \
		.hprcom_vcm_output = DT_INST_PROP(inst, hprcom_vcm_output),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, codec_initialize, NULL, &tlv320aic3104_data_##inst,            \
			      &tlv320aic3104_config_##inst, POST_KERNEL,                           \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLV320AIC3104_DEFINE)
