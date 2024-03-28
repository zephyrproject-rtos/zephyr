/*
 * Copyright 2020 - 2021 NXP
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

#define A2DP_CODEC_DAC_VOLUME (100U) /* Range: 0 ~ 100 */
#define A2DP_CODEC_HP_VOLUME  (100U)  /* Range: 0 ~ 100 */

extern hal_audio_config_t audioTxConfig;
extern codec_config_t boardCodecConfig;
__nocache __aligned(4) static HAL_AUDIO_HANDLE_DEFINE(audio_tx_handle);
static codec_handle_t codec_handle;

extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate);
extern void board_media_data_played(void);
extern void SAI1_IRQHandler(void);
extern void DMA0_DMA16_DriverIRQHandler(void);
extern void BOARD_InitHardware(void);

static void tx_callback(hal_audio_handle_t handle, hal_audio_status_t completionStatus,
					void *callbackParam)
{
	board_media_data_played();
}

void board_codec_configure(uint32_t sample_rate, uint8_t channels)
{
	audioTxConfig.sampleRate_Hz  = sample_rate;
	audioTxConfig.lineChannels = (hal_audio_channel_t)channels;
	audioTxConfig.srcClock_Hz = BOARD_SwitchAudioFreq(audioTxConfig.sampleRate_Hz);

	printk("a2dp configure sample rate %dHz\r\n", audioTxConfig.sampleRate_Hz);

	memset(audio_tx_handle, 0, sizeof(audio_tx_handle));
	HAL_AudioTxInit((hal_audio_handle_t)&audio_tx_handle[0], &audioTxConfig);
	HAL_AudioTxInstallCallback((hal_audio_handle_t)&audio_tx_handle[0], tx_callback, NULL);

	if (CODEC_Init(&codec_handle, &boardCodecConfig) != kStatus_Success) {
		printk("codec init failed!\r\n");
	}
	CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight |
		kCODEC_PlayChannelHeadphoneLeft, true);
	CODEC_SetFormat(&codec_handle, audioTxConfig.srcClock_Hz,
		audioTxConfig.sampleRate_Hz, audioTxConfig.bitWidth);
	CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, A2DP_CODEC_DAC_VOLUME);
	CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft |
		kCODEC_VolumeHeadphoneRight, A2DP_CODEC_HP_VOLUME);
	CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight |
		kCODEC_PlayChannelHeadphoneLeft, false);
}

void board_codec_deconfigure(void)
{
	CODEC_SetMute(&codec_handle,
		kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
	HAL_AudioTxDeinit((hal_audio_handle_t)&audio_tx_handle[0]);
	(void)BOARD_SwitchAudioFreq(0U);
}

void board_codec_start(void)
{
}

void board_codec_stop(void)
{
	HAL_AudioTransferAbortSend((hal_audio_handle_t)&audio_tx_handle[0]);
}

void board_codec_media_play(uint8_t *data, uint32_t length)
{
	if ((data != NULL) && (length != 0U)) {
		hal_audio_transfer_t xfer;

		xfer.dataSize = length;
		xfer.data = data;

		if (kStatus_HAL_AudioSuccess != HAL_AudioTransferSendNonBlocking(
				(hal_audio_handle_t)&audio_tx_handle[0], &xfer)) {
			printk("prime fail\r\n");
		}
	}
}

static void board_init_audio_pins(void)
{
	CLOCK_EnableClock(kCLOCK_Iomuxc);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_00_LPI2C1_SCL,
		1U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_01_LPI2C1_SDA,
		1U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_09_SAI1_MCLK,
		1U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_13_SAI1_TX_DATA00,
		1U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_14_SAI1_TX_BCLK,
		1U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_15_SAI1_TX_SYNC,
		1U);
	IOMUXC_SetPinMux(
		  IOMUXC_GPIO_AD_B1_12_SAI1_RX_DATA00,
		  1U);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_12_SAI1_RX_DATA00,
		0x10B0u);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_00_LPI2C1_SCL,
		0xD8B0u);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_01_LPI2C1_SDA,
		0xD8B0u);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_09_SAI1_MCLK,
		0x10B0u);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_13_SAI1_TX_DATA00,
		0x10B0u);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_14_SAI1_TX_BCLK,
		0x10B0u);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_15_SAI1_TX_SYNC,
		0x10B0u);



	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_04_LPUART3_CTS_B,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_05_LPUART3_RTS_B,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_06_LPUART3_TX,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_07_LPUART3_RX,
		0U);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_04_LPUART3_CTS_B,
		0x10B0U);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_05_LPUART3_RTS_B,
		0x10B0U);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_06_LPUART3_TX,
		0x1098U);
	IOMUXC_SetPinConfig(
		IOMUXC_GPIO_AD_B1_07_LPUART3_RX,
		0x10B0U);
}

