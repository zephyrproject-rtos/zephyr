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
#define z_hsi_divider(v) LL_RCC_HSI_DIV ## v
#define hsi_divider(v) z_hsi_divider(v)

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

/* Macro to check for clock feasability */
/* It is Cortex M7's responsibility to setup clock tree */
/* This check should only be performed for the M7 core code */
#ifdef CONFIG_CPU_CORTEX_M7

/* Define primary oscillator frequencies */
/* Suppress the cast to uint32_t which */
/* prevents from compare with #if > operator*/
/* original defines in stm32h7xx_hal_conf.h*/
#define HSI_FREQ 64000000UL	/* HSI_VALUE ((uint32_t)64000000) */
/* HSE_VALUE overridden by the build system without C cast to uint*/
/* Build system doesn't provide the UL type suffix for HSE_VALUE */
/* Force HSE_FREQ to be cast to preprocessor UL to prevent overflow*/
/* Only use these constants in preprocessor expressions */
#define HSE_FREQ HSE_VALUE
#define CSI_FREQ 4000000UL	/* CSI_VALUE ((uint32_t)4000000) */

/* Choose PLL SRC */
#ifdef CONFIG_CLOCK_STM32_PLL_SRC_HSI
#define PLLSRC_FREQ  ((HSI_FREQ)/(CONFIG_CLOCK_STM32_HSI_DIVISOR))
#elif defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)
#define PLLSRC_FREQ  CSI_FREQ
#elif defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE)
#define PLLSRC_FREQ  HSE_FREQ
#else
#define PLLSRC_FREQ 0
#endif

#define VCO_FREQ	(PLLSRC_FREQ / CONFIG_CLOCK_STM32_PLL_M_DIVISOR)

#if (1000000UL <= VCO_FREQ && VCO_FREQ <= 2000000UL)
#define VCO_INPUT_RANGE LL_RCC_PLLINPUTRANGE_1_2
#elif (2000000UL < VCO_FREQ && VCO_FREQ <= 4000000UL)
#define VCO_INPUT_RANGE LL_RCC_PLLINPUTRANGE_2_4
#elif (4000000UL < VCO_FREQ && VCO_FREQ <= 8000000UL)
#define VCO_INPUT_RANGE LL_RCC_PLLINPUTRANGE_4_8
#elif (8000000UL < VCO_FREQ && VCO_FREQ <= 16000000UL)
#define VCO_INPUT_RANGE LL_RCC_PLLINPUTRANGE_8_16
#else
#error "PLL1 VCO frequency input range out of range"
#endif

/* Given source clock and dividers, computed the output frequency of PLLP */
#define PLLP_FREQ(pllsrc_freq, divm, divn, divp)	(((pllsrc_freq)*\
							(divn))/((divm)*(divp)))

/* PLL P output frequency value */
#define PLLP_VALUE	PLLP_FREQ(\
				PLLSRC_FREQ,\
				CONFIG_CLOCK_STM32_PLL_M_DIVISOR,\
				CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER,\
				CONFIG_CLOCK_STM32_PLL_P_DIVISOR)

/* SYSCLKSRC before the D1CPRE prescaler */
#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL
#define SYSCLKSRC_FREQ	PLLP_VALUE
#elif defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI)
#define SYSCLKSRC_FREQ	((HSI_FREQ)/(CONFIG_CLOCK_STM32_HSI_DIVISOR))
#elif defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_CSI)
#define SYSCLKSRC_FREQ	CSI_FREQ
#elif defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE)
#define SYSCLKSRC_FREQ	HSE_FREQ
#endif

/* ARM Sys CPU Clock before HPRE prescaler */
#define SYSCLK_FREQ	((SYSCLKSRC_FREQ)/(CONFIG_CLOCK_STM32_D1CPRE))
#define AHB_FREQ	((SYSCLK_FREQ)/(CONFIG_CLOCK_STM32_HPRE))
#define APB1_FREQ	((AHB_FREQ)/(CONFIG_CLOCK_STM32_D2PPRE1))
#define APB2_FREQ	((AHB_FREQ)/(CONFIG_CLOCK_STM32_D2PPRE2))
#define APB3_FREQ	((AHB_FREQ)/(CONFIG_CLOCK_STM32_D1PPRE))
#define APB4_FREQ	((AHB_FREQ)/(CONFIG_CLOCK_STM32_D3PPRE))

