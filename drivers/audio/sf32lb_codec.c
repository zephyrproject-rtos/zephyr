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

#include <register.h>

LOG_MODULE_REGISTER(sifli_codec, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define PMUC_HXT_CR1 offsetof(PMUC_TypeDef, HXT_CR1)

#define CODEC_ID              offsetof(AUDCODEC_TypeDef, ID)
#define CODEC_CFG             offsetof(AUDCODEC_TypeDef, CFG)
#define CODEC_IRQ             offsetof(AUDCODEC_TypeDef, IRQ)
#define CODEC_IRQ_MSK         offsetof(AUDCODEC_TypeDef, IRQ_MSK)
#define CODEC_DAC_CFG         offsetof(AUDCODEC_TypeDef, DAC_CFG)
#define CODEC_ADC_CFG         offsetof(AUDCODEC_TypeDef, ADC_CFG)
#define CODEC_APB_STAT        offsetof(AUDCODEC_TypeDef, APB_STAT)
#define CODEC_RSVD5           offsetof(AUDCODEC_TypeDef, RSVD5)
#define CODEC_ADC_CH0_CFG     offsetof(AUDCODEC_TypeDef, ADC_CH0_CFG)
#define CODEC_ADC_CH1_CFG     offsetof(AUDCODEC_TypeDef, ADC_CH1_CFG)
#define CODEC_DAC_CH0_CFG     offsetof(AUDCODEC_TypeDef, DAC_CH0_CFG)
#define CODEC_DAC_CH0_CFG_EXT offsetof(AUDCODEC_TypeDef, DAC_CH0_CFG_EXT)
#define CODEC_DAC_CH1_CFG     offsetof(AUDCODEC_TypeDef, DAC_CH1_CFG)
#define CODEC_DAC_CH1_CFG_EXT offsetof(AUDCODEC_TypeDef, DAC_CH1_CFG_EXT)
#define CODEC_ADC_CH0_ENTRY   offsetof(AUDCODEC_TypeDef, ADC_CH0_ENTRY)
#define CODEC_ADC_CH1_ENTRY   offsetof(AUDCODEC_TypeDef, ADC_CH1_ENTRY)
#define CODEC_DAC_CH0_ENTRY   offsetof(AUDCODEC_TypeDef, DAC_CH0_ENTRY)
#define CODEC_DAC_CH1_ENTRY   offsetof(AUDCODEC_TypeDef, DAC_CH1_ENTRY)
#define CODEC_DAC_CH0_DEBUG   offsetof(AUDCODEC_TypeDef, DAC_CH0_DEBUG)
#define CODEC_DAC_CH1_DEBUG   offsetof(AUDCODEC_TypeDef, DAC_CH1_DEBUG)
#define CODEC_DAC_CH0_DC      offsetof(AUDCODEC_TypeDef, DAC_CH0_DC)
#define CODEC_DAC_CH1_DC      offsetof(AUDCODEC_TypeDef, DAC_CH1_DC)
#define CODEC_COMMON_CFG      offsetof(AUDCODEC_TypeDef, COMMON_CFG)
#define CODEC_BG_CFG0         offsetof(AUDCODEC_TypeDef, BG_CFG0)
#define CODEC_BG_CFG1         offsetof(AUDCODEC_TypeDef, BG_CFG1)
#define CODEC_BG_CFG2         offsetof(AUDCODEC_TypeDef, BG_CFG2)
#define CODEC_REFGEN_CFG      offsetof(AUDCODEC_TypeDef, REFGEN_CFG)
#define CODEC_PLL_CFG0        offsetof(AUDCODEC_TypeDef, PLL_CFG0)
#define CODEC_PLL_CFG1        offsetof(AUDCODEC_TypeDef, PLL_CFG1)
#define CODEC_PLL_CFG2        offsetof(AUDCODEC_TypeDef, PLL_CFG2)
#define CODEC_PLL_CFG3        offsetof(AUDCODEC_TypeDef, PLL_CFG3)
#define CODEC_PLL_CFG4        offsetof(AUDCODEC_TypeDef, PLL_CFG4)
#define CODEC_PLL_CFG5        offsetof(AUDCODEC_TypeDef, PLL_CFG5)
#define CODEC_PLL_CFG6        offsetof(AUDCODEC_TypeDef, PLL_CFG6)
#define CODEC_PLL_STAT        offsetof(AUDCODEC_TypeDef, PLL_STAT)
#define CODEC_PLL_CAL_CFG     offsetof(AUDCODEC_TypeDef, PLL_CAL_CFG)
#define CODEC_PLL_CAL_RESULT  offsetof(AUDCODEC_TypeDef, PLL_CAL_RESULT)
#define CODEC_ADC_ANA_CFG     offsetof(AUDCODEC_TypeDef, ADC_ANA_CFG)
#define CODEC_ADC1_CFG1       offsetof(AUDCODEC_TypeDef, ADC1_CFG1)
#define CODEC_ADC1_CFG2       offsetof(AUDCODEC_TypeDef, ADC1_CFG2)
#define CODEC_ADC2_CFG1       offsetof(AUDCODEC_TypeDef, ADC2_CFG1)
#define CODEC_ADC2_CFG2       offsetof(AUDCODEC_TypeDef, ADC2_CFG2)
#define CODEC_DAC1_CFG        offsetof(AUDCODEC_TypeDef, DAC1_CFG)
#define CODEC_DAC2_CFG        offsetof(AUDCODEC_TypeDef, DAC2_CFG)

#define CODEC_CLK_USING_PLL 0
#define AUDCODEC_MIN_VOLUME -36
#define AUDCODEC_MAX_VOLUME 54

/** some register read/write need delay some time, based on the chip's IP manual.
 *  no exact time, generally, it asks for at least how much.
 */
#define WAIT_PLL_STABLE_US        100
#define WAIT_VCM_STABLE_US        5
#define WAIT_AMP_STABLE_US        1
#define WAIT_DAC_STABLE_US        10
#define WAIT_MICBIAS_STABLE_US    2000
#define WAIT_RESET_LOW_TO_HIGH_US 1000

/* hardware gain of volume, The maximum gain should be tested
 * to prevent the speaker from burning out.
 */
#define VOLUME_0_GAIN  -55
#define VOLUME_1_GAIN  -34
#define VOLUME_2_GAIN  -32
#define VOLUME_3_GAIN  -30
#define VOLUME_4_GAIN  -28
#define VOLUME_5_GAIN  -26
#define VOLUME_6_GAIN  -24
#define VOLUME_7_GAIN  -22
#define VOLUME_8_GAIN  -20
#define VOLUME_9_GAIN  -17
#define VOLUME_10_GAIN -14
#define VOLUME_11_GAIN -11
#define VOLUME_12_GAIN -10
#define VOLUME_13_GAIN -8
#define VOLUME_14_GAIN -6
#define VOLUME_15_GAIN -2

/** Wait time in us before codec state become stable */
#define CODEC_STABLE_WAIT_US 10

enum audio_pll_state {
	AUDIO_PLL_CLOSED,
	AUDIO_PLL_OPEN,
	AUDIO_PLL_ENABLE,
};

struct sf32lb_codec_dac_clk {
	uint32_t samplerate;
	uint8_t clk_src_sel; /* 0:xtal 48M 1:PLL 44.1M */
	uint8_t clk_div;
	uint8_t osr_sel; /* 0:100 1:150 2:300 4:64 5:128 6:256 */
	uint16_t sinc_gain;
	uint8_t sel_clk_dac_source; /* 0:xtal 48M 1:PLL */
	uint8_t diva_clk_dac;
	uint8_t diva_clk_chop_dac;
	uint8_t divb_clk_chop_dac;
	uint8_t diva_clk_chop_bg;
	uint8_t diva_clk_chop_refgen;
	uint8_t sel_clk_dac;
};

struct sf32lb_codec_adc_clk {
	uint32_t samplerate;
	uint8_t clk_src_sel; /*0:xtal 48M 1:PLL 44.1M */
	uint8_t clk_div;
	uint8_t osr_sel;            /* 0:200 1:300 2:400 3:600 */
	uint8_t sel_clk_adc_source; /* 0:xtal 48M 1:PLL  */
	uint8_t sel_clk_adc;
	uint8_t diva_clk_adc; /* lp pll_cfg6 */
	uint8_t fsp;
};

struct sf32lb_codec_dac_cfg {
	uint8_t opmode; /* 0:audprc tx to audcode; 1: mem tx to audcodec */
	const struct sf32lb_codec_dac_clk *dac_clk;
};

struct sf32lb_codec_adc_cfg {
	uint8_t opmode;
	const struct sf32lb_codec_adc_clk *adc_clk;
};

struct sf32lb_codec_hw_config {
	uint16_t en_dly_sel; /* codec enable delay count */
	uint8_t samplerate_index;
	struct sf32lb_codec_dac_cfg dac_cfg; /* dac AUDCODEC DAC PATH configure */
	struct sf32lb_codec_adc_cfg adc_cfg; /* dac AUDCODEC ADC PATH configure */
};

struct sf32lb_audcodec_data {
	struct k_spinlock lock;
	struct sf32lb_codec_hw_config hw_config;
	audio_codec_tx_done_callback_t tx_done;
	audio_codec_rx_done_callback_t rx_done;
	void *tx_cb_user_data;
	void *rx_cb_user_data;
	uint8_t *tx_buf;
	uint8_t *tx_write_ptr;
	uint8_t *rx_buf;
	uint32_t tx_half_dma_size;
	uint32_t rx_half_dma_size;
	uint8_t tx_enable;
	uint8_t rx_enable;
	uint8_t last_volume;
	enum audio_pll_state pll_state;
	uint32_t pll_samplerate;
	int fine_vol_0;
	const struct device *dev;
};

struct sf32lb_codec_driver_config {
	uintptr_t reg;
	struct sf32lb_dma_dt_spec dma_tx;
	struct sf32lb_dma_dt_spec dma_rx;
	struct sf32lb_clock_dt_spec clock;
	struct gpio_dt_spec pa_power_dt;
};

static const int hardware_gain_of_volume[16] = {
	VOLUME_0_GAIN,  VOLUME_1_GAIN,  VOLUME_2_GAIN,  VOLUME_3_GAIN,
	VOLUME_4_GAIN,  VOLUME_5_GAIN,  VOLUME_6_GAIN,  VOLUME_7_GAIN,
	VOLUME_8_GAIN,  VOLUME_9_GAIN,  VOLUME_10_GAIN, VOLUME_11_GAIN,
	VOLUME_12_GAIN, VOLUME_13_GAIN, VOLUME_14_GAIN, VOLUME_15_GAIN};

/* DAC digital sinc filter gain compensation factor.
 * Value 0x14D (333 decimal) comes from the AUDCODEC IP reference
 * configuration. Do not modify without revalidating audio levels
 * against the codec datasheet / characterization results.
 */
#define SINC_GAIN 0x14D

static const struct sf32lb_codec_dac_clk codec_dac_clk_config[9] = {
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

const struct sf32lb_codec_adc_clk codec_adc_clk_config[9] = {
#if CODEC_CLK_USING_PLL
	{48000, 1, 5, 0, 1, 1, 5, 0},  {32000, 1, 5, 1, 1, 1, 5, 0},  {24000, 1, 10, 0, 1, 0, 5, 2},
	{16000, 1, 10, 1, 1, 0, 5, 2}, {12000, 1, 10, 2, 1, 0, 5, 2}, {8000, 1, 10, 3, 1, 0, 5, 2},
#else
	{48000, 0, 5, 0, 0, 1, 5, 0},  {32000, 0, 5, 1, 0, 1, 5, 0},  {24000, 0, 10, 0, 0, 0, 5, 2},
	{16000, 0, 10, 1, 0, 0, 5, 2}, {12000, 0, 10, 2, 0, 0, 5, 2}, {8000, 0, 10, 3, 0, 0, 5, 2},
#endif
	{44100, 1, 5, 0, 1, 1, 5, 1},  {22050, 1, 5, 2, 1, 1, 5, 1},  {11025, 1, 10, 2, 1, 0, 5, 3},
};

struct pll_vco {
	uint32_t freq;
	uint32_t vco_value;
	uint32_t target_cnt;
};

struct pll_vco g_pll_vco_tab[2] = {
	{48, 0, 2001},
	{44, 0, 1834},
};

static void pmu_enable_audio(int enable)
{
	if (enable) {
		sys_set_bit(PMUC_BASE + PMUC_HXT_CR1, PMUC_HXT_CR1_BUF_AUD_EN_Pos);
	} else {
		sys_clear_bit(PMUC_BASE + PMUC_HXT_CR1, PMUC_HXT_CR1_BUF_AUD_EN_Pos);
	}
}

static int config_dac_path(uintptr_t reg, uint16_t bypass)
{
	uint32_t reg_val;

	if (bypass) {
		sys_set_bit(reg + CODEC_DAC_CH0_CFG, AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Pos);
		reg_val = FIELD_PREP(AUDCODEC_DAC_CH0_DEBUG_BYPASS_Msk, 1) |
			  FIELD_PREP(AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Msk, 0xFF);
		sys_write32(reg_val, reg + CODEC_DAC_CH0_DEBUG);

		sys_set_bit(reg + CODEC_DAC_CH1_CFG, AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Pos);
		reg_val = FIELD_PREP(AUDCODEC_DAC_CH1_DEBUG_BYPASS_Msk, 1) |
			  FIELD_PREP(AUDCODEC_DAC_CH1_DEBUG_DATA_OUT_Msk, 0xFF);
		sys_write32(reg_val, reg + CODEC_DAC_CH1_DEBUG);
	} else {
		sys_clear_bit(reg + CODEC_DAC_CH0_CFG, AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Pos);
		reg_val = FIELD_PREP(AUDCODEC_DAC_CH0_DEBUG_BYPASS_Msk, 0) |
			  FIELD_PREP(AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Msk, 0xFF);
		sys_write32(reg_val, reg + CODEC_DAC_CH0_DEBUG);

		sys_clear_bit(reg + CODEC_DAC_CH1_CFG, AUDCODEC_DAC_CH1_CFG_DOUT_MUTE_Pos);
		reg_val = FIELD_PREP(AUDCODEC_DAC_CH1_DEBUG_BYPASS_Msk, 0) |
			  FIELD_PREP(AUDCODEC_DAC_CH1_DEBUG_DATA_OUT_Msk, 0xFF);
		sys_write32(reg_val, reg + CODEC_DAC_CH1_DEBUG);
	}

	return 0;
}

static void config_analog_dac_path(uintptr_t reg, const struct sf32lb_codec_dac_clk *clk)
{
	uint32_t reg_val;

	sys_clear_bits(reg + CODEC_PLL_CFG4, AUDCODEC_PLL_CFG4_SEL_CLK_DAC_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG4, AUDCODEC_PLL_CFG4_SEL_CLK_DAC_SOURCE_Msk);
	reg_val = sys_read32(reg + CODEC_PLL_CFG4);
	reg_val &= ~(AUDCODEC_PLL_CFG4_EN_CLK_CHOP_DAC_Msk | AUDCODEC_PLL_CFG4_EN_CLK_DAC_Msk |
		     AUDCODEC_PLL_CFG4_SEL_CLK_DAC_SOURCE_Msk | AUDCODEC_PLL_CFG4_SEL_CLK_DAC_Msk |
		     AUDCODEC_PLL_CFG4_EN_CLK_DIG_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG4_EN_CLK_CHOP_DAC_Msk, 1) |
		   FIELD_PREP(AUDCODEC_PLL_CFG4_EN_CLK_DAC_Msk, 1) |
		   FIELD_PREP(AUDCODEC_PLL_CFG4_SEL_CLK_DAC_SOURCE_Msk, clk->sel_clk_dac_source) |
		   FIELD_PREP(AUDCODEC_PLL_CFG4_SEL_CLK_DAC_Msk, clk->sel_clk_dac) |
		   FIELD_PREP(AUDCODEC_PLL_CFG4_EN_CLK_DIG_Msk, 1);
	sys_write32(reg_val, reg + CODEC_PLL_CFG4);

	reg_val = sys_read32(reg + CODEC_PLL_CFG5);
	reg_val &=
		~(AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Msk | AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Msk, 1) |
		   FIELD_PREP(AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Msk, 1);
	sys_write32(reg_val, reg + CODEC_PLL_CFG5);

	sys_clear_bits(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Msk);

	/* wait for pll stable */
	k_busy_wait(WAIT_PLL_STABLE_US);

	sys_set_bit(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Pos);
	sys_clear_bit(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_LP_MODE_Pos);
	sys_clear_bits(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_OS_DAC_Msk);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_OS_DAC_Msk);
	sys_set_bit(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_VCM_Pos);
	sys_clear_bit(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_VCM_Pos);
	/* wait for vcm stable */
	k_busy_wait(WAIT_VCM_STABLE_US);

	sys_set_bit(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_AMP_Pos);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_AMP_Msk);
	/* wait amp stable*/
	k_busy_wait(WAIT_AMP_STABLE_US);

	sys_set_bit(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_OS_DAC_Pos);
	sys_set_bit(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_OS_DAC_Pos);
	k_busy_wait(WAIT_DAC_STABLE_US);
	sys_set_bit(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_DAC_Pos);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_DAC_Msk);
	k_busy_wait(WAIT_DAC_STABLE_US);
	sys_clear_bits(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_SR_Msk);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_SR_Msk);
}