const clock_arm_pll_config_t armPllConfig_BOARD_BootClockRUN = {
	/* PLL loop divider, Fout = Fin * 50 */
	.loopDivider = 100,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};
const clock_sys_pll_config_t sysPllConfig_BOARD_BootClockRUN = {
	/* PLL loop divider, Fout = Fin * ( 20 + loopDivider*2 + numerator / denominator ) */
	.loopDivider = 1,
	/* 30 bit numerator of fractional loop divider */
	.numerator = 0,
	/* 30 bit denominator of fractional loop divider */
	.denominator = 1,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};
const clock_usb_pll_config_t usb1PllConfig_BOARD_BootClockRUN = {
	/* PLL loop divider, Fout = Fin * 20 */
	.loopDivider = 0,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};
const clock_video_pll_config_t videoPllConfig_BOARD_BootClockRUN = {
	/* PLL loop divider, Fout = Fin * ( loopDivider + numerator / denominator ) */
	.loopDivider = 31,
	/* Divider after PLL */
	.postDivider = 8,
	/* 30 bit numerator of fractional loop divider,
	 * Fout = Fin * ( loopDivider + numerator / denominator )
	 */
	.numerator = 0,
	/* 30 bit denominator of fractional loop divider,
	 * Fout = Fin * ( loopDivider + numerator / denominator )
	 */
	.denominator = 1,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};


