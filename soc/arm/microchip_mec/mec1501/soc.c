/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

/* MEC devices IDs with special PLL handling */
#define MCHP_GCFG_DID_DEV_ID_MEC150x    0x0020U
#define MCHP_TRIM_ENABLE_INT_OSCILLATOR 0x06U

#define TEST_CLK_OUT_PIN_COUNT		1

/*
 * Make sure PCR sleep enables are clear except for crypto
 * which do not have internal clock gating.
 */
static int soc_pcr_init(void)
{
	PCR_REGS->SLP_EN0 = 0;
	PCR_REGS->SLP_EN1 = 0;
	PCR_REGS->SLP_EN2 = 0;
	PCR_REGS->SLP_EN4 = 0;
	PCR_REGS->SLP_EN3 = MCHP_PCR3_CRYPTO_MASK;

	return 0;
}

/*
 * Select 32KHz clock source used for PLL reference.
 * Options are:
 * Internal 32KHz silicon oscillator.
 * External parallel resonant crystal between XTAL1 and XTAL2 pins.
 * External single ended crystal connected to XTAL2 pin.
 * External 32KHz square wave from Host chipset/board on 32KHZ_IN pin.
 * NOTES:
 *   FW Program new value to VBAT CLK32 Enable register.
 *   HW if new value != current value
 *   HW endif
 *   FW spin until PCR PLL lock is set.
 *   32K stable and PLL locked.
 *   PLL POR or clock source change can take up to 3ms to lock.
 *   32KHZ_IN pin must be configured for 32KHZ_IN function.
 *   Crystals vary and may take longer time to stabilize this will
 *   affect PLL lock time.
 *   Crystal do not like to be power cycled. If using a crystal
 *   the board should supply a battery backed (VBAT) power rail.
 *   The VBAT clock control register selecting 32KHz source is
 *   connected to the VBAT power rail. If using a battery one can
 *   check the VBAT Power Fail and Reset Status register for a VBAT POR.
 */
static void clk32_change(uint8_t new_clk32)
{
	/* Program new value. */
	VBATR_REGS->CLK32_EN = new_clk32 & MCHP_VBATR_CLKEN_MASK;

	/* Wait for PLL lock. HW state machine is configuring PLL. */
	while ((PCR_REGS->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK) == 0)
		;
}

static int soc_clk32_init(void)
{
	uint8_t new_clk32;

#ifdef CONFIG_SOC_MEC1501_EXT_32K
  #ifdef CONFIG_SOC_MEC1501_EXT_32K_CRYSTAL
    #ifdef CONFIG_SOC_MEC1501_EXT_32K_PARALLEL_CRYSTAL
	new_clk32 = MCHP_VBATR_USE_PAR_CRYSTAL;
    #else
	new_clk32 = MCHP_VBATR_USE_SE_CRYSTAL;
    #endif
  #else
	/* Use 32KHZ_PIN as 32KHz source */
	new_clk32 = MCHP_VBATR_USE_32KIN_PIN;
  #endif
#else
	/* Use internal 32KHz +/-2% silicon oscillator
	 * if required performed OTP value override
	 */
	if (MCHP_DEVICE_ID() == MCHP_GCFG_DID_DEV_ID_MEC150x) {
		if (MCHP_REVISION_ID() == MCHP_GCFG_REV_B0) {
			VBATR_REGS->CKK32_TRIM = MCHP_TRIM_ENABLE_INT_OSCILLATOR;
		}
	}

	new_clk32 = MCHP_VBATR_USE_SIL_OSC;
#endif
	clk32_change(new_clk32);

	return 0;
}

/*
 * Initialize MEC1501 EC Interrupt Aggregator (ECIA) and external NVIC
 * inputs.
 */
