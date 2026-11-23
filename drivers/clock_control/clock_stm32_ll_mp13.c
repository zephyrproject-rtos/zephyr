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
	    (src_clk == STM32_SRC_PLL4_R && IS_ENABLED(STM32_PLL4_R_ENABLED)) ||
	    (src_clk == STM32_SRC_TIMPCLK1) ||
	    (src_clk == STM32_SRC_TIMPCLK2) ||
	    (src_clk == STM32_SRC_TIMPCLK6) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK3) ||
	    (src_clk == STM32_SRC_PCLK4) ||
	    (src_clk == STM32_SRC_PCLK5) ||
	    (src_clk == STM32_SRC_PCLK6) ||
	    (src_clk == STM32_SRC_HCLK6)) {
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
	(void)temp;

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
	(void)temp;

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

/**
 * @brief Frequency of PLL output @p src and readiness of its PLL
 *
 * @return false if @p src is not a PLL output
 */
static bool get_pll_output(uint32_t src, uint32_t *rate, bool *ready)
{
	LL_PLL_ClocksTypeDef pll_clocks = {0};
	bool is_pll = true;

	switch (src) {
	case STM32_SRC_PLL1_P:
		LL_RCC_GetPLL1ClockFreq(&pll_clocks);
		*ready = (LL_RCC_PLL1_IsReady() == 1U);
		break;
	case STM32_SRC_PLL2_P:
	case STM32_SRC_PLL2_Q:
	case STM32_SRC_PLL2_R:
		LL_RCC_GetPLL2ClockFreq(&pll_clocks);
		*ready = (LL_RCC_PLL2_IsReady() == 1U);
		break;
	case STM32_SRC_PLL3_P:
	case STM32_SRC_PLL3_Q:
	case STM32_SRC_PLL3_R:
		LL_RCC_GetPLL3ClockFreq(&pll_clocks);
		*ready = (LL_RCC_PLL3_IsReady() == 1U);
		break;
	case STM32_SRC_PLL4_P:
	case STM32_SRC_PLL4_Q:
	case STM32_SRC_PLL4_R:
		LL_RCC_GetPLL4ClockFreq(&pll_clocks);
		*ready = (LL_RCC_PLL4_IsReady() == 1U);
		break;
	default:
		is_pll = false;
		break;
	}

	if (is_pll) {
		switch (src) {
		case STM32_SRC_PLL1_P:
		case STM32_SRC_PLL2_P:
		case STM32_SRC_PLL3_P:
		case STM32_SRC_PLL4_P:
			*rate = pll_clocks.PLL_P_Frequency;
			break;
		case STM32_SRC_PLL2_Q:
		case STM32_SRC_PLL3_Q:
		case STM32_SRC_PLL4_Q:
			*rate = pll_clocks.PLL_Q_Frequency;
			break;
		default:
			*rate = pll_clocks.PLL_R_Frequency;
			break;
		}
	}

	return is_pll;
}

static bool source_is_ready(uint32_t src)
{
	uint32_t rate;
	bool ready;

	if (get_pll_output(src, &rate, &ready)) {
		/* The PLL is locked and the output divider is enabled */
		ready = ready && (rate != 0U);
	} else {
		switch (src) {
		case STM32_SRC_TIMPCLK1:
		case STM32_SRC_TIMPCLK2:
		case STM32_SRC_TIMPCLK6:
		case STM32_SRC_PCLK1:
		case STM32_SRC_PCLK2:
		case STM32_SRC_PCLK3:
		case STM32_SRC_PCLK4:
		case STM32_SRC_PCLK5:
		case STM32_SRC_PCLK6:
		case STM32_SRC_HCLK6:
			/* Bus clocks run as long as the system does */
			ready = true;
			break;
		case STM32_SRC_HSE:
			ready = (LL_RCC_HSE_IsReady() == 1U);
			break;
		case STM32_SRC_HSI:
			ready = (LL_RCC_HSI_IsReady() == 1U);
			break;
		case STM32_SRC_LSE:
			ready = (LL_RCC_LSE_IsReady() == 1U);
			break;
		case STM32_SRC_LSI:
			ready = (LL_RCC_LSI_IsReady() == 1U);
			break;
		default:
			ready = false;
			break;
		}
	}

	return ready;
}

/** @brief Frequency of bus clock @p src */
static uint32_t get_bus_rate(uint32_t src)
{
	LL_RCC_ClocksTypeDef clocks;
	uint32_t rate;

	LL_RCC_GetSystemClocksFreq(&clocks);

	switch (src) {
	case STM32_SRC_PCLK1:
		rate = clocks.PCLK1_Frequency;
		break;
	case STM32_SRC_PCLK2:
		rate = clocks.PCLK2_Frequency;
		break;
	case STM32_SRC_PCLK3:
		rate = clocks.PCLK3_Frequency;
		break;
	case STM32_SRC_PCLK4:
		rate = clocks.PCLK4_Frequency;
		break;
	case STM32_SRC_PCLK5:
		rate = clocks.PCLK5_Frequency;
		break;
	case STM32_SRC_PCLK6:
		rate = clocks.PCLK6_Frequency;
		break;
	case STM32_SRC_HCLK6:
		rate = clocks.HCLK6_Frequency;
		break;
	default:
		rate = 0U;
		break;
	}

	return rate;
}

/** @brief Frequency of clock source @p src */
static int get_source_rate(uint32_t src, uint32_t *rate)
{
	bool pll_ready;
	int ret = 0;

	if (get_pll_output(src, rate, &pll_ready)) {
		/* A PLL output that is not running gives an error rather than a zero rate */
		if (*rate == 0U) {
			ret = -EIO;
		}
	} else {
		switch (src) {
		case STM32_SRC_HSE:
			*rate = HSE_VALUE;
			break;
		case STM32_SRC_HSI:
			*rate = HSI_VALUE >> LL_RCC_HSI_GetDivider();
			break;
		case STM32_SRC_LSE:
			*rate = LSE_VALUE;
			break;
		case STM32_SRC_LSI:
			*rate = LSI_VALUE;
			break;
		case STM32_SRC_TIMPCLK1:
			*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG1PRES);
			break;
		case STM32_SRC_TIMPCLK2:
			*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG2PRES);
			break;
		case STM32_SRC_TIMPCLK6:
			*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG3PRES);
			break;
		case STM32_SRC_PCLK1:
		case STM32_SRC_PCLK2:
		case STM32_SRC_PCLK3:
		case STM32_SRC_PCLK4:
		case STM32_SRC_PCLK5:
		case STM32_SRC_PCLK6:
		case STM32_SRC_HCLK6:
			*rate = get_bus_rate(src);
			break;
		default:
			ret = -ENOTSUP;
			break;
		}
	}

	return ret;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system, uint32_t *rate)
{
	const struct stm32_pclken *pclken = (const struct stm32_pclken *)sub_system;
	int ret;

	ARG_UNUSED(dev);

	if ((pclken->bus < (uint32_t)STM32_PERIPH_BUS_MIN) &&
	    (STM32_DT_CLKSEL_REG_GET(pclken->enr) == (uint32_t)ETH12CKSELR_REG)) {
		/* Ethernet kernel and PTP clocks, resolved from their actual selection */
		ret = stm32_clock_control_get_eth_domain_rate(pclken->enr, rate);
	} else {
		switch (pclken->bus) {
		case STM32_CLOCK_BUS_APB5:
			/* The BSEC has no kernel clock, it runs on pclk5 */
			if (pclken->enr == LL_APB5_GRP1_PERIPH_BSEC) {
				*rate = get_bus_rate(STM32_SRC_PCLK5);
				ret = 0;
			} else {
				ret = -ENOTSUP;
			}
			break;
		case STM32_CLOCK_BUS_AHB6:
			/* The ethernet MAC bus interfaces run on hclk6 */
			if ((pclken->enr == LL_AHB6_GRP1_PERIPH_ETH1MAC) ||
			    (pclken->enr == LL_AHB6_GRP1_PERIPH_ETH2MAC)) {
				*rate = get_bus_rate(STM32_SRC_HCLK6);
				ret = 0;
			} else {
				ret = -ENOTSUP;
			}
			break;
		default:
			ret = get_source_rate(pclken->bus, rate);
			break;
		}
	}

	if ((ret == 0) && (pclken->div != 0U)) {
		*rate /= ((uint32_t)pclken->div + 1U);
	}

	return ret;
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
	}

	/* Domain clock sources */
	return source_is_ready(pclken->bus) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
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
