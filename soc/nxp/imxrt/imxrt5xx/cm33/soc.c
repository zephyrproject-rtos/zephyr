/*
 * Copyright 2022-2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for NXP RT5XX platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the RT5XX platforms.
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include "fsl_power.h"
#include "fsl_clock.h"
#include <fsl_cache.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
#include "flash_clock_setup.h"
#endif

#if CONFIG_USB_DC_NXP_LPCIP3511
#include "usb_phy.h"
#include "usb.h"
#endif

/* Board System oscillator settling time in us */
#define BOARD_SYSOSC_SETTLING_US 100U
/* Board xtal frequency in Hz */
#define BOARD_XTAL_SYS_CLK_HZ    24000000U
/* Core clock frequency: 198000000Hz */
#define CLOCK_INIT_CORE_CLOCK    198000000U

#define CTIMER_CLOCK_SOURCE(node_id)                                                               \
	TO_CTIMER_CLOCK_SOURCE(DT_CLOCKS_CELL(node_id, name), DT_PROP(node_id, clk_source))
#define TO_CTIMER_CLOCK_SOURCE(inst, val) TO_CLOCK_ATTACH_ID(inst, val)
#define TO_CLOCK_ATTACH_ID(inst, val)     CLKCTL1_TUPLE_MUXA(CT32BIT##inst##FCLKSEL_OFFSET, val)
#define CTIMER_CLOCK_SETUP(node_id)       CLOCK_AttachClk(CTIMER_CLOCK_SOURCE(node_id));

const clock_sys_pll_config_t g_sysPllConfig_clock_init = {
	/* OSC clock */
	.sys_pll_src = kCLOCK_SysPllXtalIn,
	/* Numerator of the SYSPLL0 fractional loop divider is 0 */
	.numerator = 0,
	/* Denominator of the SYSPLL0 fractional loop divider is 1 */
	.denominator = 1,
	/* Divide by 22 */
	.sys_pll_mult = kCLOCK_SysPllMult22};

const clock_audio_pll_config_t g_audioPllConfig_clock_init = {
	/* OSC clock */
	.audio_pll_src = kCLOCK_AudioPllXtalIn,
	/* Numerator of the Audio PLL fractional loop divider is 0 */
	.numerator = 5040,
	/* Denominator of the Audio PLL fractional loop divider is 1 */
	.denominator = 27000,
	/* Divide by 22 */
	.audio_pll_mult = kCLOCK_AudioPllMult22};

const clock_frg_clk_config_t g_frg0Config_clock_init = {
	.num = 0, .sfg_clock_src = kCLOCK_FrgPllDiv, .divider = 255U, .mult = 0};

const clock_frg_clk_config_t g_frg12Config_clock_init = {
	.num = 12, .sfg_clock_src = kCLOCK_FrgMainClk, .divider = 255U, .mult = 167};

#if CONFIG_USB_DC_NXP_LPCIP3511
/* USB PHY configuration */
#define BOARD_USB_PHY_D_CAL     (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif

/* System clock frequency. */
extern uint32_t SystemCoreClock;
/* Main stack pointer */
extern char z_main_stack[];

#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
extern char _flash_used[];

extern void z_arm_reset(void);
extern void z_arm_nmi(void);
extern void z_arm_hard_fault(void);
extern void z_arm_mpu_fault(void);
extern void z_arm_bus_fault(void);
extern void z_arm_usage_fault(void);
extern void z_arm_secure_fault(void);
extern void z_arm_svc(void);
extern void z_arm_debug_monitor(void);
extern void z_arm_pendsv(void);
extern void sys_clock_isr(void);
extern void z_arm_exc_spurious(void);

