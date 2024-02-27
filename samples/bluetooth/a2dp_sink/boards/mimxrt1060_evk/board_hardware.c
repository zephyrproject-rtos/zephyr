/*
 * Copyright 2020 - 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include "fsl_iomuxc.h"
#include "fsl_sai.h"
#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"
#include "fsl_codec_common.h"
#include "fsl_wm8960.h"
#include "fsl_adapter_audio.h"
#include "board_codec.h"

/* Select Audio/Video PLL (786.48 MHz) as sai1 clock source */
#define DEMO_SAI1_CLOCK_SOURCE_SELECT (2U)
/* Clock pre divider for sai1 clock source */
#define DEMO_SAI1_CLOCK_SOURCE_PRE_DIVIDER (3U)
/* Clock divider for sai1 clock source */
#define DEMO_SAI1_CLOCK_SOURCE_DIVIDER (15U)
/* Get frequency of sai1 clock */
#define DEMO_SAI_CLK_FREQ                                                     \
(CLOCK_GetFreq(kCLOCK_AudioPllClk) / (DEMO_SAI1_CLOCK_SOURCE_DIVIDER + 1U) /  \
(DEMO_SAI1_CLOCK_SOURCE_PRE_DIVIDER + 1U))

/* Select USB1 PLL (480 MHz) as master lpi2c clock source */
#define DEMO_LPI2C_CLOCK_SOURCE_SELECT (0U)
/* Clock divider for master lpi2c clock source */
#define DEMO_LPI2C_CLOCK_SOURCE_DIVIDER (5U)

#define DEMO_SAI            SAI1
#define DEMO_AUDIO_INSTANCE (1U)

/* DMA */
#define EXAMPLE_DMAMUX_INSTANCE (0U)
#define EXAMPLE_DMA_INSTANCE    (0U)
#define EXAMPLE_TX_CHANNEL      (0U)
#define EXAMPLE_SAI_TX_SOURCE   (kDmaRequestMuxSai1Tx)

/* demo audio data channel */
#define DEMO_AUDIO_DATA_CHANNEL (kHAL_AudioStereo)
/* demo audio bit width */
#define DEMO_AUDIO_BIT_WIDTH (kHAL_AudioWordWidth16bits)
/* demo audio sample frequency */
#define DEMO_AUDIO_SAMPLING_RATE (kHAL_AudioSampleRate44100Hz)

#define BOARD_CODEC_I2C_INSTANCE             1U
#define BOARD_CODEC_I2C_CLOCK_FREQ           (10000000U)

wm8960_config_t wm8960Config = {
	.i2cConfig = {
		.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE,
		.codecI2CSourceClock = BOARD_CODEC_I2C_CLOCK_FREQ
	},
	.route = kWM8960_RoutePlaybackandRecord,
	.rightInputSource = kWM8960_InputDifferentialMicInput2,
	.playSource = kWM8960_PlaySourceDAC,
	.slaveAddress = WM8960_I2C_ADDR,
	.bus = kWM8960_BusI2S,
	.format = {
		.mclk_HZ = 11289750U,
		.sampleRate = kWM8960_AudioSampleRate44100Hz,
		.bitWidth = kWM8960_AudioBitWidth16bit},
	.master_slave	 = false,
};
codec_config_t boardCodecConfig = {.codecDevType = kCODEC_WM8960, .codecDevConfig = &wm8960Config};

/*
 * AUDIO PLL setting: Frequency = Fref * (DIV_SELECT + NUM / DENOM) / (POST)
 *                              = 24 * (30 + 106/1000) / 1
 *                              = 722.544MHz
 */
/*setting for 44.1Khz*/
const clock_audio_pll_config_t audioPllConfig = {
	.loopDivider = 30, /* PLL loop divider. Valid range for DIV_SELECT divider value: 27~54. */
	.postDivider = 1,  /* Divider after the PLL, should only be 1, 2, 4, 8, 16. */
	.numerator   = 106,  /* 30 bit numerator of fractional loop divider. */
	.denominator = 1000, /* 30 bit denominator of fractional loop divider */
};
/*
 * AUDIO PLL setting: Frequency = Fref * (DIV_SELECT + NUM / DENOM) / (POST)
 *                              = 24 * (32 + 77/100) / 1
 *                              = 786.48MHz
 */
