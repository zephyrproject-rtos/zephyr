/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * How to setup clock using clock driver functions:
 *
 * 1. Call CLOCK_InitXXXPLL() to configure corresponding PLL clock.
 *
 * 2. Call CLOCK_InitXXXpfd() to configure corresponding PLL pfd clock.
 *
 * 3. Call CLOCK_SetMux() to configure corresponding clock source for target clock out.
 *
 * 4. Call CLOCK_SetDiv() to configure corresponding clock divider for target clock out.
 *
 * 5. Call CLOCK_SetXtalFreq() to set XTAL frequency based on board settings.
 *
 */

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
 * !!GlobalInfo
 * product: Clocks v8.0
 * processor: MIMXRT1015xxxxx
 * package_id: MIMXRT1015DAF5A
 * mcu_data: ksdk2_0
 * processor_version: 10.0.0
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

#include "clock_config.h"
#include "fsl_iomuxc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 ******************************************************************************/
void BOARD_InitBootClocks(void)
{
	clock_init();
}

/*******************************************************************************
 ************************** Configuration clock_init ***************************
 ******************************************************************************/
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
 * !!Configuration
 * name: clock_init
 * called_from_default_init: true
 * outputs:
 * - {id: AHB_CLK_ROOT.outFreq, value: 500 MHz}
 * - {id: CKIL_SYNC_CLK_ROOT.outFreq, value: 32.768 kHz}
 * - {id: CLK_1M.outFreq, value: 1 MHz}
 * - {id: CLK_24M.outFreq, value: 24 MHz}
 * - {id: ENET_500M_REF_CLK.outFreq, value: 500 MHz}
 * - {id: FLEXIO1_CLK_ROOT.outFreq, value: 30 MHz}
 * - {id: FLEXSPI_CLK_ROOT.outFreq, value: 90 MHz}
 * - {id: GPT1_ipg_clk_highfreq.outFreq, value: 62.5 MHz}
 * - {id: GPT2_ipg_clk_highfreq.outFreq, value: 62.5 MHz}
 * - {id: IPG_CLK_ROOT.outFreq, value: 125 MHz}
 * - {id: LPI2C_CLK_ROOT.outFreq, value: 10 MHz}
 * - {id: LPSPI_CLK_ROOT.outFreq, value: 90 MHz}
 * - {id: MQS_MCLK.outFreq, value: 540/13 MHz}
 * - {id: PERCLK_CLK_ROOT.outFreq, value: 62.5 MHz}
 * - {id: SAI1_CLK_ROOT.outFreq, value: 540/13 MHz}
 * - {id: SAI1_MCLK1.outFreq, value: 540/13 MHz}
 * - {id: SAI1_MCLK2.outFreq, value: 540/13 MHz}
 * - {id: SAI1_MCLK3.outFreq, value: 30 MHz}
 * - {id: SAI2_CLK_ROOT.outFreq, value: 540/13 MHz}
 * - {id: SAI2_MCLK1.outFreq, value: 540/13 MHz}
 * - {id: SAI2_MCLK3.outFreq, value: 30 MHz}
 * - {id: SAI3_CLK_ROOT.outFreq, value: 540/13 MHz}
 * - {id: SAI3_MCLK1.outFreq, value: 540/13 MHz}
 * - {id: SAI3_MCLK3.outFreq, value: 30 MHz}
 * - {id: SPDIF0_CLK_ROOT.outFreq, value: 30 MHz}
 * - {id: TRACE_CLK_ROOT.outFreq, value: 99 MHz}
 * - {id: UART_CLK_ROOT.outFreq, value: 80 MHz}
 * - {id: USBPHY1_CLK.outFreq, value: 480 MHz}
 * settings:
 * - {id: CCM.AHB_PODF.scale, value: '1', locked: true}
 * - {id: CCM.ARM_PODF.scale, value: '1', locked: true}
 * - {id: CCM.CLKO2_DIV.scale, value: '1', locked: true}
 * - {id: CCM.CLKO2_SEL.sel, value: XTALOSC24M.OSC_CLK}
 * - {id: CCM.FLEXSPI_PODF.scale, value: '8', locked: true}
 * - {id: CCM.FLEXSPI_SEL.sel, value: CCM_ANALOG.PLL3_PFD0_CLK}
 * - {id: CCM.IPG_PODF.scale, value: '4', locked: true}
 * - {id: CCM.LPI2C_CLK_PODF.scale, value: '6', locked: true}
 * - {id: CCM.LPSPI_CLK_SEL.sel, value: CCM_ANALOG.PLL3_PFD0_CLK}
 * - {id: CCM.LPSPI_PODF.scale, value: '8', locked: true}
 * - {id: CCM.PERCLK_PODF.scale, value: '2', locked: true}
 * - {id: CCM.PERIPH_CLK2_SEL.sel, value: XTALOSC24M.OSC_CLK}
 * - {id: CCM.PRE_PERIPH_CLK_SEL.sel, value: CCM.ARM_PODF}
 * - {id: CCM.SAI2_CLK_PRED.scale, value: '4', locked: true}
 * - {id: CCM.SEMC_CLK_SEL.sel, value: CCM.SEMC_ALT_CLK_SEL}
 * - {id: CCM.SEMC_PODF.scale, value: '2', locked: true}
 * - {id: CCM.TRACE_PODF.scale, value: '4', locked: true}
 * - {id: CCM_ANALOG.PLL2.denom, value: '1', locked: true}
 * - {id: CCM_ANALOG.PLL2.num, value: '0', locked: true}
 * - {id: CCM_ANALOG.PLL2_BYPASS.sel, value: CCM_ANALOG.PLL2_OUT_CLK}
 * - {id: CCM_ANALOG.PLL2_PFD0_BYPASS.sel, value: CCM_ANALOG.PLL2_PFD0}
 * - {id: CCM_ANALOG.PLL2_PFD0_DIV.scale, value: '24', locked: true}
 * - {id: CCM_ANALOG.PLL2_PFD0_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL2_PFD1_BYPASS.sel, value: CCM_ANALOG.PLL2_PFD1}
 * - {id: CCM_ANALOG.PLL2_PFD2_BYPASS.sel, value: CCM_ANALOG.PLL2_PFD2}
 * - {id: CCM_ANALOG.PLL2_PFD2_DIV.scale, value: '29', locked: true}
 * - {id: CCM_ANALOG.PLL2_PFD2_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL2_PFD3_BYPASS.sel, value: CCM_ANALOG.PLL2_PFD3}
 * - {id: CCM_ANALOG.PLL2_PFD3_DIV.scale, value: '35', locked: true}
 * - {id: CCM_ANALOG.PLL2_PFD3_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL3_BYPASS.sel, value: CCM_ANALOG.PLL3}
 * - {id: CCM_ANALOG.PLL3_PFD0_BYPASS.sel, value: CCM_ANALOG.PLL3_PFD0}
 * - {id: CCM_ANALOG.PLL3_PFD0_DIV.scale, value: '12', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD0_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD1_BYPASS.sel, value: CCM_ANALOG.PLL3_PFD1}
 * - {id: CCM_ANALOG.PLL3_PFD1_DIV.scale, value: '35', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD1_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD2_BYPASS.sel, value: CCM_ANALOG.PLL3_PFD2}
 * - {id: CCM_ANALOG.PLL3_PFD2_DIV.scale, value: '26', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD2_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD3_BYPASS.sel, value: CCM_ANALOG.PLL3_PFD3}
 * - {id: CCM_ANALOG.PLL3_PFD3_DIV.scale, value: '31', locked: true}
 * - {id: CCM_ANALOG.PLL3_PFD3_MUL.scale, value: '18', locked: true}
 * - {id: CCM_ANALOG.PLL4.denom, value: '50'}
 * - {id: CCM_ANALOG.PLL4.div, value: '27', locked: true}
 * - {id: CCM_ANALOG.PLL4_POST_DIV.scale, value: '4', locked: true}
 * - {id: CCM_ANALOG.PLL6_BYPASS.sel, value: CCM_ANALOG.PLL6}
 * - {id: CCM_ANALOG_PLL_USB1_EN_USB_CLKS_CFG, value: Enabled}
 * - {id: CCM_ANALOG_PLL_USB1_EN_USB_CLKS_OUT_CFG, value: Enabled}
 * - {id: CCM_ANALOG_PLL_USB1_POWER_CFG, value: 'Yes'}
 * sources:
 * - {id: XTALOSC24M.RTC_OSC.outFreq, value: 32.768 kHz, enabled: true}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/*******************************************************************************
 * Variables for clock_init configuration
 ******************************************************************************/