static void config_analog_adc_path(uintptr_t reg, const struct sf32lb_codec_adc_clk *clk)
{
	uint32_t reg_val;

	sys_clear_bits(reg + CODEC_BG_CFG0, AUDCODEC_BG_CFG0_EN_SMPL_Msk);
	sys_set_bit(reg + CODEC_ADC_ANA_CFG, AUDCODEC_ADC_ANA_CFG_MICBIAS_EN_Pos);
	sys_clear_bits(reg + CODEC_ADC_ANA_CFG, AUDCODEC_ADC_ANA_CFG_MICBIAS_CHOP_EN_Msk);
	/* delay 2ms*/
	k_busy_wait(WAIT_MICBIAS_STABLE_US);

	sys_clear_bits(reg + CODEC_BG_CFG0, AUDCODEC_BG_CFG0_EN_SMPL_Msk); /* noise pop */

	/* adc1 and adc2 clock */
	reg_val = FIELD_PREP(AUDCODEC_PLL_CFG6_SEL_TST_CLK_Msk, 0) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_EN_TST_CLK_Msk, 0) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_EN_CLK_RCCAL_Msk, 0) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_SEL_CLK_CHOP_MICBIAS_Msk, 3) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_EN_CLK_CHOP_MICBIAS_Msk, 1) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_SEL_CLK_ADC2_Msk, clk->sel_clk_adc) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_DIVA_CLK_ADC2_Msk, clk->diva_clk_adc) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_EN_CLK_ADC2_Msk, 1) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_SEL_CLK_ADC1_Msk, clk->sel_clk_adc) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_DIVA_CLK_ADC1_Msk, clk->diva_clk_adc) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_EN_CLK_ADC1_Msk, 1) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_SEL_CLK_ADC0_Msk, 1) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_DIVA_CLK_ADC0_Msk, 5) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_EN_CLK_ADC0_Msk, 1) |
		  FIELD_PREP(AUDCODEC_PLL_CFG6_SEL_CLK_ADC_SOURCE_Msk, clk->sel_clk_adc_source);
	sys_write32(reg_val, reg + CODEC_PLL_CFG6);

	sys_clear_bits(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Msk);

	k_busy_wait(WAIT_RESET_LOW_TO_HIGH_US);

	sys_set_bit(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Pos);

	sys_clear_bit(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_DIFF_EN_Pos);

	sys_clear_bit(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_DACN_EN_Pos);

	sys_clear_bit(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_FSP_Pos);
	reg_val = sys_read32(reg + CODEC_ADC1_CFG1);
	reg_val &= ~(AUDCODEC_ADC1_CFG1_FSP_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_ADC1_CFG1_FSP_Msk, clk->fsp);
	sys_write32(reg_val, reg + CODEC_ADC1_CFG1);

	/* this make long mic startup pulse */
	sys_set_bit(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_VCMST_Pos);
	sys_set_bit(reg + CODEC_ADC1_CFG2, AUDCODEC_ADC1_CFG2_CLEAR_Pos);

	sys_clear_bits(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_GC_Msk);
	reg_val = sys_read32(reg + CODEC_ADC1_CFG1);
	reg_val &= ~(AUDCODEC_ADC1_CFG1_GC_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_ADC1_CFG1_GC_Msk, 0x4);
	sys_write32(reg_val, reg + CODEC_ADC1_CFG1);

	sys_set_bit(reg + CODEC_ADC1_CFG2, AUDCODEC_ADC1_CFG2_EN_Pos);
	sys_clear_bits(reg + CODEC_ADC1_CFG2, AUDCODEC_ADC1_CFG2_RSTB_Msk);

	sys_clear_bits(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_VREF_SEL_Msk);
	reg_val = sys_read32(reg + CODEC_ADC1_CFG1);
	reg_val &= ~(AUDCODEC_ADC1_CFG1_VREF_SEL_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_ADC1_CFG1_VREF_SEL_Msk, 2);
	sys_write32(reg_val, reg + CODEC_ADC1_CFG1);

	/* wait 20ms */
	k_sleep(K_MSEC(20));

	sys_set_bit(reg + CODEC_ADC1_CFG2, AUDCODEC_ADC1_CFG2_RSTB_Pos);
	sys_clear_bits(reg + CODEC_ADC1_CFG1, AUDCODEC_ADC1_CFG1_VCMST_Msk);
	sys_clear_bits(reg + CODEC_ADC1_CFG2, AUDCODEC_ADC1_CFG2_CLEAR_Msk);
}

