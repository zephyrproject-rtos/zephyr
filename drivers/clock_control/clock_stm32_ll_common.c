/*
 * Copyright (c) 2017-2022 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
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
#include "clock_stm32_ll_common.h"
#include "stm32_hsem.h"

/* Macros to fill up prescaler values */
#define fn_ahb_prescaler(v) LL_RCC_SYSCLK_DIV_ ## v
#define ahb_prescaler(v) fn_ahb_prescaler(v)

#define fn_apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) fn_apb1_prescaler(v)

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), apb2_prescaler)
#define fn_apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) fn_apb2_prescaler(v)
#endif

#define fn_mco1_prescaler(v) LL_RCC_MCO1_DIV_ ## v
#define mco1_prescaler(v) fn_mco1_prescaler(v)

#define fn_mco2_prescaler(v) LL_RCC_MCO2_DIV_ ## v
#define mco2_prescaler(v) fn_mco2_prescaler(v)

#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
#define RCC_CALC_FLASH_FREQ __LL_RCC_CALC_HCLK4_FREQ
#define GET_CURRENT_FLASH_PRESCALER LL_RCC_GetAHB4Prescaler
#elif DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
#define RCC_CALC_FLASH_FREQ __LL_RCC_CALC_HCLK3_FREQ
#define GET_CURRENT_FLASH_PRESCALER LL_RCC_GetAHB3Prescaler
#else
#define RCC_CALC_FLASH_FREQ __LL_RCC_CALC_HCLK_FREQ
#define GET_CURRENT_FLASH_PRESCALER LL_RCC_GetAHBPrescaler
#endif

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

__unused
static uint32_t get_msi_frequency(void)
{
#if defined(STM32_MSI_ENABLED)
#if !defined(LL_RCC_MSIRANGESEL_RUN)
	return __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
#else
	return __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGESEL_RUN,
				      LL_RCC_MSI_GetRange());
#endif
#endif
	return 0;
}

/** @brief Verifies clock is part of active clock configuration */
__unused
static int enabled_clock(uint32_t src_clk)
{
	int r = 0;

	switch (src_clk) {
#if defined(STM32_SRC_SYSCLK)
	case STM32_SRC_SYSCLK:
		break;
#endif /* STM32_SRC_SYSCLK */
#if defined(STM32_SRC_PCLK)
	case STM32_SRC_PCLK:
		break;
#endif /* STM32_SRC_PCLK */
#if defined(STM32_SRC_HSE)
	case STM32_SRC_HSE:
		if (!IS_ENABLED(STM32_HSE_ENABLED)) {
			r = -ENOTSUP;
		}
		break;
#endif /* STM32_SRC_HSE */
#if defined(STM32_SRC_HSI)
	case STM32_SRC_HSI:
		if (!IS_ENABLED(STM32_HSI_ENABLED)) {
			r = -ENOTSUP;
		}
		break;
#endif /* STM32_SRC_HSI */
#if defined(STM32_SRC_LSE)
	case STM32_SRC_LSE:
		if (!IS_ENABLED(STM32_LSE_ENABLED)) {
			r = -ENOTSUP;
		}
		break;
#endif /* STM32_SRC_LSE */
#if defined(STM32_SRC_LSI)
	case STM32_SRC_LSI:
		if (!IS_ENABLED(STM32_LSI_ENABLED)) {
			r = -ENOTSUP;
		}
		break;
#endif /* STM32_SRC_LSI */
#if defined(STM32_SRC_MSI)
	case STM32_SRC_MSI:
		if (!IS_ENABLED(STM32_MSI_ENABLED)) {
			r = -ENOTSUP;
		}
		break;
#endif /* STM32_SRC_MSI */
#if defined(STM32_SRC_PLLCLK)
	case STM32_SRC_PLLCLK:
		if (!IS_ENABLED(STM32_PLL_ENABLED)) {
			r = -ENOTSUP;
		}
		break;
#endif /* STM32_SRC_PLLCLK */
	default:
		return -ENOTSUP;
	}

	return r;
}

