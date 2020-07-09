/*
 *
 * Copyright (c) 2019 Linaro Limited.
 * Copyright (c) 2020 Jeremy LOCHE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "stm32_hsem.h"

/* Macros to fill up prescaler values */
#define z_sysclk_prescaler(v) LL_RCC_SYSCLK_DIV_ ## v
#define sysclk_prescaler(v) z_sysclk_prescaler(v)

#define z_ahb_prescaler(v) LL_RCC_AHB_DIV_ ## v
#define ahb_prescaler(v) z_ahb_prescaler(v)

#define z_apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) z_apb1_prescaler(v)

#define z_apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) z_apb2_prescaler(v)

#define z_apb3_prescaler(v) LL_RCC_APB3_DIV_ ## v
#define apb3_prescaler(v) z_apb3_prescaler(v)

#define z_apb4_prescaler(v) LL_RCC_APB4_DIV_ ## v
#define apb4_prescaler(v) z_apb4_prescaler(v)

#if defined(CONFIG_CPU_CORTEX_M7)
#if CONFIG_CLOCK_STM32_D1CPRE > 1
/*
 * D1CPRE prescaler allows to set a HCLK frequency lower than SYSCLK frequency.
 * Though, zephyr doesn't make a difference today between these two clocks.
 * So, changing this prescaler is not allowed until it is made possible to
 * use them independently in zephyr clock subsystem.
 */
#error "D1CPRE presacler can't be higher than 1"
#endif
#endif /* CONFIG_CPU_CORTEX_M7 */

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

#if !defined(CONFIG_CPU_CORTEX_M4)

static int32_t prepare_regulator_voltage_scale(void)
{
	/* Make sure to put the CPU in highest Voltage scale during clock configuration */
	LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
	/* Highest voltage is SCALE0 */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
	return 0;
}

static int32_t optimize_regulator_voltage_scale(uint32_t sysclk_freq)
{

	/* After sysclock is configured, tweak the voltage scale down */
	/* to reduce power consumption */

	/* Needs some smart work to configure properly */
	/* LL_PWR_REGULATOR_SCALE3 is lowest power consumption */
	/* Must be done in accordance to the Maximum allowed frequency vs VOS*/
	/* See RM0433 page 352 for more details */
	LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
	return 0;
}

#if defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)

static int32_t get_vco_output_range(uint32_t vco_input_range)
{
	if (vco_input_range == LL_RCC_PLLINPUTRANGE_1_2) {
		return LL_RCC_PLLVCORANGE_MEDIUM;
	}

	return LL_RCC_PLLVCORANGE_WIDE;
}

static int32_t get_vco_input_range(uint32_t pllsrc_clock, uint32_t divm)
{
	const uint32_t input_freq = pllsrc_clock/divm;

	__ASSERT(input_freq < 1000000UL || input_freq > 16000000UL,
			"PLL1 VCO frequency input range out of range");

	if (1000000UL <= input_freq && input_freq <= 2000000UL) {
		return LL_RCC_PLLINPUTRANGE_1_2;
	} else if (2000000UL < input_freq && input_freq <= 4000000UL) {
		return LL_RCC_PLLINPUTRANGE_2_4;
	} else if (4000000UL < input_freq && input_freq <= 8000000UL) {
		return LL_RCC_PLLINPUTRANGE_4_8;
	} else if (8000000UL < input_freq && input_freq <= 16000000UL) {
		return LL_RCC_PLLINPUTRANGE_8_16;
	}

	return -ERANGE;

}

#endif /* CONFIG_CLOCK_STM32_PLL_SRC_* */

#endif /* ! CONFIG_CPU_CORTEX_M4 */

static inline int stm32_clock_control_on(struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int rc = 0;

	ARG_UNUSED(dev);

	/* Both cores can access bansk by following LL API */
	/* Using "_Cn_" LL API would restrict access to one or the other */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
		LL_AHB1_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB3:
		LL_AHB3_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB4:
		LL_AHB4_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB3:
		LL_APB3_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB4:
		LL_APB4_GRP1_EnableClock(pclken->enr);
		break;
	default:
		rc = -ENOTSUP;
		break;
	}

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	return rc;
}