void board_codec_init(void)
{
	board_init_audio_pins();

	BOARD_InitHardware();

	CLOCK_SetRtcXtalFreq(32768U);
	/* Enable 1MHz clock output. */
	XTALOSC24M->OSC_CONFIG2 |= XTALOSC24M_OSC_CONFIG2_ENABLE_1M_MASK;
	/* Use free 1MHz clock output. */
	XTALOSC24M->OSC_CONFIG2 &= ~XTALOSC24M_OSC_CONFIG2_MUX_1M_MASK;
	/* Set XTAL 24MHz clock frequency. */
	CLOCK_SetXtalFreq(24000000U);
	/* Enable XTAL 24MHz clock source. */
	CLOCK_InitExternalClk(0);
	/* Enable internal RC. */
	CLOCK_InitRcOsc24M();
	/* Switch clock source to external OSC. */
	CLOCK_SwitchOsc(kCLOCK_XtalOsc);
	/* Set Oscillator ready counter value. */
	CCM->CCR = (CCM->CCR & (~CCM_CCR_OSCNT_MASK)) | CCM_CCR_OSCNT(127);
	/* Setting PeriphClk2Mux and PeriphMux to provide stable clock before PLLs are initialed */
	CLOCK_SetMux(kCLOCK_PeriphClk2Mux, 1); /* Set PERIPH_CLK2 MUX to OSC */
	CLOCK_SetMux(kCLOCK_PeriphMux, 1);     /* Set PERIPH_CLK MUX to PERIPH_CLK2 */
	/* Setting the VDD_SOC to 1.275V. It is necessary to config AHB to 600Mhz. */
	DCDC->REG3 = (DCDC->REG3 & (~DCDC_REG3_TRG_MASK)) | DCDC_REG3_TRG(0x13);
	/* Waiting for DCDC_STS_DC_OK bit is asserted */
	while (DCDC_REG0_STS_DC_OK_MASK != (DCDC_REG0_STS_DC_OK_MASK & DCDC->REG0)) {
	}
	/* Set AHB_PODF. */
	CLOCK_SetDiv(kCLOCK_AhbDiv, 0);

	/* Disable SAI1 clock gate. */
	CLOCK_DisableClock(kCLOCK_Sai1);
	/* Set SAI1_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Sai1PreDiv, 3);
	/* Set SAI1_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Sai1Div, 1);
	/* Set Sai1 clock source. */
	CLOCK_SetMux(kCLOCK_Sai1Mux, 0);

	/* Disable Lpi2c clock gate. */
	CLOCK_DisableClock(kCLOCK_Lpi2c1);
	CLOCK_DisableClock(kCLOCK_Lpi2c2);
	CLOCK_DisableClock(kCLOCK_Lpi2c3);
	/* Set LPI2C_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Lpi2cDiv, 0);
	/* Set Lpi2c clock source. */
	CLOCK_SetMux(kCLOCK_Lpi2cMux, 0);

	/* Disable Flexio1 clock gate. */
	CLOCK_DisableClock(kCLOCK_Flexio1);
	/* Set FLEXIO1_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Flexio1PreDiv, 1);
	/* Set FLEXIO1_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Flexio1Div, 7);
	/* Set Flexio1 clock source. */
	CLOCK_SetMux(kCLOCK_Flexio1Mux, 3);
	/* Disable Flexio2 clock gate. */
	CLOCK_DisableClock(kCLOCK_Flexio2);
	/* Set FLEXIO2_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Flexio2PreDiv, 1);
	/* Set FLEXIO2_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Flexio2Div, 7);
	/* Set Flexio2 clock source. */
	CLOCK_SetMux(kCLOCK_Flexio2Mux, 3);
	/* Set Pll3 sw clock source. */
	CLOCK_SetMux(kCLOCK_Pll3SwMux, 0);
	/* Init ARM PLL. */
	CLOCK_InitArmPll(&armPllConfig_BOARD_BootClockRUN);

	/* Init System PLL. */
	CLOCK_InitSysPll(&sysPllConfig_BOARD_BootClockRUN);
	/* Init System pfd0. */
	CLOCK_InitSysPfd(kCLOCK_Pfd0, 27);
	/* Init System pfd1. */
	CLOCK_InitSysPfd(kCLOCK_Pfd1, 16);
	/* Init System pfd2. */
	CLOCK_InitSysPfd(kCLOCK_Pfd2, 24);
	/* Init System pfd3. */
	CLOCK_InitSysPfd(kCLOCK_Pfd3, 16);

	/* DeInit Audio PLL. */
	CLOCK_DeinitAudioPll();
	/* Bypass Audio PLL. */
	CLOCK_SetPllBypass(CCM_ANALOG, kCLOCK_PllAudio, 1);
	/* Set divider for Audio PLL. */
	CCM_ANALOG->MISC2 &= ~CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK;
	CCM_ANALOG->MISC2 &= ~CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK;
	/* Enable Audio PLL output. */
	CCM_ANALOG->PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_ENABLE_MASK;
	/* Init Video PLL. */
	uint32_t pllVideo;
	/* Disable Video PLL output before initial Video PLL. */
	CCM_ANALOG->PLL_VIDEO &= ~CCM_ANALOG_PLL_VIDEO_ENABLE_MASK;
	/* Bypass PLL first */
	CCM_ANALOG->PLL_VIDEO = (CCM_ANALOG->PLL_VIDEO &
			(~CCM_ANALOG_PLL_VIDEO_BYPASS_CLK_SRC_MASK)) |
			CCM_ANALOG_PLL_VIDEO_BYPASS_MASK | CCM_ANALOG_PLL_VIDEO_BYPASS_CLK_SRC(0);
	CCM_ANALOG->PLL_VIDEO_NUM = CCM_ANALOG_PLL_VIDEO_NUM_A(0);
	CCM_ANALOG->PLL_VIDEO_DENOM = CCM_ANALOG_PLL_VIDEO_DENOM_B(1);
	pllVideo = (CCM_ANALOG->PLL_VIDEO & (~(CCM_ANALOG_PLL_VIDEO_DIV_SELECT_MASK |
			CCM_ANALOG_PLL_VIDEO_POWERDOWN_MASK))) |
			CCM_ANALOG_PLL_VIDEO_ENABLE_MASK | CCM_ANALOG_PLL_VIDEO_DIV_SELECT(31);
	pllVideo |= CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(1);
	CCM_ANALOG->MISC2 = (CCM_ANALOG->MISC2 & (~CCM_ANALOG_MISC2_VIDEO_DIV_MASK)) |
			CCM_ANALOG_MISC2_VIDEO_DIV(3);
	CCM_ANALOG->PLL_VIDEO = pllVideo;
	while ((CCM_ANALOG->PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_LOCK_MASK) == 0) {
	}
	/* Disable bypass for Video PLL. */
	CLOCK_SetPllBypass(CCM_ANALOG, kCLOCK_PllVideo, 0);

	/* Set preperiph clock source. */
	CLOCK_SetMux(kCLOCK_PrePeriphMux, 3);
	/* Set periph clock source. */
	CLOCK_SetMux(kCLOCK_PeriphMux, 0);
	/* Set periph clock2 clock source. */
	CLOCK_SetMux(kCLOCK_PeriphClk2Mux, 0);
	/* Set per clock source. */
	CLOCK_SetMux(kCLOCK_PerclkMux, 0);
	/* Set lvds1 clock source. */
	CCM_ANALOG->MISC1 = (CCM_ANALOG->MISC1 & (~CCM_ANALOG_MISC1_LVDS1_CLK_SEL_MASK)) |
			CCM_ANALOG_MISC1_LVDS1_CLK_SEL(0);
	/* Set clock out1 divider. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO1_DIV_MASK)) | CCM_CCOSR_CLKO1_DIV(0);
	/* Set clock out1 source. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO1_SEL_MASK)) | CCM_CCOSR_CLKO1_SEL(1);
	/* Set clock out2 divider. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO2_DIV_MASK)) | CCM_CCOSR_CLKO2_DIV(0);
	/* Set clock out2 source. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO2_SEL_MASK)) | CCM_CCOSR_CLKO2_SEL(18);
	/* Set clock out1 drives clock out1. */
	CCM->CCOSR &= ~CCM_CCOSR_CLK_OUT_SEL_MASK;
	/* Disable clock out1. */
	CCM->CCOSR &= ~CCM_CCOSR_CLKO1_EN_MASK;
	/* Disable clock out2. */
	CCM->CCOSR &= ~CCM_CCOSR_CLKO2_EN_MASK;
	/* Set SAI1 MCLK1 clock source. */
	IOMUXC_SetSaiMClkClockSource(IOMUXC_GPR, kIOMUXC_GPR_SAI1MClk1Sel, 0);
	/* Set SAI1 MCLK2 clock source. */
	IOMUXC_SetSaiMClkClockSource(IOMUXC_GPR, kIOMUXC_GPR_SAI1MClk2Sel, 0);
	/* Set SAI1 MCLK3 clock source. */
	IOMUXC_SetSaiMClkClockSource(IOMUXC_GPR, kIOMUXC_GPR_SAI1MClk3Sel, 0);

	IRQ_CONNECT(SAI1_IRQn, 0,
		    SAI1_IRQHandler, 0, 0);
	IRQ_CONNECT(DMA0_DMA16_IRQn, 0,
		    DMA0_DMA16_DriverIRQHandler, 0, 0);
}
