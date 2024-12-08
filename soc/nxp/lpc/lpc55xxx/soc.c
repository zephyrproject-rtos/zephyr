/* Copyright 2017, 2019-2023 NXP
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
#include <cortex_m/exception.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>
#ifdef CONFIG_GPIO_MCUX_LPC
#include <fsl_pint.h>
#endif
#if CONFIG_USB_DC_NXP_LPCIP3511 || CONFIG_UDC_NXP_IP3511
#include "usb_phy.h"
#include "usb.h"
#endif
#if defined(CONFIG_SOC_LPC55S36) && (defined(CONFIG_ADC_MCUX_LPADC) \
	|| defined(CONFIG_DAC_MCUX_LPDAC))
#include <fsl_vref.h>
#endif

/* System clock frequency */
extern uint32_t SystemCoreClock;

/*Should be in the range of 12MHz to 32MHz */
static uint32_t ExternalClockFrequency;


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
	.flags = PLL_SETUPFLAG_WAITLOCK
};
#endif

#ifdef CONFIG_INIT_PLL1
const pll_setup_t pll1Setup = {
	.pllctrl = SYSCON_PLL1CTRL_CLKEN_MASK | SYSCON_PLL1CTRL_SELI(53U) |
		SYSCON_PLL1CTRL_SELP(31U),
	.pllndec = SYSCON_PLL1NDEC_NDIV(8U),
	.pllpdec = SYSCON_PLL1PDEC_PDIV(1U),
	.pllmdec = SYSCON_PLL1MDEC_MDIV(144U),
	.pllRate = 144000000U,
	.flags = PLL_SETUPFLAG_WAITLOCK
};
#endif

/**
 *
 * @brief Initialize the system clock
 *
 */

static ALWAYS_INLINE void clock_init(void)
{
	ExternalClockFrequency = 0;

#if defined(CONFIG_SOC_LPC55S36)
	/* Power Management Controller initialization */
	POWER_PowerInit();
#endif

#if defined(CONFIG_SOC_LPC55S06) || defined(CONFIG_SOC_LPC55S16) || \
	defined(CONFIG_SOC_LPC55S26) || defined(CONFIG_SOC_LPC55S28) || \
	defined(CONFIG_SOC_LPC55S36) || defined(CONFIG_SOC_LPC55S69_CPU0)
	/* Set up the clock sources */
	/* Configure FRO192M */
	/* Ensure FRO is on  */
	POWER_DisablePD(kPDRUNCFG_PD_FRO192M);
	/* Set up FRO to the 12 MHz, to ensure we can change the clock freq */
	CLOCK_SetupFROClocking(12000000U);
	/* Switch to FRO 12MHz first to ensure we can change the clock */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Ensure CLK_IN is on  */
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK;
	ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;

	/* Setting the Core Clock to either 96MHz or in the case of using PLL, 144MHz */
#if defined(CONFIG_SOC_LPC55S06) || !defined(CONFIG_INIT_PLL1)
	SystemCoreClock = 96000000U;
#else
	SystemCoreClock = 144000000U;
#endif


	/* These functions must be called before increasing to a higher frequency
	 * Additionally, CONFIG_TRUSTED_EXECUTION_NONSECURE is being used
	 * since the non-secure SOCs should not have access to the flash
	 * as this will cause a secure fault to occur
	 */
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Set Voltage for one of the fastest clock outputs: System clock output */
	POWER_SetVoltageForFreq(SystemCoreClock);
	/*!< Set FLASH wait states for core */
	CLOCK_SetFLASHAccessCyclesForFreq(SystemCoreClock);
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */


#if defined(CONFIG_INIT_PLL0) || defined(CONFIG_INIT_PLL1)
	/* Configure XTAL32M */
	ExternalClockFrequency = 16000000U;
	CLOCK_SetupExtClocking(ExternalClockFrequency);
#endif

#if defined(CONFIG_SOC_LPC55S06) || !defined(CONFIG_INIT_PLL1)
	/* Enable FRO HF(SystemCoreClock) output (Default expected value 96MHz) */
	CLOCK_SetupFROClocking(SystemCoreClock);

	/* Switch MAIN_CLK to FRO_HF */
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);

#else
	/* Switch PLL1 clock source selector to XTAL32M */
	CLOCK_AttachClk(kEXT_CLK_to_PLL1);

	/* Ensure PLL1 is on */
	POWER_DisablePD(kPDRUNCFG_PD_PLL1);

	/* Configure PLL to the desired values */
	CLOCK_SetPLL1Freq(&pll1Setup);

	/* Switch MAIN_CLK to FRO_HF */
	CLOCK_AttachClk(kPLL1_to_MAIN_CLK);

#endif /* CONFIG_SOC_LPC55S06 || !CONFIG_INIT_PLL1 */


#ifdef CONFIG_INIT_PLL0
	/* Switch PLL0 clock source selector to XTAL32M */
	CLOCK_AttachClk(kEXT_CLK_to_PLL0);

	/* Configure PLL to the desired values */
	CLOCK_SetPLL0Freq(&pll0Setup);

#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivPllClk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivPllClk, 1U, false);
#else
	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 1U, false);
