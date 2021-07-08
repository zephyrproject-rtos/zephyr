/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <fsl_clock.h>
#include <fsl_gpc.h>
#include <fsl_pmu.h>
#include <fsl_dcdc.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <fsl_flexspi_nor_boot.h>
#if CONFIG_USB_DC_NXP_EHCI
#include "usb_phy.h"
#include "usb_dc_mcux.h"
#endif

#if CONFIG_USB_DC_NXP_EHCI /* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif

#ifdef CONFIG_INIT_ARM_PLL
static const clock_arm_pll_config_t armPllConfig = {
	/* Post divider, 0 - DIV by 2, 1 - DIV by 4, 2 - DIV by 8, 3 - DIV by 1 */
	.postDivider = kCLOCK_PllPostDiv2,
	/* PLL Loop divider, Fout = Fin * ( loopDivider / ( 2 * postDivider ) ) */
	.loopDivider = 166,
};
#endif

static const clock_sys_pll2_config_t sysPll2Config = {
	/* Denominator of spread spectrum */
	.mfd = 268435455,
	/* Spread spectrum parameter */
	.ss = NULL,
	/* Enable spread spectrum or not */
	.ssEnable = false,
};

#ifdef CONFIG_INIT_ENET_PLL
static const clock_sys_pll1_config_t sysPll1Config = {
	.pllDiv2En = true,
};
#endif

#ifdef CONFIG_INIT_VIDEO_PLL
static const clock_video_pll_config_t videoPllConfig = {
	/* PLL Loop divider, valid range for DIV_SELECT divider value: 27 ~ 54. */
	.loopDivider = 41,
	/* Divider after PLL, should only be 1, 2, 4, 8, 16, 32 */
	.postDivider = 0,
	/*
	 * 30 bit numerator of fractional loop divider,
	 * Fout = Fin * ( loopDivider + numerator / denominator )
	 */
	.numerator = 1,
	/*
	 * 30 bit denominator of fractional loop divider,
	 * Fout = Fin * ( loopDivider + numerator / denominator )
	 */
	.denominator = 960000,
	/* Spread spectrum parameter */
	.ss = NULL,
	/* Enable spread spectrum or not */
	.ssEnable = false,
};
#endif

#if CONFIG_USB_DC_NXP_EHCI
	usb_phy_config_struct_t usbPhyConfig = {
		BOARD_USB_PHY_D_CAL,
		BOARD_USB_PHY_TXCAL45DP,
		BOARD_USB_PHY_TXCAL45DM,
	};
#endif

#ifdef CONFIG_NXP_IMX_RT_BOOT_HEADER
const __imx_boot_data_section BOOT_DATA_T boot_data = {
	.start = CONFIG_FLASH_BASE_ADDRESS,
	.size = KB(CONFIG_FLASH_SIZE),
	.plugin = PLUGIN_FLAG,
	.placeholder = 0xFFFFFFFF,
};

extern char __start[];
const __imx_boot_ivt_section ivt image_vector_table = {
	.hdr = IVT_HEADER,
	.entry = (uint32_t) __start,
	.reserved1 = IVT_RSVD,
#ifdef CONFIG_DEVICE_CONFIGURATION_DATA
	.dcd = (uint32_t) dcd_data,
#else
	.dcd = (uint32_t) NULL,
#endif
	.boot_data = (uint32_t) &boot_data,
	.self = (uint32_t) &image_vector_table,
	.csf = (uint32_t)CSF_ADDRESS,
	.reserved2 = IVT_RSVD,
};
#endif

/**
 *
 * @brief Initialize the system clock
 *
 * @return N/A
 *
 */
