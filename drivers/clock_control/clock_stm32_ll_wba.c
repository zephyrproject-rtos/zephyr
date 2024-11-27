/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <stm32_ll_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "stm32_hsem.h"

/* Macros to fill up prescaler values */
#define fn_ahb_prescaler(v) LL_RCC_SYSCLK_DIV_ ## v
#define ahb_prescaler(v) fn_ahb_prescaler(v)

#define fn_ahb5_prescaler(v) LL_RCC_AHB5_DIV_ ## v
#define ahb5_prescaler(v) fn_ahb5_prescaler(v)

#define fn_apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) fn_apb1_prescaler(v)

#define fn_apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) fn_apb2_prescaler(v)

#define fn_apb7_prescaler(v) LL_RCC_APB7_DIV_ ## v
#define apb7_prescaler(v) fn_apb7_prescaler(v)

#define RCC_CALC_FLASH_FREQ __LL_RCC_CALC_HCLK_FREQ
#define GET_CURRENT_FLASH_PRESCALER LL_RCC_GetAHBPrescaler

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

/** @brief Verifies clock is part of active clock configuration */
int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_SYSCLK) ||
	    (src_clk == STM32_SRC_HCLK1) ||
	    (src_clk == STM32_SRC_HCLK5) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK7) ||
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI16) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_P) && IS_ENABLED(STM32_PLL_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_Q) && IS_ENABLED(STM32_PLL_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_R) && IS_ENABLED(STM32_PLL_R_ENABLED))) {
		return 0;
	}

	return -ENOTSUP;
}

static inline int stm32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile int temp;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == 0) {
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

static inline int stm32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == 0) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		       pclken->enr);

	return 0;
}

static inline int stm32_clock_control_configure(const struct device *dev,
						clock_control_subsys_t sub_system,
						void *data)
{
#if defined(STM32_SRC_CLOCK_MIN)
	/* At least one alt src clock available */
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_CLOCK_REG_GET(pclken->enr),
		       STM32_CLOCK_MASK_GET(pclken->enr) << STM32_CLOCK_SHIFT_GET(pclken->enr));
	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_CLOCK_REG_GET(pclken->enr),
		     STM32_CLOCK_VAL_GET(pclken->enr) << STM32_CLOCK_SHIFT_GET(pclken->enr));

	return 0;
#else
	/* No src clock available: Not supported */
	return -ENOTSUP;
#endif
}

__unused
static uint32_t get_pllsrc_frequency(void)
{

	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		return STM32_HSI_FREQ;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		return STM32_HSE_FREQ;
	}

	__ASSERT(0, "No PLL Source configured");
	return 0;
}

__unused
static uint32_t get_pllsrc(void)
{

	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		return LL_RCC_PLL1SOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		return LL_RCC_PLL1SOURCE_HSE;
	}

	__ASSERT(0, "No PLL Source configured");
	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	/*
	 * Get AHB Clock (= SystemCoreClock = SYSCLK/prescaler)
	 * SystemCoreClock is preferred to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
	 * since it will be updated after clock configuration and hence
	 * more likely to contain actual clock speed
	 */
	uint32_t ahb_clock = SystemCoreClock;
	uint32_t apb1_clock = get_bus_clock(ahb_clock, STM32_APB1_PRESCALER);
	uint32_t apb2_clock = get_bus_clock(ahb_clock, STM32_APB2_PRESCALER);
	uint32_t apb7_clock = get_bus_clock(ahb_clock, STM32_APB7_PRESCALER);
	uint32_t ahb5_clock;

	ARG_UNUSED(dev);

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		/* PLL is the SYSCLK source, use 'ahb5-prescaler' */
		ahb5_clock = get_bus_clock(ahb_clock * STM32_AHB_PRESCALER,
							STM32_AHB5_PRESCALER);
	} else {
		/* PLL is not the SYSCLK source, use 'ahb5-div'(if set) */
		if (IS_ENABLED(STM32_AHB5_DIV)) {
			ahb5_clock = ahb_clock * STM32_AHB_PRESCALER / 2;
		} else {
			ahb5_clock = ahb_clock * STM32_AHB_PRESCALER;
		}

	}

	__ASSERT(ahb5_clock <= MHZ(32), "AHB5 clock frequency exceeds 32 MHz");

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB4:
	case STM32_SRC_HCLK1:
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_AHB5:
	case STM32_SRC_HCLK5:
		*rate = ahb5_clock;
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
	case STM32_CLOCK_BUS_APB7:
	case STM32_SRC_PCLK7:
		*rate = apb7_clock;
		break;
	case STM32_SRC_SYSCLK:
		*rate = SystemCoreClock * STM32_CORE_PRESCALER;
		break;
