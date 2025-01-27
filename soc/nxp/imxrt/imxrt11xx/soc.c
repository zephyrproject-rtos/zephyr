/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/cache.h>
#include <fsl_clock.h>
#include <fsl_gpc.h>
#include <fsl_pmu.h>
#include <fsl_dcdc.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
#include <fsl_flexspi_nor_boot.h>
#endif
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>
#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_CPU_CORTEX_M7)
#include <zephyr_image_info.h>
/* Memcpy macro to copy segments from secondary core image stored in flash
 * to RAM section that secondary core boots from.
 * n is the segment number, as defined in zephyr_image_info.h
 */
#define MEMCPY_SEGMENT(n, _)                                                                       \
	memcpy((uint32_t *)((SEGMENT_LMA_ADDRESS_##n) - ADJUSTED_LMA),                             \
	       (uint32_t *)(SEGMENT_LMA_ADDRESS_##n), (SEGMENT_SIZE_##n))
#endif
#if CONFIG_USB_DC_NXP_EHCI
#include "usb_phy.h"
#include "usb.h"
#endif
#include "memc_nxp_flexram.h"

#include <cmsis_core.h>

#define DUAL_CORE_MU_ENABLED (CONFIG_SECOND_CORE_MCUX && CONFIG_IPM && CONFIG_IPM_IMX)

#if DUAL_CORE_MU_ENABLED
/* Dual core mode is enabled, and messaging unit is present */
#include <fsl_mu.h>
#define BOOT_FLAG 0x1U
#define MU_BASE   (MU_Type *)DT_REG_ADDR(DT_INST(0, nxp_imx_mu))
#endif

#if CONFIG_USB_DC_NXP_EHCI /* USB PHY configuration */
#define BOARD_USB_PHY_D_CAL     (0x07U)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif

#ifdef CONFIG_INIT_ARM_PLL

#if defined(CONFIG_SOC_MIMXRT1176)
#define DEFAULT_LOOPDIV 83
#define DEFAULT_POSTDIV 2
#elif defined(CONFIG_SOC_MIMXRT1166)
#define DEFAULT_LOOPDIV 100
#define DEFAULT_POSTDIV 4
#else
/*
 * Check that the ARM PLL has a multiplier and divider set
 */
BUILD_ASSERT(DT_NODE_HAS_PROP(DT_NODELABEL(arm_pll), clock_mult),
	     "ARM PLL must have clock-mult property");
BUILD_ASSERT(DT_NODE_HAS_PROP(DT_NODELABEL(arm_pll), clock_div),
	     "ARM PLL must have clock-div property");
#endif

static const clock_arm_pll_config_t armPllConfig = {
	.postDivider = CONCAT(kCLOCK_PllPostDiv,
			      DT_PROP_OR(DT_NODELABEL(arm_pll), clock_div, DEFAULT_POSTDIV)),
	.loopDivider = DT_PROP_OR(DT_NODELABEL(arm_pll), clock_mult, DEFAULT_LOOPDIV) * 2,
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

#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
const __imx_boot_data_section BOOT_DATA_T boot_data = {
#ifdef CONFIG_XIP
	.start = CONFIG_FLASH_BASE_ADDRESS,
	.size = (uint32_t)&_flash_used,
#else
	.start = CONFIG_SRAM_BASE_ADDRESS,
	.size = (uint32_t)&_image_ram_size,
#endif
	.plugin = PLUGIN_FLAG,
	.placeholder = 0xFFFFFFFF,
};

extern char __start[];
const __imx_boot_ivt_section ivt image_vector_table = {
	.hdr = IVT_HEADER,
	.entry = (uint32_t)__start,
	.reserved1 = IVT_RSVD,
#ifdef CONFIG_DEVICE_CONFIGURATION_DATA
	.dcd = (uint32_t)dcd_data,
#else
	.dcd = (uint32_t)NULL,
#endif
	.boot_data = (uint32_t)&boot_data,
	.self = (uint32_t)&image_vector_table,
	.csf = (uint32_t)CSF_ADDRESS,
	.reserved2 = IVT_RSVD,
};
#endif

/**
 * @brief Initialize the system clock
 */
__weak void clock_init(void)
{
	clock_root_config_t rootCfg = {0};

#if CONFIG_ADJUST_DCDC
	DCDC_SetVDD1P0BuckModeTargetVoltage(DCDC, kDCDC_1P0BuckTarget1P15V);
#endif

/* RT1160 does not have Forward Body Biasing on the CM7 core */
#if defined(CONFIG_SOC_MIMXRT1176_CM4) || defined(CONFIG_SOC_MIMXRT1176_CM7)
	/* Check if FBB need to be enabled in OverDrive(OD) mode */
	if (((OCOTP->FUSEN[7].FUSE & 0x10U) >> 4U) != 1) {
		PMU_EnableBodyBias(ANADIG_PMU, kPMU_FBB_CM7, true);
	} else {
		PMU_EnableBodyBias(ANADIG_PMU, kPMU_FBB_CM7, false);
	}
#endif

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
	ANADIG_OSC->OSC_24M_CTRL |=
		ANADIG_OSC_OSC_24M_CTRL_OSC_EN(1) | ANADIG_OSC_OSC_24M_CTRL_BYPASS_EN(0) |
		ANADIG_OSC_OSC_24M_CTRL_BYPASS_CLK(0) | ANADIG_OSC_OSC_24M_CTRL_LP_EN(1) |
		ANADIG_OSC_OSC_24M_CTRL_OSC_24M_GATE(0);

	/* Wait for 24M OSC to be stable. */
	while (ANADIG_OSC_OSC_24M_CTRL_OSC_24M_STABLE_MASK !=
	       (ANADIG_OSC->OSC_24M_CTRL & ANADIG_OSC_OSC_24M_CTRL_OSC_24M_STABLE_MASK)) {
	}

	rootCfg.div = 1;

#ifdef CONFIG_CPU_CORTEX_M7
	/* Switch both core, M7 Systick and Bus_Lpsr to OscRC48MDiv2 first */
	rootCfg.mux = kCLOCK_M7_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M7, &rootCfg);

	rootCfg.mux = kCLOCK_M7_SYSTICK_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M7_Systick, &rootCfg);
#endif

#if CONFIG_CPU_CORTEX_M4
	rootCfg.mux = kCLOCK_M4_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M4, &rootCfg);

	rootCfg.mux = kCLOCK_BUS_LPSR_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
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
#ifndef CONFIG_SECOND_CORE_MCUX
	/* Bypass Sys Pll1. */
	CLOCK_SetPllBypass(kCLOCK_PllSys1, true);

	/* DeInit Sys Pll1. */
	CLOCK_DeinitSysPll1();
#endif
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
#if defined(CONFIG_SOC_MIMXRT1176_CM7) || defined(CONFIG_SOC_MIMXRT1166_CM7)
	rootCfg.mux = kCLOCK_M7_ClockRoot_MuxArmPllOut;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M7, &rootCfg);
#endif

#if defined(CONFIG_SOC_MIMXRT1166_CM4)
	/* Configure M4 using SYS_PLL3_CLK */
	rootCfg.mux = kCLOCK_M4_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 2;
	CLOCK_SetRootClock(kCLOCK_Root_M4, &rootCfg);
#elif defined(CONFIG_SOC_MIMXRT1176_CM4)
	/* Configure M4 using SYS_PLL3_CLK_PFD3_CLK */
	rootCfg.mux = kCLOCK_M4_ClockRoot_MuxSysPll3Pfd3;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M4, &rootCfg);
#endif

#if CONFIG_SOC_MIMXRT1176_CM7
	/* Keep root bus clock at default 240M */
	rootCfg.mux = kCLOCK_BUS_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 2;
	CLOCK_SetRootClock(kCLOCK_Root_Bus, &rootCfg);
#elif defined(CONFIG_SOC_MIMXRT1166_CM7)
	/* Configure root bus clock at 198M */
	rootCfg.mux = kCLOCK_BUS_ClockRoot_MuxSysPll2Pfd3;
	rootCfg.div = 2;
	CLOCK_SetRootClock(kCLOCK_Root_Bus, &rootCfg);
#endif

	/* Configure BUS_LPSR using SYS_PLL3_CLK */
#if defined(CONFIG_SOC_MIMXRT1176_CM4)
	rootCfg.mux = kCLOCK_BUS_LPSR_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 3;
	CLOCK_SetRootClock(kCLOCK_Root_Bus_Lpsr, &rootCfg);
#elif defined(CONFIG_SOC_MIMXRT1166_CM4)
	rootCfg.mux = kCLOCK_BUS_LPSR_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 4;
	CLOCK_SetRootClock(kCLOCK_Root_Bus_Lpsr, &rootCfg);
#elif defined(CONFIG_SOC_MIMXRT1176_CM7) || defined(CONFIG_SOC_MIMXRT1166_CM7)
	rootCfg.mux = kCLOCK_BUS_LPSR_ClockRoot_MuxSysPll3Out;
	rootCfg.div = 2;
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
#if defined(CONFIG_SOC_MIMXRT1176_CM4) || defined(CONFIG_SOC_MIMXRT1166_CM4)
	rootCfg.mux = kCLOCK_M4_SYSTICK_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_M4_Systick, &rootCfg);
