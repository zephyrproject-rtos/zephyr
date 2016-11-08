/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * @brief Driver for Reset & Clock Control of STM32F4X family processor.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 6. Reset and Clock control (RCC) for STM43F401xB/C and STM32F401xD/E
 */

#include <soc.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <clock_control/stm32_clock_control.h>

struct stm32f4x_rcc_data {
	uint8_t *base;
};

static inline int stm32f4x_clock_control_on(struct device *dev,
					    clock_control_subsys_t sub_system)
{
	struct stm32f4x_rcc_data *data = dev->driver_data;
	volatile struct stm32f4x_rcc *rcc = (struct stm32f4x_rcc *)(data->base);
	struct stm32f4x_pclken *pclken = (struct stm32f4x_pclken *)(sub_system);
	uint32_t tmpreg = 0;	/* Register delay helper */

	switch (pclken->bus) {
	case STM32F4X_CLOCK_BUS_AHB1:
		rcc->ahb1enr |= pclken->enr;
		tmpreg = rcc->ahb1enr;
		break;
	case STM32F4X_CLOCK_BUS_AHB2:
		rcc->ahb2enr |= pclken->enr;
		tmpreg = rcc->ahb2enr;
		break;
	case STM32F4X_CLOCK_BUS_APB1:
		rcc->apb1enr |= pclken->enr;
		tmpreg = rcc->apb1enr;
		break;
	case STM32F4X_CLOCK_BUS_APB2:
		rcc->apb2enr |= pclken->enr;
		tmpreg = rcc->apb2enr;
		break;
	}

	return 0;
}

