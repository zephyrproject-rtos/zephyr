/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Reset & Clock Control of STM32F3x family processor.
 *
 * Based on reference manual:
 *   STM32F303xB.C.D.E advanced ARM-based 32-bit MCU
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 9: Reset and clock control (RCC)
 *
 *   STM32F334xx advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 8: Reset and clock control (RCC)
 */

#include <soc.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <misc/__assert.h>
#include <clock_control/stm32_clock_control.h>

struct stm32f3x_rcc_data {
	uint8_t *base;
};

static int stm32f3x_clock_control_on(struct device *dev,
				     clock_control_subsys_t sub_system)
{
	struct stm32f3x_rcc_data *data = dev->driver_data;

	volatile struct stm32f3x_rcc *rcc =
		(struct stm32f3x_rcc *)(data->base);
	uint32_t subsys = POINTER_TO_UINT(sub_system);

	if (subsys >  STM32F3X_CLOCK_AHB_BASE) {
		subsys &=  ~(STM32F3X_CLOCK_AHB_BASE);
		rcc->ahbenr |= subsys;
	} else if (subsys > STM32F3X_CLOCK_APB2_BASE) {
		subsys &=  ~(STM32F3X_CLOCK_APB2_BASE);
		rcc->apb2enr |= subsys;
	} else {
		rcc->apb1enr |= subsys;
	}

	return 0;
}

static int stm32f3x_clock_control_off(struct device *dev,
				      clock_control_subsys_t sub_system)
{
	struct stm32f3x_rcc_data *data = dev->driver_data;

	volatile struct stm32f3x_rcc *rcc =
		(struct stm32f3x_rcc *)(data->base);
	uint32_t subsys = POINTER_TO_UINT(sub_system);

	if (subsys >  STM32F3X_CLOCK_AHB_BASE) {
		subsys &=  ~(STM32F3X_CLOCK_AHB_BASE);
		rcc->ahbenr &= ~subsys;
	} else if (subsys > STM32F3X_CLOCK_APB2_BASE) {
		subsys &=  ~(STM32F3X_CLOCK_APB2_BASE);
		rcc->apb2enr &= ~subsys;
	} else {
		rcc->apb1enr &= ~subsys;
	}

	return 0;
}

/**
 * @brief helper for mapping a setting to register value
 */
struct regval_map {
	int val;
	int reg;
};

static int map_reg_val(const struct regval_map *map,
		       size_t cnt, int val, uint8_t normalize)
{
	for (int i = 0; i < cnt; i++) {
		if (map[i].val == val) {
			return (map[i].reg >> normalize);
		}
	}

	return 0;
}

/**
 * @brief map APB prescaler setting to register value
 */
static int apb_prescaler(int prescaler)
{
	if (prescaler == 0) {
		return RCC_HCLK_DIV1;
	}

	const struct regval_map map[] = {
		{0,	RCC_HCLK_DIV1},
		{2,	RCC_HCLK_DIV2},
		{4,	RCC_HCLK_DIV4},
		{8,	RCC_HCLK_DIV8},
		{16,	RCC_HCLK_DIV16},
	};

	return map_reg_val(map, ARRAY_SIZE(map),
			   prescaler, RCC_CFGR_PPRE1_Pos);
}

/**
 * @brief map AHB prescaler setting to register value
 */
static int ahb_prescaler(int prescaler)
{
	if (prescaler == 0) {
		return RCC_SYSCLK_DIV1;
	}

	const struct regval_map map[] = {
		{0,	RCC_SYSCLK_DIV1},
		{2,	RCC_SYSCLK_DIV2},
		{4,	RCC_SYSCLK_DIV4},
		{8,	RCC_SYSCLK_DIV8},
		{16,	RCC_SYSCLK_DIV16},
		{64,	RCC_SYSCLK_DIV64},
		{128,	RCC_SYSCLK_DIV128},
		{256,	RCC_SYSCLK_DIV256},
		{512,	RCC_SYSCLK_DIV512},
	};

	return map_reg_val(map, ARRAY_SIZE(map),
			   prescaler, RCC_CFGR_HPRE_Pos);
}

/**
 * @brief map PLL multiplier setting to register value
 */
static int pllmul(int mul)
{
	/* x2 -> 0x0
	 * x3 -> 0x1
	 * x4 -> 0x2
	 * ...
	 * x15 -> 0xd
	 * x16 -> 0xe
	 * x16 -> 0xf
	 */
	return mul - 2;
}

/**
 * @brief select PREDIV division factor
 */