static ALWAYS_INLINE void clock_init(void)
{
	clock_root_config_t rootCfg = {0};

#if CONFIG_ADJUST_DCDC
	DCDC_SetVDD1P0BuckModeTargetVoltage(DCDC, kDCDC_1P0BuckTarget1P15V);
#endif

	/* Check if FBB need to be enabled in OverDrive(OD) mode */
	if (((OCOTP->FUSEN[7].FUSE & 0x10U) >> 4U) != 1) {
		PMU_EnableBodyBias(ANADIG_PMU, kPMU_FBB_CM7, true);
	} else {
		PMU_EnableBodyBias(ANADIG_PMU, kPMU_FBB_CM7, false);
	}

#if CONFIG_BYPASS_LDO_LPSR
	PMU_StaticEnableLpsrAnaLdoBypassMode(ANADIG_LDO_SNVS, true);
	PMU_StaticEnableLpsrDigLdoBypassMode(ANADIG_LDO_SNVS, true);
#endif

#if CONFIG_ADJUST_LDO
	pmu_static_lpsr_ana_ldo_config_t lpsrAnaConfig;
	pmu_static_lpsr_dig_config_t lpsrDigConfig;

	if ((ANADIG_LDO_SNVS->PMU_LDO_LPSR_ANA &
		ANADIG_LDO_SNVS_PMU_LDO_LPSR_ANA_BYPASS_MODE_EN_MASK) == 0UL) {
		PMU_StaticGetLpsrAnaLdoDefaultConfig(&lpsrAnaConfig);
		PMU_StaticLpsrAnaLdoInit(ANADIG_LDO_SNVS, &lpsrAnaConfig);
	}

	if ((ANADIG_LDO_SNVS->PMU_LDO_LPSR_DIG &
		ANADIG_LDO_SNVS_PMU_LDO_LPSR_DIG_BYPASS_MODE_MASK) == 0UL) {
		PMU_StaticGetLpsrDigLdoDefaultConfig(&lpsrDigConfig);
		lpsrDigConfig.targetVoltage = kPMU_LpsrDigTargetStableVoltage1P117V;
		PMU_StaticLpsrDigLdoInit(ANADIG_LDO_SNVS, &lpsrDigConfig);
	}
#endif

	/* PLL LDO shall be enabled first before enable PLLs */

	/* Config CLK_1M */
	CLOCK_OSC_Set1MHzOutputBehavior(kCLOCK_1MHzOutEnableFreeRunning1Mhz);

	/* Init OSC RC 16M */
	ANADIG_OSC->OSC_16M_CTRL |= ANADIG_OSC_OSC_16M_CTRL_EN_IRC4M16M_MASK;

	/* Init OSC RC 400M */
	CLOCK_OSC_EnableOscRc400M();
	CLOCK_OSC_GateOscRc400M(true);

	/* Init OSC RC 48M */
	CLOCK_OSC_EnableOsc48M(true);
	CLOCK_OSC_EnableOsc48MDiv2(true);

	/* Config OSC 24M */
	ANADIG_OSC->OSC_24M_CTRL |= ANADIG_OSC_OSC_24M_CTRL_OSC_EN(1) |
				    ANADIG_OSC_OSC_24M_CTRL_BYPASS_EN(0) |
				    ANADIG_OSC_OSC_24M_CTRL_BYPASS_CLK(0) |
				    ANADIG_OSC_OSC_24M_CTRL_LP_EN(1) |
				    ANADIG_OSC_OSC_24M_CTRL_OSC_24M_GATE(0);

	/* Wait for 24M OSC to be stable. */
	while (ANADIG_OSC_OSC_24M_CTRL_OSC_24M_STABLE_MASK !=
		(ANADIG_OSC->OSC_24M_CTRL & ANADIG_OSC_OSC_24M_CTRL_OSC_24M_STABLE_MASK)) {
	}

	/* Switch both core, M7 Systick and Bus_Lpsr to OscRC48MDiv2 first */
	rootCfg.mux = kCLOCK_M7_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;

#if CONFIG_SOC_MIMXRT1176_CM7
	CLOCK_SetRootClock(kCLOCK_Root_M7, &rootCfg);
	CLOCK_SetRootClock(kCLOCK_Root_M7_Systick, &rootCfg);
#endif

#if CONFIG_SOC_MIMXRT1176_CM4
	CLOCK_SetRootClock(kCLOCK_Root_M4, &rootCfg);
	CLOCK_SetRootClock(kCLOCK_Root_Bus_Lpsr, &rootCfg);
#endif

	/*
	 * If DCD is used, please make sure the clock source of SEMC is not
	 * changed in the following PLL/PFD configuration code.
	 */

#ifdef CONFIG_INIT_ARM_PLL
	/* Init Arm Pll. */
	CLOCK_InitArmPll(&armPllConfig);
#endif

#ifdef CONFIG_INIT_ENET_PLL
	CLOCK_InitSysPll1(&sysPll1Config);
#else
	/* Bypass Sys Pll1. */
	CLOCK_SetPllBypass(kCLOCK_PllSys1, true);

	/* DeInit Sys Pll1. */
	CLOCK_DeinitSysPll1();
#endif

	/* Init Sys Pll2. */
	CLOCK_InitSysPll2(&sysPll2Config);

	/* Init System Pll2 pfd0. */
	CLOCK_InitPfd(kCLOCK_PllSys2, kCLOCK_Pfd0, 27);

	/* Init System Pll2 pfd1. */
	CLOCK_InitPfd(kCLOCK_PllSys2, kCLOCK_Pfd1, 16);

	/* Init System Pll2 pfd2. */
	CLOCK_InitPfd(kCLOCK_PllSys2, kCLOCK_Pfd2, 24);

	/* Init System Pll2 pfd3. */
	CLOCK_InitPfd(kCLOCK_PllSys2, kCLOCK_Pfd3, 32);

	/* Init Sys Pll3. */
	CLOCK_InitSysPll3();

	/* Init System Pll3 pfd0. */
	CLOCK_InitPfd(kCLOCK_PllSys3, kCLOCK_Pfd0, 13);

	/* Init System Pll3 pfd1. */
	CLOCK_InitPfd(kCLOCK_PllSys3, kCLOCK_Pfd1, 17);

	/* Init System Pll3 pfd2. */
	CLOCK_InitPfd(kCLOCK_PllSys3, kCLOCK_Pfd2, 32);

	/* Init System Pll3 pfd3. */
	CLOCK_InitPfd(kCLOCK_PllSys3, kCLOCK_Pfd3, 22);

#ifdef CONFIG_INIT_VIDEO_PLL
	/* Init Video Pll. */
	CLOCK_InitVideoPll(&videoPllConfig);
#endif

	/* Module clock root configurations. */
	/* Configure M7 using ARM_PLL_CLK */
#ifdef CONFIG_SOC_MIMXRT1176_CM7
	rootCfg.mux = kCLOCK_M7_ClockRoot_MuxArmPllOut;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M7, &rootCfg);