static void config_tx_channel(uintptr_t reg, struct sf32lb_codec_dac_cfg *cfg)
{
	uint32_t reg_val;
	const struct sf32lb_codec_dac_clk *dac_clk = cfg->dac_clk;

	reg_val = sys_read32(reg + CODEC_CFG);
	reg_val &= ~AUDCODEC_CFG_ADC_EN_DLY_SEL_Msk;
	reg_val |= FIELD_PREP(AUDCODEC_CFG_ADC_EN_DLY_SEL_Msk, 3);
	sys_write32(reg_val, reg + CODEC_CFG);

	reg_val = FIELD_PREP(AUDCODEC_DAC_CFG_OSR_SEL_Msk, dac_clk->osr_sel) |
		  FIELD_PREP(AUDCODEC_DAC_CFG_OP_MODE_Msk, cfg->opmode) |
		  FIELD_PREP(AUDCODEC_DAC_CFG_PATH_RESET_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CFG_CLK_SRC_SEL_Msk, dac_clk->clk_src_sel) |
		  FIELD_PREP(AUDCODEC_DAC_CFG_CLK_DIV_Msk, dac_clk->clk_div);
	sys_write32(reg_val, reg + CODEC_DAC_CFG);

	reg_val = FIELD_PREP(AUDCODEC_DAC_CH0_CFG_ENABLE_Msk, 1) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_DOUT_MUTE_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_DEM_MODE_Msk, 2) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_DMA_EN_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk, 6) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_DATA_FORMAT_Msk, 1) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_SINC_GAIN_Msk, dac_clk->sinc_gain) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_DITHER_GAIN_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_DITHER_EN_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_CLK_ANA_POL_Msk, 0);
	sys_write32(reg_val, reg + CODEC_DAC_CH0_CFG);

	reg_val = FIELD_PREP(AUDCODEC_DAC_CH0_CFG_EXT_RAMP_EN_Msk, 1) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_EXT_RAMP_MODE_Msk, 1) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_EXT_ZERO_ADJUST_EN_Msk, 1) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_EXT_RAMP_INTERVAL_Msk, 2) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_CFG_EXT_RAMP_STAT_Msk, 0);

	sys_write32(reg_val, reg + CODEC_DAC_CH0_CFG_EXT);

	reg_val = FIELD_PREP(AUDCODEC_DAC_CH0_DEBUG_BYPASS_Msk, 0) |
		  FIELD_PREP(AUDCODEC_DAC_CH0_DEBUG_DATA_OUT_Msk, 0xFF);
	sys_write32(reg_val, reg + CODEC_DAC_CH0_DEBUG);
}