#endif

	/* Configure M7_SYSTICK using OSC_RC_48M_DIV2 */
#if defined(CONFIG_SOC_MIMXRT1176_CM7) || defined(CONFIG_SOC_MIMXRT1166_CM7)
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

	/* Configure Lpi2c6 using Osc24M */
	rootCfg.mux = kCLOCK_LPI2C6_ClockRoot_MuxOsc24MOut;
	rootCfg.div = 12;
	CLOCK_SetRootClock(kCLOCK_Root_Lpi2c6, &rootCfg);
#endif

#if CONFIG_ETH_MCUX || CONFIG_ETH_NXP_ENET
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet))
	/* 50 MHz ENET clock */
	rootCfg.mux = kCLOCK_ENET1_ClockRoot_MuxSysPll1Div2;
	rootCfg.div = 10;
	CLOCK_SetRootClock(kCLOCK_Root_Enet1, &rootCfg);
#if CONFIG_ETH_MCUX_RMII_EXT_CLK
	/* Set ENET_REF_CLK as an input driven by PHY */
	IOMUXC_GPR->GPR4 &= ~IOMUXC_GPR_GPR4_ENET_REF_CLK_DIR(0x01U);
	IOMUXC_GPR->GPR4 |= IOMUXC_GPR_GPR4_ENET_TX_CLK_SEL(0x1U);
