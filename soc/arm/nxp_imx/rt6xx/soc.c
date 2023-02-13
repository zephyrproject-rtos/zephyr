/*
 * Copyright (c) 2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc55s69 platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc55s69 platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <aarch32/cortex_m/exc.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
#include "flash_clock_setup.h"
#endif

#if CONFIG_USB_DC_NXP_LPCIP3511
#include "usb_phy.h"
#include "usb.h"
#endif

/* Core clock frequency: 250105263Hz */
#define CLOCK_INIT_CORE_CLOCK                     250105263U

#define SYSTEM_IS_XIP_FLEXSPI() \
	((((uint32_t)nxp_rt600_init >= 0x08000000U) &&		\
	  ((uint32_t)nxp_rt600_init < 0x10000000U)) ||		\
	 (((uint32_t)nxp_rt600_init >= 0x18000000U) &&		\
	  ((uint32_t)nxp_rt600_init < 0x20000000U)))

#define CTIMER_CLOCK_SOURCE(node_id) \
	TO_CTIMER_CLOCK_SOURCE(DT_CLOCKS_CELL(node_id, name), DT_PROP(node_id, clk_source))
#define TO_CTIMER_CLOCK_SOURCE(inst, val) TO_CLOCK_ATTACH_ID(inst, val)
#define TO_CLOCK_ATTACH_ID(inst, val) CLKCTL1_TUPLE_MUXA(CT32BIT##inst##FCLKSEL_OFFSET, val)
#define CTIMER_CLOCK_SETUP(node_id) CLOCK_AttachClk(CTIMER_CLOCK_SOURCE(node_id));

#ifdef CONFIG_INIT_SYS_PLL
const clock_sys_pll_config_t g_sysPllConfig = {
	.sys_pll_src  = kCLOCK_SysPllXtalIn,
	.numerator	  = 0,
	.denominator  = 1,
	.sys_pll_mult = kCLOCK_SysPllMult22
};
#endif

#ifdef CONFIG_INIT_AUDIO_PLL
const clock_audio_pll_config_t g_audioPllConfig = {
	.audio_pll_src	= kCLOCK_AudioPllXtalIn,
	.numerator		= 5040,
	.denominator	= 27000,
	.audio_pll_mult = kCLOCK_AudioPllMult22
};
#endif

#if CONFIG_USB_DC_NXP_LPCIP3511
/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif

/* System clock frequency. */
extern uint32_t SystemCoreClock;

#ifdef CONFIG_NXP_IMX_RT6XX_BOOT_HEADER
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

__imx_boot_ivt_section void (* const image_vector_table[])(void)  = {
	(void (*)())(z_main_stack + CONFIG_MAIN_STACK_SIZE),  /* 0x00 */
	z_arm_reset,				/* 0x04 */
	z_arm_nmi,					/* 0x08 */
	z_arm_hard_fault,			/* 0x0C */
	z_arm_mpu_fault,			/* 0x10 */
	z_arm_bus_fault,			/* 0x14 */
	z_arm_usage_fault,			/* 0x18 */
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	z_arm_secure_fault,			/* 0x1C */
#else
	z_arm_exc_spurious,
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
	(void (*)())_flash_used,	/* 0x20, imageLength. */
	0,				/* 0x24, imageType (Plain Image) */
	0,				/* 0x28, authBlockOffset/crcChecksum */
	z_arm_svc,		/* 0x2C */
	z_arm_debug_monitor,	/* 0x30 */
	(void (*)())image_vector_table,		/* 0x34, imageLoadAddress. */
	z_arm_pendsv,						/* 0x38 */
#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
	defined(CONFIG_CORTEX_M_SYSTICK_INSTALL_ISR)
	sys_clock_isr,						/* 0x3C */
#else
	z_arm_exc_spurious,
#endif
};
#endif /* CONFIG_NXP_IMX_RT6XX_BOOT_HEADER */

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

	/* enable USB IP clock */
	CLOCK_SetClkDiv(kCLOCK_DivPfc1Clk, 5);
	CLOCK_AttachClk(kXTALIN_CLK_to_USB_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivUsbHsFclk, usbClockDiv);
	CLOCK_EnableUsbhsDeviceClock();
	RESET_PeripheralReset(kUSBHS_PHY_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kUSBHS_DEVICE_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kUSBHS_HOST_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kUSBHS_SRAM_RST_SHIFT_RSTn);
	/*Make sure USBHS ram buffer has power up*/
	POWER_DisablePD(kPDRUNCFG_APD_USBHS_SRAM);
	POWER_DisablePD(kPDRUNCFG_PPD_USBHS_SRAM);
	POWER_ApplyPD();

	/* save usb ip clock freq*/
	usbClockFreq = g_xtalFreq / usbClockDiv;
	/* enable USB PHY PLL clock, the phy bus clock (480MHz) source is same with USB IP */
	CLOCK_EnableUsbHs0PhyPllClock(kXTALIN_CLK_to_USB_CLK, usbClockFreq);

#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
	for (int i = 0; i < FSL_FEATURE_USBHSD_USB_RAM; i++) {
		((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS)[i] = 0x00U;
	}
#endif
	USB_EhciPhyInit(kUSB_ControllerLpcIp3511Hs0, CLK_XTAL_OSC_CLK, &phyConfig);

	/* the following code should run after phy initialization and
	 * should wait some microseconds to make sure utmi clock valid
	 */
	/* enable usb1 host clock */
	CLOCK_EnableClock(kCLOCK_UsbhsHost);
	/* Wait until host_needclk de-asserts */
	while (SYSCTL0->USBCLKSTAT & SYSCTL0_USBCLKSTAT_HOST_NEED_CLKST_MASK) {
		__ASM("nop");
	}
	/* According to reference mannual, device mode setting has to be set by
	 * access usb host register
	 */
	USBHSH->PORTMODE |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
	/* disable usb1 host clock */
	CLOCK_DisableClock(kCLOCK_UsbhsHost);
}

