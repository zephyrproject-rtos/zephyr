/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Reset & Clock Control of STM32F10x family processor.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 7: Low-, medium-, high- and XL-density reset and
 *            clock control
 */

#include <soc.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <clock_control/stm32_clock_control.h>

struct stm32f10x_rcc_data {
	uint8_t *base;
};

static inline int stm32f10x_clock_control_on(struct device *dev,
					     clock_control_subsys_t sub_system)
{
	struct stm32f10x_rcc_data *data = dev->driver_data;

	volatile struct stm32f10x_rcc *rcc = (struct stm32f10x_rcc *)(data->base);

	uint32_t subsys = POINTER_TO_UINT(sub_system);

	if (subsys > STM32F10X_CLOCK_APB2_BASE) {
		subsys &=  ~(STM32F10X_CLOCK_APB2_BASE);
		rcc->apb2enr |= subsys;
	} else {
		rcc->apb1enr |= subsys;
	}
	return 0;
}

static inline int stm32f10x_clock_control_off(struct device *dev,
					      clock_control_subsys_t sub_system)
{
	struct stm32f10x_rcc_data *data = dev->driver_data;
	volatile struct stm32f10x_rcc *rcc =
		(struct stm32f10x_rcc *)(data->base);
	uint32_t subsys = POINTER_TO_UINT(sub_system);

	if (subsys > STM32F10X_CLOCK_APB2_BASE) {
		subsys &= ~(STM32F10X_CLOCK_APB2_BASE);
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

static int map_reg_val(const struct regval_map *map, size_t cnt, int val)
{
	for (int i = 0; i < cnt; i++) {
		if (map[i].val == val) {
			return map[i].reg;
		}
	}
	return 0;
}

/**
 * @brief map APB prescaler setting to register value
 */
static int __apb_prescaler(int prescaler)
{
	if (prescaler == 0) {
		return STM32F10X_RCC_CFG_HCLK_DIV_0;
	}

	const struct regval_map map[] = {
		{0,	STM32F10X_RCC_CFG_HCLK_DIV_0},
		{2,	STM32F10X_RCC_CFG_HCLK_DIV_2},
		{4,	STM32F10X_RCC_CFG_HCLK_DIV_4},
		{8,	STM32F10X_RCC_CFG_HCLK_DIV_8},
		{16,	STM32F10X_RCC_CFG_HCLK_DIV_16},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

/**
 * @brief map AHB prescaler setting to register value
 */
static int __ahb_prescaler(int prescaler)
{
	if (prescaler == 0)
		return STM32F10X_RCC_CFG_SYSCLK_DIV_0;

	const struct regval_map map[] = {
		{0,	STM32F10X_RCC_CFG_SYSCLK_DIV_0},
		{2,	STM32F10X_RCC_CFG_SYSCLK_DIV_2},
		{4,	STM32F10X_RCC_CFG_SYSCLK_DIV_4},
		{8,	STM32F10X_RCC_CFG_SYSCLK_DIV_8},
		{16,	STM32F10X_RCC_CFG_SYSCLK_DIV_16},
		{64,	STM32F10X_RCC_CFG_SYSCLK_DIV_64},
		{128,	STM32F10X_RCC_CFG_SYSCLK_DIV_128},
		{256,	STM32F10X_RCC_CFG_SYSCLK_DIV_256},
		{512,	STM32F10X_RCC_CFG_SYSCLK_DIV_512},
	};
	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

#ifdef CONFIG_CLOCK_STM32F10X_PLL_MULTIPLIER
/**
 * @brief map PLL multiplier setting to register value
 */
static int __pllmul(int mul)
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
#endif	/* CONFIG_CLOCK_STM32F10X_PLL_MULTIPLIER */


static uint32_t __get_ahb_clock(uint32_t sysclk)
{
	/* AHB clock is generated based on SYSCLK  */
	uint32_t sysclk_div = CONFIG_CLOCK_STM32F10X_AHB_PRESCALER;

	if (sysclk_div == 0) {
		sysclk_div = 1;
	}
	return sysclk / sysclk_div;
}

static uint32_t __get_apb_clock(uint32_t ahb_clock, uint32_t prescaler)
{
	if (prescaler == 0) {
		prescaler = 1;
	}
	return ahb_clock / prescaler;
}

static int stm32f10x_clock_control_get_subsys_rate(struct device *clock,
						   clock_control_subsys_t sub_system,
						   uint32_t *rate)
{
	ARG_UNUSED(clock);

	uint32_t subsys = POINTER_TO_UINT(sub_system);

	uint32_t prescaler = CONFIG_CLOCK_STM32F10X_APB1_PRESCALER;

	if (subsys > STM32F10X_CLOCK_APB2_BASE) {
		prescaler = CONFIG_CLOCK_STM32F10X_APB2_PRESCALER;
	}

	/* assumes SYSCLK is SYS_CLOCK_HW_CYCLES_PER_SEC */
	uint32_t ahb_clock =
		__get_ahb_clock(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	*rate = __get_apb_clock(ahb_clock, prescaler);

	return 0;
}

static const struct clock_control_driver_api stm32f10x_clock_control_api = {
	.on = stm32f10x_clock_control_on,
	.off = stm32f10x_clock_control_off,
	.get_rate = stm32f10x_clock_control_get_subsys_rate,
};

/**
 * @brief setup embedded flash controller
 *
 * Configure flash access time latency depending on SYSCLK.
 */
static inline void __setup_flash(void)
{
	volatile struct stm32f10x_flash *flash =
		(struct stm32f10x_flash *)(FLASH_R_BASE);

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 24000000) {
		flash->acr.bit.latency = STM32F10X_FLASH_LATENCY_0;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 48000000) {
		flash->acr.bit.latency = STM32F10X_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 72000000) {
		flash->acr.bit.latency = STM32F10X_FLASH_LATENCY_2;
	}
}

static int stm32f10x_clock_control_init(struct device *dev)
{
	struct stm32f10x_rcc_data *data = dev->driver_data;
	volatile struct stm32f10x_rcc *rcc =
		(struct stm32f10x_rcc *)(data->base);
	/* SYSCLK source defaults to HSI */
	int sysclk_src = STM32F10X_RCC_CFG_SYSCLK_SRC_HSI;
	uint32_t hpre = __ahb_prescaler(CONFIG_CLOCK_STM32F10X_AHB_PRESCALER);
	uint32_t ppre1 = __apb_prescaler(CONFIG_CLOCK_STM32F10X_APB1_PRESCALER);
	uint32_t ppre2 = __apb_prescaler(CONFIG_CLOCK_STM32F10X_APB2_PRESCALER);
#ifdef CONFIG_CLOCK_STM32F10X_PLL_MULTIPLIER
	uint32_t pllmul = __pllmul(CONFIG_CLOCK_STM32F10X_PLL_MULTIPLIER);
#endif	/* CONFIG_CLOCK_STM32F10X_PLL_MULTIPLIER */

	/* disable PLL */
	rcc->cr.bit.pllon = 0;
	/* disable HSE */
	rcc->cr.bit.hseon = 0;

#ifdef CONFIG_CLOCK_STM32F10X_HSE_BYPASS
	/* HSE is disabled, HSE bypass can be enabled*/
	rcc->cr.bit.hsebyp = 1;
#endif

#ifdef CONFIG_CLOCK_STM32F10X_PLL_SRC_HSI
	/* enable HSI clock */
	rcc->cr.bit.hsion = 1;
	/* this should end after one test */
	while (rcc->cr.bit.hsirdy != 1) {
	}

	/* PLL input from HSI/2 = 4MHz */
	rcc->cfgr.bit.pllsrc = STM32F10X_RCC_CFG_PLL_SRC_HSI;
#endif	/* CONFIG_CLOCK_STM32F10X_PLL_SRC_HSI */

#ifdef CONFIG_CLOCK_STM32F10X_PLL_SRC_HSE

	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;
	while (rcc->cr.bit.hserdy != 1) {
	}

#ifdef CONFIG_CLOCK_STM32F10X_PLL_XTPRE
	rcc->cfgr.bit.pllxtpre = STM32F10X_RCC_CFG_PLL_XTPRE_DIV_2;
#else
	rcc->cfgr.bit.pllxtpre = STM32F10X_RCC_CFG_PLL_XTPRE_DIV_0;
#endif	/* CONFIG_CLOCK_STM32F10X_PLL_XTPRE */

	rcc->cfgr.bit.pllsrc = STM32F10X_RCC_CFG_PLL_SRC_HSE;
#endif	/* CONFIG_CLOCK_STM32F10X_PLL_SRC_HSE */

	/* setup AHB prescaler */
	rcc->cfgr.bit.hpre = hpre;

	/* setup APB1, must not exceed 36MHz */
	rcc->cfgr.bit.ppre1 = ppre1;

	/* setup APB2  */
	rcc->cfgr.bit.ppre2 = ppre2;

#ifdef CONFIG_CLOCK_STM32F10X_SYSCLK_SRC_PLL
	/* setup PLL multiplication (PLL must be disabled) */
	rcc->cfgr.bit.pllmul = pllmul;

	/* enable PLL */
	rcc->cr.bit.pllon = 1;

	/* wait for PLL to become ready */
	while (rcc->cr.bit.pllrdy != 1) {
	}

	sysclk_src = STM32F10X_RCC_CFG_SYSCLK_SRC_PLL;
#elif defined(CONFIG_CLOCK_STM32F10X_SYSCLK_SRC_HSE)
	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;
	while (rcc->cr.bit.hserdy != 1) {
	}

	sysclk_src = STM32F10X_RCC_CFG_SYSCLK_SRC_HSE;
#endif

	/* configure flash access latency before SYSCLK source
	 * switch
	 */
	__setup_flash();

	/* set SYSCLK clock value */
	rcc->cfgr.bit.sw = sysclk_src;

	/* wait for SYSCLK to switch the source */
	while (rcc->cfgr.bit.sws != sysclk_src) {
	}

	return 0;
}

static struct stm32f10x_rcc_data stm32f10x_rcc_data = {
	.base = (uint8_t *)RCC_BASE,
};

/* FIXME: move prescaler/multiplier defines into device config */

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_AND_API_INIT(rcc_stm32f10x, STM32_CLOCK_CONTROL_NAME,
		    &stm32f10x_clock_control_init,
		    &stm32f10x_rcc_data, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_STM32F10X_DEVICE_INIT_PRIORITY,
		    &stm32f10x_clock_control_api);