#else
	/* Set ENET_REF_CLK as an output driven by ENET1_CLK_ROOT */
	IOMUXC_GPR->GPR4 |=
		(IOMUXC_GPR_GPR4_ENET_REF_CLK_DIR(0x01U) | IOMUXC_GPR_GPR4_ENET_TX_CLK_SEL(0x1U));
#endif
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet1g))
	rootCfg.mux = kCLOCK_ENET2_ClockRoot_MuxSysPll1Div2;
#if DT_ENUM_HAS_VALUE(DT_CHILD(DT_NODELABEL(enet1g), ethernet), phy_connection_type, rgmii)
	/* 125 MHz ENET1G clock */
	rootCfg.div = 4;
	CLOCK_SetRootClock(kCLOCK_Root_Enet2, &rootCfg);
	/* Set ENET1G TX_CLK to be driven by ENET2_CLK_ROOT and output on TX_CLK_IO pad */
	IOMUXC_GPR->GPR5 = (IOMUXC_GPR_GPR5_ENET1G_RGMII_EN(0x01U) |
			    (IOMUXC_GPR->GPR5 & ~IOMUXC_GPR_GPR5_ENET1G_TX_CLK_SEL(0x01U)));
	/* Set ENET1G_REF_CLK as an input driven by PHY */
	IOMUXC_GPR->GPR5 &= ~IOMUXC_GPR_GPR5_ENET1G_REF_CLK_DIR(0x01U);
