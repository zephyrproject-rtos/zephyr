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

/** Offset between RCC (Reset and Clock Control) enable-set and enable-clear registers. */
#define RCC_CLR_OFFSET		0x4

#define STM32_AXI_DIV  DT_PROP_OR(DT_NODELABEL(rcc), axi_prescaler, 1)
#define STM32_APB3_DIV DT_PROP_OR(DT_NODELABEL(rcc), apb3_prescaler, 1)
#define STM32_APB4_DIV DT_PROP_OR(DT_NODELABEL(rcc), apb4_prescaler, 1)
#define STM32_APB5_DIV DT_PROP_OR(DT_NODELABEL(rcc), apb5_prescaler, 1)
#define STM32_APB6_DIV DT_PROP_OR(DT_NODELABEL(rcc), apb6_prescaler, 1)

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
	    (src_clk == STM32_SRC_PLL4_R && IS_ENABLED(STM32_PLL4_R_ENABLED)) ||
	    (src_clk == STM32_SRC_TIMPCLK1) ||
	    (src_clk == STM32_SRC_TIMPCLK2) ||
	    (src_clk == STM32_SRC_TIMPCLK6)) {
		return 0;
	}

	return -ENOTSUP;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system, void *data);

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	volatile int temp;

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Source selection entry: apply it instead of toggling a gate */
		return stm32_clock_control_configure(dev, sub_system, NULL);
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
	uint32_t enr = pclken->enr;
	uint32_t reg = STM32_DT_CLKSEL_REG_GET(enr);
	uint32_t shift = STM32_DT_CLKSEL_SHIFT_GET(enr);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	if (pclken->enr == NO_SEL) {
		/* Domain clock is fixed. Nothing to set. Exit */
		return 0;
	}

	stm32_reg_modify_bits((uint32_t *)(DT_REG_ADDR(DT_NODELABEL(rcc)) + reg),
			      STM32_DT_CLKSEL_MASK_GET(enr) << shift,
			      STM32_DT_CLKSEL_VAL_GET(enr) << shift);

	return 0;
}

static int stm32_clock_control_get_eth_rate(uint32_t eth_source, uint32_t *rate)
{
	uint32_t eth_rate = LL_RCC_GetETHClockFreq(eth_source);

	if (eth_rate == LL_RCC_PERIPH_FREQUENCY_NO) {
		return -EIO;
	}

	*rate = eth_rate;

	return 0;
}

/*
 * Rates of the domain clocks of the ethernet controllers: the kernel clock
 * selectors and the PTP dividers, which divide the kernel clock by their
 * value plus one, all live in ETH12CKSELR.
 */