/*setting for multiple of 8Khz,such as 48Khz/16Khz/32KHz*/
const clock_audio_pll_config_t audioPllConfig1 = {
	.loopDivider = 32,  /* PLL loop divider. Valid range for DIV_SELECT divider value: 27~54. */
	.postDivider = 1,   /* Divider after the PLL, should only be 1, 2, 4, 8, 16. */
	.numerator   = 77,  /* 30 bit numerator of fractional loop divider. */
	.denominator = 100, /* 30 bit denominator of fractional loop divider */
};

hal_audio_dma_mux_config_t audioTxDmaMuxConfig = {
	.dmaMuxConfig.dmaMuxInstance   = EXAMPLE_DMAMUX_INSTANCE,
	.dmaMuxConfig.dmaRequestSource = EXAMPLE_SAI_TX_SOURCE,
};

hal_audio_dma_config_t audioTxDmaConfig = {
	.instance             = EXAMPLE_DMA_INSTANCE,
	.channel              = EXAMPLE_TX_CHANNEL,
	.enablePreemption     = false,
	.enablePreemptAbility = false,
	.priority             = kHAL_AudioDmaChannelPriorityDefault,
	.dmaMuxConfig         = (void *)&audioTxDmaMuxConfig,
	.dmaChannelMuxConfig  = NULL,
};

hal_audio_ip_config_t audioTxIpConfig = {
	.sai.lineMask = 1U << 0U,
	.sai.syncMode = kHAL_AudioSaiModeAsync,
};

hal_audio_config_t audioTxConfig = {
	.dmaConfig         = &audioTxDmaConfig,
	.ipConfig          = (void *)&audioTxIpConfig,
	.srcClock_Hz       = 11289750U,
	.sampleRate_Hz     = (uint32_t)DEMO_AUDIO_SAMPLING_RATE,
	.fifoWatermark     = FSL_FEATURE_SAI_FIFO_COUNTn(DEMO_SAI) / 2U,
	.msaterSlave       = kHAL_AudioMaster,
	.bclkPolarity      = kHAL_AudioSampleOnRisingEdge,
	.frameSyncWidth    = kHAL_AudioFrameSyncWidthHalfFrame,
	.frameSyncPolarity = kHAL_AudioBeginAtFallingEdge,
	.lineChannels      = DEMO_AUDIO_DATA_CHANNEL,
	.dataFormat        = kHAL_AudioDataFormatI2sClassic,
	.bitWidth          = (uint8_t)DEMO_AUDIO_BIT_WIDTH,
	.instance          = DEMO_AUDIO_INSTANCE,
};

uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate)
{
	CLOCK_DeinitAudioPll();

	if (0U == sampleRate) {
		/* Disable MCLK output */
		IOMUXC_GPR->GPR1 &= (~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR_MASK);
	} else {
		if (44100U == sampleRate) {
			CLOCK_InitAudioPll(&audioPllConfig);
		} else if (0U == sampleRate % 8000U) {
			CLOCK_InitAudioPll(&audioPllConfig1);
		} else {
			/* no action */
		}
		/*Clock setting for LPI2C*/
		CLOCK_SetMux(kCLOCK_Lpi2cMux, DEMO_LPI2C_CLOCK_SOURCE_SELECT);
		CLOCK_SetDiv(kCLOCK_Lpi2cDiv, DEMO_LPI2C_CLOCK_SOURCE_DIVIDER);
		/*Clock setting for SAI1*/
		CLOCK_SetMux(kCLOCK_Sai1Mux, DEMO_SAI1_CLOCK_SOURCE_SELECT);
		CLOCK_SetDiv(kCLOCK_Sai1PreDiv, DEMO_SAI1_CLOCK_SOURCE_PRE_DIVIDER);
		CLOCK_SetDiv(kCLOCK_Sai1Div, DEMO_SAI1_CLOCK_SOURCE_DIVIDER);
		/* Enable MCLK output */
		IOMUXC_GPR->GPR1 |= IOMUXC_GPR_GPR1_SAI1_MCLK_DIR_MASK;
	}

	return DEMO_SAI_CLK_FREQ;
}

void BOARD_InitHardware(void)
{
	DMAMUX_Type * dmaMuxBases[] = DMAMUX_BASE_PTRS;
	edma_config_t config;
	DMA_Type * dmaBases[] = DMA_BASE_PTRS;

	DMAMUX_Init(dmaMuxBases[EXAMPLE_DMAMUX_INSTANCE]);
	EDMA_GetDefaultConfig(&config);
	EDMA_Init(dmaBases[EXAMPLE_DMA_INSTANCE], &config);
}