#endif

/**
 * @brief Initialize the system clock
 */
static ALWAYS_INLINE void clock_init(void)
{
#ifdef CONFIG_SOC_MIMXRT685S_CM33
	/* Configure LPOSC clock*/
	POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
	/* Configure FFRO clock */
	POWER_DisablePD(kPDRUNCFG_PD_FFRO);
	CLOCK_EnableFfroClk(kCLOCK_Ffro48M);
	/* Configure SFRO clock */
	POWER_DisablePD(kPDRUNCFG_PD_SFRO);
	CLOCK_EnableSfroClk();

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/*
	 * Call function flexspi_clock_safe_config() to move FlexSPI clock to a stable
	 * clock source to avoid instruction/data fetch issue when updating PLL and Main
	 * clock if XIP(execute code on FLEXSPI memory).
	 */
	flexspi_clock_safe_config();
#endif

	/* Let CPU run on FFRO for safe switching. */
	CLOCK_AttachClk(kFFRO_to_MAIN_CLK);

	/* Configure SYSOSC clock source */
	POWER_DisablePD(kPDRUNCFG_PD_SYSXTAL);
	POWER_UpdateOscSettlingTime(CONFIG_SYSOSC_SETTLING_US);
	CLOCK_EnableSysOscClk(true, true, CONFIG_SYSOSC_SETTLING_US);
	CLOCK_SetXtalFreq(CONFIG_XTAL_SYS_CLK_HZ);

#ifdef CONFIG_INIT_SYS_PLL
	/* Configure SysPLL0 clock source */
	CLOCK_InitSysPll(&g_sysPllConfig);
	CLOCK_InitSysPfd(kCLOCK_Pfd0, 19);
	CLOCK_InitSysPfd(kCLOCK_Pfd2, 24);
#endif

#ifdef CONFIG_INIT_AUDIO_PLL
	/* Configure Audio PLL clock source */
	CLOCK_InitAudioPll(&g_audioPllConfig);
	CLOCK_InitAudioPfd(kCLOCK_Pfd0, 26);
	CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 15U);
#endif

	/* Set SYSCPUAHBCLKDIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 2U);

	/* Set up clock selectors - Attach clocks to the peripheries */
	CLOCK_AttachClk(kMAIN_PLL_to_MAIN_CLK);

	/* Set up dividers */
	/* Set PFC0DIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivPfc0Clk, 2U);
	/* Set FRGPLLCLKDIV divider to value 12 */
	CLOCK_SetClkDiv(kCLOCK_DivPllFrgClk, 12U);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM0);
#endif

#if CONFIG_USB_DC_NXP_LPCIP3511
	usb_device_clock_init();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2c, okay)
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pmic_i2c), nxp_lpc_i2c, okay)
	CLOCK_AttachClk(kFFRO_to_FLEXCOMM15);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm5), nxp_lpc_spi, okay)
	CLOCK_AttachClk(kFFRO_to_FLEXCOMM5);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay))
	/* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay))
	/* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt0), nxp_lpc_wwdt, okay))
	CLOCK_AttachClk(kLPOSC_to_WDT0_CLK);
#else
	/* Allowed to select none if not being used for watchdog to
	 * reduce power
	 */
	CLOCK_AttachClk(kNONE_to_WDT0_CLK);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc0), okay) && CONFIG_IMX_USDHC
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

	DT_FOREACH_STATUS_OKAY(nxp_lpc_ctimer, CTIMER_CLOCK_SETUP)

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i3c0), nxp_mcux_i3c, okay))
	CLOCK_AttachClk(kFFRO_to_I3C_CLK);
	CLOCK_AttachClk(kLPOSC_to_I3C_TC_CLK);
#endif

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/*
	 * Call function flexspi_setup_clock() to set user configured clock source/divider
	 * for FlexSPI.
	 */
	flexspi_setup_clock(FLEXSPI, 1U, 9U);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;

#endif /* CONFIG_SOC_MIMXRT685S_CM33 */
}

#if (DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc0), okay) && CONFIG_IMX_USDHC)

void imxrt_usdhc_pinmux(uint16_t nusdhc, bool init,
	uint32_t speed, uint32_t strength)
{

}

void imxrt_usdhc_dat3_pull(bool pullup)
{

}
#endif

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int nxp_rt600_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Enable cache to accelerate boot. */
	if (SYSTEM_IS_XIP_FLEXSPI() && (CACHE64_POLSEL->POLSEL == 0)) {
		/*
		 * Set command to invalidate all ways and write GO bit
		 * to initiate command
		 */
		CACHE64->CCR = (CACHE64_CTRL_CCR_INVW1_MASK |
					CACHE64_CTRL_CCR_INVW0_MASK);
		CACHE64->CCR |= CACHE64_CTRL_CCR_GO_MASK;
		/* Wait until the command completes */
		while (CACHE64->CCR & CACHE64_CTRL_CCR_GO_MASK) {
		}
		/* Enable cache, enable write buffer */
		CACHE64->CCR = (CACHE64_CTRL_CCR_ENWRBUF_MASK |
						CACHE64_CTRL_CCR_ENCACHE_MASK);

		/* Set whole FlexSPI0 space to write through. */
		CACHE64_POLSEL->REG0_TOP = 0x07FFFC00U;
		CACHE64_POLSEL->REG1_TOP = 0x0U;
		CACHE64_POLSEL->POLSEL = 0x1U;

		__ISB();
		__DSB();
	}

	/* Initialize clock */
	clock_init();

	/*
	 * install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(nxp_rt600_init, PRE_KERNEL_1, 0);