__imx_boot_ivt_section void (*const image_vector_table[])(void) = {
	(void (*)())(z_main_stack + CONFIG_MAIN_STACK_SIZE), /* 0x00 */
	z_arm_reset,                                         /* 0x04 */
	z_arm_nmi,                                           /* 0x08 */
	z_arm_hard_fault,                                    /* 0x0C */
	z_arm_mpu_fault,                                     /* 0x10 */
	z_arm_bus_fault,                                     /* 0x14 */
	z_arm_usage_fault,                                   /* 0x18 */
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	z_arm_secure_fault, /* 0x1C */
#else
	z_arm_exc_spurious,
#endif                                  /* CONFIG_ARM_SECURE_FIRMWARE */
	(void (*)())_flash_used,        /* 0x20, imageLength. */
	0,                              /* 0x24, imageType (Plain Image) */
	0,                              /* 0x28, authBlockOffset/crcChecksum */
	z_arm_svc,                      /* 0x2C */
	z_arm_debug_monitor,            /* 0x30 */
	(void (*)())image_vector_table, /* 0x34, imageLoadAddress. */
	z_arm_pendsv,                   /* 0x38 */
#if defined(CONFIG_SYS_CLOCK_EXISTS) && defined(CONFIG_CORTEX_M_SYSTICK_INSTALL_ISR)
	sys_clock_isr, /* 0x3C */
#else
	z_arm_exc_spurious,
#endif
};
#endif /* CONFIG_NXP_IMXRT_BOOT_HEADER */

#if CONFIG_USB_DC_NXP_LPCIP3511

