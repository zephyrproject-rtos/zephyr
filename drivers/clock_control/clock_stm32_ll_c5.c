/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <stm32_ll_system.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <stm32_backup_domain.h>

/* Macros to fill up prescaler values */
#define ahb_prescaler(v) CONCAT(LL_RCC_HCLK_PRESCALER_, v)
#define apb1_prescaler(v) CONCAT(LL_RCC_APB1_PRESCALER_, v)
#define apb2_prescaler(v) CONCAT(LL_RCC_APB2_PRESCALER_, v)
#define apb3_prescaler(v) CONCAT(LL_RCC_APB3_PRESCALER_, v)
#define hsik_div(v) CONCAT(LL_RCC_HSIK_DIV_, v)
#define psik_div(v) CONCAT(LL_RCC_PSIK_DIV_, v)
#define psi_freq(v) CONCAT(LL_RCC_PSIFREQ_, v, MHZ)
#define psi_source(v) CONCAT(LL_RCC_PSISOURCE_, v)

#define PSIREF_48000000	LL_RCC_PSIREF_48MHZ
#define PSIREF_32000000	LL_RCC_PSIREF_32MHZ
#define PSIREF_24000000	LL_RCC_PSIREF_24MHZ
#define PSIREF_16000000	LL_RCC_PSIREF_16MHZ
#define PSIREF_8000000	LL_RCC_PSIREF_8MHZ

#define psi_ref(v) CONCAT(PSIREF_, v)

#define PSI_LSE_100 KHZ(100016)
#define PSI_LSE_144 KHZ(144015)
#define PSI_LSE_160 KHZ(160006)

#if (defined(STM32_PSIS_ENABLED) || defined(STM32_PSIDIV3_ENABLED) || \
	defined(STM32_PSIK_ENABLED)) && \
	defined(STM32_PSI_FREQ_MHZ_ENABLED) && defined(STM32_PSI_SOURCE_ENABLED) && \
	(psi_source(STM32_PSI_SOURCE) == LL_RCC_PSISOURCE_HSIDIV18) && \
	!defined(STM32_HSIS_ENABLED)
#error "clk_hsis should be enabled to set HSIDIV18 as PSI clock source"
#endif

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

__unused static uint32_t get_psi_freq(void)
{
	if (LL_RCC_GetPSIClkSource() == LL_RCC_PSISOURCE_LSE) {
		/* If LSE is used as reference, an intrinsic error must be anticipated on the
		 * output frequency (100.016 MHz, 144.015 MHz, and 160.006 MHz)
		 */
		switch (LL_RCC_GetPSIFreqOutput()) {
		case LL_RCC_PSIFREQ_100MHZ:
			return PSI_LSE_100;
		case LL_RCC_PSIFREQ_144MHZ:
			return PSI_LSE_144;
		case LL_RCC_PSIFREQ_160MHZ:
			return PSI_LSE_160;
		default:
			__ASSERT(0, "Unexpected PSI freq");
			return 0;
		}
	} else {
		switch (LL_RCC_GetPSIFreqOutput()) {
		case LL_RCC_PSIFREQ_100MHZ:
			return MHZ(100);
		case LL_RCC_PSIFREQ_144MHZ:
			return MHZ(144);
		case LL_RCC_PSIFREQ_160MHZ:
			return MHZ(160);
		default:
			__ASSERT(0, "Unexpected PSI freq");
			return 0;
		}
	}
}

static uint32_t get_sysclk_frequency(void)
{
	switch (LL_RCC_GetSysClkSource()) {
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSIDIV3:
		return HSI_VALUE / 3;
#ifdef STM32_HSIS_ENABLED
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSIS:
		return STM32_HSIS_FREQ;
#endif /* STM32_HSIS_ENABLED */
#ifdef STM32_HSE_ENABLED
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:
		return STM32_HSE_FREQ;
#endif /* STM32_HSE_ENABLED */
#ifdef STM32_PSIS_ENABLED
	case LL_RCC_SYS_CLKSOURCE_STATUS_PSIS:
		return get_psi_freq();
#endif /* STM32_PSIS_ENABLED */
	default:
		__ASSERT(0, "Unexpected Sysclk source");
		return 0;
	}
}

__unused static uint32_t get_xsik_freq(uint32_t input_freq, uint32_t divider)
{
	/* divider variable is 1 for a divider of 1, 2 for 1.5, ..., 15 for 8
	 * So (divider + 1) is twice the real divider value.
	 */
	return (2 * input_freq) / (divider + 1);
}