static int soc_ecia_init(void)
{
	GIRQ_Type *pg;
	uint32_t n;

	mchp_pcr_periph_slp_ctrl(PCR_ECIA, MCHP_PCR_SLEEP_DIS);

	ECS_REGS->INTR_CTRL |= MCHP_ECS_ICTRL_DIRECT_EN;

	/* gate off all aggregated outputs */
	ECIA_REGS->BLK_EN_CLR = 0xFFFFFFFFul;
	/* gate on GIRQ's that are aggregated only */
	ECIA_REGS->BLK_EN_SET = MCHP_ECIA_AGGR_BITMAP;

	/* Clear all GIRQn source enables and source status */
	pg = &ECIA_REGS->GIRQ08;
	for (n = MCHP_FIRST_GIRQ; n <= MCHP_LAST_GIRQ; n++) {
		pg->EN_CLR = 0xFFFFFFFFul;
		pg->SRC = 0xFFFFFFFFul;
		pg++;
	}

	/* Clear all external NVIC enables and pending status */
	for (n = 0u; n < MCHP_NUM_NVIC_REGS; n++) {
		NVIC->ICER[n] = 0xFFFFFFFFul;
		NVIC->ICPR[n] = 0xFFFFFFFFul;
	}

	return 0;
}

static void configure_debug_interface(void)
{
	/* No debug support */
	ECS_REGS->DEBUG_CTRL = 0;
	ECS_REGS->ETM_CTRL = 0;

#ifdef CONFIG_SOC_MEC1501_DEBUG_WITHOUT_TRACING
	/* Release JTAG TDI and JTAG TDO pins so they can be
	 * controlled by their respective PCR register (UART2).
	 * For more details see table 44-1
	 */
	ECS_REGS->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD);
#elif defined(CONFIG_SOC_MEC1501_DEBUG_AND_TRACING)
	#if defined(CONFIG_SOC_MEC1501_DEBUG_AND_ETM_TRACING)
	#pragma error "TRACE DATA are not exposed in HW connector"
	#elif defined(CONFIG_SOC_MEC1501_DEBUG_AND_SWV_TRACING)
		ECS_REGS->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD_SWV);
	#endif /* CONFIG_SOC_MEC1501_DEBUG_AND_TRACING */

#endif /* CONFIG_SOC_MEC1501_DEBUG_WITHOUT_TRACING */
}

static int soc_init(const struct device *dev)
{
	uint32_t isave;
#ifdef CONFIG_SOC_MEC1501_TEST_CLK_OUT
	const pinctrl_soc_pin_t test_clk_out_pin = {MCHP_XEC_PINMUX(060, MCHP_AF2), 0};
#endif

	ARG_UNUSED(dev);

	isave = __get_PRIMASK();
	__disable_irq();

	soc_pcr_init();

	soc_clk32_init();

	/*
	 * On HW reset PCR Processor Clock Divider = 4 for 48/4 = 12 MHz.
	 * Set clock divider = 1 for maximum speed.
	 * NOTE1: This clock divider affects all Cortex-M4 core clocks.
	 * If you change it you must reprogram SYSTICK to maintain the
	 * same absolute time interval.
	 */
	PCR_REGS->PROC_CLK_CTRL = CONFIG_SOC_MEC1501_PROC_CLK_DIV;

	soc_ecia_init();

	/* Configure GPIO bank before usage
	 * VTR1 is not configurable
	 * VTR2 doesn't need configuration if setting VTR2_STRAP
	 */
#ifdef CONFIG_SOC_MEC1501_VTR3_1_8V
	ECS_REGS->GPIO_BANK_PWR |= MCHP_ECS_VTR3_LVL_18;
#endif

	configure_debug_interface();

#ifdef CONFIG_SOC_MEC1501_TEST_CLK_OUT
	/*
	 * Deep sleep testing: Enable TEST_CLK_OUT on GPIO_060 function 2.
	 * TEST_CLK_OUT is the PLL 48MHz conditioned output.
	 */

	pinctrl_configure_pins(&test_clk_out_pin, TEST_CLK_OUT_PIN_COUNT, 0);
#endif

	if (!isave) {
		__enable_irq();
	}

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
