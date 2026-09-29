/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *) sub_system;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_CLOCK_PERIPH_MIN, STM32_CLOCK_PERIPH_MAX)) {
		/* Attempt to change a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *) sub_system;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_CLOCK_PERIPH_MIN, STM32_CLOCK_PERIPH_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);

	return 0;
}

static bool source_is_ready(uint32_t src)
{
	switch (src) {
	case STM32_SRC_SYSCLK:
	case STM32_SRC_ICN_LS_MCU:
	case STM32_SRC_PCLK1:
	case STM32_SRC_PCLK2:
	case STM32_SRC_PCLK3:
	case STM32_SRC_PCLK4:
#if defined(RCC_APB5DIVR_APB5DIV)
	case STM32_SRC_PCLK5:
#endif /* RCC_APB5DIVR_APB5DIV */
	case STM32_SRC_PCLKDBG:
		/* Running by construction: the Cortex-M33 executes off these clocks */
		return true;
	case STM32_SRC_HSI:
		return LL_RCC_HSI_IsReady() == 1U;
	case STM32_SRC_HSE:
		return LL_RCC_HSE_IsReady() == 1U;
	case STM32_SRC_MSI:
		return LL_RCC_MSI_IsReady() == 1U;
	case STM32_SRC_LSE:
		return LL_RCC_LSE_IsReady() == 1U;
	case STM32_SRC_LSI:
		return LL_RCC_LSI_IsReady() == 1U;
	case STM32_SRC_PLL4:
		return LL_RCC_PLL4_IsReady() == 1U;
	case STM32_SRC_PLL5:
		return LL_RCC_PLL5_IsReady() == 1U;
	case STM32_SRC_PLL6:
		return LL_RCC_PLL6_IsReady() == 1U;
	case STM32_SRC_PLL7:
		return LL_RCC_PLL7_IsReady() == 1U;
	case STM32_SRC_PLL8:
		return LL_RCC_PLL8_IsReady() == 1U;
	default:
		return false;
	}
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system, uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	/*
	 * ck_icn_hs_mcu (= SystemCoreClock) is the Cortex-M33 clock, every APB
	 * clock derives from ck_icn_ls_mcu. All prescalers are shift counts.
	 */
	uint32_t hs_mcu_clock = SystemCoreClock;
	uint32_t ls_mcu_clock = hs_mcu_clock >> LL_RCC_Get_LSMCUDIVR();
	uint32_t apb1_clock = ls_mcu_clock >> LL_RCC_GetAPB1Prescaler();
	uint32_t apb2_clock = ls_mcu_clock >> LL_RCC_GetAPB2Prescaler();
	uint32_t apb3_clock = ls_mcu_clock >> LL_RCC_GetAPB3Prescaler();
	uint32_t apb4_clock = ls_mcu_clock >> LL_RCC_GetAPB4Prescaler();
	uint32_t apbdbg_clock = ls_mcu_clock >> LL_RCC_GetAPBDBGPrescaler();
	LL_PLL_ClocksTypeDef pll_clocks;

	ARG_UNUSED(dev);

	switch (pclken->bus) {
#if defined(LL_RCC_USART1_CLKSOURCE)
	case STM32_CLOCK_PERIPH_USART1:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART1_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_UART24_CLKSOURCE)
	case STM32_CLOCK_PERIPH_USART2:
	case STM32_CLOCK_PERIPH_UART4:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART24_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_USART35_CLKSOURCE)
	case STM32_CLOCK_PERIPH_USART3:
	case STM32_CLOCK_PERIPH_UART5:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART35_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_USART6_CLKSOURCE)
	case STM32_CLOCK_PERIPH_USART6:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART6_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_UART78_CLKSOURCE)
	case STM32_CLOCK_PERIPH_UART7:
	case STM32_CLOCK_PERIPH_UART8:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART78_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_UART9_CLKSOURCE)
	case STM32_CLOCK_PERIPH_UART9:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART9_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_I2C12_I3C12_CLKSOURCE)
	case STM32_CLOCK_PERIPH_I2C1:
	case STM32_CLOCK_PERIPH_I2C2:
	case STM32_CLOCK_PERIPH_I3C1:
	case STM32_CLOCK_PERIPH_I3C2:
		*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C12_I3C12_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_I2C46_CLKSOURCE)
	case STM32_CLOCK_PERIPH_I2C4:
	case STM32_CLOCK_PERIPH_I2C6:
		*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C46_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_I2C35_I3C3_CLKSOURCE)
	case STM32_CLOCK_PERIPH_I2C3:
	case STM32_CLOCK_PERIPH_I2C5:
	case STM32_CLOCK_PERIPH_I3C3:
		*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C35_I3C3_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_I2C7_CLKSOURCE)
	case STM32_CLOCK_PERIPH_I2C7:
		*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C7_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_I2C8_CLKSOURCE)
	case STM32_CLOCK_PERIPH_I2C8:
		*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C8_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_I3C4_CLKSOURCE)
	case STM32_CLOCK_PERIPH_I3C4:
		*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I3C4_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_SPI1_CLKSOURCE)
	case STM32_CLOCK_PERIPH_SPI1:
		*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI1_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_SPI23_CLKSOURCE)
	case STM32_CLOCK_PERIPH_SPI2:
	case STM32_CLOCK_PERIPH_SPI3:
		*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI23_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_SPI45_CLKSOURCE)
	case STM32_CLOCK_PERIPH_SPI4:
	case STM32_CLOCK_PERIPH_SPI5:
		*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI45_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_SPI67_CLKSOURCE)
	case STM32_CLOCK_PERIPH_SPI6:
	case STM32_CLOCK_PERIPH_SPI7:
		*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI67_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_SPI8_CLKSOURCE)
	case STM32_CLOCK_PERIPH_SPI8:
		*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI8_CLKSOURCE);
		break;