static int prediv_prescaler(int prescaler)
{
	if (prescaler == 0) {
		return RCC_HSE_PREDIV_DIV1;
	}

	const struct regval_map map[] = {
		{0,	RCC_HSE_PREDIV_DIV1},
		{2,	RCC_HSE_PREDIV_DIV2},
		{3,	RCC_HSE_PREDIV_DIV3},
		{4,	RCC_HSE_PREDIV_DIV4},
		{5,	RCC_HSE_PREDIV_DIV5},
		{6,	RCC_HSE_PREDIV_DIV6},
		{7,	RCC_HSE_PREDIV_DIV7},
		{8,	RCC_HSE_PREDIV_DIV8},
		{9,	RCC_HSE_PREDIV_DIV9},
		{10,	RCC_HSE_PREDIV_DIV10},
		{11,	RCC_HSE_PREDIV_DIV11},
		{12,	RCC_HSE_PREDIV_DIV12},
		{13,	RCC_HSE_PREDIV_DIV13},
		{14,	RCC_HSE_PREDIV_DIV14},
		{15,	RCC_HSE_PREDIV_DIV15},
		{16,	RCC_HSE_PREDIV_DIV16},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler, 0);
}

/**
 * @brief select System Clock Source
 */
static int system_clock(int source)
{
	__ASSERT_NO_MSG(IS_RCC_SYSCLKSOURCE(source));
	return (source >> RCC_CFGR_SW_Pos);
}

/**
 * @brief select PLL Clock Source
 */
static int pll_source(int source)
{
	__ASSERT_NO_MSG(IS_RCC_PLLSOURCE(source));
	return (source >> RCC_CFGR_PLLSRC_Pos);
}

static uint32_t get_ahb_clock(uint32_t sysclk)
{
	/* AHB clock is generated based on SYSCLK  */
	uint32_t sysclk_div = CONFIG_CLOCK_STM32F3X_AHB_PRESCALER;

	if (sysclk_div == 0) {
		sysclk_div = 1;
	}

	return sysclk / sysclk_div;
}

static uint32_t get_apb_clock(uint32_t ahb_clock, uint32_t prescaler)
{
	if (prescaler == 0) {
		prescaler = 1;
	}

	return ahb_clock / prescaler;
}

static
int stm32f3x_clock_control_get_subsys_rate(struct device *clock,
					   clock_control_subsys_t sub_system,
					   uint32_t *rate)
{
	ARG_UNUSED(clock);

	uint32_t subsys = POINTER_TO_UINT(sub_system);
	uint32_t prescaler = CONFIG_CLOCK_STM32F3X_APB1_PRESCALER;
	/* assumes SYSCLK is SYS_CLOCK_HW_CYCLES_PER_SEC */
	uint32_t ahb_clock =
		get_ahb_clock(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	if (subsys > STM32F3X_CLOCK_AHB_BASE) {
		prescaler = CONFIG_CLOCK_STM32F3X_AHB_PRESCALER;
	} else if (subsys > STM32F3X_CLOCK_APB2_BASE) {
		prescaler = CONFIG_CLOCK_STM32F3X_APB2_PRESCALER;
	}

	*rate = get_apb_clock(ahb_clock, prescaler);

	return 0;
}

static const struct clock_control_driver_api stm32f3x_clock_control_api = {
	.on = stm32f3x_clock_control_on,
	.off = stm32f3x_clock_control_off,
	.get_rate = stm32f3x_clock_control_get_subsys_rate,
};

/**
 * @brief setup embedded flash controller
 *
 * Configure flash access time latency depending on SYSCLK.
 */
static void setup_flash(void)
{
	volatile struct stm32_flash *flash =
		(struct stm32_flash *)(FLASH_R_BASE);

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 24000000) {
		flash->acr.bit.latency = STM32_FLASH_LATENCY_0;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 48000000) {
		flash->acr.bit.latency = STM32_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 72000000) {
		flash->acr.bit.latency = STM32_FLASH_LATENCY_2;
	}
}