__unused static uint32_t get_ck48_freq(void)
{
	switch (LL_RCC_GetCKClockSource(LL_RCC_CK48_CLKSOURCE)) {
#ifdef STM32_PSIDIV3_ENABLED
	case LL_RCC_CK48_CLKSOURCE_PSIDIV3:
		return get_psi_freq() / 3;
#endif /* STM32_PSIDIV3_ENABLED */
#ifdef STM32_HSIDIV3_ENABLED
	case LL_RCC_CK48_CLKSOURCE_HSIDIV3:
		return STM32_HSIDIV3_FREQ;
#endif /* STM32_HSIDIV3_ENABLED */
#ifdef STM32_HSE_ENABLED
	case LL_RCC_CK48_CLKSOURCE_HSE:
		return STM32_HSE_FREQ;
#endif /* STM32_HSE_ENABLED */
	default:
		__ASSERT(0, "Unexpected CK48 source");
		return 0;
	}
}

/** @brief Verifies clock is part of active clock configuration */
int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_SYSCLK) ||
	    (src_clk == STM32_SRC_HCLK) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK3) ||
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSIS) && IS_ENABLED(STM32_HSIS_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSIDIV3) && IS_ENABLED(STM32_HSIDIV3_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSIK) && IS_ENABLED(STM32_HSIK_ENABLED)) ||
	    ((src_clk == STM32_SRC_PSIS) && IS_ENABLED(STM32_PSIS_ENABLED)) ||
	    ((src_clk == STM32_SRC_PSIDIV3) && IS_ENABLED(STM32_PSIDIV3_ENABLED)) ||
	    ((src_clk == STM32_SRC_PSIK) && IS_ENABLED(STM32_PSIK_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    (src_clk == STM32_SRC_CK48)) {
		return 0;
	}

	return -ENOTSUP;
}

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile int temp;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		     pclken->enr);
	/* Delay after enabling the clock, to allow it to become active */
	temp = sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
	UNUSED(temp);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);

	return 0;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	if (pclken->enr == NO_SEL) {
		/* Domain clock is fixed. Nothing to set. Exit */
		return 0;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		       STM32_DT_CLKSEL_MASK_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));
	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		     STM32_DT_CLKSEL_VAL_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sys,
					       uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sys);

	/*
	 * Get AHB Clock (= SystemCoreClock = SYSCLK/prescaler)
	 * SystemCoreClock is preferred to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
	 * since it will be updated after clock configuration and hence
	 * more likely to contain actual clock speed
	 */
	uint32_t ahb_clock = SystemCoreClock;
	uint32_t apb1_clock = get_bus_clock(ahb_clock, STM32_APB1_PRESCALER);
	uint32_t apb2_clock = get_bus_clock(ahb_clock, STM32_APB2_PRESCALER);
	uint32_t apb3_clock = get_bus_clock(ahb_clock, STM32_APB3_PRESCALER);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB4:
	case STM32_SRC_HCLK:
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_APB1:
	case STM32_CLOCK_BUS_APB1_2:
	case STM32_SRC_PCLK1:
		*rate = apb1_clock;
		break;
	case STM32_CLOCK_BUS_APB2:
	case STM32_SRC_PCLK2:
		*rate = apb2_clock;
		break;
	case STM32_CLOCK_BUS_APB3:
	case STM32_SRC_PCLK3:
		*rate = apb3_clock;
		break;
	case STM32_SRC_SYSCLK:
		*rate = get_sysclk_frequency();
		break;
#if defined(STM32_HSIS_ENABLED)
	case STM32_SRC_HSIS:
		*rate = STM32_HSIS_FREQ;
		break;
#endif /* STM32_HSIS_ENABLED */
#if defined(STM32_HSIDIV3_ENABLED)
	case STM32_SRC_HSIDIV3:
		*rate = STM32_HSIDIV3_FREQ;
		break;
#endif /* STM32_HSIDIV3_ENABLED */
#if defined(STM32_HSIK_ENABLED)
	case STM32_SRC_HSIK:
		*rate = get_xsik_freq(HSI_VALUE,
				      hsik_div(STM32_HSIK_DIVIDER) >> RCC_CR2_HSIKDIV_Pos);
		break;
#endif /* STM32_HSIK_ENABLED */
#if defined(STM32_PSIS_ENABLED)
	case STM32_SRC_PSIS:
		*rate = get_psi_freq();
		break;
#endif /* STM32_PSIS_ENABLED */
#if defined(STM32_PSIDIV3_ENABLED)
	case STM32_SRC_PSIDIV3:
		*rate = get_psi_freq() / 3;
		break;
#endif /* STM32_PSIDIV3_ENABLED */
#if defined(STM32_PSIK_ENABLED)
	case STM32_SRC_PSIK:
		*rate = get_xsik_freq(get_psi_freq(),
				      psik_div(STM32_PSIK_DIVIDER) >> RCC_CR2_PSIKDIV_Pos);
		break;
