/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <stm32_ll_utils.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	volatile int temp;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);
	/* Ensure that the write operation is completed */
	temp = sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
	UNUSED(temp);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	volatile int temp;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);
	/* Ensure that the write operation is completed */
	temp = sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
	UNUSED(temp);

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system, uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_APB1:
		switch (pclken->enr) {
		case LL_APB1_GRP1_PERIPH_UART4:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART4_CLKSOURCE);
			break;
		default:
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct clock_control_driver_api stm32_clock_control_api = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
};

static void set_up_fixed_clock_sources(void)
{
	if (IS_ENABLED(STM32_HSE_ENABLED)) {
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
}

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	set_up_fixed_clock_sources();

#if STM32_SYSCLK_SRC_HSE

	LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_HSE);
	while (LL_RCC_GetMPUClkSource() != LL_RCC_MPU_CLKSOURCE_HSE) {
	}

#elif STM32_SYSCLK_SRC_HSI

	LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_HSI);
	while (LL_RCC_GetMPUClkSource() != LL_RCC_MPU_CLKSOURCE_HSI) {
	}

#elif STM32_SYSCLK_SRC_PLL

	BUILD_ASSERT(IS_ENABLED(STM32_HSE_ENABLED),
		     "STM32MP13 PLL requires HSE to be enabled!");

	/* The default system clock source is HSI, but the bootloader may have switched it. */
	/* Switch back to HSE for clock setup as PLL1 configuration must not be modified */
	/* while active.*/

	LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_HSE);
	while ((READ_BIT(RCC->MPCKSELR, RCC_MPCKSELR_MPUSRCRDY) != RCC_MPCKSELR_MPUSRCRDY)) {
	}

	CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN);
	while (READ_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN) == RCC_PLL1CR_DIVPEN) {
	};

	CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVQEN);
	while (READ_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVQEN) == RCC_PLL1CR_DIVQEN) {
	};

	CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVREN);
	while (READ_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVREN) == RCC_PLL1CR_DIVREN) {
	};

	uint32_t pll1_n = DT_PROP(DT_NODELABEL(pll1), mul_n);
	uint32_t pll1_m = DT_PROP(DT_NODELABEL(pll1), div_m);
	uint32_t pll1_p = DT_PROP(DT_NODELABEL(pll1), div_p);
	uint32_t pll1_v = DT_PROP(DT_NODELABEL(pll1), frac_v);

	LL_RCC_PLL1_SetN(pll1_n);
	while (LL_RCC_PLL1_GetN() != pll1_n) {
	}
	LL_RCC_PLL1_SetM(pll1_m);
	while (LL_RCC_PLL1_GetM() != pll1_m) {
	}
	LL_RCC_PLL1_SetP(pll1_p);
	while (LL_RCC_PLL1_GetP() != pll1_p) {
	}
	LL_RCC_PLL1_SetFRACV(pll1_v);
	while (LL_RCC_PLL1_GetFRACV() != pll1_v) {
	}

	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1) {
	}

	SET_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN);
	while (READ_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN) != RCC_PLL1CR_DIVPEN) {
	};

	LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_PLL1);
	while (LL_RCC_GetMPUClkSource() != LL_RCC_MPU_CLKSOURCE_PLL1) {
	}

#endif

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