#if defined(STM32_PLL_ENABLED)
	case STM32_SRC_PLL1_P:
		*rate = __LL_RCC_CALC_PLL1PCLK_FREQ(get_pllsrc_frequency(),
						    STM32_PLL_M_DIVISOR,
						    STM32_PLL_N_MULTIPLIER,
						    STM32_PLL_P_DIVISOR);
		break;
	case STM32_SRC_PLL1_Q:
		*rate = __LL_RCC_CALC_PLL1QCLK_FREQ(get_pllsrc_frequency(),
						    STM32_PLL_M_DIVISOR,
						    STM32_PLL_N_MULTIPLIER,
						    STM32_PLL_Q_DIVISOR);
		break;
	case STM32_SRC_PLL1_R:
		*rate = __LL_RCC_CALC_PLL1RCLK_FREQ(get_pllsrc_frequency(),
						    STM32_PLL_M_DIVISOR,
						    STM32_PLL_N_MULTIPLIER,
						    STM32_PLL_R_DIVISOR);
		break;
#endif /* STM32_PLL_ENABLED */
#if defined(STM32_LSE_ENABLED)
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
#endif
#if defined(STM32_LSI_ENABLED)
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
#endif
#if defined(STM32_HSI_ENABLED)
	case STM32_SRC_HSI16:
		*rate = STM32_HSI_FREQ;
		break;
#endif
#if defined(STM32_HSE_ENABLED)
	case STM32_SRC_HSE:
		if (IS_ENABLED(STM32_HSE_DIV2)) {
			*rate = STM32_HSE_FREQ / 2;
		} else {
			*rate = STM32_HSE_FREQ;
		}

		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == true) {
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

__unused
static int get_vco_input_range(uint32_t m_div, uint32_t *range)
{
	uint32_t vco_freq;

	vco_freq = get_pllsrc_frequency() / m_div;

	if (MHZ(4) <= vco_freq && vco_freq <= MHZ(8)) {
		*range = LL_RCC_PLLINPUTRANGE_4_8;
	} else if (MHZ(8) < vco_freq && vco_freq <= MHZ(16)) {
		*range = LL_RCC_PLLINPUTRANGE_8_16;
	} else {
		return -ERANGE;
	}

	return 0;
}

static void set_regu_voltage(uint32_t hclk_freq)
{
	if (hclk_freq <= MHZ(16)) {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
	} else {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	}
	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}
}

/*
 * Unconditionally switch the system clock source to HSI.
 */
__unused
static void stm32_clock_switch_to_hsi(void)
{
	/* Enable HSI if not enabled */
	if (LL_RCC_HSI_IsReady() != 1) {
		/* Enable HSI */
		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1) {
		/* Wait for HSI ready */
		}
	}

	/* Set HSI as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}

	/* Erratum 2.2.4: Spurious deactivation of HSE when HSI is selected as
	 * system clock source
	 * Re-enable HSE clock if required after switch source to HSI
	 */
	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		if (IS_ENABLED(STM32_HSE_DIV2)) {
			LL_RCC_HSE_EnablePrescaler();
		}

		/* Enable HSE */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
		/* Wait for HSE ready */
		}
	}
}

__unused
static int set_up_plls(void)
{
#if defined(STM32_PLL_ENABLED)
	int r;
	uint32_t vco_input_range;

	LL_RCC_PLL1_Disable();

	/* Configure PLL source */
	/* Can be HSE, HSI */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetMainSource(LL_RCC_PLL1SOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetMainSource(LL_RCC_PLL1SOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	r = get_vco_input_range(STM32_PLL_M_DIVISOR, &vco_input_range);
	if (r < 0) {
		return r;
	}

	LL_RCC_PLL1_SetDivider(STM32_PLL_M_DIVISOR);

	LL_RCC_PLL1_SetVCOInputRange(vco_input_range);

	LL_RCC_PLL1_SetN(STM32_PLL_N_MULTIPLIER);

	LL_RCC_PLL1FRACN_Disable();

	if (IS_ENABLED(STM32_PLL_P_ENABLED)) {
		LL_RCC_PLL1_SetP(STM32_PLL_P_DIVISOR);
		LL_RCC_PLL1_EnableDomain_PLL1P();
	}

	if (IS_ENABLED(STM32_PLL_Q_ENABLED)) {
		LL_RCC_PLL1_SetQ(STM32_PLL_Q_DIVISOR);
		LL_RCC_PLL1_EnableDomain_PLL1Q();
	}

	if (IS_ENABLED(STM32_PLL_R_ENABLED)) {
		LL_RCC_PLL1_SetR(STM32_PLL_R_DIVISOR);

		LL_RCC_PLL1_EnableDomain_PLL1R();
	}

	/* Enable PLL */
	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1U) {
	/* Wait for PLL ready */
	}
#else
	/* Init PLL source to None */
	LL_RCC_PLL1_SetMainSource(LL_RCC_PLL1SOURCE_NONE);
#endif /* STM32_PLL_ENABLED */

	return 0;
}

static void set_up_fixed_clock_sources(void)
{

	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		if (IS_ENABLED(STM32_HSE_DIV2)) {
			LL_RCC_HSE_EnablePrescaler();
		}

		/* Enable HSE */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
		/* Wait for HSE ready */
		}
	}

	if (IS_ENABLED(STM32_HSI_ENABLED)) {
		/* Enable HSI if not enabled */
		if (LL_RCC_HSI_IsReady() != 1) {
			/* Enable HSI */
			LL_RCC_HSI_Enable();
			while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
			}
		}
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		/* LSI belongs to the back-up domain, enable access.*/

		/* Set the DBP bit in the Power control register 1 (PWR_CR1) */
		LL_PWR_EnableBkUpAccess();
		while (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Wait for Backup domain access */
		}

		LL_RCC_LSI1_Enable();
		while (LL_RCC_LSI1_IsReady() != 1) {
		}

		LL_PWR_DisableBkUpAccess();
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		/* LSE belongs to the back-up domain, enable access.*/

		/* Set the DBP bit in the Power control register 1 (PWR_CR1) */
		LL_PWR_EnableBkUpAccess();
		while (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Wait for Backup domain access */
		}

		/* Configure driving capability */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_BDCR1_LSEDRV_Pos);

		/* Enable LSE Oscillator (32.768 kHz) */
		LL_RCC_LSE_Enable();
		while (!LL_RCC_LSE_IsReady()) {
			/* Wait for LSE ready */
		}

		/* Enable LSESYS additionally */
		LL_RCC_LSE_EnablePropagation();
		/* Wait till LSESYS is ready */
		while (!LL_RCC_LSE_IsPropagationReady()) {
		}
	}
}