#else
	/*
	 * 50 MHz clock for 10/100Mbit RMII PHY -
	 * operate ENET1G just like ENET peripheral
	 */
	rootCfg.div = 10;
	CLOCK_SetRootClock(kCLOCK_Root_Enet2, &rootCfg);
#if CONFIG_ETH_MCUX_RMII_EXT_CLK
	/* Set ENET1G_REF_CLK as an input driven by PHY */
	IOMUXC_GPR->GPR5 &= ~IOMUXC_GPR_GPR5_ENET1G_REF_CLK_DIR(0x01U);
	IOMUXC_GPR->GPR5 |= IOMUXC_GPR_GPR5_ENET1G_TX_CLK_SEL(0x1U);
#else
	/* Set ENET1G_REF_CLK as an output driven by ENET2_CLK_ROOT */
	IOMUXC_GPR->GPR5 |= (IOMUXC_GPR_GPR5_ENET1G_REF_CLK_DIR(0x01U) |
			     IOMUXC_GPR_GPR5_ENET1G_TX_CLK_SEL(0x1U));
#endif
#endif
#endif
#endif

#if defined(CONFIG_PTP_CLOCK_MCUX) || defined(CONFIG_PTP_CLOCK_NXP_ENET)
	/* 24MHz PTP clock */
	rootCfg.mux = kCLOCK_ENET_TIMER1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Enet_Timer1, &rootCfg);
#endif

#ifdef CONFIG_SPI_MCUX_LPSPI
	/* Configure lpspi using Osc48MDiv2 */
	rootCfg.mux = kCLOCK_LPSPI1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Lpspi1, &rootCfg);
#endif

#ifdef CONFIG_VIDEO_MCUX_MIPI_CSI2RX
	/* MIPI CSI-2 Rx connects to CSI via Video Mux */
	CLOCK_EnableClock(kCLOCK_Video_Mux);
	VIDEO_MUX->VID_MUX_CTRL.SET = VIDEO_MUX_VID_MUX_CTRL_CSI_SEL_MASK;

	/* Enable power domain for MIPI CSI-2 */
	PGMC_BPC4->BPC_POWER_CTRL |= (PGMC_BPC_BPC_POWER_CTRL_PSW_ON_SOFT_MASK |
				      PGMC_BPC_BPC_POWER_CTRL_ISO_OFF_SOFT_MASK);
#endif

#ifdef CONFIG_CAN_MCUX_FLEXCAN
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan1))
	/* Configure CAN1 using Osc48MDiv2 */
	rootCfg.mux = kCLOCK_CAN1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Can1, &rootCfg);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan3))
	/* Configure CAN1 using Osc48MDiv2 */
	rootCfg.mux = kCLOCK_CAN3_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Can3, &rootCfg);
#endif
#endif

#ifdef CONFIG_MCUX_ACMP
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(acmp1))
	/* Configure ACMP1 using Osc48MDiv2*/
	rootCfg.mux = kCLOCK_ACMP_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Acmp, &rootCfg);
#endif
#endif

#ifdef CONFIG_DISPLAY_MCUX_ELCDIF
	rootCfg.mux = kCLOCK_LCDIF_ClockRoot_MuxSysPll2Out;
	/*
	 * PLL2 is fixed at 528MHz. Use desired panel clock clock to
	 * calculate LCDIF clock.
	 */
	rootCfg.div = ((SYS_PLL2_FREQ /
			DT_PROP(DT_CHILD(DT_NODELABEL(lcdif), display_timings), clock_frequency)) +
		       1);
	CLOCK_SetRootClock(kCLOCK_Root_Lcdif, &rootCfg);
#endif