static void usb_device_clock_init(void)
{
	uint8_t usbClockDiv = 1;
	uint32_t usbClockFreq;
	usb_phy_config_struct_t phyConfig = {
		BOARD_USB_PHY_D_CAL,
		BOARD_USB_PHY_TXCAL45DP,
		BOARD_USB_PHY_TXCAL45DM,
	};

	/* Make sure USBHS ram buffer and usb1 phy has power up */
	POWER_DisablePD(kPDRUNCFG_APD_USBHS_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_USBHS_SRAM);
	POWER_ApplyPD();

	RESET_PeripheralReset(kUSBHS_PHY_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kUSBHS_DEVICE_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kUSBHS_HOST_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kUSBHS_SRAM_RST_SHIFT_RSTn);

	/* enable usb ip clock */
	CLOCK_EnableUsbHs0DeviceClock(kOSC_CLK_to_USB_CLK, usbClockDiv);
	/* save usb ip clock freq*/
	usbClockFreq = g_xtalFreq / usbClockDiv;
	CLOCK_SetClkDiv(kCLOCK_DivPfc1Clk, 4);
	/* enable usb ram clock */
	CLOCK_EnableClock(kCLOCK_UsbhsSram);
	/* enable USB PHY PLL clock, the phy bus clock (480MHz) source is same with USB IP */
	CLOCK_EnableUsbHs0PhyPllClock(kOSC_CLK_to_USB_CLK, usbClockFreq);

	/* USB PHY initialization */
	USB_EhciPhyInit(kUSB_ControllerLpcIp3511Hs0, BOARD_XTAL_SYS_CLK_HZ, &phyConfig);

#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
	for (int i = 0; i < FSL_FEATURE_USBHSD_USB_RAM; i++) {
		((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS)[i] = 0x00U;
	}
#endif

	/* The following code should run after phy initialization and should wait
	 * some microseconds to make sure utmi clock valid
	 */
	/* enable usb1 host clock */
	CLOCK_EnableClock(kCLOCK_UsbhsHost);
	/*  Wait until host_needclk de-asserts */
	while (SYSCTL0->USB0CLKSTAT & SYSCTL0_USB0CLKSTAT_HOST_NEED_CLKST_MASK) {
		__ASM("nop");
	}
	/* According to reference manual, device mode setting has to be set by access
	 * usb host register
	 */
	USBHSH->PORTMODE |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
	/* disable usb1 host clock */
	CLOCK_DisableClock(kCLOCK_UsbhsHost);
}

#endif

void soc_reset_hook(void)
{
#ifndef CONFIG_NXP_IMXRT_BOOT_HEADER
	/*
	 * If boot did not proceed using a boot header, we should not assume
	 * the core is in reset state. Disable the MPU and correctly
	 * set the stack pointer, since we are about to push to
	 * the stack when we call SystemInit
	 */
	/* Clear stack limit registers */
	__set_MSPLIM(0);
	__set_PSPLIM(0);
	/* Disable MPU */
	MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;
	/* Set stack pointer */
	__set_MSP((uint32_t)(z_main_stack + CONFIG_MAIN_STACK_SIZE));
#endif /* !CONFIG_NXP_IMXRT_BOOT_HEADER */
	/* This is provided by the SDK */
	SystemInit();
}

/* Weak so that board can override with their own clock init routine. */
void __weak rt5xx_clock_init(void)
{
	/* Configure LPOSC 1M */
	/* Power on LPOSC (1MHz) */
	POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
	/* Wait until LPOSC stable */
	CLOCK_EnableLpOscClk();

	/* Configure FRO clock source */
	/* Power on FRO (192MHz or 96MHz) */
	POWER_DisablePD(kPDRUNCFG_PD_FFRO);
	/* FRO_DIV1 is always enabled and used as Main clock during PLL update. */
	/* Enable all FRO outputs */
	CLOCK_EnableFroClk(kCLOCK_FroAllOutEn);

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/*
	 * Call function flexspi_clock_safe_config() to move FlexSPI clock to a stable
	 * clock source to avoid instruction/data fetch issue when updating PLL and Main
	 * clock if XIP(execute code on FLEXSPI memory).
	 */
	flexspi_clock_safe_config();
#endif

	/* Let CPU run on FRO with divider 2 for safe switching. */
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 2);
	CLOCK_AttachClk(kFRO_DIV1_to_MAIN_CLK);

	/* Configure SYSOSC clock source. */
	/* Power on SYSXTAL */
	POWER_DisablePD(kPDRUNCFG_PD_SYSXTAL);
	/* Updated XTAL oscillator settling time */
	POWER_UpdateOscSettlingTime(BOARD_SYSOSC_SETTLING_US);
	/* Enable system OSC */
	CLOCK_EnableSysOscClk(true, true, BOARD_SYSOSC_SETTLING_US);
	/* Sets external XTAL OSC freq */
	CLOCK_SetXtalFreq(BOARD_XTAL_SYS_CLK_HZ);

	/* Configure SysPLL0 clock source. */
	CLOCK_InitSysPll(&g_sysPllConfig_clock_init);
	/* Enable MAIN PLL clock */
	CLOCK_InitSysPfd(kCLOCK_Pfd0, 24);
	/* Enable AUX0 PLL clock */
	CLOCK_InitSysPfd(kCLOCK_Pfd2, 24);

	/* Configure Audio PLL clock source. */
	CLOCK_InitAudioPll(&g_audioPllConfig_clock_init);
	/* Enable Audio PLL clock */
	CLOCK_InitAudioPfd(kCLOCK_Pfd0, 26);

	/* Set SYSCPUAHBCLKDIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 2U);

	/* Setup FRG0 clock */
	CLOCK_SetFRGClock(&g_frg0Config_clock_init);
	/* Setup FRG12 clock */
	CLOCK_SetFRGClock(&g_frg12Config_clock_init);

	/* Set up clock selectors - Attach clocks to the peripheries. */
	/* Switch MAIN_CLK to MAIN_PLL */
	CLOCK_AttachClk(kMAIN_PLL_to_MAIN_CLK);
	/* Switch SYSTICK_CLK to MAIN_CLK_DIV */
	CLOCK_AttachClk(kMAIN_CLK_DIV_to_SYSTICK_CLK);
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay)
#ifdef CONFIG_FLEXCOMM0_CLK_SRC_FRG
	/* Switch FLEXCOMM0 to FRG */
	CLOCK_AttachClk(kFRG_to_FLEXCOMM0);
#elif defined(CONFIG_FLEXCOMM0_CLK_SRC_FRO)
	CLOCK_AttachClk(kFRO_DIV4_to_FLEXCOMM0);