static inline void close_analog_adc_path(uintptr_t reg)
{
	sys_clear_bits(reg + CODEC_ADC1_CFG2, AUDCODEC_ADC1_CFG2_EN_Msk);
	sys_clear_bits(reg + CODEC_ADC2_CFG2, AUDCODEC_ADC2_CFG2_EN_Msk);
	sys_clear_bits(reg + CODEC_ADC_ANA_CFG, AUDCODEC_ADC_ANA_CFG_MICBIAS_EN_Msk);
}

static inline void close_analog_dac_path(uintptr_t reg)
{
	sys_set_bit(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_SR_Pos);
	sys_set_bit(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_SR_Pos);
	/*wait SR clear stable*/
	k_busy_wait(CODEC_STABLE_WAIT_US);
	sys_clear_bits(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_DAC_Msk);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_DAC_Msk);
	/*wait DAC clear stable*/
	k_busy_wait(CODEC_STABLE_WAIT_US);
	sys_clear_bits(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_VCM_Msk);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_VCM_Msk);
	/*wait AMP clear stable*/
	k_busy_wait(CODEC_STABLE_WAIT_US);
	sys_clear_bits(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_AMP_Msk);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_AMP_Msk);
	sys_clear_bits(reg + CODEC_DAC1_CFG, AUDCODEC_DAC1_CFG_EN_OS_DAC_Msk);
	sys_clear_bits(reg + CODEC_DAC2_CFG, AUDCODEC_DAC2_CFG_EN_OS_DAC_Msk);
}

static inline void clear_dac_channel(uintptr_t reg)
{
	sys_clear_bits(reg + CODEC_DAC_CH0_CFG, AUDCODEC_DAC_CH0_CFG_ENABLE_Msk);
	sys_clear_bits(reg + CODEC_DAC_CH1_CFG, AUDCODEC_DAC_CH1_CFG_ENABLE_Msk);
	sys_set_bit(reg + CODEC_DAC_CFG, AUDCODEC_DAC_CFG_PATH_RESET_Pos);
	sys_clear_bits(reg + CODEC_DAC_CFG, AUDCODEC_DAC_CFG_PATH_RESET_Msk);
}

static inline void clear_adc_channel(uintptr_t reg)
{
	sys_clear_bits(reg + CODEC_ADC_CH0_CFG, AUDCODEC_ADC_CH0_CFG_ENABLE_Msk);
	sys_clear_bits(reg + CODEC_ADC_CH1_CFG, AUDCODEC_ADC_CH1_CFG_ENABLE_Msk);

	sys_set_bit(reg + CODEC_ADC_CFG, AUDCODEC_ADC_CFG_PATH_RESET_Pos);
	sys_clear_bits(reg + CODEC_ADC_CFG, AUDCODEC_ADC_CFG_PATH_RESET_Msk);
}

static inline void disable_adc(uintptr_t reg)
{
	sys_clear_bit(reg + CODEC_CFG, AUDCODEC_CFG_ADC_ENABLE_Pos);
}

static inline void disable_dac(uintptr_t reg)
{
	sys_clear_bit(reg + CODEC_CFG, AUDCODEC_CFG_DAC_ENABLE_Pos);
}

static void config_dac_path_volume(uintptr_t reg, int volume)
{
	uint32_t reg_val;
	uint32_t rough_vol, fine_vol;

	/** parameter volume is 1db unit
	 *   datasheet of audio codec as following:
	 *   dac fine volume control range from 0db to 6db, step is 0.5db.
	 *   0x0   0db
	 *   0x1   0.5db
	 *   0x2   1db
	 *   ......
	 *   0xb   5.5db
	 *   0xc   mute
	 *   -----------------------------------------------------------
	 *   dac rough volume contrl range from -36db to 54db, step is 6db.
	 *   0x0   -36db
	 *   0x1   -30db
	 *    ....
	 *   0x6     0db
	 *   0x7     6db
	 *   ....
	 *   0xe    48db
	 *   0xf    54db
	 */
	rough_vol = (volume - AUDCODEC_MIN_VOLUME) / 6;
	fine_vol = (((volume - AUDCODEC_MIN_VOLUME) % 6) << 1);

	reg_val = sys_read32(reg + CODEC_DAC_CH0_CFG);
	reg_val &= ~(AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk | AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_DAC_CH0_CFG_ROUGH_VOL_Msk, rough_vol) |
		   FIELD_PREP(AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk, fine_vol);
	sys_write32(reg_val, reg + CODEC_DAC_CH0_CFG);

	LOG_DBG("set volume rough:%d, fine:%d, cfg0:0x%x", rough_vol, fine_vol,
		sys_read32(reg + CODEC_DAC_CH0_CFG));
}

static void mute_dac_path(const struct device *dev, uintptr_t reg, int mute)
{
	uint32_t reg_val;
	struct sf32lb_audcodec_data *data = dev->data;

	if (mute) {
		reg_val = sys_read32(reg + CODEC_DAC_CH0_CFG);
		data->fine_vol_0 = FIELD_GET(AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk, reg_val);
		reg_val &= ~AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk;
		reg_val |= FIELD_PREP(AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk, 0xF);
		sys_write32(reg_val, reg + CODEC_DAC_CH0_CFG);
	} else {
		reg_val = sys_read32(reg + CODEC_DAC_CH0_CFG);
		reg_val &= ~AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk;
		reg_val |= FIELD_PREP(AUDCODEC_DAC_CH0_CFG_FINE_VOL_Msk, data->fine_vol_0);
		sys_write32(reg_val, reg + CODEC_DAC_CH0_CFG);
	}
}

static void config_rx_channel(uintptr_t reg, struct sf32lb_codec_adc_cfg *cfg)
{
	uint32_t reg_val;
	const struct sf32lb_codec_adc_clk *adc_clk = cfg->adc_clk;

	reg_val = FIELD_PREP(AUDCODEC_ADC_CFG_OSR_SEL_Msk, adc_clk->osr_sel) |
		  FIELD_PREP(AUDCODEC_ADC_CFG_OP_MODE_Msk, cfg->opmode) |
		  FIELD_PREP(AUDCODEC_ADC_CFG_PATH_RESET_Msk, 0) |
		  FIELD_PREP(AUDCODEC_ADC_CFG_CLK_SRC_SEL_Msk, adc_clk->clk_src_sel) |
		  FIELD_PREP(AUDCODEC_ADC_CFG_CLK_DIV_Msk, adc_clk->clk_div);
	sys_write32(reg_val, reg + CODEC_ADC_CFG);

	reg_val = FIELD_PREP(AUDCODEC_ADC_CH0_CFG_ENABLE_Msk, 1) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_HPF_BYPASS_Msk, 0) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_HPF_COEF_Msk, 0x7) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_STB_INV_Msk, 0) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_DMA_EN_Msk, 0) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_ROUGH_VOL_Msk, 0xa) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_FINE_VOL_Msk, 0) |
		  FIELD_PREP(AUDCODEC_ADC_CH0_CFG_DATA_FORMAT_Msk, 1);
	sys_write32(reg_val, reg + CODEC_ADC_CH0_CFG);
}