static inline int stm32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile uint32_t *reg;
	uint32_t reg_val;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == 0) {
		/* Attemp to change a wrong periph clock bit */
		return -ENOTSUP;
	}

	reg = (uint32_t *)(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
	reg_val = *reg;
	reg_val |= pclken->enr;
	*reg = reg_val;

	return 0;
}

static inline int stm32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile uint32_t *reg;
	uint32_t reg_val;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == 0) {
		/* Attemp to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	reg = (uint32_t *)(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
	reg_val = *reg;
	reg_val &= ~pclken->enr;
	*reg = reg_val;

	return 0;
}

static inline int stm32_clock_control_configure(const struct device *dev,
						clock_control_subsys_t sub_system,
						void *data)
{
#if defined(STM32_SRC_CLOCK_MIN)
	/* At least one alt src clock available */
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile uint32_t *reg;
	uint32_t reg_val, dt_val;
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	dt_val = STM32_CLOCK_VAL_GET(pclken->enr) <<
					STM32_CLOCK_SHIFT_GET(pclken->enr);
	reg = (uint32_t *)(DT_REG_ADDR(DT_NODELABEL(rcc)) +
					STM32_CLOCK_REG_GET(pclken->enr));
	reg_val = *reg;
	reg_val |= dt_val;
	*reg = reg_val;

	return 0;
#else
	/* No src clock available: Not supported */
	return -ENOTSUP;
#endif
}

static int stm32_clock_control_get_subsys_rate(const struct device *clock,
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
#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), apb2_prescaler)
	uint32_t apb2_clock = get_bus_clock(ahb_clock, STM32_APB2_PRESCALER);
#elif defined(STM32_CLOCK_BUS_APB2)
	/* APB2 bus exists, but w/o dedicated prescaler */
	uint32_t apb2_clock = apb1_clock;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
	uint32_t ahb3_clock = get_bus_clock(ahb_clock * STM32_CPU1_PRESCALER,
					    STM32_AHB3_PRESCALER);
#elif defined(STM32_CLOCK_BUS_AHB3)
	/* AHB3 bus exists, but w/o dedicated prescaler */
	uint32_t ahb3_clock = ahb_clock;
#endif

#if defined(STM32_SRC_PCLK)
	if (pclken->bus == STM32_SRC_PCLK) {
		/* STM32_SRC_PCLK can't be used to request a subsys freq */
		/* Use STM32_CLOCK_BUS_FOO instead. */
		return -ENOTSUP;
	}
#endif

	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
#if defined(STM32_CLOCK_BUS_AHB2)
	case STM32_CLOCK_BUS_AHB2:
#endif
#if defined(STM32_CLOCK_BUS_IOP)
	case STM32_CLOCK_BUS_IOP:
#endif
		*rate = ahb_clock;
		break;
#if defined(STM32_CLOCK_BUS_AHB3)
	case STM32_CLOCK_BUS_AHB3:
		*rate = ahb3_clock;
		break;
#endif
	case STM32_CLOCK_BUS_APB1:
#if defined(STM32_CLOCK_BUS_APB1_2)
	case STM32_CLOCK_BUS_APB1_2:
#endif
		*rate = apb1_clock;
		break;
#if defined(STM32_CLOCK_BUS_APB2)
	case STM32_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
#endif
#if defined(STM32_CLOCK_BUS_APB3)
	case STM32_CLOCK_BUS_APB3:
		/* STM32WL: AHB3 and APB3 share the same clock and prescaler. */
		*rate = ahb3_clock;
		break;
#endif
#if defined(STM32_SRC_SYSCLK)
	case STM32_SRC_SYSCLK:
		*rate = SystemCoreClock * STM32_CORE_PRESCALER;
		break;
#endif
#if defined(STM32_SRC_PLLCLK) & defined(STM32_SYSCLK_SRC_PLL)
	case STM32_SRC_PLLCLK:
		if (get_pllout_frequency() == 0) {
			return -EIO;
		}
		*rate = get_pllout_frequency();
		break;
#endif
#if defined(STM32_SRC_LSE)
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
#endif
#if defined(STM32_SRC_LSI)
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
#endif
#if defined(STM32_SRC_HSI)
	case STM32_SRC_HSI:
		*rate = STM32_HSI_FREQ;
		break;
#endif
#if defined(STM32_SRC_HSE)
	case STM32_SRC_HSE:
		*rate = STM32_HSE_FREQ;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct clock_control_driver_api stm32_clock_control_api = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.configure = stm32_clock_control_configure,
};

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
}