#ifdef CONFIG_COUNTER_MCUX_GPT
	rootCfg.mux = kCLOCK_GPT1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Gpt1, &rootCfg);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb1)) && (CONFIG_USB_DC_NXP_EHCI || CONFIG_UDC_NXP_EHCI)
	CLOCK_EnableUsbhs0PhyPllClock(
		kCLOCK_Usb480M, DT_PROP_BY_PHANDLE(DT_NODELABEL(usb1), clocks, clock_frequency));
	CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M,
				DT_PROP_BY_PHANDLE(DT_NODELABEL(usb1), clocks, clock_frequency));
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb1)) && CONFIG_USB_DC_NXP_EHCI
	USB_EhciPhyInit(kUSB_ControllerEhci0, CPU_XTAL_CLK_HZ, &usbPhyConfig);
#endif
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb2)) && (CONFIG_USB_DC_NXP_EHCI || CONFIG_UDC_NXP_EHCI)
	CLOCK_EnableUsbhs1PhyPllClock(
		kCLOCK_Usb480M, DT_PROP_BY_PHANDLE(DT_NODELABEL(usb2), clocks, clock_frequency));
	CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M,
				DT_PROP_BY_PHANDLE(DT_NODELABEL(usb2), clocks, clock_frequency));
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb2)) && CONFIG_USB_DC_NXP_EHCI
	USB_EhciPhyInit(kUSB_ControllerEhci1, CPU_XTAL_CLK_HZ, &usbPhyConfig);
#endif
#endif

#if CONFIG_IMX_USDHC
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usdhc1))
	/* Configure USDHC1 using  SysPll2Pfd2*/
	rootCfg.mux = kCLOCK_USDHC1_ClockRoot_MuxSysPll2Pfd2;
	rootCfg.div = 2;
	CLOCK_SetRootClock(kCLOCK_Root_Usdhc1, &rootCfg);
	CLOCK_EnableClock(kCLOCK_Usdhc1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usdhc2))
	/* Configure USDHC2 using  SysPll2Pfd2*/
	rootCfg.mux = kCLOCK_USDHC2_ClockRoot_MuxSysPll2Pfd2;
	rootCfg.div = 2;
	CLOCK_SetRootClock(kCLOCK_Root_Usdhc2, &rootCfg);
	CLOCK_EnableClock(kCLOCK_Usdhc2);
#endif
#endif

#if !(DT_NODE_HAS_COMPAT(DT_PARENT(DT_CHOSEN(zephyr_flash)), nxp_imx_flexspi)) &&  \
	defined(CONFIG_MEMC_MCUX_FLEXSPI) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexspi))
	/* Configure FLEXSPI1 using OSC_RC_48M_DIV2 */
	rootCfg.mux = kCLOCK_FLEXSPI1_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Flexspi1, &rootCfg);
#endif

#if !(DT_NODE_HAS_COMPAT(DT_PARENT(DT_CHOSEN(zephyr_flash)), nxp_imx_flexspi)) &&  \
	defined(CONFIG_MEMC_MCUX_FLEXSPI) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexspi2))
	/* Configure FLEXSPI2 using OSC_RC_48M_DIV2 */
	rootCfg.mux = kCLOCK_FLEXSPI2_ClockRoot_MuxOscRc48MDiv2;
	rootCfg.div = 1;
	CLOCK_SetRootClock(kCLOCK_Root_Flexspi2, &rootCfg);
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

#if !defined(CONFIG_PM)
	/* Enable the AHB clock while the CM7 is sleeping to allow debug access
	 * to TCM
	 */
	IOMUXC_GPR->GPR16 |= IOMUXC_GPR_GPR16_CM7_FORCE_HCLK_EN_MASK;
#endif
}

#if CONFIG_I2S_MCUX_SAI
void imxrt_audio_codec_pll_init(uint32_t clock_name, uint32_t clk_src, uint32_t clk_pre_div,
				uint32_t clk_src_div)
{
	ARG_UNUSED(clk_pre_div);

	switch (clock_name) {
	case IMX_CCM_SAI1_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai1, clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai1, clk_src_div);
		break;
	case IMX_CCM_SAI2_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai2, clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai2, clk_src_div);
		break;
	case IMX_CCM_SAI3_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai3, clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai3, clk_src_div);
		break;
	case IMX_CCM_SAI4_CLK:
		CLOCK_SetRootClockMux(kCLOCK_Root_Sai4, clk_src);
		CLOCK_SetRootClockDiv(kCLOCK_Root_Sai4, clk_src_div);
		break;
	default:
		return;
	}
}
#endif

