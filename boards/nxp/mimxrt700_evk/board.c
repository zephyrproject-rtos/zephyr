/*
 * Copyright 2024  NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include "fsl_power.h"
#include "fsl_clock.h"
#include <soc.h>
#include <fsl_glikey.h>

/*!< System oscillator settling time in us */
#define SYSOSC_SETTLING_US 220U
/*!< xtal frequency in Hz */
#define XTAL_SYS_CLK_HZ    24000000U

#define SET_UP_FLEXCOMM_CLOCK(x)                                                                   \
	do {                                                                                       \
		CLOCK_AttachClk(kFCCLK0_to_FLEXCOMM##x);                                           \
		RESET_ClearPeripheralReset(kFC##x##_RST_SHIFT_RSTn);                               \
		CLOCK_EnableClock(kCLOCK_LPFlexComm##x);                                           \
	} while (0)

const clock_main_pll_config_t g_mainPllConfig_clock_init = {
	.main_pll_src = kCLOCK_MainPllOscClk, /* OSC clock */
	.numerator = 0,   /* Numerator of the SYSPLL0 fractional loop divider is 0 */
	.denominator = 1, /* Denominator of the SYSPLL0 fractional loop divider is 1 */
	.main_pll_mult = kCLOCK_MainPllMult22 /* Divide by 22 */
};

const clock_audio_pll_config_t g_audioPllConfig_clock_init = {
	.audio_pll_src = kCLOCK_AudioPllOscClk, /* OSC clock */
	.numerator = 5040,    /* Numerator of the Audio PLL fractional loop divider is 0 */
	.denominator = 27000, /* Denominator of the Audio PLL fractional loop divider is 1 */
	.audio_pll_mult = kCLOCK_AudioPllMult22, /* Divide by 22 */
	.enableVcoOut = true};

static void BOARD_InitAHBSC(void);

void board_early_init_hook(void)
{
#if CONFIG_SOC_MIMXRT798S_CM33_CPU0
	const clock_fro_config_t froAutotrimCfg = {
		.targetFreq = 300000000U,
		.range = 50U,
		.trim1DelayUs = 15U,
		.trim2DelayUs = 15U,
		.refDiv = 1U,
		.enableInt = 0U,
		.coarseTrimEn = true,
	};

#ifndef CONFIG_IMXRT7XX_CODE_CACHE
	CACHE64_DisableCache(CACHE64_CTRL0);
#endif

	POWER_DisablePD(kPDRUNCFG_PD_LPOSC);

	/* Power up OSC */
	POWER_DisablePD(kPDRUNCFG_PD_SYSXTAL);
	/* Enable system OSC */
	CLOCK_EnableSysOscClk(true, true, SYSOSC_SETTLING_US);
	/* Sets external XTAL OSC freq */
	CLOCK_SetXtalFreq(XTAL_SYS_CLK_HZ);

	/* Make sure FRO1 is enabled. */
	POWER_DisablePD(kPDRUNCFG_PD_FRO1);

	/* Switch to FRO1 for safe configure. */
	CLOCK_AttachClk(kFRO1_DIV1_to_COMPUTE_BASE);
	CLOCK_AttachClk(kCOMPUTE_BASE_to_COMPUTE_MAIN);
	CLOCK_SetClkDiv(kCLOCK_DivCmptMainClk, 1U);
	CLOCK_AttachClk(kFRO1_DIV1_to_RAM);
	CLOCK_SetClkDiv(kCLOCK_DivComputeRamClk, 1U);
	CLOCK_AttachClk(kFRO1_DIV1_to_COMMON_BASE);
	CLOCK_AttachClk(kCOMMON_BASE_to_COMMON_VDDN);
	CLOCK_SetClkDiv(kCLOCK_DivCommonVddnClk, 1U);

#if CONFIG_FLASH_MCUX_XSPI_XIP
	/* Change to common_base clock(Sourced by FRO1). */
	xspi_clock_safe_config();
#endif

	/* Ungate all FRO clock. */
	POWER_DisablePD(kPDRUNCFG_GATE_FRO0);
	/* Use close loop mode. */
	CLOCK_EnableFroClkFreqCloseLoop(FRO0, &froAutotrimCfg, kCLOCK_FroAllOutEn);
	/* Enable FRO0 MAX clock for all domains.*/
	CLOCK_EnableFro0ClkForDomain(kCLOCK_AllDomainEnable);

	CLOCK_InitMainPll(&g_mainPllConfig_clock_init);
	CLOCK_InitMainPfd(kCLOCK_Pfd0, 20U); /* 475 MHz */
	CLOCK_InitMainPfd(kCLOCK_Pfd1, 24U); /* 396 MHz */
	CLOCK_InitMainPfd(kCLOCK_Pfd2, 18U); /* 528 MHz */
	/* Main PLL kCLOCK_Pfd3 (528 * 18 / 19) = 500 MHz -need 2 div  -> 250 MHz*/
	CLOCK_InitMainPfd(kCLOCK_Pfd3, 19U);

	CLOCK_EnableMainPllPfdClkForDomain(kCLOCK_Pfd0, kCLOCK_AllDomainEnable);
	CLOCK_EnableMainPllPfdClkForDomain(kCLOCK_Pfd1, kCLOCK_AllDomainEnable);
	CLOCK_EnableMainPllPfdClkForDomain(kCLOCK_Pfd2, kCLOCK_AllDomainEnable);
	CLOCK_EnableMainPllPfdClkForDomain(kCLOCK_Pfd3, kCLOCK_AllDomainEnable);

	CLOCK_SetClkDiv(kCLOCK_DivCmptMainClk, 2U);
	CLOCK_AttachClk(kMAIN_PLL_PFD0_to_COMPUTE_MAIN); /* Switch to PLL 237.5 MHz */

	CLOCK_SetClkDiv(kCLOCK_DivMediaMainClk, 2U);
	CLOCK_AttachClk(kMAIN_PLL_PFD0_to_MEDIA_MAIN); /* Switch to PLL 237.5 MHz */

	CLOCK_SetClkDiv(kCLOCK_DivMediaVddnClk, 2U);
	CLOCK_AttachClk(kMAIN_PLL_PFD0_to_MEDIA_VDDN); /* Switch to PLL 237.5 MHz */

	CLOCK_SetClkDiv(kCLOCK_DivComputeRamClk, 2U);
	CLOCK_AttachClk(kMAIN_PLL_PFD0_to_RAM); /* Switch to PLL 237.5 MHz */

	CLOCK_SetClkDiv(kCLOCK_DivCommonVddnClk, 2U);
	CLOCK_AttachClk(kMAIN_PLL_PFD3_to_COMMON_VDDN); /* Switch to 250MHZ */

	/* Configure Audio PLL clock source. */
	CLOCK_InitAudioPll(&g_audioPllConfig_clock_init); /* 532.48MHZ */
	CLOCK_InitAudioPfd(kCLOCK_Pfd1, 24U);             /* 399.36MHz */
	CLOCK_InitAudioPfd(kCLOCK_Pfd3, 26U); /* Enable Audio PLL PFD3 clock to 368.64MHZ */
	CLOCK_EnableAudioPllPfdClkForDomain(kCLOCK_Pfd1, kCLOCK_AllDomainEnable);
	CLOCK_EnableAudioPllPfdClkForDomain(kCLOCK_Pfd3, kCLOCK_AllDomainEnable);

#if CONFIG_FLASH_MCUX_XSPI_XIP
	/* Call function xspi_setup_clock() to set user configured clock for XSPI. */
	xspi_setup_clock(XSPI0, 3U, 1U); /* Main PLL PDF1 DIV1. */
#endif                                      /* CONFIG_FLASH_MCUX_XSPI_XIP */

#elif CONFIG_SOC_MIMXRT798S_CM33_CPU1
	/* Power up OSC in case it's not enabled. */
	POWER_DisablePD(kPDRUNCFG_PD_SYSXTAL);
	/* Enable system OSC */
	CLOCK_EnableSysOscClk(true, true, SYSOSC_SETTLING_US);
	/* Sets external XTAL OSC freq */
	CLOCK_SetXtalFreq(XTAL_SYS_CLK_HZ);

	CLOCK_AttachClk(kFRO1_DIV3_to_SENSE_BASE);
	CLOCK_SetClkDiv(kCLOCK_DivSenseMainClk, 1);
	CLOCK_AttachClk(kSENSE_BASE_to_SENSE_MAIN);

	POWER_DisablePD(kPDRUNCFG_GATE_FRO2);
	CLOCK_EnableFroClkFreq(FRO2, 300000000U, kCLOCK_FroAllOutEn);

	CLOCK_EnableFro2ClkForDomain(kCLOCK_AllDomainEnable);

	CLOCK_AttachClk(kFRO2_DIV3_to_SENSE_BASE);
	CLOCK_SetClkDiv(kCLOCK_DivSenseMainClk, 1);
	CLOCK_AttachClk(kSENSE_BASE_to_SENSE_MAIN);
#endif /* CONFIG_SOC_MIMXRT798S_CM33_CPU0 */

	BOARD_InitAHBSC();

#if DT_NODE_HAS_STATUS(DT_NODELABEL(iocon), okay)
	RESET_ClearPeripheralReset(kIOPCTL0_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Iopctl0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(iocon1), okay)
	RESET_ClearPeripheralReset(kIOPCTL1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Iopctl1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(iocon2), okay)
	RESET_ClearPeripheralReset(kIOPCTL2_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Iopctl2);
#endif

#ifdef CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU0
	CLOCK_AttachClk(kOSC_CLK_to_FCCLK0);
	CLOCK_SetClkDiv(kCLOCK_DivFcclk0Clk, 1U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm0), okay)
	SET_UP_FLEXCOMM_CLOCK(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm1), okay)
	SET_UP_FLEXCOMM_CLOCK(1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm2), okay)
	SET_UP_FLEXCOMM_CLOCK(2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm3), okay)
	SET_UP_FLEXCOMM_CLOCK(3);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm4), okay)
	SET_UP_FLEXCOMM_CLOCK(4);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm5), okay)
	SET_UP_FLEXCOMM_CLOCK(5);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm6), okay)
	SET_UP_FLEXCOMM_CLOCK(6);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm7), okay)
	SET_UP_FLEXCOMM_CLOCK(7);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm8), okay)
	SET_UP_FLEXCOMM_CLOCK(8);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm9), okay)
	SET_UP_FLEXCOMM_CLOCK(9);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm10), okay)
	SET_UP_FLEXCOMM_CLOCK(10);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm11), okay)
	SET_UP_FLEXCOMM_CLOCK(11);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm12), okay)
	SET_UP_FLEXCOMM_CLOCK(12);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm13), okay)
	SET_UP_FLEXCOMM_CLOCK(13);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi14), okay)
	CLOCK_EnableClock(kCLOCK_LPSpi14);
	RESET_ClearPeripheralReset(kLPSPI14_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c15), okay)
	CLOCK_EnableClock(kCLOCK_LPI2c15);
	RESET_ClearPeripheralReset(kLPI2C15_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi16), okay)
	CLOCK_AttachClk(kFRO0_DIV1_to_LPSPI16);
	CLOCK_SetClkDiv(kCLOCK_DivLpspi16Clk, 1U);
	CLOCK_EnableClock(kCLOCK_LPSpi16);
	RESET_ClearPeripheralReset(kLPSPI16_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm17), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM17);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm17Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm18), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM18);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm18Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm19), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM19);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm19Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm20), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM20);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm20Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	CLOCK_EnableClock(kCLOCK_Gpio0);
	RESET_ClearPeripheralReset(kGPIO0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	CLOCK_EnableClock(kCLOCK_Gpio1);
	RESET_ClearPeripheralReset(kGPIO1_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio2), okay)
	CLOCK_EnableClock(kCLOCK_Gpio2);
	RESET_ClearPeripheralReset(kGPIO2_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio3), okay)
	CLOCK_EnableClock(kCLOCK_Gpio3);
	RESET_ClearPeripheralReset(kGPIO3_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio4), okay)
	CLOCK_EnableClock(kCLOCK_Gpio4);
	RESET_ClearPeripheralReset(kGPIO4_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio5), okay)
	CLOCK_EnableClock(kCLOCK_Gpio5);
	RESET_ClearPeripheralReset(kGPIO5_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio6), okay)
	CLOCK_EnableClock(kCLOCK_Gpio6);
	RESET_ClearPeripheralReset(kGPIO6_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio7), okay)
	CLOCK_EnableClock(kCLOCK_Gpio7);
	RESET_ClearPeripheralReset(kGPIO7_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio8), okay)
	CLOCK_EnableClock(kCLOCK_Gpio8);
	RESET_ClearPeripheralReset(kGPIO8_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio9), okay)
	CLOCK_EnableClock(kCLOCK_Gpio9);
	RESET_ClearPeripheralReset(kGPIO9_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio10), okay)
	CLOCK_EnableClock(kCLOCK_Gpio10);
	RESET_ClearPeripheralReset(kGPIO10_RST_SHIFT_RSTn);