static inline int stm32_clock_control_off(struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int rc = 0;

	ARG_UNUSED(dev);

	/* Both cores can access bansk by following LL API */
	/* Using "_Cn_" LL API would restrict access to one or the other */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
		LL_AHB1_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB3:
		LL_AHB3_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB4:
		LL_AHB4_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB3:
		LL_APB3_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB4:
		LL_APB4_GRP1_DisableClock(pclken->enr);
		break;
	default:
		rc = -ENOTSUP;
		break;
	}
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	return rc;
}

static int stm32_clock_control_get_subsys_rate(struct device *clock,
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
#if defined(CONFIG_CPU_CORTEX_M4)
	uint32_t ahb_clock = SystemCoreClock;
#else
	uint32_t ahb_clock = get_bus_clock(SystemCoreClock,
				CONFIG_CLOCK_STM32_HPRE);
#endif
	uint32_t apb1_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_D2PPRE1);
	uint32_t apb2_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_D2PPRE2);
	uint32_t apb3_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_D1PPRE);
	uint32_t apb4_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_D3PPRE);

	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB3:
	case STM32_CLOCK_BUS_AHB4:
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_APB1:
	case STM32_CLOCK_BUS_APB1_2:
		*rate = apb1_clock;
		break;
	case STM32_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
	case STM32_CLOCK_BUS_APB3:
		*rate = apb3_clock;
		break;
	case STM32_CLOCK_BUS_APB4:
		*rate = apb4_clock;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct clock_control_driver_api stm32_clock_control_api = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
};

static int stm32_clock_control_init(struct device *dev)
{

#if !defined(CONFIG_CPU_CORTEX_M4)
	uint32_t pllsrc_clock = 0;

#if defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)

	int32_t vco_input_range = 0;
	int32_t vco_output_range = 0;
#endif /* CONFIG_CLOCK_STM32_PLL_SRC_* */

#endif /* ! CONFIG_CPU_CORTEX_M4 */

	ARG_UNUSED(dev);

#if !defined(CONFIG_CPU_CORTEX_M4)

	/* HW semaphore Clock enable */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	/* Configure Voltage scale to comply with the desired system frequency */
	prepare_regulator_voltage_scale();

	/* Configure PLL source */
	/* Can be HSE , HSI 64Mhz/HSIDIV, CSI 4MHz*/
#if defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE)

	if (IS_ENABLED(CONFIG_CLOCK_STM32_HSE_BYPASS)) {
		LL_RCC_HSE_EnableBypass();
	} else {
		LL_RCC_HSE_DisableBypass();
	}

	/* Enable HSE oscillator */
	LL_RCC_HSE_Enable();
	while (LL_RCC_HSE_IsReady() != 1) {
	}

	/* Main PLL configuration and activation */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);

	pllsrc_clock = HSE_VALUE;

#elif defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)
	/* Support for CSI oscillator */

	LL_RCC_CSI_Enable();
	while (LL_RCC_CSI_IsReady() != 1) {
	}

	/* Main PLL configuration and activation */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_CSI);

	pllsrc_clock = CSI_VALUE;

#elif defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI)
	/* By default choose HSI as PLL clock source */

	/* Enable HSI oscillator */
	LL_RCC_HSI_Enable();
	while (LL_RCC_HSI_IsReady() != 1) {
	}

	/* Calibrate the HSI */
	LL_RCC_HSI_SetCalibTrimming(32);
	/* @TODO make HSI divider configurable */
	LL_RCC_HSI_SetDivider(LL_RCC_HSI_DIV1);

	/* Main PLL configuration and activation */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSI);

	pllsrc_clock = HSI_VALUE;

#else

	/* No clock source selected for PLL, by default, disable the PLL */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_NONE);
	pllsrc_clock = 0;
#endif

	/* Configure the PLL dividers/multipliers only if PLL source is configured */