#if CONFIG_MIPI_DSI
void imxrt_pre_init_display_interface(void)
{
	/* elcdif output to MIPI DSI */
	CLOCK_EnableClock(kCLOCK_Video_Mux);
	VIDEO_MUX->VID_MUX_CTRL.CLR = VIDEO_MUX_VID_MUX_CTRL_MIPI_DSI_SEL_MASK;

	/* Power on and isolation off. */
	PGMC_BPC4->BPC_POWER_CTRL |= (PGMC_BPC_BPC_POWER_CTRL_PSW_ON_SOFT_MASK |
				      PGMC_BPC_BPC_POWER_CTRL_ISO_OFF_SOFT_MASK);

	/* Assert MIPI reset. */
	IOMUXC_GPR->GPR62 &= ~(IOMUXC_GPR_GPR62_MIPI_DSI_PCLK_SOFT_RESET_N_MASK |
			       IOMUXC_GPR_GPR62_MIPI_DSI_ESC_SOFT_RESET_N_MASK |
			       IOMUXC_GPR_GPR62_MIPI_DSI_BYTE_SOFT_RESET_N_MASK |
			       IOMUXC_GPR_GPR62_MIPI_DSI_DPI_SOFT_RESET_N_MASK);

	/* setup clock */
	const clock_root_config_t mipiEscClockConfig = {
		.clockOff = false,
		.mux = 4,
		.div = 11,
	};

	CLOCK_SetRootClock(kCLOCK_Root_Mipi_Esc, &mipiEscClockConfig);

	/* TX esc clock */
	const clock_group_config_t mipiEscClockGroupConfig = {
		.clockOff = false,
		.resetDiv = 2,
		.div0 = 2,
	};

	CLOCK_SetGroupConfig(kCLOCK_Group_MipiDsi, &mipiEscClockGroupConfig);

	const clock_root_config_t mipiDphyRefClockConfig = {
		.clockOff = false,
		.mux = 1,
		.div = 1,
	};

	CLOCK_SetRootClock(kCLOCK_Root_Mipi_Ref, &mipiDphyRefClockConfig);

	/* Deassert PCLK and ESC reset. */
	IOMUXC_GPR->GPR62 |= (IOMUXC_GPR_GPR62_MIPI_DSI_PCLK_SOFT_RESET_N_MASK |
			      IOMUXC_GPR_GPR62_MIPI_DSI_ESC_SOFT_RESET_N_MASK);
}

void imxrt_post_init_display_interface(void)
{
	/* deassert BYTE and DBI reset */
	IOMUXC_GPR->GPR62 |= (IOMUXC_GPR_GPR62_MIPI_DSI_BYTE_SOFT_RESET_N_MASK |
			      IOMUXC_GPR_GPR62_MIPI_DSI_DPI_SOFT_RESET_N_MASK);
}

#endif

#if CONFIG_VIDEO_MCUX_MIPI_CSI2RX
int mipi_csi2rx_clock_set_freq(clock_root_t clock_root, uint32_t rate)
{
	clock_root_config_t rootCfg = {0};
	uint32_t freq;
	clock_name_t clk_source;

	switch (clock_root) {
	case kCLOCK_Root_Csi2:
		rootCfg.mux = kCLOCK_CSI2_ClockRoot_MuxSysPll3Out;
		break;
	case kCLOCK_Root_Csi2_Esc:
		rootCfg.mux = kCLOCK_CSI2_ESC_ClockRoot_MuxSysPll3Out;
		break;
	case kCLOCK_Root_Csi2_Ui:
		rootCfg.mux = kCLOCK_CSI2_UI_ClockRoot_MuxSysPll3Out;
		break;
	default:
		return -EINVAL;
	}

	clk_source = CLOCK_GetRootClockSource(clock_root, rootCfg.mux);
	freq = CLOCK_GetFreq(clk_source);
	if (rate > freq) {
		LOG_ERR("Requested rate is higher than the maximum clock frequency");
		return -EINVAL;
	}

	rootCfg.div = (uint32_t)freq / rate;
	CLOCK_SetRootClock(clock_root, &rootCfg);

	return 0;
}
#endif

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 * If dual core operation is enabled, the second core image will be loaded to RAM
 *
 * @return 0
 */

