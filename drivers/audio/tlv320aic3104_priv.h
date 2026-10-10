/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_PRIV_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_PRIV_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Register map */

#define PAGE_CONTROL_ADDR 0x00

#define SOFT_RESET_ADDR   0x01
#define SOFT_RESET_ASSERT 0x80

#define CODEC_DATAPATH_SETUP 0x07

#define ASI_CTRL_A             0x08
#define ASI_CTRL_A_HIZ_DOUT    0x20

#define ASI_CTRL_A_BCLK_MASTER 0x80
#define ASI_CTRL_A_WCLK_MASTER 0x40

#define ASI_CTRL_B                      0x09

#define ASI_CTRL_B_MODE_I2S             0x00
#define ASI_CTRL_B_MODE_DSP             0x40
#define ASI_CTRL_B_MODE_RIGHT_JUSTIFIED 0x80
#define ASI_CTRL_B_MODE_LEFT_JUSTIFIED  0xC0

#define ASI_CTRL_B_WORDLEN_16           0x00
#define ASI_CTRL_B_WORDLEN_20           0x10
#define ASI_CTRL_B_WORDLEN_24           0x20
#define ASI_CTRL_B_WORDLEN_32           0x30

#define ASI_CTRL_C               0x0A
#define CODEC_FILTER             0x0C
#define HEADSET_DETECT_B         0x0E
#define HEADSET_DETECT_B_DIFF_AC 0xC0

#define NDAC_NADC                       0x02
#define PLL_PROG_A                      0x03
#define PLL_PROG_B                      0x04
#define PLL_PROG_C                      0x05
#define PLL_PROG_D                      0x06

#define CODEC_DATAPATH_FSREF_FAMILY_BIT 0x80
#define PLL_PROG_E       0x0B

#define CODEC_CLKIN_SRC 0x65
#define R101_CODEC_CLKIN_PLLDIV_OUT 0x00
#define R101_CODEC_CLKIN_CLKDIV_OUT 0x01

#define R3_PLL_ENABLE_BIT 0x80

#define DAC_POWER_DRV 0x25
#define DAC_POWER_ON  0xC0

#define HP_OUTPUT_STAGE     0x28
#define HP_OUTPUT_1P5V_SOFT 0x40

#define OUTPUT_POP_REDUCTION 0x2A

#define OUTPUT_POP_800MS_BG  0x92

#define LEFT_DAC_VOL         0x2B
#define RIGHT_DAC_VOL        0x2C
#define DAC_L1_TO_HPLOUT_VOL 0x2F
#define HP_LEVEL_0DB_UNMUTED_POWERED 0x09
#define HPRCOM_LEVEL                     0x48
#define HPRCOM_LEVEL_0DB_UNMUTED_POWERED 0x09
#define HPLOUT_LEVEL         0x33
#define DAC_R1_TO_HPROUT_VOL 0x40
#define HPROUT_LEVEL         0x41

#define DAC_VOL_0DB  0x00
#define DAC_VOL_MUTE 0x80

#define DAC_TO_OUT_ROUTED_0DB   0x80
#define DAC_TO_OUT_ROUTE_BIT    0x80
#define CODEC_VOLUME_ATTEN_MAX  117
#define DAC_TO_OUT_ROUTED_MUTE  0xF6
#define DAC_L1_TO_LEFT_LOP_VOL  0x52
#define LOP_LEVEL_0DB_UNMUTED_POWERED 0x09
#define LEFT_LOP_LEVEL          0x56
#define DAC_R1_TO_RIGHT_LOP_VOL 0x5C
#define RIGHT_LOP_LEVEL         0x5D
#define DAC_QUIESCENT           0x6D

#define CODEC_DATAPATH_DAC_STEREO      0x0A
#define CODEC_DATAPATH_DAC_MONO        0x1E
#define CODEC_DATAPATH_MODE_FIELD_MASK 0x1E

#define LEFT_ADC_PGA_GAIN     0x0F
#define RIGHT_ADC_PGA_GAIN    0x10
#define ADC_PGA_MUTE          0x80

#define ADC_PGA_GAIN_MAX_CODE 0x77

#define MIC2_LINE2_TO_LADC      0x11
#define MIC2_LINE2_TO_RADC      0x12
#define LINE2_TO_ADC_STEREO_L   0x0F
#define LINE2_TO_ADC_STEREO_R   0xF0
#define LINE2_TO_ADC_MONO       0x44
#define LINE2_TO_ADC_DISCONNECT 0xFF

#define LINE1L_TO_LADC_CTRL              0x13
#define LINE1R_TO_RADC_CTRL              0x16
#define ADC_CHANNEL_POWER_BIT            0x04

#define LINE1_TO_ADC_FIELD_PRESERVE_MASK 0x87
#define LINE1_TO_ADC_CONNECT_0DB         0x00
#define LINE1_TO_ADC_DISCONNECT          0x78

#define MICBIAS_CTRL 0x19
#define MICBIAS_OFF  0x00
#define MICBIAS_2V   0x40
#define MICBIAS_2V5  0x80
#define MICBIAS_AVDD 0xC0