static inline void refgen_init(uintptr_t reg)
{
	sys_clear_bits(reg + CODEC_BG_CFG0, AUDCODEC_BG_CFG0_EN_SMPL_Msk);
	sys_clear_bits(reg + CODEC_REFGEN_CFG, AUDCODEC_REFGEN_CFG_EN_CHOP_Msk);
	sys_set_bit(reg + CODEC_REFGEN_CFG, AUDCODEC_REFGEN_CFG_EN_Pos);
	sys_clear_bits(reg + CODEC_REFGEN_CFG, AUDCODEC_REFGEN_CFG_LV_MODE_Msk);
	sys_set_bit(reg + CODEC_PLL_CFG5, AUDCODEC_PLL_CFG5_EN_CLK_CHOP_BG_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG5, AUDCODEC_PLL_CFG5_EN_CLK_CHOP_REFGEN_Pos);

	k_sleep(K_MSEC(2));

	sys_clear_bits(reg + CODEC_BG_CFG0, AUDCODEC_BG_CFG0_EN_SMPL_Msk);
}

static void pll_turn_off(uintptr_t reg)
{
	/* turn off pll */
	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_EN_IARY_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_EN_VCO_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_EN_ANA_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_EN_DIG_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG3, AUDCODEC_PLL_CFG3_EN_SDM_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG4, AUDCODEC_PLL_CFG4_EN_CLK_DIG_Msk);

	/* turn off refgen */
	sys_clear_bits(reg + CODEC_REFGEN_CFG, AUDCODEC_REFGEN_CFG_EN_Msk);

	/* turn off bandgap */
	sys_write32(0, reg + CODEC_BG_CFG1);
	sys_write32(0, reg + CODEC_BG_CFG2);
	sys_clear_bits(reg + CODEC_BG_CFG0, AUDCODEC_BG_CFG0_EN_Msk);
	sys_clear_bits(reg + CODEC_BG_CFG0, AUDCODEC_BG_CFG0_EN_SMPL_Msk);
}

static void pll_turn_on(uintptr_t reg)
{
	uint32_t reg_val;

	/* turn on bandgap */
	reg_val = FIELD_PREP(AUDCODEC_BG_CFG0_EN_Msk, 1) |
		  FIELD_PREP(AUDCODEC_BG_CFG0_LP_MODE_Msk, 0) |
		  FIELD_PREP(AUDCODEC_BG_CFG0_VREF_SEL_Msk, 0xc) | /* 0xc: 3.3v  2:AVDD = 1.8V */
		  FIELD_PREP(AUDCODEC_BG_CFG0_EN_SMPL_Msk, 0) |
		  FIELD_PREP(AUDCODEC_BG_CFG0_EN_RCFLT_Msk, 1) |
		  FIELD_PREP(AUDCODEC_BG_CFG0_MIC_VREF_SEL_Msk, 4) |
		  FIELD_PREP(AUDCODEC_BG_CFG0_EN_AMP_Msk, 1) |
		  FIELD_PREP(AUDCODEC_BG_CFG0_SET_VC_Msk, 0);
	sys_write32(reg_val, reg + CODEC_BG_CFG0);

	/* avoid noise */
	sys_write32(0, reg + CODEC_BG_CFG1); /* 48000 */
	sys_write32(0, reg + CODEC_BG_CFG2); /* 48000000 */

	/* wait BG CFG stable */
	k_busy_wait(100);

	sys_set_bit(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_EN_IARY_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_EN_VCO_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_EN_ANA_Pos);

	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_ICP_SEL_Msk);
	reg_val = sys_read32(reg + CODEC_PLL_CFG0);
	reg_val &= ~(AUDCODEC_PLL_CFG0_ICP_SEL_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG0_ICP_SEL_Msk, 8);
	sys_write32(reg_val, reg + CODEC_PLL_CFG0);

	sys_set_bit(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_EN_DIG_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG3, AUDCODEC_PLL_CFG3_EN_SDM_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG4, AUDCODEC_PLL_CFG4_EN_CLK_DIG_Pos);

	reg_val = FIELD_PREP(AUDCODEC_PLL_CFG1_R3_SEL_Msk, 3) |
		  FIELD_PREP(AUDCODEC_PLL_CFG1_RZ_SEL_Msk, 1) |
		  FIELD_PREP(AUDCODEC_PLL_CFG1_C2_SEL_Msk, 3) |
		  FIELD_PREP(AUDCODEC_PLL_CFG1_CZ_SEL_Msk, 6) |
		  FIELD_PREP(AUDCODEC_PLL_CFG1_CSD_RST_Msk, 0) |
		  FIELD_PREP(AUDCODEC_PLL_CFG1_CSD_EN_Msk, 0);
	sys_write32(reg_val, reg + CODEC_PLL_CFG1);

	/* wait CSD stable */
	k_busy_wait(50);

	refgen_init(reg);
}

/* type 0: 16k 1024 series  1:44.1k 1024 series 2:16k 1000 series 3: 44.1k 1000 series */
static int pll_update_freq(uintptr_t reg, uint8_t type)
{
	uint32_t reg_val = 0;

	sys_set_bit(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Pos);
	/* wait reset stable */
	k_busy_wait(50);

	reg_val = sys_read32(reg + CODEC_PLL_CFG3);

	switch (type) {
	case 0:
		/* set pll to 49.152M   [(fcw+3)+sdin/2^20]*6M */
		reg_val = FIELD_PREP(AUDCODEC_PLL_CFG3_SDIN_Msk, 201327) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_FCW_Msk, 5) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_UPDATE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_MODE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMCLK_POL_Msk, 0);
		break;
	case 1:
		/* set pll to 45.1584M */
		reg_val = FIELD_PREP(AUDCODEC_PLL_CFG3_SDIN_Msk, 551970) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_FCW_Msk, 4) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_UPDATE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_MODE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMCLK_POL_Msk, 0);
		break;
	case 2:
		/* set pll to 48M */
		reg_val = FIELD_PREP(AUDCODEC_PLL_CFG3_SDIN_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_FCW_Msk, 5) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_UPDATE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_MODE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMCLK_POL_Msk, 0);
		break;
	case 3:
		/* set pll to 44.1M */
		reg_val = FIELD_PREP(AUDCODEC_PLL_CFG3_SDIN_Msk, 0x5999A) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_FCW_Msk, 4) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_UPDATE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_MODE_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDM_DITHER_Msk, 0) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_EN_SDM_Msk, 1) |
			  FIELD_PREP(AUDCODEC_PLL_CFG3_SDMCLK_POL_Msk, 0);
		break;
	default:
		__ASSERT(0, "Invalid audio PLL configuration index in sf32lb_codec");
		break;
	}
	sys_write32(reg_val, reg + CODEC_PLL_CFG3);

	sys_set_bit(reg + CODEC_PLL_CFG3, AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos);
	sys_clear_bits(reg + CODEC_PLL_CFG3, AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Msk);

	/* RSRB change from clear to set, must has enough delay */
	k_busy_wait(50);

	sys_set_bit(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_RSTB_Pos);

	/* check pll lock */
	k_busy_wait(50);

	sys_set_bit(reg + CODEC_PLL_CFG1, AUDCODEC_PLL_CFG1_CSD_EN_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG1, AUDCODEC_PLL_CFG1_CSD_RST_Pos);

	/* CSD change from set to clear, must has enough delay */
	k_busy_wait(50);

	sys_clear_bits(reg + CODEC_PLL_CFG1, AUDCODEC_PLL_CFG1_CSD_RST_Msk);

	int ret = 0;

	if (sys_test_bit(reg + CODEC_PLL_STAT, AUDCODEC_PLL_STAT_UNLOCK_Pos)) {
		LOG_ERR("pll lock fail! freq_type:%d", type);
		ret = -1;
	} else {
		LOG_DBG("pll lock! freq_type:%d", type);
		sys_clear_bits(reg + CODEC_PLL_CFG1, AUDCODEC_PLL_CFG1_CSD_EN_Msk);
	}
	return ret;
}

