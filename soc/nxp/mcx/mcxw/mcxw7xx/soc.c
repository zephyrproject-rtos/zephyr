/*
 * Copyright 2023-2024,2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <soc.h>

#include <fsl_ccm32k.h>
#include <fsl_common.h>
#include <fsl_clock.h>

extern uint32_t SystemCoreClock;
extern void nxp_nbu_init(void);

#ifdef CONFIG_NXP_MCXW7XX_BOOT_HEADER
extern char z_main_stack[];
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
#endif                                               /* CONFIG_ARM_SECURE_FIRMWARE */
	(void (*)())((uintptr_t)_flash_used),        /* 0x20, imageLength. */
	0,                                           /* 0x24, imageType (Plain Image) */
	0,                                           /* 0x28, authBlockOffset/crcChecksum */
	z_arm_svc,                                   /* 0x2C */
	z_arm_debug_monitor,                         /* 0x30 */
	(void (*)())((uintptr_t)image_vector_table), /* 0x34, imageLoadAddress. */
	z_arm_pendsv,                                /* 0x38 */
#if defined(CONFIG_SYS_CLOCK_EXISTS) && defined(CONFIG_CORTEX_M_SYSTICK_INSTALL_ISR)
	sys_clock_isr, /* 0x3C */
#else
	z_arm_exc_spurious,
#endif
};
#endif /* CONFIG_NXP_MCXW7XX_BOOT_HEADER */

__weak void clock_init(void)
{
#if !defined(FPGA_TARGET) || (FPGA_TARGET == 0)
	/* Unlock Reference Clock Status Registers to allow writes */
	CLOCK_UnlockFircControlStatusReg();
	CLOCK_UnlockSircControlStatusReg();
	CLOCK_UnlockRoscControlStatusReg();
	CLOCK_UnlockSysOscControlStatusReg();

	/*
	 * Configuration for the 32 kHz Oscillator module
	 * Internal capatitor bank is required in order to use the more stable OSC32K source
	 */
	ccm32k_osc_config_t ccm32k_osc_config = {
		.coarseAdjustment = kCCM32K_OscCoarseAdjustmentRange0, /* ESR_Range0 */
		.enableInternalCapBank = true,      /* Internal capacitance bank is enabled */
		.xtalCap = kCCM32K_OscXtal8pFCap,   /* 8 pF */
		.extalCap = kCCM32K_OscExtal8pFCap, /* 8 pF */
	};
	/* Enable OSC32K */
	CCM32K_Set32kOscConfig(CCM32K, kCCM32K_Enable32kHzCrystalOsc, &ccm32k_osc_config);

	/* Disable ROSC Monitor, because switching the source would generate an expected error */
	CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorDisable);

	/* Select the Real Time Clock (RTC) source as OSC32K */
	while ((CCM32K_GetStatusFlag(CCM32K) & CCM32K_STATUS_OSC32K_RDY_MASK) == 0UL) {
	}
	CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectOsc32k);

	/* Wait for RTC Oscillator to be Valid */
	while (!CLOCK_IsRoscValid()) {
	}

	/* Re-enable monitor */
	CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorInt);
	/* Disable the FRO32K to save power */
	CCM32K_Enable32kFro(CCM32K, false);

	CLOCK_SetXtal32Freq(32768U);

	/* Configuration to set FIRC to maximum frequency */
	scg_firc_config_t scg_firc_config = {
		.enableMode = kSCG_FircEnable, /* Fast IRC is enabled */
		.range = kSCG_FircRange96M,    /* 96 Mhz FIRC clock selected */
		.trimConfig = NULL,
	};

	scg_sys_clk_config_t sys_clk_safe_config_source = {
		.divSlow = (uint32_t)kSCG_SysClkDivBy4,
		.divCore = (uint32_t)kSCG_SysClkDivBy1,
		.src = (uint32_t)kSCG_SysClkSrcSirc,
	};

	CLOCK_SetRunModeSysClkConfig(&sys_clk_safe_config_source);

	scg_sys_clk_config_t cur_config;

	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_safe_config_source.src);

	(void)CLOCK_InitFirc(&scg_firc_config);

	scg_sys_clk_config_t sys_clk_config = {
		.divSlow = (uint32_t)kSCG_SysClkDivBy4, /* Slow Clock Divider: divided by 4 */
		.divBus = (uint32_t)kSCG_SysClkDivBy1,  /* Bus Clock Divider: divided by 1 */
		.divCore = (uint32_t)kSCG_SysClkDivBy1, /* Core Clock Divider: divided by 1 */
		.src = (uint32_t)kSCG_SysClkSrcFirc,    /* Select Fast IRC as System Clock */
	};
	CLOCK_SetRunModeSysClkConfig(&sys_clk_config);

	/* Wait for clock source switch to finish */
	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_config.src);

	SystemCoreClock = 96000000U;

	/* OSC-RF / System Oscillator Configuration */
	scg_sosc_config_t sosc_config = {
		.freq = 32000000U,
		.monitorMode = kSCG_SysOscMonitorDisable,
		.enableMode = kSCG_SoscEnable,
	};

	/* Init OSC-RF / SOSC */
	(void)CLOCK_InitSysOsc(&sosc_config);
	CLOCK_SetXtal0Freq(sosc_config.freq);

	/* Slow internal reference clock (SIRC) configuration */
	scg_sirc_config_t sirc_config = {
		.enableMode = kSCG_SircDisableInSleep,
	};

	/* Init SIRC */
	(void)CLOCK_InitSirc(&sirc_config);