static int stm32_clock_control_get_eth_domain_rate(uint32_t enr, uint32_t *rate)
{
	uint32_t ptp_div;
	int ret;

	if (STM32_DT_CLKSEL_REG_GET(enr) != ETH12CKSELR_REG) {
		return -ENOTSUP;
	}

	switch (STM32_DT_CLKSEL_SHIFT_GET(enr)) {
	case RCC_ETH12CKSELR_ETH1SRC_Pos:
		return stm32_clock_control_get_eth_rate(LL_RCC_ETH1_CLKSOURCE, rate);
	case RCC_ETH12CKSELR_ETH2SRC_Pos:
		return stm32_clock_control_get_eth_rate(LL_RCC_ETH2_CLKSOURCE, rate);
	case RCC_ETH12CKSELR_ETH1PTPDIV_Pos:
		ret = stm32_clock_control_get_eth_rate(LL_RCC_ETH1_CLKSOURCE, rate);
		ptp_div = READ_BIT(RCC->ETH12CKSELR, RCC_ETH12CKSELR_ETH1PTPDIV) >>
			  RCC_ETH12CKSELR_ETH1PTPDIV_Pos;
		break;
	case RCC_ETH12CKSELR_ETH2PTPDIV_Pos:
		ret = stm32_clock_control_get_eth_rate(LL_RCC_ETH2_CLKSOURCE, rate);
		ptp_div = READ_BIT(RCC->ETH12CKSELR, RCC_ETH12CKSELR_ETH2PTPDIV) >>
			  RCC_ETH12CKSELR_ETH2PTPDIV_Pos;
		break;
	default:
		return -ENOTSUP;
	}

	if (ret == 0) {
		*rate /= ptp_div + 1;
	}

	return ret;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system, uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	LL_RCC_ClocksTypeDef clocks;

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_APB1:
		switch (pclken->enr) {
		case LL_APB1_GRP1_PERIPH_UART4:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART4_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_USART3:
		case LL_APB1_GRP1_PERIPH_UART5:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART35_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_UART7:
		case LL_APB1_GRP1_PERIPH_UART8:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART78_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_I2C1:
		case LL_APB1_GRP1_PERIPH_I2C2:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C12_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_SPI2:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI23_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_SPI3:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI23_CLKSOURCE);
			break;
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB2:
		switch (pclken->enr) {
		case LL_APB2_GRP1_PERIPH_SPI1:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI1_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_USART6:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART6_CLKSOURCE);
			break;
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB5:
		switch (pclken->enr) {
		case LL_APB5_GRP1_PERIPH_BSEC: {
			LL_RCC_ClocksTypeDef rcc_clocks;

			LL_RCC_GetSystemClocksFreq(&rcc_clocks);
			*rate = rcc_clocks.PCLK5_Frequency;
			break;
		}
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB6:
		switch (pclken->enr) {
		case LL_APB6_GRP1_PERIPH_USART1:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART1_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_USART2:
			*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART2_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_I2C3:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C3_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_I2C4:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C4_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_I2C5:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C5_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_SPI4:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI4_CLKSOURCE);
			break;
		case LL_APB6_GRP1_PERIPH_SPI5:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI5_CLKSOURCE);
			break;
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_AHB6:
		switch (pclken->enr) {
		case LL_AHB6_GRP1_PERIPH_ETH1MAC:
		case LL_AHB6_GRP1_PERIPH_ETH2MAC:
			LL_RCC_GetSystemClocksFreq(&clocks);
			*rate = clocks.HCLK6_Frequency;
			break;
		case LL_AHB6_GRP1_PERIPH_ETH1CK:
			return stm32_clock_control_get_eth_rate(LL_RCC_ETH1_CLKSOURCE, rate);
		case LL_AHB6_GRP1_PERIPH_ETH2CK:
			return stm32_clock_control_get_eth_rate(LL_RCC_ETH2_CLKSOURCE, rate);
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_SRC_PLL3_Q:
	case STM32_SRC_PLL4_P:
		return stm32_clock_control_get_eth_domain_rate(pclken->enr, rate);
	case STM32_SRC_TIMPCLK1:
		*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG1PRES);
		break;
	case STM32_SRC_TIMPCLK2:
		*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG2PRES);
		break;
	case STM32_SRC_TIMPCLK6:
		*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG3PRES);
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
		/* Enable the HSE (High-Speed External) oscillator. */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
			/* Wait for HSE ready */
		}
	}

	if (IS_ENABLED(STM32_HSI_ENABLED)) {
		/* Enable the HSI (High-Speed Internal) oscillator if needed. */
		if (LL_RCC_HSI_IsReady() != 1) {
			/* Enable HSI */
			LL_RCC_HSI_Enable();
			while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
			}
		}
	}
}

/**
 * Convert a power-of-two divisor to its STM32MP13 RCC encoding.
 * @param divisor Clock divisor.
 * @return Encoded RCC prescaler value.
 */
static uint32_t stm32_mp13_prescaler(uint32_t divisor)
{
	return find_lsb_set(divisor) - 1U;
}