static int imxrt_init(void)
{
#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_CPU_CORTEX_M7)
	/**
	 * Copy CM4 core from flash to memory. Note that depending on where the
	 * user decided to store CM4 code, this is likely going to read from the
	 * flexspi while using XIP. Provided we DO NOT WRITE TO THE FLEXSPI,
	 * this operation is safe.
	 *
	 * Note that this copy MUST occur before enabling the M7 caching to
	 * ensure the data is written directly to RAM (since the M4 core will use it)
	 */
	LISTIFY(SEGMENT_NUM, MEMCPY_SEGMENT, (;));
	/* Set the boot address for the second core */
	uint32_t boot_address = (uint32_t)(DT_REG_ADDR(DT_CHOSEN(zephyr_cpu1_region)));
	/* Set VTOR for the CM4 core */
	IOMUXC_LPSR_GPR->GPR0 = IOMUXC_LPSR_GPR_GPR0_CM4_INIT_VTOR_LOW(boot_address >> 3u);
	IOMUXC_LPSR_GPR->GPR1 = IOMUXC_LPSR_GPR_GPR1_CM4_INIT_VTOR_HIGH(boot_address >> 16u);
#endif

#if DUAL_CORE_MU_ENABLED && CONFIG_CPU_CORTEX_M4
	/* Set boot flag in messaging unit to indicate boot to primary core */
	MU_SetFlags(MU_BASE, BOOT_FLAG);
#endif

#if defined(CONFIG_SOC_MIMXRT1176_CM7) || defined(CONFIG_SOC_MIMXRT1166_CM7)
	sys_cache_instr_enable();
	sys_cache_data_enable();
#endif

	/* Initialize system clock */
	clock_init();

	return 0;
}

/*
 * Stack pointer is not set at this point in the early init, but we call C
 * functions from the SOC reset.
 * Set a stack pointer so that C functions will work correctly
 */

#ifdef CONFIG_SOC_RESET_HOOK
__asm__ (
	".global soc_reset_hook\n"
	"soc_reset_hook:\n"
	"ldr r0, =z_main_stack+"STRINGIFY(CONFIG_MAIN_STACK_SIZE)";\n"
	"msr msp, r0;\n"
	"b _soc_reset_hook;\n"
);

void __used _soc_reset_hook(void)
{
	SystemInit();

#if defined(FLEXRAM_RUNTIME_BANKS_USED)
	/* Configure flexram if not running from RAM */
	memc_flexram_dt_partition();
#endif
}
#endif

SYS_INIT(imxrt_init, PRE_KERNEL_1, 0);

#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_CPU_CORTEX_M7)
/**
 * @brief Kickoff secondary core.
 *
 * Kick the secondary core out of reset and wait for it to indicate boot. The
 * core image was already copied to RAM (and the boot address was set) in
 * imxrt_init()
 *
 * @return 0
 */
static int second_core_boot(void)
{
	/* Kick CM4 core out of reset */
	SRC->CTRL_M4CORE = SRC_CTRL_M4CORE_SW_RESET_MASK;
	SRC->SCR |= SRC_SCR_BT_RELEASE_M4_MASK;
#if DUAL_CORE_MU_ENABLED
	/* Wait for the secondary core to start up and set boot flag in
	 * imxrt_init
	 */
	while (MU_GetFlags(MU_BASE) != BOOT_FLAG) {
		/* Wait for secondary core to set flag */
	}
#endif
	return 0;
}

SYS_INIT(second_core_boot, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