const clock_sys_pll_config_t sysPllConfig_clock_init = {
	/* PLL loop divider, Fout = Fin * ( 20 + loopDivider*2 + numerator / denominator ) */
	.loopDivider = 1,
	/* 30 bit numerator of fractional loop divider */
	.numerator = 0,
	/* 30 bit denominator of fractional loop divider */
	.denominator = 1,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};
const clock_usb_pll_config_t usb1PllConfig_clock_init = {
	/* PLL loop divider, Fout = Fin * 20 */
	.loopDivider = 0,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};
const clock_enet_pll_config_t enetPllConfig_clock_init = {
	/* Enable the PLL providing the ENET 500MHz reference clock */
	.enableClkOutput500M = true,
	/* Bypass clock source, 0 - OSC 24M, 1 - CLK1_P and CLK1_N */
	.src = 0,
};
/*******************************************************************************
 * Code for clock_init configuration
 ******************************************************************************/
void clock_init(void)
{
	/* Init RTC OSC clock frequency. */
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
	CLOCK_SetMux(kCLOCK_PeriphClk2Mux, 1);  /* Set PERIPH_CLK2 MUX to OSC */
	CLOCK_SetMux(kCLOCK_PeriphMux, 1);      /* Set PERIPH_CLK MUX to PERIPH_CLK2 */
	/* Setting the VDD_SOC to 1.25V. It is necessary to config AHB to 500Mhz. */
	DCDC->REG3 = (DCDC->REG3 & (~DCDC_REG3_TRG_MASK)) | DCDC_REG3_TRG(0x12);
	/* Waiting for DCDC_STS_DC_OK bit is asserted */
	while (DCDC_REG0_STS_DC_OK_MASK != (DCDC_REG0_STS_DC_OK_MASK & DCDC->REG0)) {
	}
	/* Set AHB_PODF. */
	CLOCK_SetDiv(kCLOCK_AhbDiv, 0);
	/* Disable IPG clock gate. */
	CLOCK_DisableClock(kCLOCK_Adc1);
	CLOCK_DisableClock(kCLOCK_Xbar1);
	CLOCK_DisableClock(kCLOCK_Xbar2);
	/* Set IPG_PODF. */
	CLOCK_SetDiv(kCLOCK_IpgDiv, 3);
	/* Set ARM_PODF. */
	CLOCK_SetDiv(kCLOCK_ArmDiv, 0);
	/* Set PERIPH_CLK2_PODF. */
	CLOCK_SetDiv(kCLOCK_PeriphClk2Div, 0);
	/* Disable PERCLK clock gate. */
	CLOCK_DisableClock(kCLOCK_Gpt1);
	CLOCK_DisableClock(kCLOCK_Gpt1S);
	CLOCK_DisableClock(kCLOCK_Gpt2);
	CLOCK_DisableClock(kCLOCK_Gpt2S);
	CLOCK_DisableClock(kCLOCK_Pit);
	/* Set PERCLK_PODF. */
	CLOCK_SetDiv(kCLOCK_PerclkDiv, 1);
	/* Set SEMC_PODF. */
	CLOCK_SetDiv(kCLOCK_SemcDiv, 1);
	/* Set Semc alt clock source. */
	CLOCK_SetMux(kCLOCK_SemcAltMux, 0);
	/* Set Semc clock source. */
	CLOCK_SetMux(kCLOCK_SemcMux, 1);
	/*
	 * In SDK projects, external flash (configured by FLEXSPI) will be initialized by dcd.
	 * With this macro XIP_EXTERNAL_FLASH, usb1 pll (selected to be FLEXSPI clock source
	 * in SDK projects) will be left unchanged.
	 * Note: If another clock source is selected for FLEXSPI, user may want to avoid
	 * changing that clock as well.
	 */
#if !(defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
	/* Disable Flexspi clock gate. */
	CLOCK_DisableClock(kCLOCK_FlexSpi);
	/* Set FLEXSPI_PODF. */
	CLOCK_SetDiv(kCLOCK_FlexspiDiv, 7);
	/* Set Flexspi clock source. */
	CLOCK_SetMux(kCLOCK_FlexspiMux, 3);
#endif
	/* Disable LPSPI clock gate. */
	CLOCK_DisableClock(kCLOCK_Lpspi1);
	CLOCK_DisableClock(kCLOCK_Lpspi2);
	/* Set LPSPI_PODF. */
	CLOCK_SetDiv(kCLOCK_LpspiDiv, 7);
	/* Set Lpspi clock source. */
	CLOCK_SetMux(kCLOCK_LpspiMux, 1);
	/* Disable TRACE clock gate. */
	CLOCK_DisableClock(kCLOCK_Trace);
	/* Set TRACE_PODF. */
	CLOCK_SetDiv(kCLOCK_TraceDiv, 3);
	/* Set Trace clock source. */
	CLOCK_SetMux(kCLOCK_TraceMux, 2);
	/* Disable SAI1 clock gate. */
	CLOCK_DisableClock(kCLOCK_Sai1);
	/* Set SAI1_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Sai1PreDiv, 3);
	/* Set SAI1_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Sai1Div, 1);
	/* Set Sai1 clock source. */
	CLOCK_SetMux(kCLOCK_Sai1Mux, 0);
	/* Disable SAI2 clock gate. */
	CLOCK_DisableClock(kCLOCK_Sai2);
	/* Set SAI2_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Sai2PreDiv, 3);
	/* Set SAI2_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Sai2Div, 1);
	/* Set Sai2 clock source. */
	CLOCK_SetMux(kCLOCK_Sai2Mux, 0);
	/* Disable SAI3 clock gate. */
	CLOCK_DisableClock(kCLOCK_Sai3);
	/* Set SAI3_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Sai3PreDiv, 3);
	/* Set SAI3_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Sai3Div, 1);
	/* Set Sai3 clock source. */
	CLOCK_SetMux(kCLOCK_Sai3Mux, 0);
	/* Disable Lpi2c clock gate. */
	CLOCK_DisableClock(kCLOCK_Lpi2c1);
	CLOCK_DisableClock(kCLOCK_Lpi2c2);
	/* Set LPI2C_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Lpi2cDiv, 5);
	/* Set Lpi2c clock source. */
	CLOCK_SetMux(kCLOCK_Lpi2cMux, 0);
	/* Disable UART clock gate. */
	CLOCK_DisableClock(kCLOCK_Lpuart1);
	CLOCK_DisableClock(kCLOCK_Lpuart2);
	CLOCK_DisableClock(kCLOCK_Lpuart3);
	CLOCK_DisableClock(kCLOCK_Lpuart4);
	/* Set UART_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_UartDiv, 0);
	/* Set Uart clock source. */
	CLOCK_SetMux(kCLOCK_UartMux, 0);
	/* Disable SPDIF clock gate. */
	CLOCK_DisableClock(kCLOCK_Spdif);
	/* Set SPDIF0_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Spdif0PreDiv, 1);
	/* Set SPDIF0_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Spdif0Div, 7);
	/* Set Spdif clock source. */
	CLOCK_SetMux(kCLOCK_SpdifMux, 3);
	/* Disable Flexio1 clock gate. */
	CLOCK_DisableClock(kCLOCK_Flexio1);
	/* Set FLEXIO1_CLK_PRED. */
	CLOCK_SetDiv(kCLOCK_Flexio1PreDiv, 1);
	/* Set FLEXIO1_CLK_PODF. */
	CLOCK_SetDiv(kCLOCK_Flexio1Div, 7);
	/* Set Flexio1 clock source. */
	CLOCK_SetMux(kCLOCK_Flexio1Mux, 3);
	/* Set Pll3 sw clock source. */
	CLOCK_SetMux(kCLOCK_Pll3SwMux, 0);
	/* Init System PLL. */
	CLOCK_InitSysPll(&sysPllConfig_clock_init);
	/* Init System pfd0. */
	CLOCK_InitSysPfd(kCLOCK_Pfd0, 24);
	/* Init System pfd1. */
	CLOCK_InitSysPfd(kCLOCK_Pfd1, 16);
	/* Init System pfd2. */
	CLOCK_InitSysPfd(kCLOCK_Pfd2, 29);
	/* Init System pfd3. */
	CLOCK_InitSysPfd(kCLOCK_Pfd3, 35);
	/*
	 * In SDK projects, external flash (configured by FLEXSPI) will be initialized by dcd.
	 * With this macro XIP_EXTERNAL_FLASH, usb1 pll (selected to be FLEXSPI clock source
	 * in SDK projects) will be left unchanged.
	 * Note: If another clock source is selected for FLEXSPI, user may want to avoid
	 * changing that clock as well.
	 */
#if !(defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
	/* Init Usb1 PLL. */
	CLOCK_InitUsb1Pll(&usb1PllConfig_clock_init);
	/* Init Usb1 pfd0. */
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd0, 12);
	/* Init Usb1 pfd1. */
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd1, 35);
	/* Init Usb1 pfd2. */
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd2, 26);
	/* Init Usb1 pfd3. */
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd3, 31);
#endif
	/* DeInit Audio PLL. */
	CLOCK_DeinitAudioPll();
	/* Bypass Audio PLL. */
	CLOCK_SetPllBypass(CCM_ANALOG, kCLOCK_PllAudio, 1);
	/* Set divider for Audio PLL. */
	CCM_ANALOG->MISC2 &= ~CCM_ANALOG_MISC2_AUDIO_DIV_LSB_MASK;
	CCM_ANALOG->MISC2 &= ~CCM_ANALOG_MISC2_AUDIO_DIV_MSB_MASK;
	/* Enable Audio PLL output. */
	CCM_ANALOG->PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_ENABLE_MASK;
	/* Init Enet PLL. */
	CLOCK_InitEnetPll(&enetPllConfig_clock_init);
	/* Set preperiph clock source. */
	CLOCK_SetMux(kCLOCK_PrePeriphMux, 3);
	/* Set periph clock source. */
	CLOCK_SetMux(kCLOCK_PeriphMux, 0);
	/* Set periph clock2 clock source. */
	CLOCK_SetMux(kCLOCK_PeriphClk2Mux, 1);
	/* Set per clock source. */
	CLOCK_SetMux(kCLOCK_PerclkMux, 0);
	/* Set clock out1 divider. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO1_DIV_MASK)) | CCM_CCOSR_CLKO1_DIV(0);
	/* Set clock out1 source. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO1_SEL_MASK)) | CCM_CCOSR_CLKO1_SEL(1);
	/* Set clock out2 divider. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO2_DIV_MASK)) | CCM_CCOSR_CLKO2_DIV(0);
	/* Set clock out2 source. */
	CCM->CCOSR = (CCM->CCOSR & (~CCM_CCOSR_CLKO2_SEL_MASK)) | CCM_CCOSR_CLKO2_SEL(14);
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
	/* Set SAI2 MCLK3 clock source. */
	IOMUXC_SetSaiMClkClockSource(IOMUXC_GPR, kIOMUXC_GPR_SAI2MClk3Sel, 0);
	/* Set SAI3 MCLK3 clock source. */
	IOMUXC_SetSaiMClkClockSource(IOMUXC_GPR, kIOMUXC_GPR_SAI3MClk3Sel, 0);
	/* Set MQS configuration. */
	IOMUXC_MQSConfig(IOMUXC_GPR, kIOMUXC_MqsPwmOverSampleRate32, 0);
	/* Set GPT1 High frequency reference clock source. */
	IOMUXC_GPR->GPR5 &= ~IOMUXC_GPR_GPR5_VREF_1M_CLK_GPT1_MASK;
	/* Set GPT2 High frequency reference clock source. */
	IOMUXC_GPR->GPR5 &= ~IOMUXC_GPR_GPR5_VREF_1M_CLK_GPT2_MASK;
	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;
}