#endif

	/* Configure M4 using SYS_PLL3_PFD3_CLK */
#ifdef CONFIG_SOC_MIMXRT1176_CM4
	rootCfg.mux = kCLOCK_M4_ClockRoot_MuxSysPll3Pfd3;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M4, &rootCfg);
#endif

	/* Configure BUS using SYS_PLL3_CLK */
#ifdef CONFIG_SOC_MIMXRT1176_CM7
	rootCfg.mux = kCLOCK_BUS_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 2;
	CLOCK_SetRootClock(kCLOCK_Root_Bus, &rootCfg);
#endif

	/* Configure BUS_LPSR using SYS_PLL3_CLK */
#ifdef CONFIG_SOC_MIMXRT1176_CM4
	rootCfg.mux = kCLOCK_BUS_LPSR_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 3;
	CLOCK_SetRootClock(kCLOCK_Root_Bus_Lpsr, &rootCfg);
#endif

	/* Configure CSSYS using OSC_RC_48M_DIV2 */
	rootCfg.mux = kCLOCK_CSSYS_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Cssys, &rootCfg);

	/* Configure CSTRACE using SYS_PLL2_CLK */
	rootCfg.mux = kCLOCK_CSTRACE_ClockRoot_MuxSysPll2Out;
	rootCfg.div = 4;
	CLOCK_SetRootClock(kCLOCK_Root_Cstrace, &rootCfg);

	/* Configure M4_SYSTICK using OSC_RC_48M_DIV2 */
#ifdef CONFIG_SOC_MIMXRT1176_CM4
	rootCfg.mux = kCLOCK_M4_SYSTICK_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M4_Systick, &rootCfg);
#endif

	/* Configure M7_SYSTICK using OSC_RC_48M_DIV2 */
#ifdef CONFIG_SOC_MIMXRT1176_CM7
	rootCfg.mux = kCLOCK_M7_SYSTICK_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 240;
	CLOCK_SetRootClock(kCLOCK_Root_M7_Systick, &rootCfg);
#endif

