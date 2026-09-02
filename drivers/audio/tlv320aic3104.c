/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlv320aic3104

#include <errno.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include "tlv320aic3104_bus.h"
#include "tlv320aic3104_clock.h"
#include "tlv320aic3104_dai.h"
#include "tlv320aic3104_fault.h"
#include "tlv320aic3104_in.h"
#include "tlv320aic3104_out.h"
#include "tlv320aic3104_priv.h"
#include "tlv320aic3104_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlv320aic3104);

static void codec_hw_reset(const struct device *dev);
static void codec_soft_reset(const struct device *dev);
static void codec_configure_output(const struct device *dev);
static int apply_channel_mode_regs(const struct device *dev, enum tlv320aic3104_channel_mode mode);
static int apply_clock_solution(const struct device *dev, const tlv320aic3104_clock_solution *sol);

static int codec_initialize(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;

	if (!tlv320aic3104_bus_is_ready(dev)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{

	static const struct {
		uint8_t addr;
		uint8_t val;
	} k_init_regs[] = {

		{ASI_CTRL_C, 0x00},

		{CODEC_FILTER, 0x00},

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

		{DAC_QUIESCENT, 0x00},
	};

	const struct tlv320aic3104_config *dev_cfg = dev->config;
	struct tlv320aic3104_data *data = dev->data;
	tlv320aic3104_clock_solution clock_sol;
	tlv320aic3104_dai_solution dai_sol;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	ret = tlv320aic3104_clock_solve(cfg->mclk_freq, cfg->dai_cfg.i2s.frame_clk_freq,
					&clock_sol);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_dai_solve(cfg->dai_type, cfg->dai_cfg.i2s.word_size,
				      cfg->dai_cfg.i2s.options, &dai_sol);
	if (ret < 0) {
		return ret;
	}

	gpio_pin_configure_dt(&dev_cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
	codec_hw_reset(dev);
	codec_soft_reset(dev);

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

	data->last_dac_vol = DAC_VOL_0DB;
	data->last_routing_vol = DAC_TO_OUT_ROUTED_0DB;
	data->output_running = false;

	codec_configure_output(dev);

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

		(void)tlv320aic3104_fault_check(dev);
		return ret;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (channel != AUDIO_CHANNEL_ALL) {
			LOG_ERR("Only AUDIO_CHANNEL_ALL supported for output mute");
			return -EINVAL;
		}
		tlv320aic3104_out_set_mute(dev, val.mute);
		(void)tlv320aic3104_fault_check(dev);
		return 0;
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

	if (dir & AUDIO_DAI_DIR_RX) {
		ret = tlv320aic3104_in_start(dev);
		if (ret < 0) {
			return ret;
		}
	}
	if (dir & AUDIO_DAI_DIR_TX) {
		tlv320aic3104_out_start(dev);
	}

	return 0;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;

	if (dir & AUDIO_DAI_DIR_RX) {
		ret = tlv320aic3104_in_stop(dev);
		if (ret < 0) {
			return ret;
		}
	}
	if (dir & AUDIO_DAI_DIR_TX) {
		tlv320aic3104_out_stop(dev);
	}

	return 0;
}

static int codec_apply_properties(const struct device *dev)
{
	(void)dev;
	return 0;
}

static void codec_hw_reset(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;

	gpio_pin_set_dt(&cfg->reset_gpio, 1);
	k_msleep(100);
	gpio_pin_set_dt(&cfg->reset_gpio, 0);
	k_msleep(100);
}

static void codec_soft_reset(const struct device *dev)
{
	tlv320aic3104_bus_write_reg(dev, 0, SOFT_RESET_ADDR, SOFT_RESET_ASSERT);
	k_msleep(100);
}

static void codec_configure_output(const struct device *dev)
{
	(void)dev;

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

static int apply_clock_solution(const struct device *dev, const tlv320aic3104_clock_solution *sol)
{
	uint8_t r7;
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
	ret = tlv320aic3104_bus_write_reg(dev, 0, AUDIO_CODEC_OVERFLOW_FLAG, sol->r11_pll_r);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, CLOCK_REG, sol->r101_codec_clkin_src);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_read_reg(dev, 0, CODEC_DATAPATH_SETUP, &r7);
	if (ret < 0) {
		return ret;
	}
	r7 &= (uint8_t)~CODEC_DATAPATH_FSREF_FAMILY_BIT;
	r7 |= sol->r7_fsref_family;
	return tlv320aic3104_bus_write_reg(dev, 0, CODEC_DATAPATH_SETUP, r7);
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

static DEVICE_API(audio_codec, codec_driver_api) = {
	.configure = codec_configure,
	.start_output = tlv320aic3104_out_start,
	.stop_output = tlv320aic3104_out_stop,
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
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, codec_initialize, NULL, &tlv320aic3104_data_##inst,            \
			      &tlv320aic3104_config_##inst, POST_KERNEL,                           \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLV320AIC3104_DEFINE)