static inline int stm32f4x_clock_control_off(struct device *dev,
					     clock_control_subsys_t sub_system)
{
	struct stm32f4x_rcc_data *data = dev->driver_data;
	volatile struct stm32f4x_rcc *rcc = (struct stm32f4x_rcc *)(data->base);
	struct stm32f4x_pclken *pclken = (struct stm32f4x_pclken *)(sub_system);
	uint32_t tmpreg = 0;	/* Register delay helper */

	switch (pclken->bus) {
	case STM32F4X_CLOCK_BUS_AHB1:
		rcc->ahb1enr &= ~pclken->enr;
		tmpreg = rcc->ahb1enr;
		break;
	case STM32F4X_CLOCK_BUS_AHB2:
		rcc->ahb2enr &= ~pclken->enr;
		tmpreg = rcc->ahb2enr;
		break;
	case STM32F4X_CLOCK_BUS_APB1:
		rcc->apb1enr &= ~pclken->enr;
		tmpreg = rcc->apb1enr;
		break;
	case STM32F4X_CLOCK_BUS_APB2:
		rcc->apb2enr &= ~pclken->enr;
		tmpreg = rcc->apb2enr;
		break;
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

int map_reg_val(const struct regval_map *map, size_t cnt, int val)
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
		return STM32F4X_RCC_CFG_HCLK_DIV_0;
	}

	const struct regval_map map[] = {
		{0,	STM32F4X_RCC_CFG_HCLK_DIV_0},
		{2,	STM32F4X_RCC_CFG_HCLK_DIV_2},
		{4,	STM32F4X_RCC_CFG_HCLK_DIV_4},
		{8,	STM32F4X_RCC_CFG_HCLK_DIV_8},
		{16,	STM32F4X_RCC_CFG_HCLK_DIV_16},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

/**
 * @brief map AHB prescaler setting to register value
 */
static int __ahb_prescaler(int prescaler)
{
	if (prescaler == 0) {
		return STM32F4X_RCC_CFG_SYSCLK_DIV_0;
	}

	const struct regval_map map[] = {
		{0,	STM32F4X_RCC_CFG_SYSCLK_DIV_0},
		{2,	STM32F4X_RCC_CFG_SYSCLK_DIV_2},
		{4,	STM32F4X_RCC_CFG_SYSCLK_DIV_4},
		{8,	STM32F4X_RCC_CFG_SYSCLK_DIV_8},
		{16,	STM32F4X_RCC_CFG_SYSCLK_DIV_16},
		{64,	STM32F4X_RCC_CFG_SYSCLK_DIV_64},
		{128,	STM32F4X_RCC_CFG_SYSCLK_DIV_128},
		{256,	STM32F4X_RCC_CFG_SYSCLK_DIV_256},
		{512,	STM32F4X_RCC_CFG_SYSCLK_DIV_512},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

#ifdef CONFIG_CLOCK_STM32F4X_SYSCLK_SRC_PLL
/**
 * @brief map PPLP division factor to register value
 */
static int __pllp_div(int div)
{
	if (div == 0) {
		return STM32F4X_RCC_CFG_PLLP_DIV_2;
	}

	const struct regval_map map[] = {
		{2,	STM32F4X_RCC_CFG_PLLP_DIV_2},
		{4,	STM32F4X_RCC_CFG_PLLP_DIV_4},
		{6,	STM32F4X_RCC_CFG_PLLP_DIV_6},
		{8,	STM32F4X_RCC_CFG_PLLP_DIV_8},
	};

	return map_reg_val(map, ARRAY_SIZE(map), div);
}
#endif	/* CONFIG_CLOCK_STM32F4X_SYSCLK_SRC_PLL */

uint32_t __get_ahb_clock(uint32_t sysclk)
{
	/* AHB clock is generated based on SYSCLK */
	uint32_t sysclk_div = CONFIG_CLOCK_STM32F4X_AHB_PRESCALER;

	if (sysclk_div == 0) {
		sysclk_div = 1;
	}

	return sysclk / sysclk_div;
}

uint32_t __get_apb_clock(uint32_t ahb_clock, uint32_t prescaler)
{
	if (prescaler == 0) {
		prescaler = 1;
	}

	return ahb_clock / prescaler;
}

static int stm32f4x_clock_control_get_subsys_rate(struct device *clock,
						  clock_control_subsys_t sub_system,
						  uint32_t *rate)
{
	struct stm32f4x_pclken *pclken = (struct stm32f4x_pclken *)(sub_system);
	/* assumes SYSCLK is SYS_CLOCK_HW_CYCLES_PER_SEC */
	uint32_t ahb_clock =
		__get_ahb_clock(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	uint32_t apb1_clock = __get_apb_clock(ahb_clock,
				CONFIG_CLOCK_STM32F4X_APB1_PRESCALER);
	uint32_t apb2_clock = __get_apb_clock(ahb_clock,
				CONFIG_CLOCK_STM32F4X_APB2_PRESCALER);
	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32F4X_CLOCK_BUS_AHB1:
	case STM32F4X_CLOCK_BUS_AHB2:
		*rate = ahb_clock;
		break;
	case STM32F4X_CLOCK_BUS_APB1:
		*rate = apb1_clock;
		break;
	case STM32F4X_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
	}

	return 0;
}

static struct clock_control_driver_api stm32f4x_clock_control_api = {
	.on = stm32f4x_clock_control_on,
	.off = stm32f4x_clock_control_off,
	.get_rate = stm32f4x_clock_control_get_subsys_rate,
};

int stm32f4x_clock_control_init(struct device *dev)
{
	struct stm32f4x_rcc_data *data = dev->driver_data;
	volatile struct stm32f4x_rcc *rcc =
		(struct stm32f4x_rcc *)(data->base);
	/* SYSCLK source defaults to HSI */
	int sysclk_src = STM32F4X_RCC_CFG_SYSCLK_SRC_HSI;
	uint32_t hpre = __ahb_prescaler(CONFIG_CLOCK_STM32F4X_AHB_PRESCALER);
	uint32_t ppre1 = __apb_prescaler(CONFIG_CLOCK_STM32F4X_APB1_PRESCALER);
	uint32_t ppre2 = __apb_prescaler(CONFIG_CLOCK_STM32F4X_APB2_PRESCALER);
#ifdef CONFIG_CLOCK_STM32F4X_SYSCLK_SRC_PLL
	uint32_t pllmdiv = CONFIG_CLOCK_STM32F4X_PLLM_DIV_FACTOR;
	uint32_t pllnmul = CONFIG_CLOCK_STM32F4X_PLLN_MULTIPLIER;
	uint32_t pllpdiv = __pllp_div(CONFIG_CLOCK_STM32F4X_PLLP_DIV_FACTOR);
	uint32_t pllqdiv = CONFIG_CLOCK_STM32F4X_PLLQ_DIV_FACTOR;
#endif	/* CONFIG_CLOCK_STM32F4X_SYSCLK_SRC_PLL */
	/* Register delay helper */
	uint32_t tmpreg = 0;

	/* Enable power control clock */
	rcc->apb1enr |= STM32F4X_RCC_APB1ENR_PWREN;
	tmpreg = rcc->apb1enr;

	/* disable PLL */
	rcc->cr.bit.pllon = 0;
	/* disable HSE */
	rcc->cr.bit.hseon = 0;

#ifdef CONFIG_CLOCK_STM32F4X_HSE_BYPASS
	/* HSE is disabled, HSE bypass can be enabled */
	rcc->cr.bit.hsebyp = 1;
#endif

#ifdef CONFIG_CLOCK_STM32F4X_PLL_SRC_HSI
	/* enable HSI clock */
	rcc->cr.bit.hsion = 1;

	/* this should end after one test */
	while (rcc->cr.bit.hsirdy != 1) {
	}

	/* TODO: should we care about HSI calibration adjustment? */

	/* PLL input from HSI */
	rcc->pllcfgr.bit.pllsrc = STM32F4X_RCC_CFG_PLL_SRC_HSI;
#endif	/* CONFIG_CLOCK_STM32F4X_PLL_SRC_HSI */

#ifdef CONFIG_CLOCK_STM32F4X_PLL_SRC_HSE
	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;

	while (rcc->cr.bit.hserdy != 1) {
	}

	/* TODO: should we disable HSI if HSE gets used? */

	rcc->pllcfgr.bit.pllsrc = STM32F4X_RCC_CFG_PLL_SRC_HSE;
#endif	/* CONFIG_CLOCK_STM32F4X_PLL_SRC_HSE */

	/* setup AHB prescaler */
	rcc->cfgr.bit.hpre = hpre;

	/* setup APB1, must not exceed 42MHz */
	rcc->cfgr.bit.ppre1 = ppre1;

	/* setup APB2  */
	rcc->cfgr.bit.ppre2 = ppre2;

#ifdef CONFIG_CLOCK_STM32F4X_SYSCLK_SRC_PLL
	/* default set of dividers and multipliers (PLL must be disabled) */
	rcc->pllcfgr.bit.pllm = pllmdiv;
	rcc->pllcfgr.bit.plln = pllnmul;
	rcc->pllcfgr.bit.pllp = pllpdiv;
	rcc->pllcfgr.bit.pllq = pllqdiv;

	/* enable PLL */
	rcc->cr.bit.pllon = 1;

	/* wait for PLL to become ready */
	while (rcc->cr.bit.pllrdy != 1) {
	}

	sysclk_src = STM32F4X_RCC_CFG_SYSCLK_SRC_PLL;
#elif defined(CONFIG_CLOCK_STM32F4X_SYSCLK_SRC_HSE)
	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;
	while (rcc->cr.bit.hserdy != 1) {
	}

	sysclk_src = STM32F4X_RCC_CFG_SYSCLK_SRC_HSE;
#endif

	/* configure flash access latency before SYSCLK source switch */
	__setup_flash();

	/* set SYSCLK clock value */
	rcc->cfgr.bit.sw = sysclk_src;

	/* wait for SYSCLK to switch the source */
	while (rcc->cfgr.bit.sws != sysclk_src) {
	}

	return 0;
}

static struct stm32f4x_rcc_data stm32f4x_rcc_data = {
	.base = (uint8_t *)RCC_BASE,
};

/* FIXME: move prescaler/multiplier defines into device config */

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_AND_API_INIT(rcc_stm32f4x, STM32_CLOCK_CONTROL_NAME,
		    &stm32f4x_clock_control_init,
		    &stm32f4x_rcc_data, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_STM32F4X_DEVICE_INIT_PRIORITY,
		    &stm32f4x_clock_control_api);