#endif

	/* Attach Clocks */
	CLOCK_SetIpSrc(kCLOCK_Lpuart0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpuart1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpspi0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpspi1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Can0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpit0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Tpm0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Tpm1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpi2c0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrcDiv(kCLOCK_Lpi2c0, kSCG_SysClkDivBy16);
	CLOCK_SetIpSrc(kCLOCK_Lpi2c1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrcDiv(kCLOCK_Lpi2c1, kSCG_SysClkDivBy16);
	CLOCK_SetIpSrc(kCLOCK_Lpspi0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpspi1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpadc0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrcDiv(kCLOCK_Lpadc0, kSCG_SysClkDivBy10);
#ifndef CONFIG_SOC_MCXW70AC
	CLOCK_SetIpSrc(kCLOCK_Flexio0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrcDiv(kCLOCK_Flexio0, kSCG_SysClkDivBy6);
#endif

	/* Ungate clocks if the peripheral is enabled in devicetree */
	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(gpioa), nxp_kinetis_gpio, okay)) {
		CLOCK_EnableClock(kCLOCK_PortA);
		CLOCK_EnableClock(kCLOCK_GpioA);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(gpiob), nxp_kinetis_gpio, okay)) {
		CLOCK_EnableClock(kCLOCK_PortB);
		CLOCK_EnableClock(kCLOCK_GpioB);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(gpioc), nxp_kinetis_gpio, okay)) {
		CLOCK_EnableClock(kCLOCK_PortC);
		CLOCK_EnableClock(kCLOCK_GpioC);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart0), nxp_lpuart, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpuart0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart1), nxp_lpuart, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpuart1);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tpm0), nxp_lpit, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpit0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tpm0), nxp_kinetis_tpm, okay)) {
		CLOCK_EnableClock(kCLOCK_Tpm0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tpm1), nxp_kinetis_tpm, okay)) {
		CLOCK_EnableClock(kCLOCK_Tpm1);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpi2c0), nxp_lpi2c, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpi2c0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpi2c1), nxp_lpi2c, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpi2c1);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpspi0), nxp_lpspi, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpspi0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpspi1), nxp_lpspi, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpspi1);
	}

	if (IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN)) {
		CLOCK_EnableClock(kCLOCK_Can0);
	}

#ifndef CONFIG_SOC_MCXW70AC
	/* MCX W70 does not support VREF and FLEXIO */
	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(vref), nxp_vref, okay)) {
		CLOCK_EnableClock(kCLOCK_Vref0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexio), nxp_flexio, okay)) {
		CLOCK_EnableClock(kCLOCK_Flexio0);
	}
#endif

	if (DT_NODE_HAS_COMPAT_STATUS(adc0, nxp_lpadc, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpadc0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(edma), nxp_mcux_edma, okay)) {
		CLOCK_EnableClock(kCLOCK_Dma0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ewm0), nxp_ewm, okay)) {
		CLOCK_EnableClock(kCLOCK_Ewm0);
	}
}

#ifndef CONFIG_SOC_MCXW70AC
static void vbat_init(void)
{
	VBAT_Type *base = (VBAT_Type *)DT_REG_ADDR(DT_NODELABEL(vbat));

	/* Write 1 to Clear POR detect status bit.
	 *
	 * Clearing this bit is acknowledement
	 * that software has recognized a power on reset.
	 *
	 * This avoids also niche issues with NVIC read/write
	 * when searching for available interrupt lines.
	 */
	base->STATUSA |= VBAT_STATUSA_POR_DET_MASK;
};
#endif

void soc_early_init_hook(void)
{
	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Initialize system clock to 96 MHz */
	clock_init();

#ifndef CONFIG_SOC_MCXW70AC
	/* Smart power switch initialization */
	vbat_init();
#endif

	if (IS_ENABLED(CONFIG_PM)) {
		nxp_mcxw7x_power_init();
	}

	/* restore interrupt state */
	irq_unlock(oldLevel);

#if defined(CONFIG_NXP_NBU)
	nxp_nbu_init();
#endif
}
