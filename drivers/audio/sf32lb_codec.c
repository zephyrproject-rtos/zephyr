/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_audcodec

#include <errno.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma/sf32lb.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <bf0_hal_pmu.h>
#include "sf32lb_codec.h"

LOG_MODULE_REGISTER(siflicodec, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define CODEC_CLK_USING_PLL         0
#define AUDCODEC_MIN_VOLUME         -36
#define AUDCODEC_MAX_VOLUME         54

/* hardware gain of volume, The maximum gain should be tested
 * to prevent the speaker from burning out.
 */
#define VOLUME_0_GAIN	-55
#define VOLUME_1_GAIN	-34
#define VOLUME_2_GAIN	-32
#define VOLUME_3_GAIN	-30
#define VOLUME_4_GAIN	-28
#define VOLUME_5_GAIN	-26
#define VOLUME_6_GAIN	-24
#define VOLUME_7_GAIN	-22
#define VOLUME_8_GAIN	-20
#define VOLUME_9_GAIN	-17
#define VOLUME_10_GAIN	-14
#define VOLUME_11_GAIN	-11
#define VOLUME_12_GAIN	-10
#define VOLUME_13_GAIN	-8
#define VOLUME_14_GAIN	-6
#define VOLUME_15_GAIN	-2

typedef enum {
	AUDIO_PLL_CLOSED,
	AUDIO_PLL_OPEN,
	AUDIO_PLL_ENABLE,
} audio_pll_state_t;

struct sf32lb_audcodec_data {
	sf32lb_codec_hw_config_t	hw_config;
	audio_codec_tx_done_callback_t	tx_done;
	audio_codec_rx_done_callback_t	rx_done;
	void				*tx_cb_user_data;
	void				*rx_cb_user_data;
	uint8_t				*tx_buf;
	uint8_t				*tx_write_ptr;
	uint8_t				*rx_buf;
	uint32_t			tx_half_dma_size;
	uint32_t			rx_half_dma_size;
	uint8_t				tx_enable;
	uint8_t				rx_enable;
	uint8_t				last_volume;
	audio_pll_state_t		pll_state;
	uint32_t			pll_samplerate;
};

struct sf32lb_codec_driver_config {
	sf32lb_codec_reg_t		*reg;
	struct sf32lb_dma_dt_spec	dma_tx;
	struct sf32lb_dma_dt_spec	dma_rx;
	struct sf32lb_clock_dt_spec	clock;
};

static const int hardware_gain_of_volume[16] = {
	VOLUME_0_GAIN,
	VOLUME_1_GAIN,
	VOLUME_2_GAIN,
	VOLUME_3_GAIN,
	VOLUME_4_GAIN,
	VOLUME_5_GAIN,
	VOLUME_6_GAIN,
	VOLUME_7_GAIN,
	VOLUME_8_GAIN,
	VOLUME_9_GAIN,
	VOLUME_10_GAIN,
	VOLUME_11_GAIN,
	VOLUME_12_GAIN,
	VOLUME_13_GAIN,
	VOLUME_14_GAIN,
	VOLUME_15_GAIN
};

#if AVDD_V18_ENABLE
	#define SINC_GAIN   0xa0
#else
	#define SINC_GAIN   0x14D
#endif

static const sf32lb_codec_dac_clk_t codec_dac_clk_config[9] = {
#if CODEC_CLK_USING_PLL
	{48000, 1, 1, 0, SINC_GAIN, 1, 5, 4, 2, 20, 20, 0},
	{32000, 1, 1, 1, SINC_GAIN, 1, 5, 4, 2, 20, 20, 0},
	{24000, 1, 1, 5, SINC_GAIN, 1, 10, 2, 2, 10, 10, 1},
	{16000, 1, 1, 4, SINC_GAIN, 1, 5, 4, 2, 20, 20, 0},
	{12000, 1, 1, 7, SINC_GAIN, 1, 20, 2, 1, 5, 5, 1},
	{8000, 1, 1, 8, SINC_GAIN, 1, 10, 2, 2, 10, 10, 1},
#else
	{48000, 0, 1, 0, SINC_GAIN, 0, 5, 4, 2, 20, 20, 0},
	{32000, 0, 1, 1, SINC_GAIN, 0, 5, 4, 2, 20, 20, 0},
	{24000, 0, 1, 5, SINC_GAIN, 0, 10, 2, 2, 10, 10, 1},
	{16000, 0, 1, 4, SINC_GAIN, 0, 5, 4, 2, 20, 20, 0},
	{12000, 0, 1, 7, SINC_GAIN, 0, 20, 2, 1, 5, 5, 1},
	{8000, 0, 1, 8, SINC_GAIN, 0, 10, 2, 2, 10, 10, 1},
#endif
	{44100, 1, 1, 0, SINC_GAIN, 1, 5, 4, 2, 20, 20, 0},
	{22050, 1, 1, 5, SINC_GAIN, 1, 10, 2, 2, 10, 10, 1},
	{11025, 1, 1, 7, SINC_GAIN, 1, 20, 2, 1, 5, 5, 1},
};

const sf32lb_codec_adc_clk_t codec_adc_clk_config[9] = {
#if CODEC_CLK_USING_PLL
	{48000, 1, 5, 0, 1, 1, 5, 0},
	{32000, 1, 5, 1, 1, 1, 5, 0},
	{24000, 1, 10, 0, 1, 0, 5, 2},
	{16000, 1, 10, 1, 1, 0, 5, 2},
	{12000, 1, 10, 2, 1, 0, 5, 2},
	{8000, 1, 10, 3, 1, 0, 5, 2},
#else
	{48000, 0, 5, 0, 0, 1, 5, 0},
	{32000, 0, 5, 1, 0, 1, 5, 0},
	{24000, 0, 10, 0, 0, 0, 5, 2},
	{16000, 0, 10, 1, 0, 0, 5, 2},
	{12000, 0, 10, 2, 0, 0, 5, 2},
	{8000, 0, 10, 3, 0, 0, 5, 2},
#endif
	{44100, 1, 5, 0, 1, 1, 5, 1},
	{22050, 1, 5, 2, 1, 1, 5, 1},
	{11025, 1, 10, 2, 1, 0, 5, 3},
};

struct pll_vco_t {
	uint32_t freq;
	uint32_t vco_value;
	uint32_t target_cnt;
};

struct pll_vco_t g_pll_vco_tab[2] = {
	{48, 0, 2001},
	{44, 0, 1834},
};

static int config_dac_path(sf32lb_codec_reg_t *reg, uint16_t bypass)
{
	if (reg == NULL) {
		return -EINVAL;
	}

	if (bypass) {
		MODIFY_REG(reg->DAC_CH0_CFG, AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Msk,
			MAKE_REG_VAL(1, AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Msk,
				AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Pos));

		reg->DAC_CH0_DEBUG = (1 << AUDCODEC_DAC_CH0_DEBUG_BYPASS_Pos)
				| (0xFF << AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Pos);

		MODIFY_REG(reg->DAC_CH1_CFG, AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Msk,
			MAKE_REG_VAL(1, AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Msk,
				AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Pos));

		reg->DAC_CH1_DEBUG = (1 << AUDCODEC_DAC_CH1_DEBUG_BYPASS_Pos)
				| (0xFF << AUDCODEC_DAC_CH1_DEBUG_DATA_OUT_Pos);
	} else {
		MODIFY_REG(reg->DAC_CH0_CFG, AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Msk,
			MAKE_REG_VAL(0, AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Msk,
				AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Pos));
		reg->DAC_CH0_DEBUG = (0 << AUDCODEC_DAC_CH0_DEBUG_BYPASS_Pos)
			|(0xFF << AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Pos);
		MODIFY_REG(reg->DAC_CH1_CFG, AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Msk,
			MAKE_REG_VAL(0, AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Msk,
				AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Pos));
		reg->DAC_CH1_DEBUG = (0 << AUDCODEC_DAC_CH1_DEBUG_BYPASS_Pos)
			| (0xFF << AUDCODEC_DAC_CH1_DEBUG_DATA_OUT_Pos);
	}

	return 0;
}

static void config_analog_dac_path(sf32lb_codec_reg_t *reg, const sf32lb_codec_dac_clk_t *clk)
{
	reg->PLL_CFG4 &= ~AUDCODEC_PLL_CFG4_SEL_CLK_DAC;
	reg->PLL_CFG4 &= ~AUDCODEC_PLL_CFG4_SEL_CLK_DAC_SOURCE;
	reg->PLL_CFG4 |= (1 << AUDCODEC_PLL_CFG4_EN_CLK_CHOP_DAC_Pos)
			| (1 << AUDCODEC_PLL_CFG4_EN_CLK_DAC_Pos)
			| (clk->sel_clk_dac_source << AUDCODEC_PLL_CFG4_SEL_CLK_DAC_SOURCE_Pos)
			| (clk->sel_clk_dac << AUDCODEC_PLL_CFG4_SEL_CLK_DAC_Pos)
			| (1 << AUDCODEC_PLL_CFG4_EN_CLK_DIG_Pos);

	reg->PLL_CFG5 |= (1 << AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Pos)
			| (1 << AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Pos);

	reg->PLL_CFG2  &= ~AUDCODEC_PLL_CFG2_RSTB;
	k_busy_wait(100);
	reg->PLL_CFG2  |= AUDCODEC_PLL_CFG2_RSTB;

#if AVDD_V18_ENABLE
	reg->DAC1_CFG |= AUDCODEC_DAC1_CFG_LP_MODE;
#else
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_LP_MODE;
#endif
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_EN_OS_DAC;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_OS_DAC;
	reg->DAC1_CFG |= AUDCODEC_DAC1_CFG_EN_VCM;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_VCM;
	k_busy_wait(5);

	reg->DAC1_CFG |= AUDCODEC_DAC1_CFG_EN_AMP;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_AMP;
	k_busy_wait(1);
	reg->DAC1_CFG |= AUDCODEC_DAC1_CFG_EN_OS_DAC;
	reg->DAC2_CFG |= AUDCODEC_DAC2_CFG_EN_OS_DAC;
	k_busy_wait(10);
	reg->DAC1_CFG |= AUDCODEC_DAC1_CFG_EN_DAC;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_DAC;
	k_busy_wait(10);
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_SR;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_SR;
}

static void config_analog_adc_path(sf32lb_codec_reg_t *reg, const sf32lb_codec_adc_clk_t *clk)
{
	reg->BG_CFG0 &= ~AUDCODEC_BG_CFG0_EN_SMPL;
	reg->ADC_ANA_CFG |= AUDCODEC_ADC_ANA_CFG_MICBIAS_EN;
	reg->ADC_ANA_CFG &= ~AUDCODEC_ADC_ANA_CFG_MICBIAS_CHOP_EN;
	/* delay 2ms*/
	k_busy_wait(2000);

	reg->BG_CFG0 &= ~AUDCODEC_BG_CFG0_EN_SMPL; /* noise pop */

	/* adc1 and adc2 clock */
	reg->PLL_CFG6 = (0 << AUDCODEC_PLL_CFG6_SEL_TST_CLK_Pos) |
	(0 << AUDCODEC_PLL_CFG6_EN_TST_CLK_Pos) |
	(0 << AUDCODEC_PLL_CFG6_EN_CLK_RCCAL_Pos) |
	(3 << AUDCODEC_PLL_CFG6_SEL_CLK_CHOP_MICBIAS_Pos) |
	(1 << AUDCODEC_PLL_CFG6_EN_CLK_CHOP_MICBIAS_Pos) |
	(clk->sel_clk_adc << AUDCODEC_PLL_CFG6_SEL_CLK_ADC2_Pos) |
	(clk->diva_clk_adc << AUDCODEC_PLL_CFG6_DIVA_CLK_ADC2_Pos) |
	(1 << AUDCODEC_PLL_CFG6_EN_CLK_ADC2_Pos) |
	(clk->sel_clk_adc << AUDCODEC_PLL_CFG6_SEL_CLK_ADC1_Pos) |
	(clk->diva_clk_adc << AUDCODEC_PLL_CFG6_DIVA_CLK_ADC1_Pos) |
	(1 << AUDCODEC_PLL_CFG6_EN_CLK_ADC1_Pos) |
	(1 << AUDCODEC_PLL_CFG6_SEL_CLK_ADC0_Pos) |
	(5 << AUDCODEC_PLL_CFG6_DIVA_CLK_ADC0_Pos) |
	(1 << AUDCODEC_PLL_CFG6_EN_CLK_ADC0_Pos) |
	(clk->sel_clk_adc_source << AUDCODEC_PLL_CFG6_SEL_CLK_ADC_SOURCE_Pos);

	reg->PLL_CFG2  &= ~AUDCODEC_PLL_CFG2_RSTB;
	k_busy_wait(1000);
	reg->PLL_CFG2  |= AUDCODEC_PLL_CFG2_RSTB;

	reg->ADC1_CFG1 &= ~AUDCODEC_ADC1_CFG1_DIFF_EN;
	/* reg->ADC1_CFG1 |= AUDCODEC_ADC1_CFG1_DIFF_EN; */
	reg->ADC1_CFG1 &= ~AUDCODEC_ADC1_CFG1_DACN_EN;

	reg->ADC1_CFG1 &= ~AUDCODEC_ADC1_CFG1_FSP;
	reg->ADC1_CFG1 |= (clk->fsp << AUDCODEC_ADC1_CFG1_FSP_Pos);

	/* this make long mic startup pulse */
	reg->ADC1_CFG1 |= AUDCODEC_ADC1_CFG1_VCMST;
	reg->ADC1_CFG2 |= AUDCODEC_ADC1_CFG2_CLEAR;

	reg->ADC1_CFG1 &= ~AUDCODEC_ADC1_CFG1_GC_Msk;
	reg->ADC1_CFG1 |= (0x4 << AUDCODEC_ADC1_CFG1_GC_Pos);

	reg->ADC1_CFG2 |= AUDCODEC_ADC1_CFG2_EN;
	reg->ADC1_CFG2 &= ~AUDCODEC_ADC1_CFG2_RSTB;

	reg->ADC1_CFG1 &= ~AUDCODEC_ADC1_CFG1_VREF_SEL;
	reg->ADC1_CFG1 |= (2 << AUDCODEC_ADC1_CFG1_VREF_SEL_Pos);

	reg->ADC2_CFG1 &= ~AUDCODEC_ADC2_CFG1_FSP;
	reg->ADC2_CFG1 |= (clk->fsp << AUDCODEC_ADC2_CFG1_FSP_Pos);

	reg->ADC2_CFG1 |= AUDCODEC_ADC2_CFG1_VCMST;
	reg->ADC2_CFG2 |= AUDCODEC_ADC2_CFG2_CLEAR;

	reg->ADC2_CFG1 &= ~AUDCODEC_ADC2_CFG1_GC_Msk;
	reg->ADC2_CFG1 |= (0x1E << AUDCODEC_ADC2_CFG1_GC_Pos);

	reg->ADC2_CFG2 |= AUDCODEC_ADC2_CFG2_EN;
	reg->ADC2_CFG2 &= ~AUDCODEC_ADC2_CFG2_RSTB;

	reg->ADC2_CFG2 &= ~AUDCODEC_ADC2_CFG1_VREF_SEL;
	reg->ADC2_CFG2 |= (2 << AUDCODEC_ADC2_CFG1_VREF_SEL_Pos);
	/* wait 20ms */
	k_sleep(K_MSEC(20));
	/* reg->BG_CFG0  |= AUDCODEC_BG_CFG0_EN_SMPL; */
	reg->ADC1_CFG2 |= AUDCODEC_ADC1_CFG2_RSTB;
	reg->ADC1_CFG1 &= ~AUDCODEC_ADC1_CFG1_VCMST;
	reg->ADC1_CFG2 &= ~AUDCODEC_ADC1_CFG2_CLEAR;
	reg->ADC2_CFG2 |= AUDCODEC_ADC2_CFG2_RSTB;
	reg->ADC2_CFG1 &= ~AUDCODEC_ADC2_CFG1_VCMST;
	reg->ADC2_CFG2 &= ~AUDCODEC_ADC2_CFG2_CLEAR;
}

static void config_tx_channel(sf32lb_codec_reg_t *reg, int channel, sf32lb_codec_dac_cfg_t *cfg)
{
	const sf32lb_codec_dac_clk_t *dac_clk = cfg->dac_clk;

	reg->CFG |= (3 << AUDCODEC_CFG_ADC_EN_DLY_SEL_Pos);
	reg->DAC_CFG = (dac_clk->osr_sel << AUDCODEC_DAC_CFG_OSR_SEL_Pos)
		| (cfg->opmode << AUDCODEC_DAC_CFG_OP_MODE_Pos)
		| (0 << AUDCODEC_DAC_CFG_PATH_RESET_Pos)
		| (dac_clk->clk_src_sel << AUDCODEC_DAC_CFG_CLK_SRC_SEL_Pos)
		| (dac_clk->clk_div << AUDCODEC_DAC_CFG_CLK_DIV_Pos);

	switch (channel) {
	case 0:
		reg->DAC_CH0_CFG = (1 << AUDCODEC_DAC_CH0_CFG_ENABLE_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Pos)
			| (2 << AUDCODEC_DAC_CH0_CFG_DEM_MODE_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DMA_EN_Pos)
			| (6 << AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos)
			| (1 << AUDCODEC_DAC_CH0_CFG_DATA_FORMAT_Pos)
			| (dac_clk->sinc_gain << AUDCODEC_DAC_CH0_CFG_SINC_GAIN_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DITHER_GAIN_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DITHER_EN_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_CLK_ANA_POL_Pos);

		reg->DAC_CH0_CFG_EXT = (1 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_EN_Pos)
			| (1 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_MODE_Pos)
			| (1 << AUDCODEC_DAC_CH0_CFG_EXT_ZERO_ADJUST_EN_Pos)
			| (2 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_INTERVAL_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_STAT_Pos);

		reg->DAC_CH0_DEBUG = (0 << AUDCODEC_DAC_CH0_DEBUG_BYPASS_Pos)
			| (0xFF << AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Pos);

		break;
	case 1:
		reg->DAC_CH1_CFG = (1 << AUDCODEC_DAC_CH0_CFG_ENABLE_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Pos)
			| (2 << AUDCODEC_DAC_CH0_CFG_DEM_MODE_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DMA_EN_Pos)
			| (6 << AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos)
			| (1 << AUDCODEC_DAC_CH0_CFG_DATA_FORMAT_Pos)
			| (dac_clk->sinc_gain << AUDCODEC_DAC_CH0_CFG_SINC_GAIN_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DITHER_GAIN_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_DITHER_EN_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_CLK_ANA_POL_Pos);

		reg->DAC_CH1_CFG_EXT = (1 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_EN_Pos)
			| (1 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_MODE_Pos)
			| (1 << AUDCODEC_DAC_CH0_CFG_EXT_ZERO_ADJUST_EN_Pos)
			| (2 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_INTERVAL_Pos)
			| (0 << AUDCODEC_DAC_CH0_CFG_EXT_RAMP_STAT_Pos);

		reg->DAC_CH1_DEBUG = (0 << AUDCODEC_DAC_CH0_DEBUG_BYPASS_Pos)
			| (0xFF << AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Pos);

		break;
	default:
		break;
	}
}

static inline void close_analog_adc_path(sf32lb_codec_reg_t *reg)
{
	reg->ADC1_CFG2 &= ~AUDCODEC_ADC1_CFG2_EN;
	reg->ADC2_CFG2 &= ~AUDCODEC_ADC2_CFG2_EN;
	reg->ADC_ANA_CFG &= ~AUDCODEC_ADC_ANA_CFG_MICBIAS_EN;
}

static inline void close_analog_dac_path(sf32lb_codec_reg_t *reg)
{
	reg->DAC1_CFG |= AUDCODEC_DAC1_CFG_SR;
	reg->DAC2_CFG |= AUDCODEC_DAC2_CFG_SR;
	k_busy_wait(10);
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_EN_DAC;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_DAC;
	k_busy_wait(10);
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_EN_VCM;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_VCM;
	k_busy_wait(10);
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_EN_AMP;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_AMP;
	reg->DAC1_CFG &= ~AUDCODEC_DAC1_CFG_EN_OS_DAC;
	reg->DAC2_CFG &= ~AUDCODEC_DAC2_CFG_EN_OS_DAC;
}

static inline void clear_dac_channel(sf32lb_codec_reg_t *reg)
{
	reg->DAC_CH0_CFG &= (~AUDCODEC_DAC_CH0_CFG_ENABLE);
	reg->DAC_CH1_CFG &= (~AUDCODEC_DAC_CH1_CFG_ENABLE);
	reg->DAC_CFG |= AUDCODEC_DAC_CFG_PATH_RESET;
	reg->DAC_CFG &= ~AUDCODEC_DAC_CFG_PATH_RESET;
}

static inline void clear_adc_channel(sf32lb_codec_reg_t *reg)
{
	reg->ADC_CH0_CFG &= (~AUDCODEC_ADC_CH0_CFG_ENABLE);
	reg->ADC_CH1_CFG &= (~AUDCODEC_ADC_CH1_CFG_ENABLE);
	reg->ADC_CFG |= AUDCODEC_ADC_CFG_PATH_RESET;
	reg->ADC_CFG &= ~AUDCODEC_ADC_CFG_PATH_RESET;
}

static inline void disable_adc(sf32lb_codec_reg_t *reg)
{
	reg->CFG &= (~AUDCODEC_CFG_ADC_ENABLE);
}

static inline void disable_dac(sf32lb_codec_reg_t *reg)
{
	reg->CFG &= (~AUDCODEC_CFG_DAC_ENABLE);
}

static void config_dac_path_volume(sf32lb_codec_reg_t *reg, int channel, int volume)
{
	uint32_t rough_vol, fine_vol;
	int volume2;

	if (volume >= 0) {
		volume2 = volume;
	} else {
		volume2 = 0 - volume;
	}

	if (volume2 & 1) {
		fine_vol = 1;
	} else {
		fine_vol = 0;
	}

	volume2 = (volume2 >> 1);

	if (volume < 0) {
		volume = 0 - volume2;
		if (fine_vol) {
			volume--;
		}
	} else {
		volume = volume2;
	}

	if ((volume < -36) || (volume > 54) || (volume == 54 && fine_vol)) {
		return;
	}

	rough_vol = (volume + 36) / 6;
	fine_vol  = fine_vol + (((volume + 36) % 6) << 1);

	if (channel == 0) {
		MODIFY_REG(reg->DAC_CH0_CFG,
			AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk | AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
			MAKE_REG_VAL(rough_vol,
					AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk,
					AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Pos) |
			MAKE_REG_VAL(fine_vol,
					AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
					AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos));
	} else {
		MODIFY_REG(reg->DAC_CH1_CFG,
			AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk | AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
			MAKE_REG_VAL(rough_vol,
				AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk,
				AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Pos) |
			MAKE_REG_VAL(fine_vol,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos));
	}

	LOG_INF("set volume rough:%d, fine:%d, cfg0:0x%x", rough_vol, fine_vol, reg->DAC_CH0_CFG);
}

void mute_dac_path(sf32lb_codec_reg_t *reg, int mute)
{
	static int fine_vol_0, fine_vol_1;

	if (mute) {
		fine_vol_0 = GET_REG_VAL(reg->DAC_CH0_CFG,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos);

		fine_vol_1 = GET_REG_VAL(reg->DAC_CH1_CFG,
				AUDCODEC_DAC_CH1_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH1_CFG_FINE_VOL_Pos);

		MODIFY_REG(reg->DAC_CH0_CFG,
			AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
			MAKE_REG_VAL(0xF,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos));

		MODIFY_REG(reg->DAC_CH1_CFG,
			AUDCODEC_DAC_CH1_CFG_FINE_VOL_Msk,
			MAKE_REG_VAL(0xF,
				AUDCODEC_DAC_CH1_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH1_CFG_FINE_VOL_Pos));
	} else {
		MODIFY_REG(reg->DAC_CH0_CFG,
			AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
			MAKE_REG_VAL(fine_vol_0,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH0_CFG_FINE_VOL_Pos));

		MODIFY_REG(reg->DAC_CH1_CFG,
			AUDCODEC_DAC_CH1_CFG_FINE_VOL_Msk,
			MAKE_REG_VAL(fine_vol_1,
				AUDCODEC_DAC_CH1_CFG_FINE_VOL_Msk,
				AUDCODEC_DAC_CH1_CFG_FINE_VOL_Pos));
	}
}

static void config_rx_channel(sf32lb_codec_reg_t *reg, int channel, sf32lb_codec_adc_cfg_t *cfg)
{
	const sf32lb_codec_adc_clk_t *adc_clk = cfg->adc_clk;

	reg->ADC_CFG = (adc_clk->osr_sel << AUDCODEC_ADC_CFG_OSR_SEL_Pos)
		| (cfg->opmode << AUDCODEC_ADC_CFG_OP_MODE_Pos)
		| (0 << AUDCODEC_ADC_CFG_PATH_RESET_Pos)
		| (adc_clk->clk_src_sel << AUDCODEC_ADC_CFG_CLK_SRC_SEL_Pos)
		| (adc_clk->clk_div << AUDCODEC_ADC_CFG_CLK_DIV_Pos);

	switch (channel) {
	case 0:
		reg->ADC_CH0_CFG = (1 << AUDCODEC_ADC_CH0_CFG_ENABLE_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_HPF_BYPASS_Pos)
			| (0x7 << AUDCODEC_ADC_CH0_CFG_HPF_COEF_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_STB_INV_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_DMA_EN_Pos)
			| (0xc << AUDCODEC_ADC_CH0_CFG_ROUGH_VOL_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_FINE_VOL_Pos)
			| (1 << AUDCODEC_ADC_CH0_CFG_DATA_FORMAT_Pos);

		break;
	case 1:
		reg->ADC_CH1_CFG = (1 << AUDCODEC_ADC_CH0_CFG_ENABLE_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_HPF_BYPASS_Pos)
			| (0xf << AUDCODEC_ADC_CH0_CFG_HPF_COEF_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_STB_INV_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_DMA_EN_Pos)
			| (0xc << AUDCODEC_ADC_CH0_CFG_ROUGH_VOL_Pos)
			| (0 << AUDCODEC_ADC_CH0_CFG_FINE_VOL_Pos)
			| (1 << AUDCODEC_ADC_CH0_CFG_DATA_FORMAT_Pos);

		break;
	default:
		break;
	}
}

static inline void refgen_init(sf32lb_codec_reg_t *reg)
{
	reg->BG_CFG0 &= ~AUDCODEC_BG_CFG0_EN_SMPL;
	reg->REFGEN_CFG &= ~AUDCODEC_REFGEN_CFG_EN_CHOP;
	reg->REFGEN_CFG |= AUDCODEC_REFGEN_CFG_EN;
#if AVDD_V18_ENABLE
	reg->REFGEN_CFG |= AUDCODEC_REFGEN_CFG_LV_MODE;
#else
	reg->REFGEN_CFG &= ~AUDCODEC_REFGEN_CFG_LV_MODE;
#endif
	MODIFY_REG(reg->PLL_CFG5,
		AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Msk,
		MAKE_REG_VAL(1,
			AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Msk,
			AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Pos));

	MODIFY_REG(reg->PLL_CFG5,
		AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Msk,
		MAKE_REG_VAL(1,
			AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Msk,
			AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Pos));

	k_sleep(K_MSEC(2));
#if 1
	reg->BG_CFG0 &= ~AUDCODEC_BG_CFG0_EN_SMPL;
#else
	reg->BG_CFG0 |= AUDCODEC_BG_CFG0_EN_SMPL; /* has noise pop */
#endif
}

static void pll_turn_off(sf32lb_codec_reg_t *reg)
{
	/* turn off pll */
	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_EN_IARY;
	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_EN_VCO;
	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_EN_ANA;
	reg->PLL_CFG2 &= ~AUDCODEC_PLL_CFG2_EN_DIG;
	reg->PLL_CFG3 &= ~AUDCODEC_PLL_CFG3_EN_SDM;
	reg->PLL_CFG4 &= ~AUDCODEC_PLL_CFG4_EN_CLK_DIG;

	/* turn off refgen */
	reg->REFGEN_CFG &= ~AUDCODEC_REFGEN_CFG_EN;

	/* turn off bandgap */
	reg->BG_CFG1 = 0;
	reg->BG_CFG2 = 0;
	reg->BG_CFG0 &= ~AUDCODEC_BG_CFG0_EN;
	reg->BG_CFG0 &= ~AUDCODEC_BG_CFG0_EN_SMPL;
}

static void pll_turn_on(sf32lb_codec_reg_t *reg)
{
	/* turn on bandgap */
	reg->BG_CFG0 = (1 << AUDCODEC_BG_CFG0_EN_Pos)
		| (0 << AUDCODEC_BG_CFG0_LP_MODE_Pos)
#if AVDD_V18_ENABLE
		| (2 << AUDCODEC_BG_CFG0_VREF_SEL_Pos) /* 0xc: 3.3v  2:AVDD = 1.8V */
#else
		| (0xc << AUDCODEC_BG_CFG0_VREF_SEL_Pos) /* 0xc: 3.3v  2:AVDD = 1.8V */
#endif
		/* | (1 << AUDCODEC_BG_CFG0_EN_CHOP_Pos) */
		| (0 << AUDCODEC_BG_CFG0_EN_SMPL_Pos)
		| (1 << AUDCODEC_BG_CFG0_EN_RCFLT_Pos)
		| (4 << AUDCODEC_BG_CFG0_MIC_VREF_SEL_Pos)
		| (1 << AUDCODEC_BG_CFG0_EN_AMP_Pos)
		| (0 << AUDCODEC_BG_CFG0_SET_VC_Pos);

	/* turn on BG sample clock */

	/* avoid noise */
	reg->BG_CFG1 = 0; /* 48000 */
	reg->BG_CFG2 = 0; /* 48000000 */

	k_busy_wait(100);

	reg->PLL_CFG0 |= AUDCODEC_PLL_CFG0_EN_IARY;
	reg->PLL_CFG0 |= AUDCODEC_PLL_CFG0_EN_VCO;
	reg->PLL_CFG0 |= AUDCODEC_PLL_CFG0_EN_ANA;
	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_ICP_SEL_Msk;
	reg->PLL_CFG0 |= (8 << AUDCODEC_PLL_CFG0_ICP_SEL_Pos);
	reg->PLL_CFG2 |= AUDCODEC_PLL_CFG2_EN_DIG;
	reg->PLL_CFG3 |= AUDCODEC_PLL_CFG3_EN_SDM;
	reg->PLL_CFG4 |= AUDCODEC_PLL_CFG4_EN_CLK_DIG;
	reg->PLL_CFG1 = (3 << AUDCODEC_PLL_CFG1_R3_SEL_Pos)
		| (1 << AUDCODEC_PLL_CFG1_RZ_SEL_Pos)
		| (3 << AUDCODEC_PLL_CFG1_C2_SEL_Pos)
		| (6 << AUDCODEC_PLL_CFG1_CZ_SEL_Pos)
		| (0 << AUDCODEC_PLL_CFG1_CSD_RST_Pos)
		| (0 << AUDCODEC_PLL_CFG1_CSD_EN_Pos);

	k_busy_wait(50);

	refgen_init(reg);
}

/* type 0: 16k 1024 series  1:44.1k 1024 series 2:16k 1000 series 3: 44.1k 1000 series */
static int pll_update_freq(sf32lb_codec_reg_t *reg, uint8_t type)
{
	reg->PLL_CFG2 |= AUDCODEC_PLL_CFG2_RSTB;
	/* wait 50us */
	k_busy_wait(50);
	switch (type) {
	case 0:
		 /* set pll to 49.152M   [(fcw+3)+sdin/2^20]*6M */
		reg->PLL_CFG3 = (201327 << AUDCODEC_PLL_CFG3_SDIN_Pos) |
				(5 << AUDCODEC_PLL_CFG3_FCW_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
				(1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
				(0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
				(1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
		break;
	case 1:
		/* set pll to 45.1584M */
		reg->PLL_CFG3 = (551970 << AUDCODEC_PLL_CFG3_SDIN_Pos) |
				(4 << AUDCODEC_PLL_CFG3_FCW_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
				(1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
				(0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
				(1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
		break;
	case 2:
		/* set pll to 48M */
		reg->PLL_CFG3 = (0 << AUDCODEC_PLL_CFG3_SDIN_Pos) |
				(5 << AUDCODEC_PLL_CFG3_FCW_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
				(1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
				(0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
				(1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
		break;
	case 3:
		/* set pll to 44.1M */
		reg->PLL_CFG3 = (0x5999A << AUDCODEC_PLL_CFG3_SDIN_Pos) |
				(4 << AUDCODEC_PLL_CFG3_FCW_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
				(1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
				(0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
				(1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
				(0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
		break;
	default:
		__ASSERT(0, "");
		break;
	}
	reg->PLL_CFG3 |= AUDCODEC_PLL_CFG3_SDM_UPDATE;
	reg->PLL_CFG3 &= ~AUDCODEC_PLL_CFG3_SDMIN_BYPASS;
	reg->PLL_CFG2 &= ~AUDCODEC_PLL_CFG2_RSTB;

	/* wait 50us */
	k_busy_wait(50);
	reg->PLL_CFG2 |= AUDCODEC_PLL_CFG2_RSTB;
	/* check pll lock */
	k_busy_wait(50);

	reg->PLL_CFG1 |= AUDCODEC_PLL_CFG1_CSD_EN | AUDCODEC_PLL_CFG1_CSD_RST;
	k_busy_wait(50);
	reg->PLL_CFG1 &= ~AUDCODEC_PLL_CFG1_CSD_RST;

	int ret = 0;

	if (reg->PLL_STAT & AUDCODEC_PLL_STAT_UNLOCK_Msk) {
		LOG_INF("pll lock fail! freq_type:%d", type);
		ret = -1;
	} else {
		LOG_INF("pll lock! freq_type:%d", type);
		reg->PLL_CFG1 &= ~AUDCODEC_PLL_CFG1_CSD_EN;
	}
	return ret;
}

static void pll_calibration(sf32lb_codec_reg_t *reg)
{
	uint32_t pll_cnt;
	uint32_t xtal_cnt;
	uint32_t fc_vco;
	uint32_t fc_vco_min;
	uint32_t fc_vco_max;
	uint32_t delta_cnt = 0;
	uint32_t delta_cnt_min = 0;
	uint32_t delta_cnt_max = 0;
	uint32_t delta_fc_vco;
	uint32_t target_cnt;

	pll_turn_on(reg);

	/* VCO freq calibration */
	reg->PLL_CFG0 |= AUDCODEC_PLL_CFG0_OPEN;
	reg->PLL_CFG2 |= AUDCODEC_PLL_CFG2_EN_LF_VCIN;
	reg->PLL_CAL_CFG = (0 << AUDCODEC_PLL_CAL_CFG_EN_Pos)
				 | (2000 << AUDCODEC_PLL_CAL_CFG_LEN_Pos);

	for (uint8_t i = 0; i < ARRAY_SIZE(g_pll_vco_tab); i++) {
		target_cnt = g_pll_vco_tab[i].target_cnt;
		fc_vco = 16;
		delta_fc_vco = 8;
		/* setup calibration and run
		 * target pll_cnt = ceil(46MHz/48MHz*2000)+1 = 1918
		 * target difference between pll_cnt and
		 * xtal_cnt should be less than 1
		 */
		while (delta_fc_vco != 0) {
			reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
			reg->PLL_CFG0 |= (fc_vco << AUDCODEC_PLL_CFG0_FC_VCO_Pos);
			reg->PLL_CAL_CFG |= AUDCODEC_PLL_CAL_CFG_EN;

			while (!(reg->PLL_CAL_CFG & AUDCODEC_PLL_CAL_CFG_DONE_Msk)) {
				;
			}

			pll_cnt = (reg->PLL_CAL_RESULT	>> AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Pos);
			xtal_cnt = (reg->PLL_CAL_RESULT & AUDCODEC_PLL_CAL_RESULT_XTAL_CNT_Msk);
			reg->PLL_CAL_CFG &= ~AUDCODEC_PLL_CAL_CFG_EN;
			if (pll_cnt < target_cnt) {
				fc_vco = fc_vco + delta_fc_vco;
				delta_cnt = target_cnt - pll_cnt;
			} else if (pll_cnt > target_cnt) {
				fc_vco = fc_vco - delta_fc_vco;
				delta_cnt = pll_cnt - target_cnt;
			}
			delta_fc_vco = delta_fc_vco >> 1;
		}

		LOG_INF("call par CFG1(%x)", reg->PLL_CFG1);

		if (fc_vco == 0) {
			fc_vco_min = 0;
		} else {
			fc_vco_min = fc_vco - 1;
		}
		if (fc_vco == 31) {
			fc_vco_max = fc_vco;
		} else {
			fc_vco_max = fc_vco + 1;
		}

		reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
		reg->PLL_CFG0 |= (fc_vco_min << AUDCODEC_PLL_CFG0_FC_VCO_Pos);
		reg->PLL_CAL_CFG |= AUDCODEC_PLL_CAL_CFG_EN;

		LOG_INF("fc %d, xtal %d, pll %d", fc_vco, xtal_cnt, pll_cnt);

		while (!(reg->PLL_CAL_CFG & AUDCODEC_PLL_CAL_CFG_DONE_Msk)) {
			;
		}

		pll_cnt = (reg->PLL_CAL_RESULT >> AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Pos);
		reg->PLL_CAL_CFG &= ~AUDCODEC_PLL_CAL_CFG_EN;
		if (pll_cnt < target_cnt) {
			delta_cnt_min = target_cnt - pll_cnt;
		} else if (pll_cnt > target_cnt) {
			delta_cnt_min = pll_cnt - target_cnt;
		}

		reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
		reg->PLL_CFG0 |= (fc_vco_max << AUDCODEC_PLL_CFG0_FC_VCO_Pos);
		reg->PLL_CAL_CFG |= AUDCODEC_PLL_CAL_CFG_EN;

		while (!(reg->PLL_CAL_CFG & AUDCODEC_PLL_CAL_CFG_DONE_Msk)) {
			;
		}

		pll_cnt = (reg->PLL_CAL_RESULT >> AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Pos);
		reg->PLL_CAL_CFG &= ~AUDCODEC_PLL_CAL_CFG_EN;
		if (pll_cnt < target_cnt) {
			delta_cnt_max = target_cnt - pll_cnt;
		} else if (pll_cnt > target_cnt) {
			delta_cnt_max = pll_cnt - target_cnt;
		}

		if (delta_cnt_min <= delta_cnt && delta_cnt_min <= delta_cnt_max) {
			g_pll_vco_tab[i].vco_value = fc_vco_min;
		} else if (delta_cnt_max <= delta_cnt && delta_cnt_max <= delta_cnt_min) {
			g_pll_vco_tab[i].vco_value = fc_vco_max;
		} else {
			g_pll_vco_tab[i].vco_value = fc_vco;
		}
	}
	reg->PLL_CFG2 &= ~AUDCODEC_PLL_CFG2_EN_LF_VCIN;
	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_OPEN;

	pll_turn_off(reg);
}

/**
 * @brief  enable PLL function
 * @param freq - frequency
 * @param type - 0:1024 series, 1:1000 series
 * @return
 */
static int pll_enable(sf32lb_codec_reg_t *reg, uint32_t freq, uint8_t type)
{
	uint8_t freq_type;
	uint8_t test_result = -1;
	uint8_t vco_index = 0;

	LOG_DBG("enable pll");

	freq_type = type << 1;
	if ((freq == 44100) || (freq == 22050) || (freq == 11025)) {
		vco_index = 1;
		freq_type += 1;
	}

	pll_turn_on(reg);

	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
	reg->PLL_CFG0 |= (g_pll_vco_tab[vco_index].vco_value << AUDCODEC_PLL_CFG0_FC_VCO_Pos);

	LOG_INF("new PLL_ENABLE vco:%d, freq_type:%d",
			g_pll_vco_tab[vco_index].vco_value, freq_type);
	do {
		test_result = pll_update_freq(reg, freq_type);
	} while (test_result != 0);

	return test_result;
}

static int bf0_update_pll(sf32lb_codec_reg_t *reg, uint32_t freq, uint8_t type)
{
	uint8_t freq_type;
	uint8_t test_result = -1;
	uint8_t vco_index = 0;

	freq_type = type << 1;
	if ((freq == 44100) || (freq == 22050) || (freq == 11025)) {
		vco_index = 1;
		freq_type += 1;
	}

	reg->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
	reg->PLL_CFG0 |= (g_pll_vco_tab[vco_index].vco_value << AUDCODEC_PLL_CFG0_FC_VCO_Pos);

	LOG_INF("new PLL_ENABLE vco:%d, freq_type:%d",
			g_pll_vco_tab[vco_index].vco_value, freq_type);
	do {
		test_result = pll_update_freq(reg, freq_type);
	} while (test_result != 0);

	return test_result;
}

static void bf0_audio_pll_config(const struct sf32lb_codec_driver_config *cfg,
				struct sf32lb_audcodec_data *data,
				const sf32lb_codec_adc_clk_t *adc_clk,
				const sf32lb_codec_dac_clk_t *dac_clk,
				audio_dai_dir_t dir)
{
	if ((dir & AUDIO_DAI_DIR_TX)) {
		if ((dir & AUDIO_DAI_DIR_RX)) {
			__ASSERT(dac_clk->samplerate == adc_clk->samplerate, "");
		}
		if (dac_clk->clk_src_sel) { /* pll */
			if (data->pll_state == AUDIO_PLL_CLOSED) {
				pll_enable(cfg->reg, dac_clk->samplerate, 1);
				data->pll_state = AUDIO_PLL_ENABLE;
				data->pll_samplerate = dac_clk->samplerate;
			} else {
				bf0_update_pll(cfg->reg, dac_clk->samplerate, 1);
				data->pll_state = AUDIO_PLL_ENABLE;
				data->pll_samplerate = dac_clk->samplerate;
			}
		} else { /* xtal */
			if (data->pll_state == AUDIO_PLL_CLOSED) {
				pll_turn_on(cfg->reg);
				data->pll_state = AUDIO_PLL_OPEN;
			}
		}
	} else if ((dir & AUDIO_DAI_DIR_RX)) {
		if (adc_clk->clk_src_sel) { /* pll */
			if (data->pll_state == AUDIO_PLL_CLOSED) {
				pll_enable(cfg->reg, adc_clk->samplerate, 1);
				data->pll_state = AUDIO_PLL_ENABLE;
				data->pll_samplerate = adc_clk->samplerate;
			} else {
				bf0_update_pll(cfg->reg, adc_clk->samplerate, 1);
				data->pll_state = AUDIO_PLL_ENABLE;
				data->pll_samplerate = adc_clk->samplerate;
			}
		} else { /* xtal */
			if (data->pll_state == AUDIO_PLL_CLOSED) {
				pll_turn_on(cfg->reg);
				data->pll_state = AUDIO_PLL_OPEN;
			}
		}
	}

	LOG_DBG("pll config state:%d, samplerate:%d", data->pll_state, data->pll_samplerate);
}

static void sf32lb_codec_set_dac_volume(const struct device *dev, uint8_t volume)
{
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *cfg = dev->config;

	if (volume > 15) {
		volume = 15;
	}

	int gain = hardware_gain_of_volume[volume];

	if (gain > AUDCODEC_MAX_VOLUME) {
		gain = AUDCODEC_MAX_VOLUME;
	}
	if (gain < AUDCODEC_MIN_VOLUME) {
		gain = AUDCODEC_MIN_VOLUME;
	}

	config_dac_path_volume(cfg->reg, 0, gain * 2);
	/*config_dac_path_volume(reg, 1, gain * 2); */
	data->last_volume = volume;
}

static int codec_set_property(const struct device *dev,
		    audio_property_t property,
		    audio_channel_t channel,
		    audio_property_value_t val)
{
	int ret = 0;
	const struct sf32lb_codec_driver_config *cfg = dev->config;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		mute_dac_path(cfg->reg, val.mute);
		break;
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		sf32lb_codec_set_dac_volume(dev, (uint8_t)val.vol);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

int codec_apply_properties(const struct device *dev)
{
	return 0;
}

int codec_register_done_callback(const struct device *dev,
		audio_codec_tx_done_callback_t tx_cb,
		void *tx_cb_user_data,
		audio_codec_rx_done_callback_t rx_cb,
		void *rx_cb_user_data)
{
	struct sf32lb_audcodec_data *data = dev->data;

	data->tx_cb_user_data = tx_cb_user_data,
	data->rx_cb_user_data = rx_cb_user_data,
	data->tx_done = tx_cb;
	data->rx_done = rx_cb;

	return 0;
}
static int codec_write(const struct device *dev, uint8_t *pcm, uint32_t size)
{
	struct sf32lb_audcodec_data *data = dev->data;

	if (!pcm || !size) {
		return -EINVAL;
	}

	__ASSERT(data->tx_buf && data->tx_write_ptr, "write buf err");
	__ASSERT(size <= data->tx_half_dma_size, "tx size err");

	memcpy(data->tx_write_ptr, pcm, size);
	if (size < data->tx_half_dma_size) {
		memset(data->tx_write_ptr + size, 0, data->tx_half_dma_size - size);
	}
	sys_cache_data_flush_range((uint32_t *)data->tx_write_ptr, data->tx_half_dma_size);

	return size;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *sf32lb_cfg = dev->config;
	sf32lb_codec_hw_config_t *hw_cfg = &data->hw_config;

	if (cfg->dai_type != AUDIO_DAI_TYPE_PCM) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_PCM");
		return -EINVAL;
	}
	(void)sf32lb_clock_control_on_dt(&sf32lb_cfg->clock);

	sf32lb_codec_reg_t *reg = sf32lb_cfg->reg;
	struct pcm_config *pcm_cfg = &cfg->dai_cfg.pcm;

	if ((pcm_cfg->dir & AUDIO_DAI_DIR_TX)) {
		uint8_t i;

		for (i = 0; i < 9; i++) {
			if (pcm_cfg->samplerate == codec_dac_clk_config[i].samplerate) {
				hw_cfg->samplerate_index = i;
				hw_cfg->dac_cfg.dac_clk = &codec_dac_clk_config[i];
				break;
			}
		}
		__ASSERT(i < 9, "tx smprate error");

		data->tx_half_dma_size = pcm_cfg->block_size;
		if (!data->tx_buf) {
			data->tx_buf = malloc(data->tx_half_dma_size * 2);
		}

		if (!data->tx_buf) {
			__ASSERT(data->tx_buf, "tx mem");
			return -ENOMEM;
		}
		__ASSERT(((uint32_t)data->tx_buf & 0x03) == 0, "tx align 4");
		memset(data->tx_buf, 0, data->tx_half_dma_size * 2);
		sys_write32((uint32_t)data->tx_buf, (uintptr_t)&data->tx_write_ptr);

		hw_cfg->dac_cfg.opmode = 1; /* not work with audprc */
		config_tx_channel(reg, 0, &hw_cfg->dac_cfg);

		LOG_INF("tx samperate=%d", hw_cfg->dac_cfg.dac_clk->samplerate);
	}

	if ((pcm_cfg->dir & AUDIO_DAI_DIR_RX)) {
		uint8_t i;

		for (i = 0; i < 9; i++) {
			if (pcm_cfg->samplerate == codec_adc_clk_config[i].samplerate) {
				hw_cfg->samplerate_index = i;
				hw_cfg->adc_cfg.adc_clk = &codec_adc_clk_config[i];
				break;
			}
		}
		__ASSERT(i < 9, "rx smprate error");

		data->rx_half_dma_size = pcm_cfg->block_size;
		if (!data->rx_buf) {
			data->rx_buf = malloc(data->rx_half_dma_size * 2);
		}

		if (!data->rx_buf) {
			__ASSERT(data->rx_buf, "rx mem");
			return -ENOMEM;
		}
		__ASSERT(((uint32_t) data->rx_buf & 0x03) == 0, "rx align 4");
		memset(data->rx_buf, 0, data->rx_half_dma_size * 2);

		hw_cfg->adc_cfg.opmode = 1; /* not work with audprc */
		config_rx_channel(reg, 0, &hw_cfg->adc_cfg);

		LOG_INF("rx samperate=%d", hw_cfg->adc_cfg.adc_clk->samplerate);
	}

	return 0;
}

void dma_tx_callback(const struct device *dev_dma, void *user_data,
			       uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)user_data;
	struct sf32lb_audcodec_data *data = dev->data;

	if (status == DMA_STATUS_BLOCK) {
		/* half DMA finished, update poiter of DMA circle buffer for writing new data */
		sys_write32((uint32_t)data->tx_buf, (uintptr_t)&data->tx_write_ptr);

		if (data->tx_done) {
			data->tx_done(dev, data->tx_cb_user_data);
		}
	} else if (status == DMA_STATUS_COMPLETE) {

		sys_write32((uint32_t)data->tx_buf + data->tx_half_dma_size,
				(uintptr_t)&data->tx_write_ptr);

		if (data->tx_done) {
			data->tx_done(dev, data->tx_cb_user_data);
		}
	} else {
		LOG_ERR("dma err");
	}


}

void dma_rx_callback(const struct device *dev_dma, void *user_data,
			       uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)user_data;
	struct sf32lb_audcodec_data *data = dev->data;

	if (status == DMA_STATUS_COMPLETE) {
		sys_cache_data_invd_range((uint32_t *)(data->rx_buf + data->rx_half_dma_size),
					data->rx_half_dma_size);
		if (data->rx_done) {
			data->rx_done(dev, data->rx_buf + data->rx_half_dma_size,
					data->rx_half_dma_size, data->rx_cb_user_data);
		}
	} else if (status == DMA_STATUS_BLOCK) {
		sys_cache_data_invd_range((uint32_t *)data->rx_buf, data->rx_half_dma_size);
		if (data->rx_done) {
			data->rx_done(dev, data->rx_buf,
				data->rx_half_dma_size,
				data->rx_cb_user_data);
		}
	} else {
		LOG_ERR("dma err");
	}
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	uint32_t val, key;
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	sf32lb_codec_hw_config_t *hw_cfg = &data->hw_config;

	bool start_rx = (!data->rx_enable && (dir & AUDIO_DAI_DIR_RX));
	bool start_tx = (!data->tx_enable && (dir & AUDIO_DAI_DIR_TX));

	if (start_rx || start_tx) {
		bf0_audio_pll_config(cfg, data,
			&codec_adc_clk_config[hw_cfg->samplerate_index],
			&codec_dac_clk_config[hw_cfg->samplerate_index], dir);
	} else {
		LOG_ERR("start err");
		return -EIO;
	}

	if (start_rx) {
		LOG_INF("codec start rx, blk=%d", data->rx_half_dma_size);
		if (!data->rx_buf) {
			LOG_ERR("must configure before start rx");
			return -EIO;
		}
		sys_cache_data_invd_range((uint32_t *)data->rx_buf, data->rx_half_dma_size * 2);
		sf32lb_dma_reload_dt(&cfg->dma_rx, (uintptr_t)&cfg->reg->ADC_CH0_ENTRY,
			(uintptr_t)data->rx_buf, data->rx_half_dma_size * 2);

		(void)sf32lb_dma_start_dt(&cfg->dma_rx);

		key = irq_lock();

		val = cfg->reg->ADC_CH0_CFG;
		val &= ~AUDCODEC_ADC_CH0_CFG_DMA_EN_Msk;
		val |= AUDCODEC_ADC_CH0_CFG_DMA_EN;
		cfg->reg->ADC_CH0_CFG = val;

		irq_unlock(key);

		config_analog_adc_path(cfg->reg, hw_cfg->adc_cfg.adc_clk);
	}

	if (start_tx) {
		LOG_INF("codec start tx, blk=%d", data->tx_half_dma_size);
		if (!data->tx_buf) {
			LOG_ERR("must configure before start tx");
			return -EIO;
		}

		sys_cache_data_flush_range((uint32_t *)data->tx_buf, data->tx_half_dma_size * 2);

		sf32lb_dma_reload_dt(&cfg->dma_tx, (uintptr_t)data->tx_buf,
			(uintptr_t)&cfg->reg->DAC_CH0_ENTRY, data->tx_half_dma_size * 2);

		(void)sf32lb_dma_start_dt(&cfg->dma_tx);

		key = irq_lock();

		val = cfg->reg->DAC_CH0_CFG;
		val &= ~AUDCODEC_DAC_CH0_CFG_DMA_EN_Msk;
		val |= AUDCODEC_DAC_CH0_CFG_DMA_EN;
		cfg->reg->DAC_CH0_CFG = val;

		irq_unlock(key);

		config_dac_path(cfg->reg, 1);
		config_analog_dac_path(cfg->reg, hw_cfg->dac_cfg.dac_clk);
		config_dac_path(cfg->reg, 0);

	}

	/* speech echo cancellation algorithm requires a fixed delay time between ADC and DAC,
	 * enable at last.
	 */
	if (start_tx) {
		data->tx_enable = 1;
		cfg->reg->CFG |= AUDCODEC_CFG_DAC_ENABLE;
	}

	if (start_rx) {
		data->rx_enable = 1;
		cfg->reg->CFG |= AUDCODEC_CFG_ADC_ENABLE;
	}

	return 0;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	bool stop_rx = (data->rx_enable && (dir & AUDIO_DAI_DIR_RX));
	bool stop_tx = (data->tx_enable && (dir & AUDIO_DAI_DIR_TX));

	if (stop_tx) {
		LOG_DBG("stop tx");
		sf32lb_dma_stop_dt(&cfg->dma_tx);
		config_dac_path(cfg->reg, 1);
		close_analog_dac_path(cfg->reg);
		disable_dac(cfg->reg);
		clear_dac_channel(cfg->reg);
		if (data->tx_buf) {
			free(data->tx_buf);
			data->tx_buf = NULL;
		}
		data->tx_enable = 0;
	}

	if (stop_rx) {
		LOG_DBG("stop rx");
		sf32lb_dma_stop_dt(&cfg->dma_rx);
		disable_adc(cfg->reg);
		close_analog_adc_path(cfg->reg);
		clear_adc_channel(cfg->reg);

		if (data->rx_buf) {
			free(data->rx_buf);
			data->rx_buf = NULL;
		}
		data->rx_enable = 0;
	}

	if (stop_rx || stop_tx) {
		pll_turn_off(cfg->reg);
		data->pll_state = AUDIO_PLL_CLOSED;
		/* (void)sf32lb_clock_control_off_dt(&cfg->clock); */
	} else {
		LOG_DBG("stop err");
	}

	return 0;
}

static void codec_start_output(const struct device *dev)
{
	UNUSED(dev);
}

static void codec_stop_output(const struct device *dev)
{
	UNUSED(dev);
}

static int codec_driver_init(const struct device *dev)
{
	int ret = 0;
	struct dma_config config_dma = {0};
	struct dma_block_config block_cfg = {0};
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	struct sf32lb_audcodec_data *data = dev->data;
	sf32lb_codec_hw_config_t *hw_cfg = &data->hw_config;

	memset(data, 0, sizeof(struct sf32lb_audcodec_data));

	if (!sf32lb_dma_is_ready_dt(&cfg->dma_tx) || !sf32lb_dma_is_ready_dt(&cfg->dma_rx)) {
		return -ENODEV;
	}

	if (!sf32lb_clock_is_ready_dt(&cfg->clock)) {
		return -ENODEV;
	}

	/* set clock */
	hw_cfg->en_dly_sel = 0;
	hw_cfg->dac_cfg.opmode = 1;
	hw_cfg->adc_cfg.opmode = 1;

	HAL_PMU_EnableAudio(1);

	(void)sf32lb_clock_control_on_dt(&cfg->clock);

	sf32lb_dma_config_init_dt(&cfg->dma_tx, &config_dma);
	block_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	block_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	block_cfg.source_reload_en = 1;

	config_dma.head_block = &block_cfg;
	config_dma.block_count = 1U;
	config_dma.channel_direction = MEMORY_TO_PERIPHERAL;
	config_dma.source_data_size = 4U;
	config_dma.dest_data_size = 4U;
	config_dma.complete_callback_en = 1;
	config_dma.dma_callback = dma_tx_callback;
	config_dma.user_data = (void *)dev;

	ret = sf32lb_dma_config_dt(&cfg->dma_tx, &config_dma);
	if (ret < 0) {
		LOG_ERR("dma tx cfg err=%d", ret);
		return ret;
	}

	memset(&config_dma, 0, sizeof(config_dma));
	memset(&block_cfg, 0, sizeof(block_cfg));
	sf32lb_dma_config_init_dt(&cfg->dma_rx, &config_dma);

	block_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	block_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	block_cfg.dest_reload_en = 1;

	config_dma.head_block = &block_cfg;
	config_dma.block_count = 1U;
	config_dma.channel_direction = PERIPHERAL_TO_MEMORY;
	config_dma.source_data_size = 4U;
	config_dma.dest_data_size = 4U;
	config_dma.complete_callback_en = 1;
	config_dma.dma_callback = dma_rx_callback;
	config_dma.user_data = (void *)dev;

	ret = sf32lb_dma_config_dt(&cfg->dma_rx, &config_dma);
	if (ret < 0) {
		LOG_ERR("dma rx cfg err=%d", ret);
		return ret;
	}

	pll_calibration(cfg->reg);

	LOG_INF("%s done", __func__);

	return 0;
}

static const struct audio_codec_api codec_driver_api = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
	.start = codec_start,
	.stop = codec_stop,
	.write = codec_write,
	.register_done_callback = codec_register_done_callback,
};

#define SF32LB_AUDIO_CODEC_DEFINE(n)                                           \
	static const struct sf32lb_codec_driver_config config##n = {           \
		.reg = (sf32lb_codec_reg_t *)DT_INST_REG_ADDR_BY_IDX(n, 0),    \
		.dma_tx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, tx),          \
		.dma_rx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, rx),          \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                     \
	};                                                                     \
                                                                               \
	static struct sf32lb_audcodec_data data##n = {                         \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, codec_driver_init, NULL, &data##n,            \
			&config##n,                                            \
			POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY,         \
			&codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_AUDIO_CODEC_DEFINE)