/*
 * MCO configure doesn't active requested clock source,
 * so please make sure the clock source was enabled.
 */
static inline void stm32_clock_control_mco_init(void)
{
#ifndef CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK
	LL_RCC_ConfigMCO(MCO1_SOURCE,
			 mco1_prescaler(CONFIG_CLOCK_STM32_MCO1_DIV));
#endif /* CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK */

#ifndef CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK
	LL_RCC_ConfigMCO(MCO2_SOURCE,
			 mco2_prescaler(CONFIG_CLOCK_STM32_MCO2_DIV));
#endif /* CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK */
}

__unused
static void set_up_plls(void)
{
#if defined(STM32_PLL_ENABLED)

	/*
	 * Case of chain-loaded applications:
	 * Switch to HSI and disable the PLL before configuration.
	 * (Switching to HSI makes sure we have a SYSCLK source in
	 * case we're currently running from the PLL we're about to
	 * turn off and reconfigure.)
	 *
	 */
	if (LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
		LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
		stm32_clock_switch_to_hsi();
	}
	LL_RCC_PLL_Disable();

#ifdef CONFIG_SOC_SERIES_STM32F7X
	/* Assuming we stay on Power Scale default value: Power Scale 1 */
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 180000000) {
		LL_PWR_EnableOverDriveMode();
		while (LL_PWR_IsActiveFlag_OD() != 1) {
		/* Wait for OverDrive mode ready */
		}
		LL_PWR_EnableOverDriveSwitching();
		while (LL_PWR_IsActiveFlag_ODSW() != 1) {
		/* Wait for OverDrive switch ready */
		}
	}
#endif

#if STM32_PLL_Q_ENABLED
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ,
		   STM32_PLL_Q_DIVISOR
		   << RCC_PLLCFGR_PLLQ_Pos);
#endif

	config_pll_sysclock();

	/* Enable PLL */
	LL_RCC_PLL_Enable();
	while (LL_RCC_PLL_IsReady() != 1U) {
	/* Wait for PLL ready */
	}

#endif /* STM32_PLL_ENABLED */
}

static void set_up_fixed_clock_sources(void)
{

	if (IS_ENABLED(STM32_HSE_ENABLED)) {
#if defined(STM32_HSE_BYPASS)
		/* Check if need to enable HSE bypass feature or not */
		if (IS_ENABLED(STM32_HSE_BYPASS)) {
			LL_RCC_HSE_EnableBypass();
		} else {
			LL_RCC_HSE_DisableBypass();
		}
#endif
#if STM32_HSE_TCXO
		LL_RCC_HSE_EnableTcxo();
#endif
#if STM32_HSE_DIV2
		LL_RCC_HSE_EnableDiv2();
#endif
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

#if defined(STM32_MSI_ENABLED)
	if (IS_ENABLED(STM32_MSI_ENABLED)) {
		/* Set MSI Range */
#if defined(RCC_CR_MSIRGSEL)
		LL_RCC_MSI_EnableRangeSelection();
#endif /* RCC_CR_MSIRGSEL */

#if defined(CONFIG_SOC_SERIES_STM32L0X) || defined(CONFIG_SOC_SERIES_STM32L1X)
		LL_RCC_MSI_SetRange(STM32_MSI_RANGE << RCC_ICSCR_MSIRANGE_Pos);
#else
		LL_RCC_MSI_SetRange(STM32_MSI_RANGE << RCC_CR_MSIRANGE_Pos);
#endif /* CONFIG_SOC_SERIES_STM32L0X || CONFIG_SOC_SERIES_STM32L1X */

#if STM32_MSI_PLL_MODE
		/* Enable MSI hardware auto calibration */
		LL_RCC_MSI_EnablePLLMode();
#endif

		LL_RCC_MSI_SetCalibTrimming(0);

		/* Enable MSI if not enabled */
		if (LL_RCC_MSI_IsReady() != 1) {
			/* Enable MSI */
			LL_RCC_MSI_Enable();
			while (LL_RCC_MSI_IsReady() != 1) {
				/* Wait for MSI ready */
			}
		}
	}
#endif /* STM32_MSI_ENABLED */

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
#if defined(CONFIG_SOC_SERIES_STM32WBX)
		LL_RCC_LSI1_Enable();
		while (LL_RCC_LSI1_IsReady() != 1) {
		}
#else
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
		}
#endif
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		/* LSE belongs to the back-up domain, enable access.*/

		z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);

		/* Set the DBP bit in the Power control register 1 (PWR_CR1) */
		LL_PWR_EnableBkUpAccess();
		while (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Wait for Backup domain access */
		}