#endif
}

static void GlikeyWriteEnable(GLIKEY_Type *base, uint8_t idx)
{
	(void)GLIKEY_SyncReset(base);

	(void)GLIKEY_StartEnable(base, idx);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP1);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP2);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP3);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP_EN);
}

static void GlikeyClearConfig(GLIKEY_Type *base)
{
	(void)GLIKEY_SyncReset(base);
}

/* Disable the secure check for AHBSC and enable periperhals/sram access for masters */
static void BOARD_InitAHBSC(void)
{
#if defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
	GlikeyWriteEnable(GLIKEY0, 1U);
	AHBSC0->MISC_CTRL_DP_REG = 0x000086aa;
	/* AHBSC0 MISC_CTRL_REG, disable Privilege & Secure checking. */
	AHBSC0->MISC_CTRL_REG = 0x000086aa;

	GlikeyWriteEnable(GLIKEY0, 7U);
	/* Enable arbiter0 accessing SRAM */
	AHBSC0->COMPUTE_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->SENSE_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->MEDIA_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->NPU_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->HIFI4_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
#endif

	GlikeyWriteEnable(GLIKEY1, 1U);
	AHBSC3->MISC_CTRL_DP_REG = 0x000086aa;
	/* AHBSC3 MISC_CTRL_REG, disable Privilege & Secure checking.*/
	AHBSC3->MISC_CTRL_REG = 0x000086aa;

	GlikeyWriteEnable(GLIKEY1, 9U);
	/* Enable arbiter1 accessing SRAM */
	AHBSC3->COMPUTE_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->SENSE_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->MEDIA_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->NPU_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->HIFI4_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->HIFI1_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;

	GlikeyWriteEnable(GLIKEY1, 8U);
	/* Access enable for COMPUTE domain masters to common APB peripherals.*/
	AHBSC3->COMPUTE_APB_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;
	AHBSC3->SENSE_APB_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;
	GlikeyWriteEnable(GLIKEY1, 7U);
	AHBSC3->COMPUTE_AIPS_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;
	AHBSC3->SENSE_AIPS_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;

	GlikeyWriteEnable(GLIKEY2, 1U);
	/*Disable secure and secure privilege checking. */
	AHBSC4->MISC_CTRL_DP_REG = 0x000086aa;
	AHBSC4->MISC_CTRL_REG = 0x000086aa;

#if defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
	GlikeyClearConfig(GLIKEY0);
#endif
	GlikeyClearConfig(GLIKEY1);
	GlikeyClearConfig(GLIKEY2);
}