#ifdef CONFIG_UART_MCUX_LPUART
	/* Configure Lpuart1 using SysPll2*/
	rootCfg.mux = kCLOCK_LPUART1_ClockRoot_MuxSysPll2Out;
	rootCfg.div = 22;
	CLOCK_SetRootClock(kCLOCK_Root_Lpuart1, &rootCfg);

	/* Configure Lpuart2 using SysPll2*/
	rootCfg.mux = kCLOCK_LPUART2_ClockRoot_MuxSysPll2Out;
	rootCfg.div = 22;
	CLOCK_SetRootClock(kCLOCK_Root_Lpuart2, &rootCfg);
#endif

#ifdef CONFIG_I2C_MCUX_LPI2C
	/* Configure Lpi2c1 using Osc48MDiv2 */
	rootCfg.mux = kCLOCK_LPI2C1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Lpi2c1, &rootCfg);

	/* Configure Lpi2c5 using Osc48MDiv2 */
	rootCfg.mux = kCLOCK_LPI2C5_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Lpi2c5, &rootCfg);
#endif

#ifdef CONFIG_SPI_MCUX_LPSPI
	/* Configure lpspi using Osc48MDiv2 */
	rootCfg.mux = kCLOCK_LPSPI1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Lpspi1, &rootCfg);
#endif

#ifdef CONFIG_COUNTER_MCUX_GPT
	rootCfg.mux = kCLOCK_GPT1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Gpt1, &rootCfg);
#endif

	/* Keep core clock ungated during WFI */
	CCM->GPR_PRIVATE1_SET = 0x1;
	/* Keep the system clock running so SYSTICK can wake up the system from
	 * wfi.
	 */
	GPC_CM_SetNextCpuMode(GPC_CPU_MODE_CTRL_0, kGPC_RunMode);
	GPC_CM_SetNextCpuMode(GPC_CPU_MODE_CTRL_1, kGPC_RunMode);
	GPC_CM_EnableCpuSleepHold(GPC_CPU_MODE_CTRL_0, false);
	GPC_CM_EnableCpuSleepHold(GPC_CPU_MODE_CTRL_1, false);

#ifdef CONFIG_SEGGER_RTT_SECTION_DTCM
	/* Enable the AHB clock while the CM7 is sleeping to allow debug access
	 * to TCM
	 */
	IOMUXC_GPR->GPR16 |= IOMUXC_GPR_GPR16_CM7_FORCE_HCLK_EN_MASK;
#endif
}

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int imxrt_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Disable Systick which might be enabled by bootrom */
	if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}

#if CONFIG_SOC_MIMXRT1176_CM7
	if (SCB_CCR_IC_Msk != (SCB_CCR_IC_Msk & SCB->CCR)) {
		SCB_EnableICache();
	}
	if (SCB_CCR_DC_Msk != (SCB_CCR_DC_Msk & SCB->CCR)) {
		SCB_EnableDCache();
	}
#endif

#if CONFIG_SOC_MIMXRT1176_CM4
	/* Initialize Cache */
	/* Enable Code Bus Cache */
	if (0U == (LMEM->PCCCR & LMEM_PCCCR_ENCACHE_MASK)) {
		/*
		 * set command to invalidate all ways,
		 * and write GO bit to initiate command
		 */
		LMEM->PCCCR |= LMEM_PCCCR_INVW1_MASK
			| LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_GO_MASK;
		/* Wait until the command completes */
		while ((LMEM->PCCCR & LMEM_PCCCR_GO_MASK) != 0U) {
		}
		/* Enable cache, enable write buffer */
		LMEM->PCCCR |= (LMEM_PCCCR_ENWRBUF_MASK
				| LMEM_PCCCR_ENCACHE_MASK);
	}

	/* Enable System Bus Cache */
	if (0U == (LMEM->PSCCR & LMEM_PSCCR_ENCACHE_MASK)) {
		/*
		 * set command to invalidate all ways,
		 * and write GO bit to initiate command
		 */
		LMEM->PSCCR |= LMEM_PSCCR_INVW1_MASK
			| LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_GO_MASK;
		/* Wait until the command completes */
		while ((LMEM->PSCCR & LMEM_PSCCR_GO_MASK) != 0U) {
		}
		/* Enable cache, enable write buffer */
		LMEM->PSCCR |= (LMEM_PSCCR_ENWRBUF_MASK
				| LMEM_PSCCR_ENCACHE_MASK);
	}
#endif

	/* Initialize system clock */
	clock_init();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(imxrt_init, PRE_KERNEL_1, 0);