/**
 * @brief Initialize clocks for the stm32
 *
 * This routine is called to enable and configure the clocks and PLL
 * of the soc on the board. It depends on the board definition.
 * This function is called on the startup and also to restore the config
 * when exiting for low power mode.
 *
 * @param dev clock device struct
 *
 * @return 0
 */
int stm32_clock_control_init(const struct device *dev)
{
	uint32_t old_flash_freq;
	int r;

	ARG_UNUSED(dev);

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL) &&
			(LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_PLL1R)) {
		/* In case of chainloaded application, it may happen that PLL
		 * was already configured as sysclk src by bootloader.
		 * Don't test other cases as there are multiple options but
		 * they will be handled smoothly by the function.
		 */
		SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		return 0;
	}

	old_flash_freq = RCC_CALC_FLASH_FREQ(HAL_RCC_GetSysClockFreq(),
					       GET_CURRENT_FLASH_PRESCALER());

	/* Set up individual enabled clocks */
	set_up_fixed_clock_sources();

	/* Set voltage regulator to comply with targeted system frequency */
	set_regu_voltage(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* If required, apply max step freq for Sysclock w/ PLL input */
	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		LL_RCC_PLL1_SetPLL1RCLKDivisionStep(LL_RCC_PLL1RCLK_2_STEP_DIV);

		/* Send 2 pulses on CLKPRE like it is done in STM32Cube HAL */
		LL_RCC_PLL1_DisablePLL1RCLKDivision();
		LL_RCC_PLL1_EnablePLL1RCLKDivision();
		LL_RCC_PLL1_DisablePLL1RCLKDivision();
		LL_RCC_PLL1_EnablePLL1RCLKDivision();
	}

	/* Set up PLLs */
	r = set_up_plls();
	if (r < 0) {
		return r;
	}

	/* If freq increases, set flash latency before any clock setting */
	if (old_flash_freq < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LL_SetFlashLatency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_CORE_PRESCALER));

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		/* PLL is the SYSCLK source, use 'ahb5-prescaler' */
		LL_RCC_SetAHB5Prescaler(ahb5_prescaler(STM32_AHB5_PRESCALER));
	} else {
		/* PLL is not the SYSCLK source, use 'ahb5-div'(if set) */
		if (IS_ENABLED(STM32_AHB5_DIV)) {
			LL_RCC_SetAHB5Divider(LL_RCC_AHB5_DIVIDER_2);
		} else {
			LL_RCC_SetAHB5Divider(LL_RCC_AHB5_DIVIDER_1);
		}
	}

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		/* Set PLL as System Clock Source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1R);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1R) {
		}
		LL_RCC_PLL1_DisablePLL1RCLKDivision();
		while (LL_RCC_PLL1_IsPLL1RCLKDivisionReady() == 0) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSE)) {
		/* Set HSE as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSI)) {
		stm32_clock_switch_to_hsi();
	}

	/* If freq not increased, set flash latency after all clock setting */
	if (old_flash_freq >= CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LL_SetFlashLatency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	/* Set voltage regulator to comply with targeted system frequency */
	set_regu_voltage(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	/* Set bus prescalers prescaler */
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
	LL_RCC_SetAPB7Prescaler(apb7_prescaler(STM32_APB7_PRESCALER));

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