#if STM32_LSE_DRIVING
		/* Configure driving capability */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_BDCR_LSEDRV_Pos);
#endif

		/* Enable LSE Oscillator (32.768 kHz) */
		LL_RCC_LSE_Enable();
		while (!LL_RCC_LSE_IsReady()) {
			/* Wait for LSE ready */
		}

		LL_PWR_DisableBkUpAccess();

		z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
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
	ARG_UNUSED(dev);

	/* Some clocks would be activated by default */
	config_enable_default_clocks();

#if defined(FLASH_ACR_LATENCY)
	uint32_t old_flash_freq;
	uint32_t new_flash_freq;

	old_flash_freq = RCC_CALC_FLASH_FREQ(HAL_RCC_GetSysClockFreq(),
					       GET_CURRENT_FLASH_PRESCALER());

	new_flash_freq = RCC_CALC_FLASH_FREQ(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
				      STM32_FLASH_PRESCALER);

	/* If freq increases, set flash latency before any clock setting */
	if (old_flash_freq < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LL_SetFlashLatency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}
#endif /* FLASH_ACR_LATENCY */

	/* Set up indiviual enabled clocks */
	set_up_fixed_clock_sources();

	/* Set up PLLs */
	set_up_plls();

	if (DT_PROP(DT_NODELABEL(rcc), undershoot_prevention) &&
		(STM32_CORE_PRESCALER == LL_RCC_SYSCLK_DIV_1) &&
		(MHZ(80) < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)) {
		LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);
	} else {
		LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_CORE_PRESCALER));
	}

#if STM32_SYSCLK_SRC_PLL
	/* Set PLL as System Clock Source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
	}
#elif STM32_SYSCLK_SRC_HSE
	/* Set HSE as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
	}
#elif STM32_SYSCLK_SRC_MSI
	/* Set MSI as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI) {
	}
#elif STM32_SYSCLK_SRC_HSI
	stm32_clock_switch_to_hsi();
#endif /* STM32_SYSCLK_SRC_... */

	if (DT_PROP(DT_NODELABEL(rcc), undershoot_prevention) &&
		(STM32_CORE_PRESCALER == LL_RCC_SYSCLK_DIV_1) &&
		(MHZ(80) < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)) {
		LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_CORE_PRESCALER));
	}

#if defined(FLASH_ACR_LATENCY)
	/* If freq not increased, set flash latency after all clock setting */
	if (old_flash_freq >= CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LL_SetFlashLatency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}
#endif /* FLASH_ACR_LATENCY */

	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	/* Set bus prescalers prescaler */
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), apb2_prescaler)
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), cpu2_prescaler)
	LL_C2_RCC_SetAHBPrescaler(ahb_prescaler(STM32_CPU2_PRESCALER));
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb3_prescaler)
	LL_RCC_SetAHB3Prescaler(ahb_prescaler(STM32_AHB3_PRESCALER));
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(rcc), ahb4_prescaler)
	LL_RCC_SetAHB4Prescaler(ahb_prescaler(STM32_AHB4_PRESCALER));
#endif

	/* configure MCO1/MCO2 based on Kconfig */
	stm32_clock_control_mco_init();

	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    &stm32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &stm32_clock_control_api);