/* Datasheet maximum frequency definitions */
#define SYSCLK_FREQ_MAX	480000000UL
#define AHB_FREQ_MAX	240000000UL
#define APBx_FREQ_MAX	120000000UL

#if SYSCLK_FREQ > SYSCLK_FREQ_MAX
#error "SYSCLK frequency is too high, max is 480MHz"
#endif
#if AHB_FREQ > AHB_FREQ_MAX
#error "AHB frequency is too high, max is 240MHz"
#endif
#if APB1_FREQ > APBx_FREQ_MAX
#error "APB1 frequency is too high, max is 120MHz"
#endif
#if APB2_FREQ > APBx_FREQ_MAX
#error "APB2 frequency is too high, max is 120MHz"
#endif
#if APB3_FREQ > APBx_FREQ_MAX
#error "APB3 frequency is too high, max is 120MHz"
#endif
#if APB4_FREQ > APBx_FREQ_MAX
#error "APB4 frequency is too high, max is 120MHz"
#endif

#if SYSCLK_FREQ != CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#error "Calculated CPU clock frequency (SYS clock) for M7 core doesn't match \
CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC"
#endif

/* end of clock feasability check */
#endif /* CONFIG_CPU_CORTEX_M7 */


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

static inline uint32_t get_pllsrc_frequency(void)
{
	switch (LL_RCC_PLL_GetSource()) {
	case LL_RCC_PLLSOURCE_HSI:
		return HSI_VALUE;
	case LL_RCC_PLLSOURCE_CSI:
		return CSI_VALUE;
	case LL_RCC_PLLSOURCE_HSE:
		return HSE_VALUE;
	case LL_RCC_PLLSOURCE_NONE:
	default:
		return 0;
	}
}

static uint32_t get_hclk_frequency(void)
{
	uint32_t sysclk = 0;
	uint32_t hpre = 0;
	uint32_t hsidiv = 0;

	/* Get the current HSI divider */
	switch (LL_RCC_HSI_GetDivider()) {
	case LL_RCC_HSI_DIV2:
		hsidiv = 2;
		break;
	case LL_RCC_HSI_DIV4:
		hsidiv = 4;
		break;
	case LL_RCC_HSI_DIV8:
		hsidiv = 8;
		break;
	case LL_RCC_HSI_DIV1:
	default:
		hsidiv = 1;
		break;
	}

	/* Get the current system clock source */
	switch (LL_RCC_GetSysClkSource()) {
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSI:
		sysclk = HSI_VALUE/hsidiv;
		break;
	case LL_RCC_SYS_CLKSOURCE_STATUS_CSI:
		sysclk = CSI_VALUE;
		break;
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:
		sysclk = HSE_VALUE;
		break;
	case LL_RCC_SYS_CLKSOURCE_STATUS_PLL1:
		sysclk = PLLP_FREQ(get_pllsrc_frequency(),
				   LL_RCC_PLL1_GetM(),
				   LL_RCC_PLL1_GetN(),
				   LL_RCC_PLL1_GetP());
		break;
	}
	/* AHB/HCLK clock is sysclk/HPRE AHB prescaler*/
	switch (LL_RCC_GetAHBPrescaler()) {
	case LL_RCC_AHB_DIV_1:
		hpre = 1;
		break;
	case LL_RCC_AHB_DIV_2:
		hpre = 2;
		break;
	case LL_RCC_AHB_DIV_4:
		hpre = 4;
		break;
	case LL_RCC_AHB_DIV_8:
		hpre = 8;
		break;
	case LL_RCC_AHB_DIV_16:
		hpre = 16;
		break;
	case LL_RCC_AHB_DIV_64:
		hpre = 64;
		break;
	case LL_RCC_AHB_DIV_128:
		hpre = 128;
		break;
	case LL_RCC_AHB_DIV_256:
		hpre = 256;
		break;
	case LL_RCC_AHB_DIV_512:
		hpre = 512;
		break;
	default:
		hpre = 1;
		break;
	}
	return get_bus_clock(sysclk, hpre);
}

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
#endif /* CONFIG_CLOCK_STM32_PLL_SRC_* */

