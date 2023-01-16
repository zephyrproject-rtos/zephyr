/*
 * Copyright (c) 2017, NXP
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
#ifdef CONFIG_GPIO_MCUX_LPC
#include <fsl_pint.h>
#endif
#if CONFIG_USB_DC_NXP_LPCIP3511
#include "usb_phy.h"
#include "usb.h"
#endif

#define CTIMER_CLOCK_SOURCE(node_id) \
	TO_CTIMER_CLOCK_SOURCE(DT_CLOCKS_CELL(node_id, name), DT_PROP(node_id, clk_source))
#define TO_CTIMER_CLOCK_SOURCE(inst, val) TO_CLOCK_ATTACH_ID(inst, val)
#define TO_CLOCK_ATTACH_ID(inst, val) MUX_A(CM_CTIMERCLKSEL##inst, val)
#define CTIMER_CLOCK_SETUP(node_id) CLOCK_AttachClk(CTIMER_CLOCK_SOURCE(node_id));

#ifdef CONFIG_INIT_PLL0
const pll_setup_t pll0Setup = {
	.pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(2U) |
		   SYSCON_PLL0CTRL_SELP(31U),
	.pllndec = SYSCON_PLL0NDEC_NDIV(125U),
	.pllpdec = SYSCON_PLL0PDEC_PDIV(8U),
	.pllsscg = {0x0U, (SYSCON_PLL0SSCG1_MDIV_EXT(3072U) | SYSCON_PLL0SSCG1_SEL_EXT_MASK)},
	.pllRate = 24576000U,
	.flags = PLL_SETUPFLAG_WAITLOCK}
;
#endif

/**
 *
 * @brief Initialize the system clock
 *
 */

static ALWAYS_INLINE void clock_init(void)
{

#if defined(CONFIG_SOC_LPC55S36)
	/* Power Management Controller initialization */
	POWER_PowerInit();
#endif

#if defined(CONFIG_SOC_LPC55S06) || defined(CONFIG_SOC_LPC55S16) || \
	defined(CONFIG_SOC_LPC55S28) || defined(CONFIG_SOC_LPC55S36) || \
	defined(CONFIG_SOC_LPC55S69_CPU0)
    /*!< Set up the clock sources */
    /*!< Configure FRO192M */
	/*!< Ensure FRO is on  */
	POWER_DisablePD(kPDRUNCFG_PD_FRO192M);
	/*!< Set up FRO to the 12 MHz, just for sure */
	CLOCK_SetupFROClocking(12000000U);
	/*!< Switch to FRO 12MHz first to ensure we can change the clock */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Enable FRO HF(96MHz) output */
	CLOCK_SetupFROClocking(96000000U);

#ifdef CONFIG_INIT_PLL0
	/*!< Ensure XTAL16M is on  */
	PMC->PDRUNCFGCLR0 |= PMC_PDRUNCFG0_PDEN_XTAL32M_MASK;
	PMC->PDRUNCFGCLR0 |= PMC_PDRUNCFG0_PDEN_LDOXO32M_MASK;

	/*!< Ensure CLK_IN is on  */
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK;
	ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;

	/*!< Switch PLL0 clock source selector to XTAL16M */
	CLOCK_AttachClk(kEXT_CLK_to_PLL0);

	/*!< Configure PLL to the desired values */
	CLOCK_SetPLL0Freq(&pll0Setup);

	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 1U, false);
#endif

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/*!< Set FLASH wait states for core */
	CLOCK_SetFLASHAccessCyclesForFreq(96000000U);
#endif

    /*!< Set up dividers */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

    /*!< Set up clock selectors - Attach clocks to the peripheries */
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);

	/* Enables the clock for the I/O controller.: Enable Clock. */
    CLOCK_EnableClock(kCLOCK_Iocon);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_usart, okay)
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1U, false);
#endif
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1U, false);
#endif
	/* attach 12 MHz clock to FLEXCOMM4 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);

	/* reset FLEXCOMM for I2C */
	RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hs_lspi), okay)
	/* Attach 12 MHz clock to HSLSPI */
	CLOCK_AttachClk(kFRO_HF_DIV_to_HSLSPI);

	/* reset HSLSPI for SPI */
	RESET_PeripheralReset(kHSLSPI_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt0), nxp_lpc_wwdt, okay)
	/* Enable 1 MHz FRO clock for WWDT */
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mailbox0), nxp_lpc_mailbox, okay)
	CLOCK_EnableClock(kCLOCK_Mailbox);
	/* Reset the MAILBOX module */
	RESET_PeripheralReset(kMAILBOX_RST_SHIFT_RSTn);
#endif