/** Program all STM32MP13 bus prescalers from devicetree. */
static void set_up_bus_prescalers(void)
{
	/* Set the Cortex-A7 interconnect and microcontroller bus divider. */
	LL_RCC_SetMLHCLKPrescaler(stm32_mp13_prescaler(STM32_AHB_PRESCALER));
	while ((RCC->MLAHBDIVR & RCC_MLAHBDIVR_MLAHBDIVRDY) == 0U) {
	}

	LL_RCC_SetAPB1Prescaler(stm32_mp13_prescaler(STM32_APB1_PRESCALER));
	while ((RCC->APB1DIVR & RCC_APB1DIVR_APB1DIVRDY) == 0U) {
	}

	LL_RCC_SetAPB2Prescaler(stm32_mp13_prescaler(STM32_APB2_PRESCALER));
	while ((RCC->APB2DIVR & RCC_APB2DIVR_APB2DIVRDY) == 0U) {
	}

	LL_RCC_SetAPB3Prescaler(stm32_mp13_prescaler(STM32_APB3_DIV));
	while ((RCC->APB3DIVR & RCC_APB3DIVR_APB3DIVRDY) == 0U) {
	}

	LL_RCC_SetAPB4Prescaler(stm32_mp13_prescaler(STM32_APB4_DIV));
	while ((RCC->APB4DIVR & RCC_APB4DIVR_APB4DIVRDY) == 0U) {
	}

	LL_RCC_SetAPB5Prescaler(stm32_mp13_prescaler(STM32_APB5_DIV));
	while ((RCC->APB5DIVR & RCC_APB5DIVR_APB5DIVRDY) == 0U) {
	}

	LL_RCC_SetAPB6Prescaler(stm32_mp13_prescaler(STM32_APB6_DIV));
	while ((RCC->APB6DIVR & RCC_APB6DIVR_APB6DIVRDY) == 0U) {
	}
}

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	set_up_fixed_clock_sources();
	if (IS_ENABLED(CONFIG_SOC_SERIES_STM32MP13X_FSBL)) {
		set_up_bus_prescalers();
	}

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
		     "STM32MP13 phase-locked loop requires HSE to be enabled!");

	/*
	 * PLLs (Phase-Locked Loops) 1 and 2 share an input. An application running
	 * from DDR (Double Data Rate) memory must preserve PLL2 and its input clock.
	 * Its previous boot stage must already have selected HSE for PLL1 setup.
	 */
	if (!IS_ENABLED(CONFIG_SOC_SERIES_STM32MP13X_FSBL) &&
	    LL_RCC_PLL12_GetSource() != LL_RCC_PLL12SOURCE_HSE) {
		return -ENOTSUP;
	}

	/* Move the Cortex-A7 off PLL1 before reprogramming it. */
	LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_HSE);
	while (stm32_reg_read_bits(&RCC->MPCKSELR, RCC_MPCKSELR_MPUSRCRDY) !=
	       RCC_MPCKSELR_MPUSRCRDY) {
	}

#if defined(STM32_PLL2_ENABLED)
	if (IS_ENABLED(CONFIG_SOC_SERIES_STM32MP13X_FSBL)) {
		/* Only the FSBL (First-Stage Bootloader) may stop clocks before DDR is in use. */
		/*
		 * Take the AXI (Advanced eXtensible Interface) bus off PLL2 before
		 * reprogramming it.
		 */
		LL_RCC_SetAXISSClkSource(LL_RCC_AXISS_CLKSOURCE_HSE);
		while ((RCC->ASSCKSELR & RCC_ASSCKSELR_AXISSRCRDY) == 0U) {
		}

		/* Stop the PLL2 outputs and VCO (Voltage-Controlled Oscillator). */
		LL_RCC_PLL2P_Disable();
		LL_RCC_PLL2Q_Disable();
		LL_RCC_PLL2R_Disable();
		LL_RCC_PLL2_Disable();
		while (LL_RCC_PLL2_IsReady() != 0U) {
		}
	}