static inline void wait_pll_done(uintptr_t reg)
{
	while (!FIELD_GET(AUDCODEC_PLL_CAL_CFG_DONE_Msk, sys_read32(reg + CODEC_PLL_CAL_CFG))) {
	}
}

static inline void fix_pll_vco_table(struct pll_vco *vco, uint32_t delta_cnt,
				     uint32_t delta_cnt_min, uint32_t delta_cnt_max,
				     uint32_t fc_vco_min, uint32_t fc_vco_max, uint32_t fc_vco)
{
	if (delta_cnt_min <= delta_cnt && delta_cnt_min <= delta_cnt_max) {
		vco->vco_value = fc_vco_min;
	} else if (delta_cnt_max <= delta_cnt && delta_cnt_max <= delta_cnt_min) {
		vco->vco_value = fc_vco_max;
	} else {
		vco->vco_value = fc_vco;
	}
}

static inline void adjust_pll_vco(uintptr_t reg, struct pll_vco *vco)
{
	uint32_t reg_val;
	uint32_t pll_cnt = 0;
	uint32_t xtal_cnt = 0;
	uint32_t fc_vco = 16;
	uint32_t fc_vco_min = 0;
	uint32_t fc_vco_max = 0;
	uint32_t delta_cnt = 0;
	uint32_t delta_cnt_min = 0;
	uint32_t delta_cnt_max = 0;
	uint32_t delta_fc_vco = 8;
	uint32_t target_cnt = vco->target_cnt;

	/* setup calibration and run
	 * target pll_cnt = ceil(46MHz/48MHz*2000)+1 = 1918
	 * target difference between pll_cnt and
	 * xtal_cnt should be less than 1
	 */
	while (delta_fc_vco != 0) {
		sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_FC_VCO_Msk);
		reg_val = sys_read32(reg + CODEC_PLL_CFG0);
		reg_val &= ~(AUDCODEC_PLL_CFG0_FC_VCO_Msk);
		reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG0_FC_VCO_Msk, fc_vco);
		sys_write32(reg_val, reg + CODEC_PLL_CFG0);
		sys_set_bit(reg + CODEC_PLL_CAL_CFG, AUDCODEC_PLL_CAL_CFG_EN_Pos);

		wait_pll_done(reg);

		reg_val = sys_read32(reg + CODEC_PLL_CAL_RESULT);
		pll_cnt = FIELD_GET(AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Msk, reg_val);

		reg_val = sys_read32(reg + CODEC_PLL_CAL_RESULT);
		xtal_cnt = FIELD_GET(AUDCODEC_PLL_CAL_RESULT_XTAL_CNT_Msk, reg_val);

		sys_clear_bits(reg + CODEC_PLL_CAL_CFG, AUDCODEC_PLL_CAL_CFG_EN_Msk);

		if (pll_cnt < target_cnt) {
			fc_vco = fc_vco + delta_fc_vco;
			delta_cnt = target_cnt - pll_cnt;
		} else {
			fc_vco = fc_vco - delta_fc_vco;
			delta_cnt = pll_cnt - target_cnt;
		}

		delta_fc_vco = delta_fc_vco >> 1;
	}

	LOG_DBG("call par CFG1(%x)", sys_read32(reg + CODEC_PLL_CFG1));

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

	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_FC_VCO_Msk);
	reg_val = sys_read32(reg + CODEC_PLL_CFG0);
	reg_val &= ~(AUDCODEC_PLL_CFG0_FC_VCO_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG0_FC_VCO_Msk, fc_vco_min);
	sys_write32(reg_val, reg + CODEC_PLL_CFG0);

	sys_set_bit(reg + CODEC_PLL_CAL_CFG, AUDCODEC_PLL_CAL_CFG_EN_Pos);

	LOG_DBG("fc %d, xtal %d, pll %d", fc_vco, xtal_cnt, pll_cnt);

	wait_pll_done(reg);

	reg_val = sys_read32(reg + CODEC_PLL_CAL_RESULT);
	pll_cnt = FIELD_GET(AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Msk, reg_val);

	sys_clear_bits(reg + CODEC_PLL_CAL_CFG, AUDCODEC_PLL_CAL_CFG_EN_Msk);

	if (pll_cnt < target_cnt) {
		delta_cnt_min = target_cnt - pll_cnt;
	} else {
		delta_cnt_min = pll_cnt - target_cnt;
	}

	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_FC_VCO_Msk);
	reg_val = sys_read32(reg + CODEC_PLL_CFG0);
	reg_val &= ~(AUDCODEC_PLL_CFG0_FC_VCO_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG0_FC_VCO_Msk, fc_vco_max);
	sys_write32(reg_val, reg + CODEC_PLL_CFG0);
	sys_set_bit(reg + CODEC_PLL_CAL_CFG, AUDCODEC_PLL_CAL_CFG_EN_Pos);

	wait_pll_done(reg);

	reg_val = sys_read32(reg + CODEC_PLL_CAL_RESULT);
	pll_cnt = FIELD_GET(AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Msk, reg_val);
	sys_clear_bits(reg + CODEC_PLL_CAL_CFG, AUDCODEC_PLL_CAL_CFG_EN_Msk);

	if (pll_cnt < target_cnt) {
		delta_cnt_max = target_cnt - pll_cnt;
	} else {
		delta_cnt_max = pll_cnt - target_cnt;
	}

	fix_pll_vco_table(vco, delta_cnt, delta_cnt_min, delta_cnt_max, fc_vco_min, fc_vco_max,
			  fc_vco);
}

static void pll_calibration(uintptr_t reg)
{
	uint32_t reg_val;

	pll_turn_on(reg);

	/* VCO freq calibration */
	sys_set_bit(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_OPEN_Pos);
	sys_set_bit(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_EN_LF_VCIN_Pos);

	reg_val = FIELD_PREP(AUDCODEC_PLL_CAL_CFG_EN_Msk, 0) |
		  FIELD_PREP(AUDCODEC_PLL_CAL_CFG_LEN_Msk, 2000);
	sys_write32(reg_val, reg + CODEC_PLL_CAL_CFG);

	for (uint8_t i = 0; i < ARRAY_SIZE(g_pll_vco_tab); i++) {
		adjust_pll_vco(reg, &g_pll_vco_tab[i]);
	}
	sys_clear_bits(reg + CODEC_PLL_CFG2, AUDCODEC_PLL_CFG2_EN_LF_VCIN_Msk);
	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_OPEN_Msk);

	pll_turn_off(reg);
}

static int bf0_update_pll(uintptr_t reg, uint32_t freq, uint8_t type)
{
	uint32_t reg_val;
	uint8_t freq_type;
	uint8_t test_result;
	uint8_t vco_index = 0;

	freq_type = type << 1;
	if ((freq == 44100) || (freq == 22050) || (freq == 11025)) {
		vco_index = 1;
		freq_type += 1;
	}
	sys_clear_bits(reg + CODEC_PLL_CFG0, AUDCODEC_PLL_CFG0_FC_VCO_Msk);
	reg_val = sys_read32(reg + CODEC_PLL_CFG0);
	reg_val &= ~(AUDCODEC_PLL_CFG0_FC_VCO_Msk);
	reg_val |= FIELD_PREP(AUDCODEC_PLL_CFG0_FC_VCO_Msk, g_pll_vco_tab[vco_index].vco_value);
	sys_write32(reg_val, reg + CODEC_PLL_CFG0);

	LOG_DBG("new PLL_ENABLE vco:%d, freq_type:%d", g_pll_vco_tab[vco_index].vco_value,
		freq_type);
	do {
		test_result = pll_update_freq(reg, freq_type);
	} while (test_result != 0);

	return test_result;
}

