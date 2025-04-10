/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util_macro.h>

#include <cortex_m/exception.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>
#include "soc.h"
#include "flexspi_clock_setup.h"
#include "fsl_ocotp.h"

extern void nxp_nbu_init(void);
#ifdef CONFIG_NXP_RW6XX_BOOT_HEADER
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
#endif /* CONFIG_NXP_RW6XX_BOOT_HEADER */

/**
 * @brief Initialize the system clocks and peripheral clocks
 *
 * This function is called from the power management code as the
 * clock needs to be re-initialized on exit from Standby mode. Hence
 * this function is relocated to RAM.
 */
__weak __ramfunc void clock_init(void)
{
	POWER_DisableGDetVSensors();

	if ((PMU->CAU_SLP_CTRL & PMU_CAU_SLP_CTRL_SOC_SLP_RDY_MASK) == 0U) {
		/* LPOSC not enabled, enable it */
		CLOCK_EnableClock(kCLOCK_RefClkCauSlp);
	}
	if ((SYSCTL2->SOURCE_CLK_GATE & SYSCTL2_SOURCE_CLK_GATE_REFCLK_SYS_CG_MASK) != 0U) {
		/* REFCLK_SYS not enabled, enable it */
		CLOCK_EnableClock(kCLOCK_RefClkSys);
	}

	/* Initialize T3 clocks and t3pll_mci_48_60m_irc configured to 48.3MHz */
	CLOCK_InitT3RefClk(kCLOCK_T3MciIrc48m);
	/* Enable T3 256M clock and SFRO */
	CLOCK_EnableClock(kCLOCK_T3PllMci256mClk);

	/* Move FLEXSPI clock source to T3 256m / 4 to avoid instruction/data fetch issue in XIP
	 * when updating PLL and main clock.
	 */
	set_flexspi_clock(FLEXSPI, 6U, 4U);

	/* First let M33 run on SOSC */
	CLOCK_AttachClk(kSYSOSC_to_MAIN_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 1);

	/* tcpu_mci_clk configured to 260MHz, tcpu_mci_flexspi_clk 312MHz. */
	CLOCK_InitTcpuRefClk(3120000000UL, kCLOCK_TcpuFlexspiDiv10);
	/* Enable tcpu_mci_clk 260MHz. Keep tcpu_mci_flexspi_clk gated. */
	CLOCK_EnableClock(kCLOCK_TcpuMciClk);

	/* Enable AUX0 PLL to 260 MHz */
	CLOCK_SetClkDiv(kCLOCK_DivAux0PllClk, 1U);

	/* Configure MainPll to 260MHz, then let CM33 run on Main PLL. */
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 1U);
	CLOCK_SetClkDiv(kCLOCK_DivMainPllClk, 1U);
	CLOCK_AttachClk(kMAIN_PLL_to_MAIN_CLK);

	/* Set SYSTICKFCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivSystickClk, 1U);
	CLOCK_AttachClk(kSYSTICK_DIV_to_SYSTICK_CLK);

	SystemCoreClockUpdate();

	/* Set PLL FRG clock to 20MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivPllFrgClk, 13U);

	/* Call function set_flexspi_clock() to set flexspi clock source to aux0_pll_clk in XIP. */
	set_flexspi_clock(FLEXSPI, 2U, 2U);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(os_timer), nxp_os_timer, okay)
	CLOCK_AttachClk(kLPOSC_to_OSTIMER_CLK);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt), nxp_lpc_wwdt, okay))
	CLOCK_AttachClk(kLPOSC_to_WDT0_CLK);
#else
	/* Allowed to select none if not being used for watchdog to
	 * reduce power
	 */
	CLOCK_AttachClk(kNONE_to_WDT0_CLK);
#endif

#if defined(CONFIG_ADC_MCUX_GAU) || defined(CONFIG_DAC_MCUX_GAU)
	/* Attack clock for GAU and reset */
	CLOCK_AttachClk(kMAIN_CLK_to_GAU_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivGauClk, 1U);
	CLOCK_EnableClock(kCLOCK_Gau);
	RESET_PeripheralReset(kGAU_RST_SHIFT_RSTn);
#endif /* GAU */

/* Any flexcomm can be USART */
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay)) && CONFIG_SERIAL
	CLOCK_SetFRGClock(&(const clock_frg_clk_config_t){0, kCLOCK_FrgPllDiv, 255, 0});
	CLOCK_AttachClk(kFRG_to_FLEXCOMM0);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_usart, okay)) && CONFIG_SERIAL
	CLOCK_SetFRGClock(&(const clock_frg_clk_config_t){1, kCLOCK_FrgPllDiv, 255, 0});
	CLOCK_AttachClk(kFRG_to_FLEXCOMM1);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_usart, okay)) && CONFIG_SERIAL
	CLOCK_SetFRGClock(&(const clock_frg_clk_config_t){2, kCLOCK_FrgPllDiv, 255, 0});
	CLOCK_AttachClk(kFRG_to_FLEXCOMM2);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_usart, okay)) && CONFIG_SERIAL
	CLOCK_SetFRGClock(&(const clock_frg_clk_config_t){3, kCLOCK_FrgPllDiv, 255, 0});
	CLOCK_AttachClk(kFRG_to_FLEXCOMM3);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm14), nxp_lpc_usart, okay)) && CONFIG_SERIAL
	CLOCK_SetFRGClock(&(const clock_frg_clk_config_t){14, kCLOCK_FrgPllDiv, 255, 0});
	CLOCK_AttachClk(kFRG_to_FLEXCOMM14);