#endif
#endif
#if CONFIG_USB_DC_NXP_LPCIP3511
	usb_device_clock_init();
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_i2s, okay) && CONFIG_I2S)
	/* attach AUDIO PLL clock to FLEXCOMM1 (I2S_PDM) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM0);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay) && CONFIG_I2S)
	/* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay) && CONFIG_I2S)
	/* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
	/* Switch FLEXCOMM4 to FRO_DIV4 */
	CLOCK_AttachClk(kFRO_DIV4_to_FLEXCOMM4);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i3c0), nxp_mcux_i3c, okay)
	/* Attach main clock to I3C */
	CLOCK_AttachClk(kMAIN_CLK_to_I3C_CLK);
	CLOCK_AttachClk(kLPOSC_to_I3C_TC_CLK);

	CLOCK_SetClkDiv(kCLOCK_DivI3cClk,
			DT_PROP(DT_NODELABEL(i3c0), clk_divider));
	CLOCK_SetClkDiv(kCLOCK_DivI3cSlowClk,
			DT_PROP(DT_NODELABEL(i3c0), clk_divider_slow));
	CLOCK_SetClkDiv(kCLOCK_DivI3cTcClk,
			DT_PROP(DT_NODELABEL(i3c0), clk_divider_tc));
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(hs_spi1), nxp_lpc_spi, okay)
	CLOCK_AttachClk(kFRO_DIV4_to_FLEXCOMM16);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm12), nxp_lpc_usart, okay)
	/* Switch FLEXCOMM12 to FRG */
	CLOCK_AttachClk(kFRG_to_FLEXCOMM12);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm12), nxp_lpc_spi, okay)
	/* Switch FLEXCOMM12 to FRG */
	CLOCK_AttachClk(kFRG_to_FLEXCOMM12);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pmic_i2c), nxp_lpc_i2c, okay)
	CLOCK_AttachClk(kFRO_DIV4_to_FLEXCOMM15);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lcdif), nxp_dcnano_lcdif, okay) && CONFIG_DISPLAY
	POWER_DisablePD(kPDRUNCFG_APD_DCNANO_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_DCNANO_SRAM);
	POWER_ApplyPD();

	CLOCK_AttachClk(kAUX0_PLL_to_DCPIXEL_CLK);
	/* Note- pixel clock follows formula
	 * (height + VSW + VFP + VBP) * (width + HSW + HFP + HBP) * frame rate.
	 * this means the clock divider will vary depending on
	 * the attached display.
	 *
	 * The root clock used here is the AUX0 PLL (PLL0 PFD2).
	 */
	CLOCK_SetClkDiv(
		kCLOCK_DivDcPixelClk,
		((CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) /
		  DT_PROP(DT_CHILD(DT_NODELABEL(lcdif), display_timings), clock_frequency)) +
		 1));

	CLOCK_EnableClock(kCLOCK_DisplayCtrl);
	RESET_ClearPeripheralReset(kDISP_CTRL_RST_SHIFT_RSTn);

	CLOCK_EnableClock(kCLOCK_AxiSwitch);
	RESET_ClearPeripheralReset(kAXI_SWITCH_RST_SHIFT_RSTn);
#if defined(CONFIG_MEMC) && DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexspi2), nxp_imx_flexspi, okay)
	/* Enable write-through for FlexSPI1 space */
	CACHE64_POLSEL0->REG1_TOP = 0x27FFFC00U;
	CACHE64_POLSEL0->POLSEL = 0x11U;
