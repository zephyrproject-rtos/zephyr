/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
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
 * @brief Driver for Reset & Clock Control of STM32F10x family processor.
 *
 * Based on reference manual:
 *   STM32L4x1, STM32L4x2, STM32L431xx STM32L443xx STM32L433xx, STM32L4x5,
 *   STM32l4x6
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 7: Low-, medium-, high- and XL-density reset and
 *            clock control
 */

#include <soc.h>
#include <errno.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <clock_control/stm32_clock_control.h>

struct stm32l4x_rcc_data {
	uint8_t *base;
};

static inline int stm32l4x_clock_control_on(struct device *dev,
					    clock_control_subsys_t sub_system)
{
	struct stm32l4x_rcc_data *data = dev->driver_data;
	volatile struct stm32l4x_rcc *rcc = (struct stm32l4x_rcc *)(data->base);
	uint32_t subsys = POINTER_TO_UINT(sub_system);
	uint32_t base = STM32L4X_CLOCK_BASE(subsys);
	uint32_t bit = 1 << STM32L4X_CLOCK_BIT(subsys);

	switch (base) {
	case STM32L4X_CLOCK_AHB1_BASE:
		rcc->ahb1enr |= bit;
		break;
	case STM32L4X_CLOCK_AHB2_BASE:
		rcc->ahb2enr |= bit;
		break;
	case STM32L4X_CLOCK_AHB3_BASE:
		rcc->ahb3enr |= bit;
		break;
	case STM32L4X_CLOCK_APB1_1_BASE:
		rcc->apb1enr1 |= bit;
		break;
	case STM32L4X_CLOCK_APB1_2_BASE:
		rcc->apb1enr2 |= bit;
		break;
	case STM32L4X_CLOCK_APB2_BASE:
		rcc->apb2enr |= bit;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int stm32l4x_clock_control_off(struct device *dev,
					     clock_control_subsys_t sub_system)
{
	struct stm32l4x_rcc_data *data = dev->driver_data;
	volatile struct stm32l4x_rcc *rcc =
		(struct stm32l4x_rcc *)(data->base);
	uint32_t subsys = POINTER_TO_UINT(sub_system);
	uint32_t base = STM32L4X_CLOCK_BASE(subsys);
	uint32_t bit = 1 << STM32L4X_CLOCK_BIT(subsys);

	switch (base) {
	case STM32L4X_CLOCK_AHB1_BASE:
		rcc->ahb1enr &= bit;
		break;
	case STM32L4X_CLOCK_AHB2_BASE:
		rcc->ahb2enr &= bit;
		break;
	case STM32L4X_CLOCK_AHB3_BASE:
		rcc->ahb3enr &= bit;
		break;
	case STM32L4X_CLOCK_APB1_1_BASE:
		rcc->apb1enr1 &= bit;
		break;
	case STM32L4X_CLOCK_APB1_2_BASE:
		rcc->apb1enr2 &= bit;
		break;
	case STM32L4X_CLOCK_APB2_BASE:
		rcc->apb2enr &= bit;
		break;
	default:
		return -EINVAL;
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
	size_t i;

	for (i = 0; i < cnt; i++) {
		if (map[i].val == val) {
			return map[i].reg;
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
		return STM32L4X_RCC_CFG_HCLK_DIV_0;
	}

	const struct regval_map map[] = {
		{0,	STM32L4X_RCC_CFG_HCLK_DIV_0},
		{2,	STM32L4X_RCC_CFG_HCLK_DIV_2},
		{4,	STM32L4X_RCC_CFG_HCLK_DIV_4},
		{8,	STM32L4X_RCC_CFG_HCLK_DIV_8},
		{16,	STM32L4X_RCC_CFG_HCLK_DIV_16},
	};

	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

/**
 * @brief map AHB prescaler setting to register value
 */
static int ahb_prescaler(int prescaler)
{
	if (prescaler == 0)
		return STM32L4X_RCC_CFG_SYSCLK_DIV_0;

	const struct regval_map map[] = {
		{0,	STM32L4X_RCC_CFG_SYSCLK_DIV_0},
		{2,	STM32L4X_RCC_CFG_SYSCLK_DIV_2},
		{4,	STM32L4X_RCC_CFG_SYSCLK_DIV_4},
		{8,	STM32L4X_RCC_CFG_SYSCLK_DIV_8},
		{16,	STM32L4X_RCC_CFG_SYSCLK_DIV_16},
		{64,	STM32L4X_RCC_CFG_SYSCLK_DIV_64},
		{128,	STM32L4X_RCC_CFG_SYSCLK_DIV_128},
		{256,	STM32L4X_RCC_CFG_SYSCLK_DIV_256},
		{512,	STM32L4X_RCC_CFG_SYSCLK_DIV_512},
	};
	return map_reg_val(map, ARRAY_SIZE(map), prescaler);
}

static uint32_t get_ahb_clock(uint32_t sysclk)
{
	/* AHB clock is generated based on SYSCLK  */
	uint32_t sysclk_div = CONFIG_CLOCK_STM32L4X_AHB_PRESCALER;

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
int stm32l4x_clock_control_get_subsys_rate(struct device *clock,
					   clock_control_subsys_t sub_system,
					   uint32_t *rate)
{
	ARG_UNUSED(clock);

	uint32_t subsys = POINTER_TO_UINT(sub_system);
	uint32_t base = STM32L4X_CLOCK_BASE(subsys);

	/* assumes SYSCLK is SYS_CLOCK_HW_CYCLES_PER_SEC */
	uint32_t ahb_clock =
		get_ahb_clock(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	switch (base) {
	case STM32L4X_CLOCK_AHB1_BASE:
	case STM32L4X_CLOCK_AHB2_BASE:
	case STM32L4X_CLOCK_AHB3_BASE:
		*rate = ahb_clock;
		break;
	case STM32L4X_CLOCK_APB1_1_BASE:
	case STM32L4X_CLOCK_APB1_2_BASE:
		*rate = get_apb_clock(ahb_clock,
				CONFIG_CLOCK_STM32L4X_APB1_PRESCALER);
		break;
	case STM32L4X_CLOCK_APB2_BASE:
		*rate = get_apb_clock(ahb_clock,
				CONFIG_CLOCK_STM32L4X_APB2_PRESCALER);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct clock_control_driver_api stm32l4x_clock_control_api = {
	.on = stm32l4x_clock_control_on,
	.off = stm32l4x_clock_control_off,
	.get_rate = stm32l4x_clock_control_get_subsys_rate,
};

/**
 * @brief setup embedded flash controller
 *
 * Configure flash access time latency depending on SYSCLK.
 */
static inline void setup_flash(void)
{
	volatile struct stm32l4x_flash *flash =
		(struct stm32l4x_flash *)(FLASH_R_BASE);

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 16000000) {
		flash->acr.bit.latency = STM32L4X_FLASH_LATENCY_0;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 32000000) {
		flash->acr.bit.latency = STM32L4X_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 48000000) {
		flash->acr.bit.latency = STM32L4X_FLASH_LATENCY_2;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 64000000) {
		flash->acr.bit.latency = STM32L4X_FLASH_LATENCY_3;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 80000000) {
		flash->acr.bit.latency = STM32L4X_FLASH_LATENCY_4;
	}
}

static int pllqrdiv(int val)
{
	switch (val) {
	case 2:
		return 0;
	case 4:
		return 1;
	case 6:
		return 2;
	case 8:
		return 3;
	}

	return 0;
}

static int stm32l4x_clock_control_init(struct device *dev)
{
	struct stm32l4x_rcc_data *data = dev->driver_data;
	volatile struct stm32l4x_rcc *rcc;
	/* SYSCLK source defaults to MSI */
	int sysclk_src = STM32L4X_RCC_CFG_SYSCLK_SRC_MSI;
	uint32_t hpre = ahb_prescaler(CONFIG_CLOCK_STM32L4X_AHB_PRESCALER);
	uint32_t ppre1 = apb_prescaler(CONFIG_CLOCK_STM32L4X_APB1_PRESCALER);
	uint32_t ppre2 = apb_prescaler(CONFIG_CLOCK_STM32L4X_APB2_PRESCALER);
#ifdef CONFIG_CLOCK_STM32L4X_SYSCLK_SRC_PLL
	uint32_t pllm = CONFIG_CLOCK_STM32L4X_PLL_DIVISOR-1;
	uint32_t plln = CONFIG_CLOCK_STM32L4X_PLL_MULTIPLIER;
	uint32_t pllpdiv = CONFIG_CLOCK_STM32L4X_PLL_P_DIVISOR;
	uint32_t pllqdiv = pllqrdiv(CONFIG_CLOCK_STM32L4X_PLL_Q_DIVISOR);
	uint32_t pllrdiv = pllqrdiv(CONFIG_CLOCK_STM32L4X_PLL_R_DIVISOR);
#endif	/* CONFIG_CLOCK_STM32L4X_PLL_MULTIPLIER */


	rcc = (struct stm32l4x_rcc *)(data->base);
	/* disable PLL */
	rcc->cr.bit.pllon = 0;
	/* disable HSE */
	rcc->cr.bit.hseon = 0;

#ifdef CONFIG_CLOCK_STM32L4X_HSE_BYPASS
	/* HSE is disabled, HSE bypass can be enabled*/
	rcc->cr.bit.hsebyp = 1;
#endif

#ifdef CONFIG_CLOCK_STM32L4X_PLL_SRC_MSI
	/* enable MSI clock */
	rcc->cr.bit.msion = 1;
	/* this should end after one test */
	while (rcc->cr.bit.msirdy != 1) {
	}

	/* PLL input from HSI/2 = 4MHz */
	rcc->pllcfgr.bit.pllsrc = STM32L4X_RCC_CFG_PLL_SRC_MSI;
#endif	/* CONFIG_CLOCK_STM32L4X_PLL_SRC_MSI */

#ifdef CONFIG_CLOCK_STM32L4X_PLL_SRC_HSI

	/* wait for to become ready */
	rcc->cr.bit.hsion = 1;
	while (rcc->cr.bit.hsirdy != 1) {
	}

	rcc->pllcfgr.bit.pllsrc = STM32L4X_RCC_CFG_PLL_SRC_HSI;
#endif	/* CONFIG_CLOCK_STM32L4X_PLL_SRC_HSI */

	/* setup AHB prescaler */
	rcc->cfgr.bit.hpre = hpre;

	/* setup APB1, must not exceed 36MHz */
	rcc->cfgr.bit.ppre1 = ppre1;

	/* setup APB2  */
	rcc->cfgr.bit.ppre2 = ppre2;

#ifdef CONFIG_CLOCK_STM32L4X_SYSCLK_SRC_PLL
	/* setup PLL multiplication and divisor (PLL must be disabled) */
	rcc->pllcfgr.bit.pllm = pllm;
	rcc->pllcfgr.bit.plln = plln;

	/* Setup PLL output divisors */
	rcc->pllcfgr.bit.pllp = pllpdiv == 17 ? 1 : 0;
	rcc->pllcfgr.bit.pllpen = !!pllpdiv;
	rcc->pllcfgr.bit.pllq = pllqdiv;
	rcc->pllcfgr.bit.pllqen = !!CONFIG_CLOCK_STM32L4X_PLL_Q_DIVISOR;
	rcc->pllcfgr.bit.pllr = pllrdiv;
	rcc->pllcfgr.bit.pllren = !!CONFIG_CLOCK_STM32L4X_PLL_R_DIVISOR;

	/* enable PLL */
	rcc->cr.bit.pllon = 1;

	/* wait for PLL to become ready */
	while (rcc->cr.bit.pllrdy != 1) {
	}

	sysclk_src = STM32L4X_RCC_CFG_SYSCLK_SRC_PLL;
#elif defined(CONFIG_CLOCK_STM32L4X_SYSCLK_SRC_HSE)
	/* wait for to become ready */
	rcc->cr.bit.hseon = 1;
	while (rcc->cr.bit.hserdy != 1) {
	}

	sysclk_src = STM32L4X_RCC_CFG_SYSCLK_SRC_HSE;
#else
#error "Need to select or implement support for this STM32L4X SYSCLK source"
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

	return 0;
}

static struct stm32l4x_rcc_data stm32l4x_rcc_data = {
	.base = (uint8_t *)RCC_BASE,
};

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_AND_API_INIT(rcc_stm32l4x, STM32_CLOCK_CONTROL_NAME,
		    &stm32l4x_clock_control_init,
		    &stm32l4x_rcc_data, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_STM32L4X_DEVICE_INIT_PRIORITY,
		    &stm32l4x_clock_control_api);