#endif /* STM32_PSIK_ENABLED */
#if defined(STM32_HSE_ENABLED)
	case STM32_SRC_HSE:
		*rate = STM32_HSE_FREQ;
		break;
#endif /* STM32_HSE_ENABLED */
#if defined(STM32_LSE_ENABLED)
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
#endif /* STM32_LSE_ENABLED */
#if defined(STM32_LSI_ENABLED)
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
#endif /* STM32_LSI_ENABLED */
	case STM32_SRC_CK48:
		*rate = get_ck48_freq();
		break;
	default:
		return -ENOTSUP;
	}

	if (pclken->div) {
		*rate /= (pclken->div + 1);
	}

	return 0;
}

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Gated clocks */
		if ((sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus) & pclken->enr)
		    == pclken->enr) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else {
		/* Domain clock sources */
		if (enabled_clock(pclken->bus) == 0) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.get_status = stm32_clock_control_get_status,
	.configure = stm32_clock_control_configure,
};

__unused void set_psi_ref(void)
{
	switch (psi_source(STM32_PSI_SOURCE)) {
	case LL_RCC_PSISOURCE_HSE:
		if (IS_ENABLED(STM32_HSE_ENABLED)) {
			LL_RCC_SetPSIRef(psi_ref(STM32_HSE_FREQ));
		} else {
			__ASSERT(0, "HSE selected as PSI source but not enabled");
		}
		break;

	case LL_RCC_PSISOURCE_LSE:
		LL_RCC_SetPSIRef(LL_RCC_PSIREF_32768HZ);
		break;

	case LL_RCC_PSISOURCE_HSIDIV18:
		LL_RCC_SetPSIRef(LL_RCC_PSIREF_8MHZ);
		break;

	default:
		__ASSERT(0, "Unexpected PSI clock source");
	}

}
static void set_up_fixed_clock_sources(void)
{
	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		/* Check if need to enable HSE bypass feature or not */
		if (IS_ENABLED(STM32_HSE_BYPASS)) {
			LL_RCC_HSE_EnableBypass();
		} else {
			LL_RCC_HSE_DisableBypass();
		}

		/* Enable HSE */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
		/* Wait for HSE ready */
		}

#ifdef STM32_HSE_CSS
		/* Enable HSE clock security system */
		z_arm_nmi_set_handler(HAL_RCC_NMI_IRQHandler);
		LL_RCC_HSE_EnableCSS();
#endif /* STM32_HSE_CSS */
	}

	if (IS_ENABLED(STM32_HSIS_ENABLED)) {
		/* Enable HSIS if not enabled */
		if (LL_RCC_HSIS_IsReady() != 1) {
			/* Enable HSIS */
			LL_RCC_HSIS_Enable();
			while (LL_RCC_HSIS_IsReady() != 1) {
				/* Wait for HSIS ready */
			}
		}
	}

	if (IS_ENABLED(STM32_HSIDIV3_ENABLED)) {
		/* Enable HSIDIV3 if not enabled */
		if (LL_RCC_HSIDIV3_IsReady() != 1) {
			/* Enable HSIDIV3 */
			LL_RCC_HSIDIV3_Enable();
			while (LL_RCC_HSIDIV3_IsReady() != 1) {
				/* Wait for HSIDIV3 ready */
			}
		}
	}

	if (IS_ENABLED(STM32_HSIK_ENABLED)) {
		/* Enable HSIK if not enabled */
		if (LL_RCC_HSIK_IsReady() != 1) {
			/* Set HSIK divider */
			LL_RCC_HSIK_SetDivider(hsik_div(STM32_HSIK_DIVIDER));

			/* Enable HSIK */
			LL_RCC_HSIK_Enable();
			while (LL_RCC_HSIK_IsReady() != 1) {
				/* Wait for HSIK ready */
			}
		}
	}

	if (IS_ENABLED(STM32_PSI_FREQ_MHZ_ENABLED) && IS_ENABLED(STM32_PSI_SOURCE_ENABLED)) {
		if (LL_RCC_PSIS_IsEnabled() == 0) {
			LL_RCC_SetPSIFreqOutput(psi_freq(STM32_PSI_FREQ_MHZ));
			LL_RCC_SetPSIClkSource(psi_source(STM32_PSI_SOURCE));
			set_psi_ref();
		}
	}

	if (IS_ENABLED(STM32_PSIS_ENABLED)) {
		/* Enable PSIS if not enabled */
		if (LL_RCC_PSIS_IsReady() != 1) {
			/* Enable PSIS */
			LL_RCC_PSIS_Enable();
			while (LL_RCC_PSIS_IsReady() != 1) {
				/* Wait for PSIS ready */
			}
		}
	}

	if (IS_ENABLED(STM32_PSIDIV3_ENABLED)) {
		/* Enable PSIDIV3 if not enabled */
		if (LL_RCC_PSIDIV3_IsReady() != 1) {
			/* Enable PSIDIV3 */
			LL_RCC_PSIDIV3_Enable();
			while (LL_RCC_PSIDIV3_IsReady() != 1) {
				/* Wait for PSIDIV3 ready */
			}
		}
	}

	if (IS_ENABLED(STM32_PSIK_ENABLED)) {
		/* Enable PSIK if not enabled */
		if (LL_RCC_PSIK_IsEnabled() == 0) {
			/* Set PSIK divider */
			LL_RCC_PSIK_SetDivider(psik_div(STM32_PSIK_DIVIDER));

			/* Enable PSIK */
			LL_RCC_PSIK_Enable();
			while (LL_RCC_PSIK_IsReady() != 1) {
				/* Wait for PSIK ready */
			}
		}
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		stm32_backup_domain_enable_access();

		/* Configure driving capability */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_RTCCR_LSEDRV_Pos);

		if (IS_ENABLED(STM32_LSE_BYPASS)) {
			/* Configure LSE bypass */
			LL_RCC_LSE_EnableBypass();
		}

		/* Enable LSE Oscillator */
		LL_RCC_LSE_Enable();
		/* Wait for LSE ready */
		while (!LL_RCC_LSE_IsReady()) {
		}

		stm32_backup_domain_disable_access();
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		stm32_backup_domain_enable_access();

		/* Enable LSI oscillator */
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
		}

		stm32_backup_domain_disable_access();
	}
}