static int pll_enable(uintptr_t reg, uint32_t freq, uint8_t type)
{
	LOG_DBG("enable pll");
	pll_turn_on(reg);
	return bf0_update_pll(reg, freq, type);
}

static void bf0_audio_pll_config(const struct sf32lb_codec_driver_config *cfg,
				 struct sf32lb_audcodec_data *data,
				 const struct sf32lb_codec_adc_clk *adc_clk,
				 const struct sf32lb_codec_dac_clk *dac_clk, audio_dai_dir_t dir)
{
	if ((dir & AUDIO_DAI_DIR_TX)) {
		if (dac_clk->clk_src_sel != 0) { /* pll */
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
	}
	if ((dir & AUDIO_DAI_DIR_RX)) {
		if (adc_clk->clk_src_sel != 0) { /* pll */
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

	config_dac_path_volume(cfg->reg, gain);

	data->last_volume = volume;
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	int ret = 0;
	const struct sf32lb_codec_driver_config *cfg = dev->config;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		mute_dac_path(dev, cfg->reg, val.mute);
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
	/* Properties are applied immediately in codec_set_property(), so nothing to do here. */
	(void)dev;
	return 0;
}

int codec_register_done_callback(const struct device *dev, audio_codec_tx_done_callback_t tx_cb,
				 void *tx_cb_user_data, audio_codec_rx_done_callback_t rx_cb,
				 void *rx_cb_user_data)
{
	struct sf32lb_audcodec_data *data = dev->data;

	data->tx_cb_user_data = tx_cb_user_data;
	data->rx_cb_user_data = rx_cb_user_data;
	data->tx_done = tx_cb;
	data->rx_done = rx_cb;

	return 0;
}

static int codec_write(const struct device *dev, uint8_t *data, size_t size)
{
	struct sf32lb_audcodec_data *dev_data = dev->data;

	if (!data || size > dev_data->tx_half_dma_size) {
		return -EINVAL;
	}

	K_SPINLOCK(&dev_data->lock) {
		memcpy(dev_data->tx_write_ptr, data, size);
		if (size < dev_data->tx_half_dma_size) {
			memset(dev_data->tx_write_ptr + size, 0, dev_data->tx_half_dma_size - size);
		}
	}
	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *sf32lb_cfg = dev->config;
	struct sf32lb_codec_hw_config *hw_cfg = &data->hw_config;
	struct pcm_config *pcm_cfg;
	int r;

	if (cfg->dai_type != AUDIO_DAI_TYPE_PCM) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_PCM");
		return -EINVAL;
	}

	r = sf32lb_clock_control_on_dt(&sf32lb_cfg->clock);
	if (r < 0) {
		LOG_ERR("Clock required is not on");
		return r;
	}
	pcm_cfg = &cfg->dai_cfg.pcm;

	if ((pcm_cfg->dir & AUDIO_DAI_DIR_TX)) {
		uint8_t i;

		for (i = 0; i < ARRAY_SIZE(codec_dac_clk_config); i++) {
			if (pcm_cfg->samplerate == codec_dac_clk_config[i].samplerate) {
				hw_cfg->samplerate_index = i;
				hw_cfg->dac_cfg.dac_clk = &codec_dac_clk_config[i];
				break;
			}
		}
		__ASSERT(i < ARRAY_SIZE(codec_dac_clk_config), "tx smprate error");

		data->tx_half_dma_size = pcm_cfg->block_size;
		if (!data->tx_buf) {
			data->tx_buf =
				k_aligned_alloc(sizeof(uint32_t), data->tx_half_dma_size * 2);
		}

		if (!data->tx_buf) {
			return -ENOMEM;
		}

		memset(data->tx_buf, 0, data->tx_half_dma_size * 2);
		sys_write32((uint32_t)data->tx_buf, (uintptr_t)&data->tx_write_ptr);

		hw_cfg->dac_cfg.opmode = 1; /* not work with audprc */
		config_tx_channel(sf32lb_cfg->reg, &hw_cfg->dac_cfg);

		LOG_DBG("tx samperate=%d", hw_cfg->dac_cfg.dac_clk->samplerate);
	}

	if ((pcm_cfg->dir & AUDIO_DAI_DIR_RX)) {
		uint8_t i;

		for (i = 0; i < ARRAY_SIZE(codec_adc_clk_config); i++) {
			if (pcm_cfg->samplerate == codec_adc_clk_config[i].samplerate) {
				hw_cfg->samplerate_index = i;
				hw_cfg->adc_cfg.adc_clk = &codec_adc_clk_config[i];
				break;
			}
		}
		__ASSERT(i < ARRAY_SIZE(codec_adc_clk_config), "rx smprate error");

		data->rx_half_dma_size = pcm_cfg->block_size;
		if (!data->rx_buf) {
			data->rx_buf =
				k_aligned_alloc(sizeof(uint32_t), data->rx_half_dma_size * 2);
		}

		if (!data->rx_buf) {
			return -ENOMEM;
		}
		memset(data->rx_buf, 0, data->rx_half_dma_size * 2);

		hw_cfg->adc_cfg.opmode = 1; /* not work with audprc */
		config_rx_channel(sf32lb_cfg->reg, &hw_cfg->adc_cfg);

		LOG_DBG("rx samperate=%d", hw_cfg->adc_cfg.adc_clk->samplerate);
	}

	return r;
}

void dma_tx_callback(const struct device *dev_dma, void *user_data, uint32_t channel, int status)
{
	struct sf32lb_audcodec_data *data = (struct sf32lb_audcodec_data *)user_data;
	const struct device *dev = data->dev;

	if (status == DMA_STATUS_HALF_COMPLETE) {
		/* half DMA finished, update pointer of DMA circle buffer for writing new data
		 */
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
		LOG_ERR("dma tx err:%d", status);
	}
}

void dma_rx_callback(const struct device *dev_dma, void *user_data, uint32_t channel, int status)
{
	struct sf32lb_audcodec_data *data = (struct sf32lb_audcodec_data *)user_data;
	const struct device *dev = data->dev;

	if (status == DMA_STATUS_COMPLETE) {
		if (data->rx_done) {
			data->rx_done(dev, data->rx_buf + data->rx_half_dma_size,
				      data->rx_half_dma_size, data->rx_cb_user_data);
		}
	} else if (status == DMA_STATUS_HALF_COMPLETE) {
		if (data->rx_done) {
			data->rx_done(dev, data->rx_buf, data->rx_half_dma_size,
				      data->rx_cb_user_data);
		}
	} else {
		LOG_ERR("dma rx err:%d", status);
	}
}

static void pa_power_enable(const struct gpio_dt_spec *spec)
{
	/* configure PA power pin */
	(void)gpio_pin_configure_dt(spec, GPIO_OUTPUT_HIGH);
	k_sleep(K_MSEC(10)); /* wait for PA power stable */
}

static void pa_power_disable(const struct gpio_dt_spec *spec)
{
	/* configure PA power pin */
	(void)gpio_pin_configure_dt(spec, GPIO_OUTPUT_LOW);
	k_sleep(K_MSEC(10)); /* wait for PA power stable */
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	struct sf32lb_codec_hw_config *hw_cfg = &data->hw_config;

	bool start_rx = (!data->rx_enable && (dir & AUDIO_DAI_DIR_RX));
	bool start_tx = (!data->tx_enable && (dir & AUDIO_DAI_DIR_TX));

	if (start_rx || start_tx) {
		bf0_audio_pll_config(cfg, data, &codec_adc_clk_config[hw_cfg->samplerate_index],
				     &codec_dac_clk_config[hw_cfg->samplerate_index], dir);
	} else {
		LOG_ERR("start err");
		return -EIO;
	}

	if (start_rx) {
		LOG_DBG("codec start rx, blk=%d", data->rx_half_dma_size);
		if (!data->rx_buf) {
			LOG_ERR("must configure before start rx");
			return -EIO;
		}

		if (sf32lb_dma_reload_dt(&cfg->dma_rx, (uintptr_t)(cfg->reg + CODEC_ADC_CH0_ENTRY),
					 (uintptr_t)data->rx_buf, data->rx_half_dma_size * 2) < 0) {
			LOG_ERR("DMA Rx reload failed\n");
			return -EIO;
		}

		if (sf32lb_dma_start_dt(&cfg->dma_rx) < 0) {
			LOG_ERR("DMA Rx start failed\n");
			return -EIO;
		}

		sys_set_bit(cfg->reg + CODEC_ADC_CH0_CFG, AUDCODEC_ADC_CH0_CFG_DMA_EN_Pos);

		config_analog_adc_path(cfg->reg, hw_cfg->adc_cfg.adc_clk);
	}

	if (start_tx) {
		LOG_DBG("codec start tx, blk=%d", data->tx_half_dma_size);
		if (!data->tx_buf) {
			LOG_ERR("must configure before start tx");
			return -EIO;
		}

		if (sf32lb_dma_reload_dt(&cfg->dma_tx, (uintptr_t)data->tx_buf,
					 (uintptr_t)(cfg->reg + CODEC_DAC_CH0_ENTRY),
					 data->tx_half_dma_size * 2) < 0) {
			LOG_ERR("DMA Tx reload failed\n");
			return -EIO;
		}

		mute_dac_path(dev, cfg->reg, 1);

		if (sf32lb_dma_start_dt(&cfg->dma_tx) < 0) {
			LOG_ERR("DMA Tx start failed\n");
			return -EIO;
		}

		sys_set_bit(cfg->reg + CODEC_DAC_CH0_CFG, AUDCODEC_DAC_CH0_CFG_DMA_EN_Pos);

		config_dac_path(cfg->reg, 1);
		config_analog_dac_path(cfg->reg, hw_cfg->dac_cfg.dac_clk);
		config_dac_path(cfg->reg, 0);
	}

	/* speech echo cancellation algorithm requires a fixed delay time between ADC and DAC,
	 * enable at last.
	 */
	if (start_tx) {
		data->tx_enable = 1;
		sys_set_bit(cfg->reg + CODEC_CFG, AUDCODEC_CFG_DAC_ENABLE_Pos);

		pa_power_enable(&cfg->pa_power_dt);
		mute_dac_path(dev, cfg->reg, 0);
	}

	if (start_rx) {
		data->rx_enable = 1;
		sys_set_bit(cfg->reg + CODEC_CFG, AUDCODEC_CFG_ADC_ENABLE_Pos);
	}

	return 0;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct sf32lb_audcodec_data *data = dev->data;
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	bool stop_rx = (data->rx_enable && (dir & AUDIO_DAI_DIR_RX));
	bool stop_tx = (data->tx_enable && (dir & AUDIO_DAI_DIR_TX));
	int r = 0;

	if (stop_tx) {
		LOG_DBG("stop tx");
		pa_power_disable(&cfg->pa_power_dt);
		mute_dac_path(dev, cfg->reg, 1); /* avoid pop noise */
		sf32lb_dma_stop_dt(&cfg->dma_tx);
		config_dac_path(cfg->reg, 1);
		close_analog_dac_path(cfg->reg);
		disable_dac(cfg->reg);
		clear_dac_channel(cfg->reg);
		if (data->tx_buf) {
			k_free(data->tx_buf);
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
			k_free(data->rx_buf);
			data->rx_buf = NULL;
		}
		data->rx_enable = 0;
	}

	if (stop_rx || stop_tx) {
		pll_turn_off(cfg->reg);
		data->pll_state = AUDIO_PLL_CLOSED;
	} else {
		LOG_ERR("stop err");
		r = -EIO;
	}
	return r;
}

static void codec_start_output(const struct device *dev)
{
	LOG_WRN("start_output is not supported, please use start function for this device");
	ARG_UNUSED(dev);
}

static void codec_stop_output(const struct device *dev)
{
	LOG_WRN("stop_output is not supported, please use stop function for this device");
	ARG_UNUSED(dev);
}

static void config_audcodec_dma(const struct device *dev, uint8_t is_tx)
{
	int ret;
	const struct sf32lb_dma_dt_spec *spec;
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	struct dma_config config_dma = {0};
	struct dma_block_config block_cfg = {0};
	struct sf32lb_audcodec_data *data = dev->data;

	if (is_tx) {
		block_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		block_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		config_dma.channel_direction = MEMORY_TO_PERIPHERAL;
		block_cfg.source_reload_en = 1U;
		block_cfg.dest_reload_en = 0U;
		config_dma.dma_callback = dma_tx_callback;
		spec = &cfg->dma_tx;
	} else {
		block_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		block_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		config_dma.channel_direction = PERIPHERAL_TO_MEMORY;
		block_cfg.source_reload_en = 0U;
		block_cfg.dest_reload_en = 1U;
		config_dma.dma_callback = dma_rx_callback;
		spec = &cfg->dma_rx;
	}

	sf32lb_dma_config_init_dt(spec, &config_dma);

	config_dma.head_block = &block_cfg;
	/* audio must 4 bytes unit */
	config_dma.source_data_size = 4U;
	config_dma.dest_data_size = 4U;
	config_dma.half_complete_callback_en = 1U;
	config_dma.error_callback_dis = 1U;
	config_dma.block_count = 1U;
	config_dma.user_data = (void *)data;
	data->dev = dev;

	ret = sf32lb_dma_config_dt(spec, &config_dma);

	if (ret < 0) {
		LOG_ERR("dma cfg err=%d", ret);
	}
}

static int codec_driver_init(const struct device *dev)
{
	const struct sf32lb_codec_driver_config *cfg = dev->config;
	struct sf32lb_audcodec_data *data = dev->data;
	struct sf32lb_codec_hw_config *hw_cfg = &data->hw_config;
	int r = 0;

	if (!sf32lb_dma_is_ready_dt(&cfg->dma_tx) || !sf32lb_dma_is_ready_dt(&cfg->dma_rx)) {
		r = -ENODEV;
		goto end;
	}

	if (!sf32lb_clock_is_ready_dt(&cfg->clock)) {
		r = -ENODEV;
		goto end;
	}

	/* set clock */
	hw_cfg->en_dly_sel = 0;
	hw_cfg->dac_cfg.opmode = 1;
	hw_cfg->adc_cfg.opmode = 1;

	pmu_enable_audio(1);

	r = sf32lb_clock_control_on_dt(&cfg->clock);
	if (r < 0) {
		LOG_ERR("Clock required is not on");
	} else {
		config_audcodec_dma(dev, 1);
		config_audcodec_dma(dev, 0);
		pll_calibration(cfg->reg);
	}

end:
	return r;
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

#define SF32LB_AUDIO_CODEC_DEFINE(n)                                                               \
	static const struct sf32lb_codec_driver_config config##n = {                               \
		.reg = DT_INST_REG_ADDR(n),                                                        \
		.dma_tx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, tx),                              \
		.dma_rx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, rx),                              \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.pa_power_dt = GPIO_DT_SPEC_INST_GET(n, pa_power_gpios),                           \
	};                                                                                         \
                                                                                                   \
	static struct sf32lb_audcodec_data data##n;                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, codec_driver_init, NULL, &data##n, &config##n, POST_KERNEL,       \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_AUDIO_CODEC_DEFINE)