#endif

/* Any flexcomm can be I2C */
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_i2c, okay)) && CONFIG_I2C
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM0);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2c, okay)) && CONFIG_I2C
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM1);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2c, okay)) && CONFIG_I2C
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM2);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2c, okay)) && CONFIG_I2C
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM3);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm14), nxp_lpc_i2c, okay)) && CONFIG_I2C
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM14);
#endif
#if CONFIG_XTAL32K
	CLOCK_EnableXtal32K(true);
	CLOCK_AttachClk(kXTAL32K_to_CLK32K);
#endif

/* Clock flexcomms when used as SPI */
#ifdef CONFIG_SPI
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_spi, okay))
	/* Set up Flexcomm0 FRG to clock at 260 MHz from main clock */
	const clock_frg_clk_config_t flexcomm0_frg = {0, kCLOCK_FrgMainClk, 255, 0};

	CLOCK_SetFRGClock(&flexcomm0_frg);
	CLOCK_AttachClk(kFRG_to_FLEXCOMM0);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_spi, okay))
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM1);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_spi, okay))
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM2);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_spi, okay))
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM3);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm14), nxp_lpc_spi, okay))
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM14);
#endif
#endif /* CONFIG_SPI */

#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmic0)) && CONFIG_AUDIO_DMIC_MCUX) || CONFIG_I2S
	const clock_avpll_config_t avpll_config = {
		.ch1Freq = kCLOCK_AvPllChFreq12p288m,
		.ch2Freq = kCLOCK_AvPllChFreq64m,
		.enableCali = true
	};

	/* Init AVPLL and enable both channels */
	CLOCK_InitAvPll(&avpll_config);
	CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 1U);

	/* Clock DMIC from Audio PLL. PLL output is sourced from AVPLL
	 * channel 1, which is clocked at 12.288 MHz. We can divide this
	 * by 4 to achieve the desired DMIC bit clk of 3.072 MHz
	 */
	CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 4);

	#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_i2s, okay))
		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM0);
	#endif
	#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay))
		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
	#endif
	#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2s, okay))
		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM2);
	#endif
	#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay))
		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
	#endif
	#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm14), nxp_lpc_i2s, okay))
		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM14);
	#endif
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lcdic)) && CONFIG_MIPI_DBI_NXP_LCDIC
	CLOCK_AttachClk(kMAIN_CLK_to_LCD_CLK);
	RESET_PeripheralReset(kLCDIC_RST_SHIFT_RSTn);
#endif

#ifdef CONFIG_COUNTER_MCUX_CTIMER
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ctimer0), nxp_lpc_ctimer, okay))
	CLOCK_AttachClk(kSFRO_to_CTIMER0);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ctimer1), nxp_lpc_ctimer, okay))
	CLOCK_AttachClk(kSFRO_to_CTIMER1);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ctimer2), nxp_lpc_ctimer, okay))
	CLOCK_AttachClk(kSFRO_to_CTIMER2);
#endif
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ctimer3), nxp_lpc_ctimer, okay))
	CLOCK_AttachClk(kSFRO_to_CTIMER3);
#endif
#endif /* CONFIG_COUNTER_MCUX_CTIMER */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb_otg)) && \
	(CONFIG_USB_DC_NXP_EHCI || CONFIG_UDC_NXP_EHCI)
	/* Enable system xtal from Analog */
	SYSCTL2->ANA_GRP_CTRL |= SYSCTL2_ANA_GRP_CTRL_PU_AG_MASK;
	/* reset USB */
	RESET_PeripheralReset(kUSB_RST_SHIFT_RSTn);
	/* enable usb clock */
	CLOCK_EnableClock(kCLOCK_Usb);
	/* enable usb phy clock */
	CLOCK_EnableUsbhsPhyClock();
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet)) && CONFIG_NET_L2_ETHERNET
	RESET_PeripheralReset(kENET_IPG_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kENET_IPG_S_RST_SHIFT_RSTn);
#endif

}

extern void nxp_rw6xx_power_init(void);
/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 */

void soc_early_init_hook(void)
{
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt), nxp_lpc_wwdt, okay))
	POWER_EnableResetSource(kPOWER_ResetSourceWdt);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(pmu), reset_causes_en)
#define PMU_RESET_CAUSES_ \
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(pmu), reset_causes_en, DT_PROP_BY_IDX, (|))
#define PMU_RESET_CAUSES \
	COND_CODE_0(IS_EMPTY(PMU_RESET_CAUSES_), (PMU_RESET_CAUSES_), (0))
#define WDT_RESET \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(wwdt), (kPOWER_ResetSourceWdt), (0))
#define RESET_CAUSES \
	(PMU_RESET_CAUSES | WDT_RESET)
#else
#define RESET_CAUSES 0
#endif

	POWER_EnableResetSource(RESET_CAUSES);

	/* Initialize clock */
	clock_init();

#if defined(CONFIG_ADC_MCUX_GAU) || defined(CONFIG_DAC_MCUX_GAU)
	POWER_PowerOnGau();
#endif
#if CONFIG_PM
	nxp_rw6xx_power_init();
#endif

#if defined(CONFIG_BT) || defined(CONFIG_IEEE802154)
	nxp_nbu_init();
#endif
}

void soc_reset_hook(void)
{
	/* This is provided by the SDK */
	SystemInit();
}