#define HP_OUTPUT_DRIVER_CTRL                 0x26
#define HP_OUTPUT_DRIVER_SHORT_CCT_PROTECT_EN 0x04
#define HP_OUTPUT_DRIVER_SHORT_CCT_AUTO_PWRDN 0x02
#define HP_OUTPUT_DRIVER_HPRCOM_MODE_MASK     0x38
#define HP_OUTPUT_DRIVER_HPRCOM_MODE_VCM      0x08

#define STICKY_INTERRUPT_FLAGS                0x60
#define STICKY_INTERRUPT_FLAGS_SHORT_CCT_MASK 0xF0

/* Shared instance state */

static inline int tlv320aic3104_channel_to_lr(audio_channel_t channel, bool *do_left,
					      bool *do_right)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		*do_left = true;
		*do_right = false;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		*do_left = false;
		*do_right = true;
		break;
	case AUDIO_CHANNEL_ALL:
		*do_left = true;
		*do_right = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

enum tlv320aic3104_channel_mode {
	TLV320AIC3104_CHANNEL_STEREO = 0,
	TLV320AIC3104_CHANNEL_MONO = 1,
};

struct tlv320aic3104_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;

	uint8_t mic_bias_level;

	uint8_t adc_hpf;

	bool hprcom_vcm_output;
};

struct tlv320aic3104_data {
	uint8_t page_cache;
	uint8_t channel_mode;

	uint8_t last_dac_vol;
	uint8_t last_routing_vol;

	bool output_running;
	bool output_muted;
	uint8_t output_left;
	uint8_t output_right;

	uint8_t last_left_adc_pga;
	uint8_t last_right_adc_pga;

	audio_codec_error_callback_t fault_cb;
	uint32_t fault_sticky_errors;
};

/* Bus */

bool tlv320aic3104_bus_is_ready(const struct device *dev);

int tlv320aic3104_bus_write_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t val);

int tlv320aic3104_bus_update_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t mask,
				 uint8_t val);

int tlv320aic3104_bus_read_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t *val);

/* Clock (PLL / divider) solver */

typedef struct {

	bool pll_enabled;

	uint8_t r2_ndac_nadc;

	uint8_t r3_pll_enable_q_p;

	uint8_t r4_pll_j;

	uint8_t r5_pll_d_msb;

	uint8_t r6_pll_d_lsb;

	uint8_t r7_fsref_family;

	uint8_t r11_pll_r;

	uint8_t r101_codec_clkin_src;
} tlv320aic3104_clock_solution;

int tlv320aic3104_clock_solve(uint32_t mclk_hz, uint32_t sample_rate_hz,
			      tlv320aic3104_clock_solution *out);

/* DAI (ASI) solver */

typedef struct {

	uint8_t r8_asi_ctrl_a;

	uint8_t r9_asi_ctrl_b;
} tlv320aic3104_dai_solution;

int tlv320aic3104_dai_solve(audio_dai_type_t dai_type, uint8_t word_size, uint8_t i2s_options,
			    tlv320aic3104_dai_solution *out);

/* Fault reporting */

int tlv320aic3104_fault_init(const struct device *dev);

int tlv320aic3104_fault_register_callback(const struct device *dev,
					  audio_codec_error_callback_t cb);

int tlv320aic3104_fault_clear(const struct device *dev);

int tlv320aic3104_fault_check(const struct device *dev);

int tlv320aic3104_fault_get_errors(const struct device *dev, uint32_t *out_errors);

/* Input path */

enum tlv320aic3104_input {
	TLV320AIC3104_INPUT_LINE1 = 0,
	TLV320AIC3104_INPUT_LINE2 = 1,
};

int tlv320aic3104_in_set_line2_routing(const struct device *dev, bool is_mono);

int tlv320aic3104_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input);

int tlv320aic3104_in_set_pga_gain(const struct device *dev, audio_channel_t channel, int gain_didb);

int tlv320aic3104_in_start(const struct device *dev);

int tlv320aic3104_in_stop(const struct device *dev);

/* Output path */

enum tlv320aic3104_output {
	TLV320AIC3104_OUTPUT_HP = 0,
	TLV320AIC3104_OUTPUT_LOP = 1,
};

/**
 * @brief Apply the devicetree ``hprcom-vcm-output`` board property.
 *
 * No-op unless the property is set. Must run after
 * @ref tlv320aic3104_fault_init, which writes register R38 as a whole byte
 * and would otherwise clear the mode field this sets.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int tlv320aic3104_out_init_hprcom(const struct device *dev);

void tlv320aic3104_out_start(const struct device *dev);

void tlv320aic3104_out_stop(const struct device *dev);

int tlv320aic3104_out_set_volume(const struct device *dev, int vol);

void tlv320aic3104_out_set_mute(const struct device *dev, bool mute);

int tlv320aic3104_out_route_output(const struct device *dev, audio_channel_t channel,
				   uint32_t output);

int tlv320aic3104_out_set_channel_mode(const struct device *dev, bool is_mono);

#ifdef __cplusplus
}
#endif

#endif