#endif
#endif

	/* Switch CLKOUT to FRO_DIV2 */
	CLOCK_AttachClk(kFRO_DIV2_to_CLKOUT);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usdhc0)) && CONFIG_IMX_USDHC
	/* Make sure USDHC ram buffer has been power up*/
	POWER_DisablePD(kPDRUNCFG_APD_USDHC0_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_USDHC0_SRAM);
	POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
	POWER_ApplyPD();

	/* usdhc depend on 32K clock also */
	CLOCK_AttachClk(kLPOSC_DIV32_to_32KHZWAKE_CLK);
	CLOCK_AttachClk(kAUX0_PLL_to_SDIO0_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivSdio0Clk, 1);
	CLOCK_EnableClock(kCLOCK_Sdio0);
	RESET_PeripheralReset(kSDIO0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(smartdma)) && CONFIG_DMA_MCUX_SMARTDMA
	/* Power up SMARTDMA ram */
	POWER_DisablePD(kPDRUNCFG_APD_SMARTDMA_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_SMARTDMA_SRAM);
	POWER_ApplyPD();

	RESET_ClearPeripheralReset(kSMART_DMA_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Smartdma);
#endif

	DT_FOREACH_STATUS_OKAY(nxp_lpc_ctimer, CTIMER_CLOCK_SETUP)

	/* Set up dividers. */
	/* Set AUDIOPLLCLKDIV divider to value 15 */
	CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 15U);
	/* Set FRGPLLCLKDIV divider to value 11 */
	CLOCK_SetClkDiv(kCLOCK_DivPLLFRGClk, 11U);
	/* Set SYSTICKFCLKDIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivSystickClk, 2U);
	/* Set PFC0DIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivPfc0Clk, 2U);
	/* Set PFC1DIV divider to value 4 */
	CLOCK_SetClkDiv(kCLOCK_DivPfc1Clk, 4U);
	/* Set CLKOUTFCLKDIV divider to value 100 */
	CLOCK_SetClkDiv(kCLOCK_DivClockOut, 100U);

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/*
	 * Call function flexspi_setup_clock() to set user configured clock source/divider
	 * for FlexSPI.
	 */
	flexspi_setup_clock(FLEXSPI0, 0U, 2U);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexspi2), nxp_imx_flexspi, okay)
	/* Power up FlexSPI1 SRAM */
	POWER_DisablePD(kPDRUNCFG_APD_FLEXSPI1_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_FLEXSPI1_SRAM);
	POWER_ApplyPD();
	/* Setup clock frequency for FlexSPI1 */
	CLOCK_AttachClk(kMAIN_CLK_to_FLEXSPI1_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivFlexspi1Clk, 1);
	/* Reset peripheral module */
	RESET_PeripheralReset(kFLEXSPI1_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpadc0), nxp_lpc_lpadc, okay)
	SYSCTL0->PDRUNCFG0_CLR = SYSCTL0_PDRUNCFG0_ADC_PD_MASK;
	SYSCTL0->PDRUNCFG0_CLR = SYSCTL0_PDRUNCFG0_ADC_LP_MASK;
	RESET_PeripheralReset(kADC0_RST_SHIFT_RSTn);
	CLOCK_AttachClk(kFRO_DIV4_to_ADC_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivAdcClk, 1);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(dmic0), nxp_dmic, okay)
	/* Using the Audio PLL as input clock leads to better clock dividers
	 * for typical PCM sample rates ({8,16,24,32,48,96} kHz.
	 */
	/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ
	 * Select Audio PLL as clock source. This should produce a bit clock
	 * of 3.072MHZ
	 */
	CLOCK_AttachClk(kAUDIO_PLL_to_DMIC);
	CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);

#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;

	/* Set main clock to FRO as deep sleep clock by default. */
	POWER_SetDeepSleepClock(kDeepSleepClk_Fro);

#if CONFIG_AUDIO_CODEC_WM8904
	/* attach AUDIO PLL clock to MCLK */
	CLOCK_AttachClk(kAUDIO_PLL_to_MCLK_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivMclkClk, 1);
	SYSCTL1->MCLKPINDIR = SYSCTL1_MCLKPINDIR_MCLKPINDIR_MASK;
#endif
}