void stm32_clock_control_set_latency(void)
{
	uint32_t latency;
	uint32_t delay;

	if (IN_RANGE(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, MHZ(136), MHZ(144))) {
		latency = LL_FLASH_LATENCY_4WS;
		delay = LL_FLASH_PROGRAM_DELAY_2;
	} else if (IN_RANGE(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, MHZ(102), MHZ(136))) {
		latency = LL_FLASH_LATENCY_3WS;
		delay = LL_FLASH_PROGRAM_DELAY_1;
	} else if (IN_RANGE(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, MHZ(68), MHZ(102))) {
		latency = LL_FLASH_LATENCY_2WS;
		delay = LL_FLASH_PROGRAM_DELAY_1;
	} else if (IN_RANGE(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, MHZ(34), MHZ(68))) {
		latency = LL_FLASH_LATENCY_1WS;
		delay = LL_FLASH_PROGRAM_DELAY_0;
	} else if (IN_RANGE(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, MHZ(0), MHZ(34))) {
		latency = LL_FLASH_LATENCY_0WS;
		delay = LL_FLASH_PROGRAM_DELAY_0;
	} else {
		__ASSERT(0, "Invalid SYSCLK");
	}

	LL_FLASH_SetLatency(FLASH, latency);
	LL_FLASH_SetProgrammingDelay(FLASH, delay);

	/* Read back the ACR register to make sure the values are correctly written */
	if ((LL_FLASH_GetLatency(FLASH) != latency) ||
	    (LL_FLASH_GetProgrammingDelay(FLASH) != delay)) {
		__ASSERT(0, "Error setting Flash latency");
	}
}

int stm32_clock_control_init(const struct device *dev)
{
	uint32_t old_hclk_freq;

	ARG_UNUSED(dev);

	/* Current hclk value */
	old_hclk_freq = get_sysclk_frequency() >> (LL_RCC_GetAHBPrescaler() >> RCC_CFGR2_HPRE_Pos);

	/* Set flash latency */
	if (old_hclk_freq < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		stm32_clock_control_set_latency();
	}

	/* Set up individual enabled clocks */
	set_up_fixed_clock_sources();

	/* Set peripheral buses prescalers */
	LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_AHB_PRESCALER));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
	LL_RCC_SetAPB3Prescaler(apb3_prescaler(STM32_APB3_PRESCALER));

	if (IS_ENABLED(STM32_SYSCLK_SRC_PSIS)) {
		/* Set PSIS as System Clock Source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PSIS);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PSIS) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSE)) {
		/* Set HSE as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSIS)) {
		/* Set MSIS as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSIS);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSIS) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSIDIV3)) {
		/* Set HSI as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSIDIV3);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSIDIV3) {
		}
	} else {
		return -ENOTSUP;
	}

	/* Set FLASH latency */
	/* If freq not increased, set flash latency after all clock setting */
	if (old_hclk_freq >= CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		stm32_clock_control_set_latency();
	}

	/* Update CMSIS variable */
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    stm32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &stm32_clock_control_api);