#endif
#if defined(LL_RCC_FDCAN_CLKSOURCE)
	case STM32_CLOCK_PERIPH_FDCAN:
		*rate = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);
		break;
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(timers12))
	case STM32_CLOCK_PERIPH_TIM12:
		/* Timer group 1 kernel clock equals APB1 when APB1 is not divided */
		if (LL_RCC_GetAPB1Prescaler() != 0U) {
			return -ENOTSUP;
		}
		*rate = SystemCoreClock >> LL_RCC_Get_LSMCUDIVR();
		break;
#endif
	case STM32_CLOCK_PERIPH_WWDG1:
		/* The WWDG1 clock is derived from the APB3 clock */
		*rate = apb3_clock;
		break;
	case STM32_SRC_SYSCLK:
		*rate = hs_mcu_clock;
		break;
	case STM32_SRC_ICN_LS_MCU:
		*rate = ls_mcu_clock;
		break;
	case STM32_SRC_PCLK1:
		*rate = apb1_clock;
		break;
	case STM32_SRC_PCLK2:
		*rate = apb2_clock;
		break;
	case STM32_SRC_PCLK3:
		*rate = apb3_clock;
		break;
	case STM32_SRC_PCLK4:
		*rate = apb4_clock;
		break;
#if defined(RCC_APB5DIVR_APB5DIV)
	case STM32_SRC_PCLK5:
		*rate = ls_mcu_clock >> LL_RCC_GetAPB5Prescaler();
		break;
#endif /* RCC_APB5DIVR_APB5DIV */
	case STM32_SRC_PCLKDBG:
		*rate = apbdbg_clock;
		break;
	case STM32_SRC_HSI:
		*rate = HSI_VALUE;
		break;
	case STM32_SRC_HSE:
		*rate = HSE_VALUE;
		break;
	case STM32_SRC_MSI:
		*rate = MSI_VALUE;
		break;
	case STM32_SRC_LSE:
		*rate = LSE_VALUE;
		break;
	case STM32_SRC_LSI:
		*rate = LSI_VALUE;
		break;
	case STM32_SRC_PLL4:
		LL_RCC_GetPLL4ClockFreq(&pll_clocks);
		*rate = pll_clocks.freq;
		break;
	case STM32_SRC_PLL5:
		LL_RCC_GetPLL5ClockFreq(&pll_clocks);
		*rate = pll_clocks.freq;
		break;
	case STM32_SRC_PLL6:
		LL_RCC_GetPLL6ClockFreq(&pll_clocks);
		*rate = pll_clocks.freq;
		break;
	case STM32_SRC_PLL7:
		LL_RCC_GetPLL7ClockFreq(&pll_clocks);
		*rate = pll_clocks.freq;
		break;
	case STM32_SRC_PLL8:
		LL_RCC_GetPLL8ClockFreq(&pll_clocks);
		*rate = pll_clocks.freq;
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

	if (IN_RANGE(pclken->bus, STM32_CLOCK_PERIPH_MIN, STM32_CLOCK_PERIPH_MAX)) {
		if ((sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus) & pclken->enr)
		    == pclken->enr) {
			return CLOCK_CONTROL_STATUS_ON;
		}

		return CLOCK_CONTROL_STATUS_OFF;
	}

	return source_is_ready(pclken->bus) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.get_status = stm32_clock_control_get_status,
};

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc), stm32_clock_control_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &stm32_clock_control_api);