#if CONFIG_MIPI_DSI
/* Weak so board can override this function */
void __weak imxrt_pre_init_display_interface(void)
{
	/* Assert MIPI DPHY reset. */
	RESET_SetPeripheralReset(kMIPI_DSI_PHY_RST_SHIFT_RSTn);
	POWER_DisablePD(kPDRUNCFG_APD_MIPIDSI_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_MIPIDSI_SRAM);
	POWER_DisablePD(kPDRUNCFG_PD_MIPIDSI);
	POWER_ApplyPD();

	/* RxClkEsc max 60MHz, TxClkEsc 12 to 20MHz. */
	CLOCK_AttachClk(kFRO_DIV1_to_MIPI_DPHYESC_CLK);
	/* RxClkEsc = 192MHz / 4 = 48MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivDphyEscRxClk, 4);
	/* TxClkEsc = 192MHz / 4 / 3 = 16MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivDphyEscTxClk, 3);

	/*
	 * The DPHY bit clock must be fast enough to send out the pixels,
	 * it should be larger than:
	 *
	 *     (Pixel clock * bit per output pixel) / number of MIPI data lane
	 *
	 * DPHY supports up to 895.1MHz bit clock.
	 * We set the divider of the PFD3 output of the SYSPLL, which has a
	 * fixed multiplied of 18, and use this output frequency for the DPHY.
	 */

#ifdef CONFIG_MIPI_DPHY_CLK_SRC_AUX1_PLL
	/* Note: AUX1 PLL clock is system pll clock * 18 / pfd.
	 * system pll clock is configured at 528MHz by default.
	 */
	CLOCK_AttachClk(kAUX1_PLL_to_MIPI_DPHY_CLK);
	CLOCK_InitSysPfd(kCLOCK_Pfd3,
			 ((CLOCK_GetSysPllFreq() * 18ull) /
			  ((unsigned long long)(DT_PROP(DT_NODELABEL(mipi_dsi), phy_clock)))));
	CLOCK_SetClkDiv(kCLOCK_DivDphyClk, 1);
#elif defined(CONFIG_MIPI_DPHY_CLK_SRC_FRO)
	CLOCK_AttachClk(kFRO_DIV1_to_MIPI_DPHY_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivDphyClk,
			(CLK_FRO_CLK / DT_PROP(DT_NODELABEL(mipi_dsi), phy_clock)));
#endif
	/* Clear DSI control reset (Note that DPHY reset is cleared later)*/
	RESET_ClearPeripheralReset(kMIPI_DSI_CTRL_RST_SHIFT_RSTn);
}

void __weak imxrt_post_init_display_interface(void)
{
	/* Deassert MIPI DPHY reset. */
	RESET_ClearPeripheralReset(kMIPI_DSI_PHY_RST_SHIFT_RSTn);
}

void __weak imxrt_deinit_display_interface(void)
{
	/* Assert MIPI DPHY and DSI reset */
	RESET_SetPeripheralReset(kMIPI_DSI_PHY_RST_SHIFT_RSTn);
	RESET_SetPeripheralReset(kMIPI_DSI_CTRL_RST_SHIFT_RSTn);
	/* Remove clock from DPHY */
	CLOCK_AttachClk(kNONE_to_MIPI_DPHY_CLK);
}

#endif

extern void rt5xx_power_init(void);

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 */
void soc_early_init_hook(void)
{
	/* Initialize clocks with tool generated code */
	rt5xx_clock_init();

#ifndef CONFIG_IMXRT5XX_CODE_CACHE
	CACHE64_DisableCache(CACHE64_CTRL0);
#endif

	/* Some ROM versions may have errata leaving these pins in a non-reset state,
	 * which can often cause power leakage on most expected board designs,
	 * restore the reset state here and leave the pin configuration up to board/user DT
	 */
	IOPCTL->PIO[1][15] = 0;
	IOPCTL->PIO[3][28] = 0;
	IOPCTL->PIO[3][29] = 0;
#ifdef CONFIG_PM
	rt5xx_power_init();
#endif

}