#endif /* CONFIG_SOC_LPC55S36 */
#endif /* CONFIG_INIT_PLL0 */


	/* Set up dividers */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

	/* Enables the clock for the I/O controller.: Enable Clock. */
	CLOCK_EnableClock(kCLOCK_Iocon);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_usart, okay)
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1U, false);
#endif
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM3);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1U, false);
#endif
	/* attach 12 MHz clock to FLEXCOMM4 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm5), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM5);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm6), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM6);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm7), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM7);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hs_lspi))
	/* Attach 12 MHz clock to HSLSPI */
	CLOCK_AttachClk(kFRO_HF_DIV_to_HSLSPI);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt0), nxp_lpc_wwdt, okay)
	/* Enable 1 MHz FRO clock for WWDT */
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mailbox0), nxp_lpc_mailbox, okay)
	CLOCK_EnableClock(kCLOCK_Mailbox);
#endif

#if CONFIG_USB_DC_NXP_LPCIP3511 || CONFIG_UDC_NXP_IP3511

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(usbfs), nxp_lpcip3511, okay)
	/*< Turn on USB Phy */
#if defined(CONFIG_SOC_LPC55S36)
	POWER_DisablePD(kPDRUNCFG_PD_USBFSPHY);
#else
	POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY);
#endif
	CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1, false);
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_AttachClk(kFRO_HF_to_USB0);
#else
	CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
#endif
	/* enable usb0 host clock */
	CLOCK_EnableClock(kCLOCK_Usbhsl0);
	/*
	 * According to reference mannual, device mode setting has to be set by access
	 * usb host register
	 */
	USBFSH->PORTMODE |= USBFSH_PORTMODE_DEV_ENABLE_MASK;
	/* disable usb0 host clock */
	CLOCK_DisableClock(kCLOCK_Usbhsl0);

	/* enable USB IP clock */
	CLOCK_EnableUsbfs0DeviceClock(kCLOCK_UsbfsSrcFro, CLOCK_GetFroHfFreq());
#if defined(FSL_FEATURE_USB_USB_RAM) && (FSL_FEATURE_USB_USB_RAM)
	memset((uint8_t *)FSL_FEATURE_USB_USB_RAM_BASE_ADDRESS, 0, FSL_FEATURE_USB_USB_RAM);
#endif

#endif /* USB_DEVICE_TYPE_FS */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(usbhs), nxp_lpcip3511, okay)
	/* enable usb1 host clock */
	CLOCK_EnableClock(kCLOCK_Usbh1);
	/* Put PHY powerdown under software control */
	USBHSH->PORTMODE = USBHSH_PORTMODE_SW_PDCOM_MASK;
	/*
	 * According to reference manual, device mode setting has to be set by
	 * access usb host register
	 */
	USBHSH->PORTMODE |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
	/* disable usb1 host clock */
	CLOCK_DisableClock(kCLOCK_Usbh1);

	/* enable USB IP clock */
	CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_UsbPhySrcExt, CLK_CLK_IN);
	CLOCK_EnableUsbhs0DeviceClock(kCLOCK_UsbSrcUnused, 0U);
#if CONFIG_USB_DC_NXP_LPCIP3511
	USB_EhciPhyInit(kUSB_ControllerLpcIp3511Hs0, CLK_CLK_IN, NULL);
#endif
#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
	memset((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS, 0, FSL_FEATURE_USBHSD_USB_RAM);
#endif

#endif /* USB_DEVICE_TYPE_HS */

#endif /* CONFIG_USB_DC_NXP_LPCIP3511 */

DT_FOREACH_STATUS_OKAY(nxp_lpc_ctimer, CTIMER_CLOCK_SETUP)

DT_FOREACH_STATUS_OKAY(nxp_ctimer_pwm, CTIMER_CLOCK_SETUP)

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

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(adc0), nxp_lpc_lpadc, okay)
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivAdc0Clk, 2U, true);
	CLOCK_AttachClk(kFRO_HF_to_ADC0);
#else /* not LPC55s36 */
	CLOCK_SetClkDiv(kCLOCK_DivAdcAsyncClk,
			DT_PROP(DT_NODELABEL(adc0), clk_divider), true);
	CLOCK_AttachClk(MUX_A(CM_ADCASYNCCLKSEL, DT_PROP(DT_NODELABEL(adc0), clk_source)));

	/* Power up the ADC */
	POWER_DisablePD(kPDRUNCFG_PD_LDOGPADC);
#endif /* SOC platform */
#endif /* ADC */

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(vref0), nxp_vref, okay))
	CLOCK_EnableClock(kCLOCK_Vref);
	POWER_DisablePD(kPDRUNCFG_PD_VREF);
#endif /* vref0 */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(dac0), nxp_lpdac, okay)
#if defined(CONFIG_SOC_LPC55S36)
	CLOCK_SetClkDiv(kCLOCK_DivDac0Clk, 1U, true);
	CLOCK_AttachClk(kMAIN_CLK_to_DAC0);

	/* Disable DAC0 power down */
	POWER_DisablePD(kPDRUNCFG_PD_DAC0);
#endif /* SOC platform */
#endif /* DAC */

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

static int nxp_lpc55xxx_init(void)
{
	z_arm_clear_faults();

	/* Initialize FRO/system clock to 96 MHz */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

	return 0;
}

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();


#ifndef CONFIG_LOG_BACKEND_SWO
	/*
	 * SystemInit unconditionally enables the trace clock.
	 * Disable the trace clock unless SWO is used
	 */
	SYSCON->TRACECLKDIV = 0x4000000;
#endif
}

#endif /* CONFIG_SOC_RESET_HOOK */

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
int _second_core_init(void)
{
	int32_t temp;


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