#endif

	/* Stop the PLL1 outputs and VCO before changing its factors. */
	stm32_reg_clear_bits(&RCC->PLL1CR,
			     RCC_PLL1CR_DIVPEN | RCC_PLL1CR_DIVQEN | RCC_PLL1CR_DIVREN);
	while ((RCC->PLL1CR & (RCC_PLL1CR_DIVPEN | RCC_PLL1CR_DIVQEN | RCC_PLL1CR_DIVREN)) != 0U) {
	}
	LL_RCC_PLL1_Disable();
	while (LL_RCC_PLL1_IsReady() != 0U) {
	}

	if (IS_ENABLED(CONFIG_SOC_SERIES_STM32MP13X_FSBL)) {
		/* Select the shared PLL1/PLL2 input before DDR initialization. */
		LL_RCC_PLL12_SetSource(LL_RCC_PLL12SOURCE_HSE);
		while ((RCC->RCK12SELR & RCC_RCK12SELR_PLL12SRCRDY) == 0U) {
		}
	}

	/* Configure the Cortex-A7 clock using the PLL1 factors specified in devicetree. */
	LL_RCC_PLL1_SetN(STM32_PLL_N_MULTIPLIER);
	LL_RCC_PLL1_SetM(STM32_PLL_M_DIVISOR);
	LL_RCC_PLL1_SetP(STM32_PLL_P_DIVISOR);
	/* Latch the new PLL1 fractional value while fractional updates are disabled. */
	LL_RCC_PLL1FRACV_Disable();
	LL_RCC_PLL1_SetFRACV(STM32_PLL_FRACN_VALUE);
	LL_RCC_PLL1FRACV_Enable();

	/* Start PLL1, then enable its P output for the Cortex-A7 clock. */
	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1U) {
	}

	LL_RCC_PLL1P_Enable();

#if defined(STM32_PLL2_ENABLED)
	if (IS_ENABLED(CONFIG_SOC_SERIES_STM32MP13X_FSBL)) {
		/* Program PLL2 outputs for the AXI and DDR clock domains. */
		LL_RCC_PLL2_SetN(STM32_PLL2_N_MULTIPLIER);
		LL_RCC_PLL2_SetM(STM32_PLL2_M_DIVISOR);
		LL_RCC_PLL2_SetP(STM32_PLL2_P_DIVISOR);
		LL_RCC_PLL2_SetQ(STM32_PLL2_Q_DIVISOR);
		LL_RCC_PLL2_SetR(STM32_PLL2_R_DIVISOR);
		/* Latch the new PLL2 fractional value while fractional updates are disabled. */
		LL_RCC_PLL2FRACV_Disable();
		LL_RCC_PLL2_SetFRACV(STM32_PLL2_FRACN_VALUE);
		LL_RCC_PLL2FRACV_Enable();
		/* Start PLL2 before enabling the outputs requested by devicetree. */
		LL_RCC_PLL2_Enable();
		while (LL_RCC_PLL2_IsReady() != 1U) {
		}

		if (IS_ENABLED(STM32_PLL2_P_ENABLED)) {
			LL_RCC_PLL2P_Enable();
		}
		if (IS_ENABLED(STM32_PLL2_Q_ENABLED)) {
			LL_RCC_PLL2Q_Enable();
		}
		if (IS_ENABLED(STM32_PLL2_R_ENABLED)) {
			LL_RCC_PLL2R_Enable();
		}

		/* Set the AXI interconnect divider. */
		LL_RCC_SetACLKPrescaler(STM32_AXI_DIV - 1U);
		while ((RCC->AXIDIVR & RCC_AXIDIVR_AXIDIVRDY) == 0U) {
		}
		/* Move the AXI interconnect to the configured PLL2P output. */
		LL_RCC_SetAXISSClkSource(LL_RCC_AXISS_CLKSOURCE_PLL2);
		while ((RCC->ASSCKSELR & RCC_ASSCKSELR_AXISSRCRDY) == 0U) {
		}
	}
#endif

	LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_PLL1);
	while (LL_RCC_GetMPUClkSource() != LL_RCC_MPU_CLKSOURCE_PLL1) {
	}

#endif

	SystemCoreClock = STM32_HCLK_FREQUENCY;

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
