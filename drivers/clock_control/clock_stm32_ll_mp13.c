/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_bitops.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <stm32_ll_utils.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

/** Offset between RCC_MP_xxxENSETR and RCC_MP_xxxENCLRR registers */
#define RCC_CLR_OFFSET		0x4

/** @brief Verifies clock is part of active clock configuration */
int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_HSE && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    (src_clk == STM32_SRC_HSI && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    (src_clk == STM32_SRC_LSE && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    (src_clk == STM32_SRC_LSI && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL1_P && IS_ENABLED(STM32_PLL_P_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL2_P && IS_ENABLED(STM32_PLL2_P_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL2_Q && IS_ENABLED(STM32_PLL2_Q_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL2_R && IS_ENABLED(STM32_PLL2_R_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL3_P && IS_ENABLED(STM32_PLL3_P_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL3_Q && IS_ENABLED(STM32_PLL3_Q_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL3_R && IS_ENABLED(STM32_PLL3_R_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL4_P && IS_ENABLED(STM32_PLL4_P_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL4_Q && IS_ENABLED(STM32_PLL4_Q_ENABLED)) ||
	    (src_clk == STM32_SRC_PLL4_R && IS_ENABLED(STM32_PLL4_R_ENABLED))) {
		return 0;
	}

	return -ENOTSUP;
}

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	volatile int temp;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	/* STM32MP13 has EN_SET registers - no need for RMW */
	sys_write32(pclken->enr, DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
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

	/* STM32MP13 has EN_CLR register at pclken->bus + RCC_CLR_OFFSET - no need for RMW */
	sys_write32(pclken->enr, DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus + RCC_CLR_OFFSET);
	/* Ensure that the write operation is completed */
	temp = sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus + RCC_CLR_OFFSET);
	UNUSED(temp);

	return 0;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		       STM32_DT_CLKSEL_MASK_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));
	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		     STM32_DT_CLKSEL_VAL_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));

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
		case LL_APB1_GRP1_PERIPH_I2C1:
		case LL_APB1_GRP1_PERIPH_I2C2:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C12_CLKSOURCE);
			break;
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB6:
		switch (pclken->enr) {
		case LL_APB6_GRP1_PERIPH_I2C3:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C3_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_I2C4:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C4_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_I2C5:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C5_CLKSOURCE);
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

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
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
	.configure = stm32_clock_control_configure,
	.get_status = stm32_clock_control_get_status,
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
	while (stm32_reg_read_bits(&RCC->MPCKSELR, RCC_MPCKSELR_MPUSRCRDY) !=
	       RCC_MPCKSELR_MPUSRCRDY) {
	}

	stm32_reg_clear_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVPEN);
	while (stm32_reg_read_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVPEN) == RCC_PLL1CR_DIVPEN) {
	};

	stm32_reg_clear_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVQEN);
	while (stm32_reg_read_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVQEN) == RCC_PLL1CR_DIVQEN) {
	};

	stm32_reg_clear_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVREN);
	while (stm32_reg_read_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVREN) == RCC_PLL1CR_DIVREN) {
	};

	uint32_t pll1_n = DT_PROP(DT_NODELABEL(pll1), mul_n);
	uint32_t pll1_m = DT_PROP(DT_NODELABEL(pll1), div_m);
	uint32_t pll1_p = DT_PROP(DT_NODELABEL(pll1), div_p);
	uint32_t pll1_fracn = DT_PROP(DT_NODELABEL(pll1), fracn);

	LL_RCC_PLL1_SetN(pll1_n);
	while (LL_RCC_PLL1_GetN() != pll1_n) {
	}
	LL_RCC_PLL1_SetM(pll1_m);
	while (LL_RCC_PLL1_GetM() != pll1_m) {
	}
	LL_RCC_PLL1_SetP(pll1_p);
	while (LL_RCC_PLL1_GetP() != pll1_p) {
	}
	LL_RCC_PLL1_SetFRACV(pll1_fracn);
	while (LL_RCC_PLL1_GetFRACV() != pll1_fracn) {
	}

	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1) {
	}

	stm32_reg_set_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVPEN);
	while (stm32_reg_read_bits(&RCC->PLL1CR, RCC_PLL1CR_DIVPEN) != RCC_PLL1CR_DIVPEN) {
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