#if CONFIG_USB_DC_NXP_LPCIP3511
	/* enable usb1 host clock */
	CLOCK_EnableClock(kCLOCK_Usbh1);
	/* Put PHY powerdown under software control */
	*((uint32_t *)(USBHSH_BASE + 0x50)) = USBHSH_PORTMODE_SW_PDCOM_MASK;
	/*
	 * According to reference manual, device mode setting has to be set by
	 * access usb host register
	 */
	*((uint32_t *)(USBHSH_BASE + 0x50)) |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
	/* enable usb1 host clock */
	CLOCK_DisableClock(kCLOCK_Usbh1);

	/* enable USB IP clock */
	CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_UsbPhySrcExt, CLK_CLK_IN);
	CLOCK_EnableUsbhs0DeviceClock(kCLOCK_UsbSrcUnused, 0U);
	USB_EhciPhyInit(kUSB_ControllerLpcIp3511Hs0, CLK_CLK_IN, NULL);
#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
	for (int i = 0; i < FSL_FEATURE_USBHSD_USB_RAM; i++) {
		((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS)[i] = 0x00U;
	}
#endif

#endif

DT_FOREACH_STATUS_OKAY(nxp_lpc_ctimer, CTIMER_CLOCK_SETUP)

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm6), nxp_lpc_i2s, okay))
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom6Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom6Clk, 1U, false);
#endif
	/* attach PLL0 clock to FLEXCOMM6 */
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM6);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm7), nxp_lpc_i2s, okay))
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom7Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom7Clk, 1U, false);
#endif
	/* attach PLL0 clock to FLEXCOMM7 */
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM7);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can0), nxp_lpc_mcan, okay)
	CLOCK_SetClkDiv(kCLOCK_DivCanClk, 1U, false);
	CLOCK_AttachClk(kMCAN_DIV_to_MCAN);
	RESET_PeripheralReset(kMCAN_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sdif), nxp_lpc_sdif, okay) && \
	CONFIG_MCUX_SDIF
	/* attach main clock to SDIF */
	CLOCK_AttachClk(kMAIN_CLK_to_SDIO_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivSdioClk, 3, true);
#endif

#endif /* CONFIG_SOC_LPC55S69_CPU0 */

#if defined(CONFIG_SOC_LPC55S36) && defined(CONFIG_PWM)
	/* Set the Submodule Clocks for FlexPWM */
	SYSCON->PWM0SUBCTL |=
	  (SYSCON_PWM0SUBCTL_CLK0_EN_MASK | SYSCON_PWM0SUBCTL_CLK1_EN_MASK |
	   SYSCON_PWM0SUBCTL_CLK2_EN_MASK);
	SYSCON->PWM1SUBCTL |=
	  (SYSCON_PWM1SUBCTL_CLK0_EN_MASK | SYSCON_PWM1SUBCTL_CLK1_EN_MASK |
	   SYSCON_PWM1SUBCTL_CLK2_EN_MASK);
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

static int nxp_lpc55xxx_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	z_arm_clear_faults();

	/* Initialize FRO/system clock to 96 MHz */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

	/*
	 * install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(nxp_lpc55xxx_init, PRE_KERNEL_1, 0);

#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_LPC55S69_CPU0)
/**
 *
 * @brief Second Core Init
 *
 * This routine boots the secondary core
 *
 * @retval 0 on success.
 *
 */
/* This function is also called at deep sleep resume. */
int _second_core_init(const struct device *arg)
{
	int32_t temp;

	ARG_UNUSED(arg);

	/* Setup the reset handler pointer (PC) and stack pointer value.
	 * This is used once the second core runs its startup code.
	 * The second core first boots from flash (address 0x00000000)
	 * and then detects its identity (Core no. 1, second) and checks
	 * registers CPBOOT and use them to continue the boot process.
	 * Make sure the startup code for the first core is
	 * appropriate and shareable with the second core!
	 */
	SYSCON->CPUCFG |= SYSCON_CPUCFG_CPU1ENABLE_MASK;

	/* Boot source for Core 1 from flash */
	SYSCON->CPBOOT = SYSCON_CPBOOT_CPBOOT(DT_REG_ADDR(
						DT_CHOSEN(zephyr_code_cpu1_partition)));

	temp = SYSCON->CPUCTRL;
	temp |= 0xc0c48000;
	SYSCON->CPUCTRL = temp | SYSCON_CPUCTRL_CPU1RSTEN_MASK |
						SYSCON_CPUCTRL_CPU1CLKEN_MASK;
	SYSCON->CPUCTRL = (temp | SYSCON_CPUCTRL_CPU1CLKEN_MASK) &
						(~SYSCON_CPUCTRL_CPU1RSTEN_MASK);

	return 0;
}

SYS_INIT(_second_core_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /*defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_LPC55S69_CPU0)*/