#if defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)


	vco_input_range = get_vco_input_range(
			pllsrc_clock,
			CONFIG_CLOCK_STM32_PLL_M_DIVISOR);

	__ASSERT(vco_input_range == -ERANGE, "PLL VCO input frequency out of range. Should be from 1 to 16 MHz");

	vco_output_range = get_vco_output_range(vco_input_range);


	/* Configure PLL1 */
	/* According to the RM0433 datasheet */
	/* Select clock source */
	/* Init pre divider DIVM */
	LL_RCC_PLL1_SetM(CONFIG_CLOCK_STM32_PLL_M_DIVISOR);
	/* Config PLL */

	/* VCO sel, VCO range */
	LL_RCC_PLL1_SetVCOInputRange(vco_input_range);
	LL_RCC_PLL1_SetVCOOutputRange(vco_output_range);

	/* FRACN disable DIVP,DIVQ,DIVR enable*/
	LL_RCC_PLL1FRACN_Disable();
	LL_RCC_PLL1P_Enable();
	LL_RCC_PLL1Q_Enable();
	LL_RCC_PLL1R_Enable();

	/* DIVN,DIVP,DIVQ,DIVR div*/
	LL_RCC_PLL1_SetN(CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER);
	LL_RCC_PLL1_SetP(CONFIG_CLOCK_STM32_PLL_P_DIVISOR);
	LL_RCC_PLL1_SetQ(CONFIG_CLOCK_STM32_PLL_Q_DIVISOR);
	LL_RCC_PLL1_SetR(CONFIG_CLOCK_STM32_PLL_R_DIVISOR);


#else
	/* PLL will stay in reset state configuration */
#endif /* CONFIG_CLOCK_STM32_PLL_SRC_* */

#if defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL)

	/* Enable PLL*/
	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1) {
	}


	/* Set PLL1 as System Clock Source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
	}

#elif defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE)

	/* Enable HSI oscillator */
	LL_RCC_HSE_Enable();
	while (LL_RCC_HSE_IsReady() != 1) {
	}

	/* Set sysclk source to HSE */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
	}

#elif defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI)

	/* Enable HSI oscillator */
	LL_RCC_HSI_Enable();
	while (LL_RCC_HSI_IsReady() != 1) {
	}

	/* Set sysclk source to HSI */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}

#elif defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_CSI)

	/* Enable CSI oscillator */
	LL_RCC_CSI_Enable();
	while (LL_RCC_CSI_IsReady() != 1) {
	}

	/* Set sysclk source to CSI */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_CSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_CSI) {
	}

#endif /* CLOCK_STM32_SYSCLK_SRC */

	/* Set buses (Sys,AHB, APB1, APB2 & APB4) prescalers */
	LL_RCC_SetSysPrescaler(sysclk_prescaler(CONFIG_CLOCK_STM32_D1CPRE));
	LL_RCC_SetAHBPrescaler(ahb_prescaler(CONFIG_CLOCK_STM32_HPRE));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(CONFIG_CLOCK_STM32_D2PPRE1));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(CONFIG_CLOCK_STM32_D2PPRE2));
	LL_RCC_SetAPB3Prescaler(apb3_prescaler(CONFIG_CLOCK_STM32_D1PPRE));
	LL_RCC_SetAPB4Prescaler(apb4_prescaler(CONFIG_CLOCK_STM32_D3PPRE));

	/* Set FLASH latency */
	/* AXI clock is SYSCLK / HPRE */
	LL_SetFlashLatency(get_bus_clock(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
									 CONFIG_CLOCK_STM32_HPRE));


	optimize_regulator_voltage_scale(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

#endif /* CONFIG_CPU_CORTEX_M4 */

	/* Set systick to 1ms */
	SysTick_Config(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000);
	/* Update CMSIS variable */
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_AND_API_INIT(rcc_stm32, STM32_CLOCK_CONTROL_NAME,
		    &stm32_clock_control_init,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_STM32_DEVICE_INIT_PRIORITY,
		    &stm32_clock_control_api);