static int stm32f3x_clock_control_init(struct device *dev)
{
	struct stm32f3x_rcc_data *data = dev->driver_data;
	volatile struct stm32f3x_rcc *rcc =
		(struct stm32f3x_rcc *)(data->base);
	/* SYSCLK source defaults to HSI */
	int sysclk_src = system_clock(RCC_SYSCLKSOURCE_HSI);
	uint32_t hpre = ahb_prescaler(CONFIG_CLOCK_STM32F3X_AHB_PRESCALER);
	uint32_t ppre1 = apb_prescaler(CONFIG_CLOCK_STM32F3X_APB1_PRESCALER);
	uint32_t ppre2 = apb_prescaler(CONFIG_CLOCK_STM32F3X_APB2_PRESCALER);
#ifdef CONFIG_CLOCK_STM32F3X_PLL_MULTIPLIER
	uint32_t pll_mul = pllmul(CONFIG_CLOCK_STM32F3X_PLL_MULTIPLIER);
#endif	/* CONFIG_CLOCK_STM32F3X_PLL_MULTIPLIER */
#ifdef CONFIG_CLOCK_STM32F3X_PLL_PREDIV
	uint32_t prediv =
		prediv_prescaler(CONFIG_CLOCK_STM32F3X_PLL_PREDIV);
#endif /* CONFIG_CLOCK_STM32F3X_PLL_PREDIV */

	/* disable PLL */
	rcc->cr.bit.pllon = 0;
	/* disable HSE */
	rcc->cr.bit.hseon = 0;

#ifdef CONFIG_CLOCK_STM32F3X_HSE_BYPASS
	/* HSE is disabled, HSE bypass can be enabled*/
	rcc->cr.bit.hsebyp = 1;
#endif

#ifdef CONFIG_CLOCK_STM32F3X_PLL_SRC_HSI
	/* enable HSI clock */
	rcc->cr.bit.hsion = 1;
	/* this should end after one test */
	while (rcc->cr.bit.hsirdy != 1) {
	}

	/* HSI clock divided by 2 selected as PLL entry clock source. */
	rcc->cfgr.bit.pllsrc = pll_source(RCC_PLLSOURCE_HSI);
#endif	/* CONFIG_CLOCK_STM32F3X_PLL_SRC_HSI */

#ifdef CONFIG_CLOCK_STM32F3X_PLL_SRC_HSE

	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;
	while (rcc->cr.bit.hserdy != 1) {
	}

#ifdef CONFIG_CLOCK_STM32F3X_PLL_PREDIV
	rcc->cfgr2.bit.prediv = prediv;
#endif	/* CONFIG_CLOCK_STM32F3X_PLL_PREDIV */
	/* HSE clock selected as PLL entry clock source. */
	rcc->cfgr.bit.pllsrc = pll_source(RCC_PLLSOURCE_HSE);
#endif	/* CONFIG_CLOCK_STM32F3X_PLL_SRC_HSE */

	/* setup AHB prescaler */
	rcc->cfgr.bit.hpre = hpre;

	/* setup APB1, must not exceed 36MHz */
	rcc->cfgr.bit.ppre1 = ppre1;

	/* setup APB2  */
	rcc->cfgr.bit.ppre2 = ppre2;

#ifdef CONFIG_CLOCK_STM32F3X_SYSCLK_SRC_HSI
	/* enable HSI clock */
	rcc->cr.bit.hsion = 1;
	/* this should end after one test */
	while (rcc->cr.bit.hsirdy != 1) {
	}
	sysclk_src = system_clock(RCC_SYSCLKSOURCE_HSI);
#elif defined(CONFIG_CLOCK_STM32F3X_SYSCLK_SRC_PLL)
	/* setup PLL multiplication (PLL must be disabled) */
	rcc->cfgr.bit.pllmul = pll_mul;

	/* enable PLL */
	rcc->cr.bit.pllon = 1;

	/* wait for PLL to become ready */
	while (rcc->cr.bit.pllrdy != 1) {
	}

	sysclk_src = system_clock(RCC_SYSCLKSOURCE_PLLCLK);
#elif defined(CONFIG_CLOCK_STM32F3X_SYSCLK_SRC_HSE)
	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;
	while (rcc->cr.bit.hserdy != 1) {
	}

	sysclk_src = system_clock(RCC_SYSCLKSOURCE_HSE);
#endif

	/* configure flash access latency before SYSCLK source
	 * switch
	 */
	setup_flash();

	/* set SYSCLK clock value */
	rcc->cfgr.bit.sw = sysclk_src;

	/* wait for SYSCLK to switch the source */
	while (rcc->cfgr.bit.sws != sysclk_src) {
	}

	dev->driver_api = &stm32f3x_clock_control_api;

	return 0;
}

static struct stm32f3x_rcc_data stm32f3x_rcc_data = {
	.base = (uint8_t *)RCC_BASE,
};

/* FIXME: move prescaler/multiplier defines into device config */

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_INIT(rcc_stm32f3x, STM32_CLOCK_CONTROL_NAME,
	    &stm32f3x_clock_control_init,
	    &stm32f3x_rcc_data, NULL,
	    PRE_KERNEL_1,
	    CONFIG_CLOCK_CONTROL_STM32F3X_DEVICE_INIT_PRIORITY);