#endif /* ! CONFIG_CPU_CORTEX_M4 */

static inline int stm32_clock_control_on(const struct device *dev,
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

static inline int stm32_clock_control_off(const struct device *dev,
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

static int stm32_clock_control_init(const struct device *dev)
{

#if !defined(CONFIG_CPU_CORTEX_M4)
	uint32_t old_hclk_freq = 0;
	uint32_t new_hclk_freq = 0;

#if defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)

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

	/* Current hclk value */
	old_hclk_freq = get_hclk_frequency();
	/* AHB is HCLK clock to configure */
	new_hclk_freq = get_bus_clock(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
				      CONFIG_CLOCK_STM32_HPRE);

	/* Set flash latency */
	/* AHB/AXI/HCLK clock is SYSCLK / HPRE */
	/* If freq increases, set flash latency before any clock setting */
	if (new_hclk_freq > old_hclk_freq) {
		LL_SetFlashLatency(new_hclk_freq);
	}

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

#elif defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)
	/* Support for CSI oscillator */

	LL_RCC_CSI_Enable();
	while (LL_RCC_CSI_IsReady() != 1) {
	}

	/* Main PLL configuration and activation */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_CSI);

#elif defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI)
	/* By default choose HSI as PLL clock source */

	/* Enable HSI oscillator */
	LL_RCC_HSI_Enable();
	while (LL_RCC_HSI_IsReady() != 1) {
	}

	/* HSI divider configuration */
	LL_RCC_HSI_SetDivider(hsi_divider(CONFIG_CLOCK_STM32_HSI_DIVISOR));

	/* Main PLL configuration and activation */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSI);

#else

	/* No clock source selected for PLL, by default, disable the PLL */
	LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_NONE);
#endif

	/* Configure the PLL dividers/multipliers only if PLL source is configured */
#if defined(CONFIG_CLOCK_STM32_PLL_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_HSI) || \
	defined(CONFIG_CLOCK_STM32_PLL_SRC_CSI)

	vco_output_range = get_vco_output_range(VCO_INPUT_RANGE);

	/* Configure PLL1 */
	/* According to the RM0433 datasheet */
	/* Select clock source */
	/* Init pre divider DIVM */
	LL_RCC_PLL1_SetM(CONFIG_CLOCK_STM32_PLL_M_DIVISOR);
	/* Config PLL */

	/* VCO sel, VCO range */
	LL_RCC_PLL1_SetVCOInputRange(VCO_INPUT_RANGE);
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


	/* Preset the prescalers prior to chosing SYSCLK */
	/* Prevents APB clock to go over limits */
	/* Set buses (Sys,AHB, APB1, APB2 & APB4) prescalers */
	LL_RCC_SetSysPrescaler(sysclk_prescaler(CONFIG_CLOCK_STM32_D1CPRE));
	LL_RCC_SetAHBPrescaler(ahb_prescaler(CONFIG_CLOCK_STM32_HPRE));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(CONFIG_CLOCK_STM32_D2PPRE1));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(CONFIG_CLOCK_STM32_D2PPRE2));
	LL_RCC_SetAPB3Prescaler(apb3_prescaler(CONFIG_CLOCK_STM32_D1PPRE));
	LL_RCC_SetAPB4Prescaler(apb4_prescaler(CONFIG_CLOCK_STM32_D3PPRE));


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

	/* HSI divider configuration */
	LL_RCC_HSI_SetDivider(hsi_divider(CONFIG_CLOCK_STM32_HSI_DIVISOR));

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

	/* Set FLASH latency */
	/* AHB/AXI/HCLK clock is SYSCLK / HPRE */
	/* If freq not increased, set flash latency after all clock setting */
	if (new_hclk_freq <= old_hclk_freq) {
		LL_SetFlashLatency(new_hclk_freq);
	}

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
