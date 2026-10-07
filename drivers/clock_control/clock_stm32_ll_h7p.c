/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <cmsis_core.h>
#include <stm32cube_hal.h>
#include <stm32_backup_domain.h>
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_flash.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#define DT_DRV_COMPAT st_stm32h7p_rcc

#define RCC_REG(_reg_offset) ((volatile uint32_t *)(DT_INST_REG_ADDR(0) + (_reg_offset)))

/* Devicetree property -> LL conversion macros */
#define presc2ll(bus, val) CONCAT(LL_RCC_, bus, _PRESCALER_, val)
#define div2ll(src, val) CONCAT(LL_RCC_, src, _DIV_, val)

BUILD_ASSERT(
	DT_INST_PROP_LEN(0, st_clock_sources) == DT_INST_PROP_LEN(0, st_clock_source_identifiers),
	"Properties 'st,clock-sources' and 'st,clock-source-identifiers' of RCC node must have "
	"the same number of elements.");

/* Use unsigned long long size so mulitplication is done on 64 bits */
#define PLL_FRACN_DIVISOR 8192ULL

#define PLLX_VCO_FREQ(srcfreq, divm, divn, fracn) \
	((((srcfreq) / (divm)) * (((divn) * PLL_FRACN_DIVISOR) + (fracn))) / PLL_FRACN_DIVISOR)

#define PLLX_OUT_FREQ(srcfreq, divm, divn, fracn, out_div) \
	((out_div == 0) ? 0 : PLLX_VCO_FREQ((srcfreq), (divm), (divn), (fracn)) / (out_div))

#if defined(STM32_PLL_SRC_HSI) || defined(STM32_PLL2_SRC_HSI) || defined(STM32_PLL3_SRC_HSI)
#define STM32_PLL_SOURCE LL_RCC_PLLSOURCE_HSI
#define PLLSRC_FREQ  ((STM32_HSI_FREQ)/(STM32_HSI_DIVISOR))
#endif

#if defined(STM32_PLL_SRC_CSI) || defined(STM32_PLL2_SRC_CSI) || defined(STM32_PLL3_SRC_CSI)
#if !defined(PLLSRC_FREQ)
#define STM32_PLL_SOURCE LL_RCC_PLLSOURCE_CSI
#define PLLSRC_FREQ  STM32_CSI_FREQ
#else
#error "All PLLs must have the same clock source"
#endif
#endif

#if defined(STM32_PLL_SRC_HSE) || defined(STM32_PLL2_SRC_HSE) || defined(STM32_PLL3_SRC_HSE)
#if !defined(PLLSRC_FREQ)
#define STM32_PLL_SOURCE LL_RCC_PLLSOURCE_HSE
#define PLLSRC_FREQ  STM32_HSE_FREQ
#else
#error "All PLLs must have the same clock source"
#endif
#endif

/* Dummy value to enable compilation when PLLs are unused */
#if !defined(STM32_PLL_SOURCE)
#define STM32_PLL_SOURCE 0
#endif /* !defined(STM32_PLL_SOURCE) */

#if !defined(PLLSRC_FREQ)
#define PLLSRC_FREQ 0
#endif /* !defined(PLLSRC_FREQ) */

/* helpers for the two macros below */
#define FE_DEVSRC_HELPER(rcc_node_id, devices_prop, idx, identifiers_prop, _fn)			\
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_PHANDLE_BY_IDX(rcc_node_id, devices_prop, idx)),	\
	(_fn(DT_PROP_BY_IDX(rcc_node_id, identifiers_prop, idx),				\
	     DT_PHANDLE_BY_IDX(rcc_node_id, devices_prop, idx))))

#define FE_DEVSRC_VARGS_HELPER(rcc_node_id, devices_prop, idx, identifiers_prop, _fn, ...)	\
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_PHANDLE_BY_IDX(rcc_node_id, devices_prop, idx)),	\
	(_fn(DT_PROP_BY_IDX(rcc_node_id, identifiers_prop, idx),				\
	     DT_PHANDLE_BY_IDX(rcc_node_id, devices_prop, idx),					\
	     __VA_ARGS__)))

/**
 * Iterate over <(source ID), (device)> pairs for active sources
 * in the st,clock-sources / st,clock-source-identifiers properties.
 *
 * @param fn Macro fn(src_id, src_dev_node_id)
 */
#define FOR_EACH_ENABLED_DEVSRC(_fn)						\
	DT_INST_FOREACH_PROP_ELEM_VARGS(0, st_clock_sources, FE_DEVSRC_HELPER,	\
		st_clock_source_identifiers, _fn)

/**
 * Same as @ref FOR_EACH_ENABLED_DEVSRC, but allows providing arbitrary arguments
 * to the @p fn macro function.
 *
 * @param fn Macro fn(src_id, src_dev_node_id, ...)
 * @param ... Additional arguments to pass to the @p fn macro function.
 */
#define FOR_EACH_ENABLED_DEVSRC_VARGS(_fn, ...)						\
	DT_INST_FOREACH_PROP_ELEM_VARGS(0, st_clock_sources, FE_DEVSRC_VARGS_HELPER,	\
		st_clock_source_identifiers, _fn, __VA_ARGS__)

/**
 * @brief @ref enabled_clock() switch case for an enabled device source
 * @note The FOR_EACH macro expands the fn() argument only for enabled
 *       nodes, so no explicit IS_ENABLED() check is required inside this.
 */
#define CHECK_DEVSOURCE_ENABLED_CASE(ck_src_id, ck_src_dev_id)			\
	case ck_src_id:								\
		break;

/**
 * @brief @ref stm32_clock_control_get_subsys_rate() switch case for PLL output
 */
#define GET_SUBSYS_CASE_PLL_FREQUENCY(num, output, numsfx)			\
	case STM32_SRC_PLL##num##_##output:					\
	{									\
		*rate = PLLX_OUT_FREQ(PLLSRC_FREQ,				\
				      STM32_PLL##numsfx##_M_DIVISOR,		\
				      STM32_PLL##numsfx##_N_MULTIPLIER,		\
				      STM32_PLL##numsfx##_FRACN_VALUE,		\
				      STM32_PLL##numsfx##_##output##_DIVISOR);	\
		break;								\
	}

/**
 * @brief @ref stm32_clock_control_get_subsys_rate() switch case for enabled device source
 */
#define CASE_DEVSRC_FREQUENCY_INNER(ck_src_id, ck_src_dev_id, _pclken_name)	\
	case ck_src_id: {							\
		static const struct stm32_pclken _pclken_name =			\
			STM32_CLOCK_INFO(0, ck_src_dev_id);			\
										\
		res = stm32_clock_control_get_subsys_rate(dev,			\
			(clock_control_subsys_t)&_pclken_name, rate);		\
		break;								\
	}

#define GET_SUBSYS_CASE_DEVSRC_FREQUENCY(ck_src_id, ck_src_dev_id)		\
	CASE_DEVSRC_FREQUENCY_INNER(ck_src_id, ck_src_dev_id,			\
		CONCAT(dev_, DT_DEP_ORD(ck_src_dev_id), _clk))

#ifdef STM32_HSE_CSS
/*
 * A wrapper is necessary because Zephyr NMI handlers are expected to return void,
 * but the HAL2 NMI handler returns hal_status_t instead.
 */
void hal_rcc_nmi_irqhandler_wrapper(void)
{
	(void)HAL_RCC_NMI_IRQHandler();
}
#endif /* STM32_HSE_CSS */

/** @brief Verifies if provided domain / bus clock is currently active */
int enabled_clock(uint32_t src_clk)
{
	int res = 0;

	if ((src_clk == STM32_SRC_SYSCLK) ||
	    (src_clk == STM32_SRC_HCLKN) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK4) ||
	    (src_clk == STM32_SRC_TIMCK) || /* tim_ck = sys_cpu_ck, always on */
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_CSI) && IS_ENABLED(STM32_CSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI48) && IS_ENABLED(STM32_HSI48_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_P) && IS_ENABLED(STM32_PLL_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_Q) && IS_ENABLED(STM32_PLL_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_S) && IS_ENABLED(STM32_PLL_S_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_P) && IS_ENABLED(STM32_PLL2_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_Q) && IS_ENABLED(STM32_PLL2_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_R) && IS_ENABLED(STM32_PLL2_R_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_S) && IS_ENABLED(STM32_PLL2_S_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_T) && IS_ENABLED(STM32_PLL2_T_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_P) && IS_ENABLED(STM32_PLL3_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_Q) && IS_ENABLED(STM32_PLL3_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_R) && IS_ENABLED(STM32_PLL3_R_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_S) && IS_ENABLED(STM32_PLL3_S_ENABLED))) {
		return 0;
	}

	switch (src_clk) {
	default:
		res = -ENOTSUP;
		break;
	FOR_EACH_ENABLED_DEVSRC(CHECK_DEVSOURCE_ENABLED_CASE)
	}


	return res;
}

/* Forward declaration as we need this function right now */
static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data);

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile uint32_t *reg = RCC_REG(pclken->bus);
	volatile uint32_t temp;

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Not a clock gate; parse as mux configuration instead */
		return stm32_clock_control_configure(dev, sub_system, NULL);
	}

	stm32_reg_set_bits(reg, pclken->enr);

	/* Read back register to be blocked by RCC
	 * until peripheral clock enabling is complete
	 */
	temp = stm32_reg_read(reg);
	(void)temp;

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempting to change domain clock */
		return -ENOTSUP;
	}

	stm32_reg_clear_bits(RCC_REG(pclken->bus), pclken->enr);

	return 0;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	uint32_t enr = pclken->enr;
	uint32_t reg_offset = STM32_DT_CLKSEL_REG_GET(enr);
	uint32_t shift = STM32_DT_CLKSEL_SHIFT_GET(enr);
	int err;

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempting to configure an unavailable or invalid clock */
		return err;
	}

	if (pclken->enr == NO_SEL) {
		/* Domain clock is fixed - nothing to configure. */
		return 0;
	}

	stm32_reg_modify_bits(RCC_REG(reg_offset),
			      STM32_DT_CLKSEL_MASK_GET(enr) << shift,
			      STM32_DT_CLKSEL_VAL_GET(enr) << shift);

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	ARG_UNUSED(dev);
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int res = 0;

	const uint32_t sys_cpu_ck = SystemCoreClock;
	const uint32_t bmclk = sys_cpu_ck / STM32_BMPRE;

	switch (pclken->bus) {
	default:
		return -ENOTSUP;
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB3:
	case STM32_CLOCK_BUS_AHB4:
	case STM32_CLOCK_BUS_AHB5:
	case STM32_SRC_HCLKN:
		*rate = bmclk;
		break;
	case STM32_CLOCK_BUS_APB1:
	case STM32_CLOCK_BUS_APB1_2:
	case STM32_SRC_PCLK1:
		*rate = bmclk / STM32_PPRE1;
		break;
	case STM32_CLOCK_BUS_APB2:
	case STM32_SRC_PCLK2:
		*rate = bmclk / STM32_PPRE2;
		break;
	case STM32_CLOCK_BUS_APB4:
	case STM32_SRC_PCLK4:
		*rate = bmclk / STM32_PPRE4;
		break;
	case STM32_SRC_TIMCK:
		*rate = sys_cpu_ck;
		break;
	case STM32_SRC_SYSCLK: /* sys_ck */
		*rate = sys_cpu_ck * STM32_CPRE;
		break;
#if defined(STM32_LSE_ENABLED)
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
#endif /* STM32_LSE_ENABLED */
#if defined(STM32_LSI_ENABLED)
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
#endif /* STM32_LSI_ENABLED */
#if defined(STM32_HSE_ENABLED)
	case STM32_SRC_HSE:
		*rate = STM32_HSE_FREQ;
		break;
#endif /* STM32_HSE_ENABLED */
#if defined(STM32_HSI48_ENABLED)
	case STM32_SRC_HSI48:
		*rate = STM32_HSI48_FREQ;
		break;
#endif /* STM32_HSI48_ENABLED */
#if defined(STM32_HSI_ENABLED)
	case STM32_SRC_HSI:
		*rate = STM32_HSI_FREQ / STM32_HSI_DIVISOR;
		break;
#endif /* STM32_HSI_ENABLED */
#if defined(STM32_CSI_ENABLED)
	case STM32_SRC_CSI:
		*rate = STM32_CSI_FREQ;
		break;
#endif /* STM32_CSI_ENABLED */

#if defined(STM32_PLL_ENABLED)
	GET_SUBSYS_CASE_PLL_FREQUENCY(1, P, /* empty on purpose */);
	GET_SUBSYS_CASE_PLL_FREQUENCY(1, Q, /* empty on purpose */);
	GET_SUBSYS_CASE_PLL_FREQUENCY(1, S, /* empty on purpose */);
#endif /* STM32_PLL_ENABLED */
#if defined(STM32_PLL2_ENABLED)
	GET_SUBSYS_CASE_PLL_FREQUENCY(2, P, 2);
	GET_SUBSYS_CASE_PLL_FREQUENCY(2, Q, 2);
	GET_SUBSYS_CASE_PLL_FREQUENCY(2, R, 2);
	GET_SUBSYS_CASE_PLL_FREQUENCY(2, S, 2);
	GET_SUBSYS_CASE_PLL_FREQUENCY(2, T, 2);
#endif /* STM32_PLL2_ENABLED */
#if defined(STM32_PLL3_ENABLED)
	GET_SUBSYS_CASE_PLL_FREQUENCY(3, P, 3);
	GET_SUBSYS_CASE_PLL_FREQUENCY(3, Q, 3);
	GET_SUBSYS_CASE_PLL_FREQUENCY(3, R, 3);
	GET_SUBSYS_CASE_PLL_FREQUENCY(3, S, 3);
#endif /* STM32_PLL3_ENABLED */

	/*
	 * Generic code to handle sources described in st,clock-sources.
	 * For each entry in this property, this expands to a case which consumes
	 * the "clocks" property of the clock source node and recursively calls
	 * stm32_clock_control_get_subsys_rate() with it as the `subsys` argument.
	 *
	 * For example:
	 *	st,clock-sources = <&perck>;
	 *	st,clock-source-identifiers = <STM32_SRC_CKPER>;
	 *
	 * results in the FOR_EACH_ENABLED_DEVSRC macro below expanding to:
	 *	case STM32_SRC_CKPER: {
	 *		static const struct stm32_pclken dev_XXX_clk =
	 *			STM32_CLOCK_INFO(0, DT_NODELABEL(perck));
	 *
	 *		res = stm32_clock_control_get_subsys_rate(dev, &dev_XXX_clk, rate);
	 *		break;
	 *	}
	 *
	 * where 'dev_XXX_clk' is initialized from the 'clocks' property of the &perck node.
	 * With the &perck mux configured as follows:
	 *	clocks = <&rcc STM32_SRC_HSE CKPER_SEL(2)>;
	 *
	 * The recursive get_subsys_rate() call would see 'subsys->bus = STM32_SRC_HSE' and
	 * return the HSE frequency, which is also the output frequency of the mux; ditto
	 * if another source such as STM32_SRC_HSI had been used instead.
	 *
	 * Note that this also works if the source of a mux is itself a mux: each recursive call
	 * to get_subsys_rate() walks up one level in the clock tree, and recursive ends at some
	 * point when a "root clock" - for which we have a hardcoded case here - is encountered.
	 * Note that the divider (pclken->div) is applied at each level so those work fine too.
	 * Finally, note that we don't emit any "case" block for disabled mux nodes - any query
	 * for them will fall in the default case and obtain an error instead.
	 *
	 * On STM32H7P, this generic code covers the `perck` and `adckerck` clock muxes.
	 */
	FOR_EACH_ENABLED_DEVSRC(GET_SUBSYS_CASE_DEVSRC_FREQUENCY)
	}

	if (res == 0 && pclken->div) {
		*rate /= (pclken->div + 1);
	}

	return res;
}

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Bus / gated clock */
		if ((stm32_reg_read(RCC_REG(pclken->bus)) & pclken->enr) == pclken->enr) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else {
		/* Domain clock */
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
	.get_status = stm32_clock_control_get_status,
	.configure = stm32_clock_control_configure,
};

static uint32_t get_core_frequency(void)
{
	uint32_t sys_ck;

#if defined(STM32_SYSCLK_SRC_PLL)
	/* sys_ck = pll1_p_ck */
	sys_ck = PLLX_OUT_FREQ(PLLSRC_FREQ,
			     STM32_PLL_M_DIVISOR,
			     STM32_PLL_N_MULTIPLIER,
			     STM32_PLL_FRACN_VALUE,
			     STM32_PLL_P_DIVISOR);
#elif defined(STM32_SYSCLK_SRC_CSI)
	sys_ck = STM32_CSI_FREQ;
#elif defined(STM32_SYSCLK_SRC_HSE)
	sys_ck = STM32_HSE_FREQ;
#elif defined(STM32_SYSCLK_SRC_HSI)
	sys_ck = STM32_HSI_FREQ / STM32_HSI_DIVISOR;
#else
	__ASSERT(0, "No SYSCLK source configured");
	return 0;
#endif

	return sys_ck / STM32_CPRE;
}

static int configure_flash_timings(const uint32_t flash_freq)
{
	const uint32_t vos = LL_PWR_GetRegulatorVoltageScaling();
	uint32_t wait_states;

	if (vos == LL_PWR_REGU_VOLT_SCALE_2 && flash_freq > MHZ(150)) {
		__ASSERT(0, "Unsupported flash frequency for VOS2");
		return -EINVAL;
	}
	if (vos == LL_PWR_REGU_VOLT_SCALE_1 && flash_freq > MHZ(217)) {
		__ASSERT(0, "Unsupported flash frequency for VOS1");
		return -EINVAL;
	}
	if (flash_freq > MHZ(275)) {
		__ASSERT(0, "Unsupported flash frequency");
		return -EINVAL;
	}

	/* See Table 25 in RM0533 Rev 1 */
	if (flash_freq <= MHZ(40)) {
		wait_states = 0;
	} else if (flash_freq <= MHZ(80)) {
		wait_states = 1;
	} else if (flash_freq <= MHZ(120)) {
		wait_states = 2;
	} else if (vos == LL_PWR_REGU_VOLT_SCALE_2) {
		/* Last VOS2 range: up to 150 MHz => 3 WS */
		wait_states = 3;
	} else if (flash_freq <= MHZ(160)) {
		/* VOS1/VOS0 only: up to 160 MHz => 3 WS */
		wait_states = 3;
	} else if (flash_freq <= MHZ(200)) {
		/* VOS1/VOS0 only: up to 200 MHz => 4 WS */
		wait_states = 4;
	} else if (vos == LL_PWR_REGU_VOLT_SCALE_1) {
		/* Last VOS1 range: up to 217 MHz => 5 WS */
		wait_states = 5;
	} else if (flash_freq <= MHZ(240)) {
		/* VOS0 only: up to 240 MHz => 5 WS */
		wait_states = 5;
	} else {
		/* VOS0 only: up to 260 MHz => 6 WS */
		wait_states = 6;
	}

	/*
	 * Programming delay value is the number of wait states divided by 2
	 * (0b00 for 0/1 WS, 0b01 for 2/3 WS, 0b10 for 4/5 WS, 0b11 for 6 WS)
	 */
	const uint32_t prg_delay = wait_states / 2;

	/*
	 * Use a raw register access to program both access latency
	 * and programming delay in a single operation
	 */
	const uint32_t mask = FLASH_ACCR_LATENCY_Msk | FLASH_ACCR_WRHIGHFREQ_Msk;
	const uint32_t accr = (wait_states << FLASH_ACCR_LATENCY_Pos) |
				(prg_delay << FLASH_ACCR_WRHIGHFREQ_Pos);

	stm32_reg_modify_bits(&FLASH->ACCR, mask, accr);
	while (stm32_reg_read_bits(&FLASH->ACCR, mask) != accr) {
		/* Wait until the new configuration is taken into account */
	}

	return 0;
}

static void enable_root_sources(void)
{
	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
			/* Wait for LSI ready */
		}
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		/*
		 * The LSE configuration bits are in register RCC_BDCR,
		 * which the backup domain access protection covers.
		 */
		stm32_backup_domain_enable_access();

		/* Configure driving capability */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_BDCR_LSEDRV_Pos);

		if (IS_ENABLED(STM32_LSE_BYPASS)) {
			LL_RCC_LSE_EnableBypass();
		} else {
			LL_RCC_LSE_DisableBypass();
		}

		LL_RCC_LSE_Enable();
		while (!LL_RCC_LSE_IsReady()) {
			/* Wait for LSE ready */
		}

		stm32_backup_domain_disable_access();
	}

	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		if (IS_ENABLED(STM32_HSE_BYPASS)) {
			LL_RCC_HSE_EnableBypass();
		} else {
			LL_RCC_HSE_DisableBypass();
		}

		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
			/* Wait for HSE ready */
		}

#ifdef STM32_HSE_CSS
		/* Enable HSE clock security system */
		z_arm_nmi_set_handler(hal_rcc_nmi_irqhandler_wrapper);
		LL_RCC_HSE_EnableCSS();
#endif /* STM32_HSE_CSS */
	}

	if (IS_ENABLED(STM32_HSI_ENABLED)) {
		/* Configure HSI divider */
		LL_RCC_HSI_SetDivider(div2ll(HSI, STM32_HSI_DIVISOR));

		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
		}
	}

	if (IS_ENABLED(STM32_CSI_ENABLED)) {
		LL_RCC_CSI_Enable();
		while (LL_RCC_CSI_IsReady() != 1) {
			/* Wait for CSI ready */
		}
	}

	if (IS_ENABLED(STM32_HSI48_ENABLED)) {
		LL_RCC_HSI48_Enable();
		while (LL_RCC_HSI48_IsReady() != 1) {
			/* Wait for HSI48 ready */
		}
	}
}

static void configure_sysclk_prescalers(void)
{
	LL_RCC_SetSysPrescaler(presc2ll(SYSCLK, STM32_CPRE));
	LL_RCC_SetBMPrescaler(presc2ll(BMCLK, STM32_BMPRE));
	LL_RCC_SetAPB1Prescaler(presc2ll(APB1, STM32_PPRE1));
	LL_RCC_SetAPB2Prescaler(presc2ll(APB2, STM32_PPRE2));
	LL_RCC_SetAPB4Prescaler(presc2ll(APB4, STM32_PPRE4));
}

static void configure_sysclk_mux(void)
{
	uint32_t sysclk_src, sysclk_status;

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		sysclk_src = LL_RCC_SYS_CLKSOURCE_PLL1;
		sysclk_status = LL_RCC_SYS_CLKSOURCE_STATUS_PLL1;
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSE)) {
		sysclk_src = LL_RCC_SYS_CLKSOURCE_HSE;
		sysclk_status = LL_RCC_SYS_CLKSOURCE_STATUS_HSE;
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSI)) {
		sysclk_src = LL_RCC_SYS_CLKSOURCE_HSI;
		sysclk_status = LL_RCC_SYS_CLKSOURCE_STATUS_HSI;
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_CSI)) {
		sysclk_src = LL_RCC_SYS_CLKSOURCE_CSI;
		sysclk_status = LL_RCC_SYS_CLKSOURCE_STATUS_CSI;
	} else {
		__ASSERT(0, "No system clock source defined");
		return;
	}

	LL_RCC_SetSysClkSource(sysclk_src);
	while (LL_RCC_GetSysClkSource() != sysclk_status) {
		/* Wait for system clock source to be switched */
	}
}

/*
 * Wait for FRACN latch delay (5 µs) by polling SysTick.
 * The SysTick driver hasn't initialized yet - control timer directly.
 * We will make sure to turn it off when we're done so driver is fine.
 */
__maybe_unused
static void wait_for_fracn_latch_delay(void)
{
	/* Assume worst case scenario: fCPU = 64 MHz (HSI, no divider).
	 * If we're running at a lower frequency, this routine merely
	 * waits more than 5 µs... which is safe.
	 *
	 * Note that 320 cycles = 5 µs / ((1 / 64 MHz) seconds-per-cycle)
	 */
	SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk
			   | SysTick_CTRL_TICKINT_Msk
			   | SysTick_CTRL_CLKSOURCE_Msk);
	SysTick->LOAD = 320;
	SysTick->VAL = 0; /* force counter reload */
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {
		/* Wait until counter transitions from 1 to 0 */
	}

	/* Turn off SysTick */
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/*
 * Returns a negative error code on failure, 0 on success,
 * or 1 on success if switch to HSI was performed.
 */
static int set_up_plls(void)
{
	int res;

	if (!IS_ENABLED(STM32_PLL_ENABLED)
	 && !IS_ENABLED(STM32_PLL2_ENABLED)
	 && !IS_ENABLED(STM32_PLL3_ENABLED)) {
		/* No PLL enabled - switch off pll_ck mux to save power */
		LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_NO_CLK);
		return 0;
	}

	if (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
		/* If PLL1 isn't used as SYSCLK, it can be safely disabled. */
		res = 0;
	} else {
		/*
		 * If PLL1 is used as SYSCLK, we must switch to another source
		 * temporarily because it is not possible to disable PLL1, which
		 * is needed to reconfigure it, while it is used as SYSCLK source.
		 * To ensure this code always works, the temporary SYSCLK source
		 * should be an on-chip source; HSI is chosen instead of CSI since
		 * it runs much faster (64 MHz vs. 4 MHz).
		 *
		 * Return special value 1 to indicate SYSCLK was switched to HSI.
		 * The caller is responsible for reconfiguring the SYSCLK mux if so,
		 * as well as disabling the HSI if no longer necessary.
		 */
		res = 1;

		if (!IS_ENABLED(STM32_HSI_ENABLED)) {
			/* Enable HSI temporarily; caller must disable later */
			LL_RCC_HSI_Enable();
			while (!LL_RCC_HSI_IsReady()) {
				/* Wait for HSI ready */
			}
		}

		/*
		 * Switch system clock to HSI.
		 * NOTE: we assume that fPLL was higher than fHSI
		 * and thus no overshoot can occur.
		 */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
			/* Wait for HSI to be system clock source */
		}
	}

	/*
	 * Disable all PLLs per RM0533 "PLL disabling procedure":
	 * - set PLLx{P,Q,R,S,T}EN = 0 to disable post-dividers
	 * - set PLLxON = 0 to disable the PLL itself
	 * - wait for PLLxRDY = 0
	 */
	LL_RCC_PLL1_DisableOutput(
		LL_RCC_PLL1_OUTPUT_P |
		LL_RCC_PLL1_OUTPUT_Q |
		LL_RCC_PLL1_OUTPUT_S);
	LL_RCC_PLL1_Disable();

	LL_RCC_PLL2_DisableOutput(
		LL_RCC_PLL2_OUTPUT_P |
		LL_RCC_PLL2_OUTPUT_Q |
		LL_RCC_PLL2_OUTPUT_R |
		LL_RCC_PLL2_OUTPUT_S |
		LL_RCC_PLL2_OUTPUT_T);
	LL_RCC_PLL2_Disable();

	LL_RCC_PLL3_DisableOutput(
		LL_RCC_PLL3_OUTPUT_P |
		LL_RCC_PLL3_OUTPUT_Q |
		LL_RCC_PLL3_OUTPUT_R |
		LL_RCC_PLL3_OUTPUT_S);
	LL_RCC_PLL3_Disable();

	while (LL_RCC_PLL1_IsReady()) {
		/* Wait for PLL1 to be disabled */
	}
	while (LL_RCC_PLL2_IsReady()) {
		/* Wait for PLL2 to be disabled */
	}
	while (LL_RCC_PLL3_IsReady()) {
		/* Wait for PLL3 to be disabled */
	}

	/* Configure common source */
	LL_RCC_PLL_SetSource(STM32_PLL_SOURCE);
	while (LL_RCC_PLL_GetSource() != STM32_PLL_SOURCE) {
		/* Wait for PLL source to be set */
	}

	/*
	 * Configure individual PLLs according to DTS.
	 * The code is implemented by the two following macros
	 * in a generic manner, then instantiated for each PLL.
	 */

/*
 * The COND_CODE_1() pasta is a workaround for not being able
 * to pass `numsfx` to this macro via FOR_EACH_FIXED_ARG().
 * It would be unnecessary if the PLL1 macros used "PLL1"
 * as suffix instead of "PLL"...
 */
#define CONFIGURE_PLL_OUTPUT(output, pllnum)	 COND_CODE_1(IS_EQ(pllnum, 1),		\
	(COND_CODE_1(IS_ENABLED(CONCAT(STM32_PLL_, output, _ENABLED)),			\
		(LL_RCC_PLL##pllnum##_Set##output(STM32_PLL_##output##_DIVISOR);),	\
		(LL_RCC_PLL##pllnum##_Set##output(1);))),				\
	(COND_CODE_1(IS_ENABLED(CONCAT(STM32_PLL, pllnum, _, output, _ENABLED)),	\
		(LL_RCC_PLL##pllnum##_Set##output(					\
			STM32_PLL##pllnum##_##output##_DIVISOR);),			\
		(LL_RCC_PLL##pllnum##_Set##output(1);))))

#define ENABLE_PLL_OUTPUT(output, pllnum) COND_CODE_1(IS_EQ(pllnum, 1),			\
	(COND_CODE_1(IS_ENABLED(CONCAT(STM32_PLL_, output, _ENABLED)),			\
		(LL_RCC_PLL##pllnum##_EnableOutput##output();), ())),			\
	(COND_CODE_1(IS_ENABLED(CONCAT(STM32_PLL, pllnum, _, output, _ENABLED)),	\
		(LL_RCC_PLL##pllnum##_EnableOutput##output();), ())))

/*
 * Configure a PLL according to RM0533 "PLL initialization procedure",
 * assuming PLLs are disabled and source has already been selected.
 *
 * Variable argument = list of outputs to enable
 */
#define CONFIGURE_PLL(num, numsfx, ...)							\
	do {										\
		const uint32_t _plln_input =						\
			PLLSRC_FREQ / STM32_PLL##numsfx##_M_DIVISOR;			\
		const uint32_t _plln_vcoout = PLLX_VCO_FREQ(PLLSRC_FREQ,		\
						 STM32_PLL##numsfx##_M_DIVISOR,		\
						 STM32_PLL##numsfx##_N_MULTIPLIER,	\
						 STM32_PLL##numsfx##_FRACN_VALUE);	\
		const bool vcol_ok = IN_RANGE(_plln_vcoout, MHZ(150), MHZ(420))		\
			&& IN_RANGE(_plln_input, MHZ(1), MHZ(2));			\
		const bool vcoh_ok = IN_RANGE(_plln_vcoout, MHZ(384), MHZ(1672))	\
			&& IN_RANGE(_plln_input, MHZ(2), MHZ(16));			\
		uint32_t _inrge, _vcosel;						\
											\
		if (vcol_ok) {								\
			/* Use VCOL if possible (lower power consumption) */		\
			_vcosel = LL_RCC_PLL##num##VCORANGE_150_420;			\
			_inrge = LL_RCC_PLL##num##INPUTRANGE_1_2;			\
		} else if (vcoh_ok) {							\
			_vcosel = LL_RCC_PLL##num##VCORANGE_384_1672;			\
			if (IN_RANGE(_plln_input, MHZ(2), MHZ(4))) {			\
				_inrge = LL_RCC_PLL##num##INPUTRANGE_2_4;		\
			} else if (IN_RANGE(_plln_input, MHZ(4), MHZ(8))) {		\
				_inrge = LL_RCC_PLL##num##INPUTRANGE_4_8;		\
			} else if (IN_RANGE(_plln_input, MHZ(8), MHZ(16))) {		\
				_inrge = LL_RCC_PLL##num##INPUTRANGE_8_16;		\
			} else {							\
				__ASSERT_NO_MSG(0); /* unreachable */			\
				return -ERANGE;						\
			}								\
		} else {								\
			__ASSERT(0, "Invalid configuration for PLL" #num ": "		\
				"input frequency=%u, output frequency=%u",		\
				_plln_input, _plln_vcoout);				\
			return -ERANGE;							\
		}									\
		/* Initialize pre-divider */						\
		LL_RCC_PLL##num##_SetM(STM32_PLL##numsfx##_M_DIVISOR);			\
											\
		/* Configure integer/fractional mode */					\
		/* NOTE: STM32_PLL_FRACN_VALUE is 0 if fractional mode is not used */	\
		LL_RCC_PLL##num##_DisableFRACN();					\
		wait_for_fracn_latch_delay();						\
		LL_RCC_PLL##num##_SetFRACN(STM32_PLL##numsfx##_FRACN_VALUE);		\
		LL_RCC_PLL##num##_EnableFRACN();					\
		LL_RCC_PLL##num##_SetFRACN(STM32_PLL##numsfx##_FRACN_VALUE);		\
		wait_for_fracn_latch_delay();						\
											\
		/* Program PLLxRGE, PLLxVCOSEL and post-dividers */			\
		LL_RCC_PLL##num##_SetInputRange(_inrge);				\
		LL_RCC_PLL##num##_SetVCORange(_vcosel);					\
		LL_RCC_PLL##num##_SetN(STM32_PLL##numsfx##_N_MULTIPLIER);		\
		FOR_EACH_FIXED_ARG(CONFIGURE_PLL_OUTPUT, (), num, __VA_ARGS__)	\
											\
		/* Enable the PLL */							\
		LL_RCC_PLL##num##_Enable();						\
		while (!LL_RCC_PLL##num##_IsReady()) {					\
			/* Wait for PLL ready */					\
		}									\
											\
		/* Enable the wanted outputs */						\
		FOR_EACH_FIXED_ARG(ENABLE_PLL_OUTPUT, (), num, __VA_ARGS__)		\
	} while (0)

	/*
	 * NOTE: instantiation must be gated behind #ifdef
	 * rather than if(IS_ENABLED(...)) because the macros
	 * expands to garbage if values for the required
	 * properties are not provided, but `required: true`
	 * is only asserted for enabled nodes.
	 */
#if defined(STM32_PLL_ENABLED)
	CONFIGURE_PLL(1, /* empty on purpose */, P, Q, S);
#endif /* STM32_PLL_ENABLED */
#if defined(STM32_PLL2_ENABLED)
	CONFIGURE_PLL(2, 2, P, Q, R, S, T);
#endif /* STM32_PLL2_ENABLED */
#if defined(STM32_PLL3_ENABLED)
	CONFIGURE_PLL(3, 3, P, Q, R, S);
#endif /* STM32_PLL3_ENABLED */

	return res;
}

int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int res;

	/* Set worst-case flash latency during initialization */
	LL_FLASH_SetLatency(FLASH, LL_FLASH_LATENCY_7WS);

	/* Enable root clock sources (i.e., generators) */
	enable_root_sources();

	/* Configure and enable PLLs */
	res = set_up_plls();
	if (res < 0) {
		__ASSERT(0, "PLLs setup failed");
		return res;
	}

	/* Configure system clock tree */
	configure_sysclk_prescalers();
	configure_sysclk_mux();

	if (res == 1 && !IS_ENABLED(STM32_HSI_ENABLED)) {
		/*
		 * HSI was temporarily enabled for PLL1 reconfiguration.
		 * Disable it now that we switched SYSCLK if it is not
		 * meant to be enabled.
		 */
		LL_RCC_HSI_Disable();
	}

	/* SystemCoreClock = rcc_cpu_ck = sys_cpu_ck */
	SystemCoreClock = get_core_frequency();

	/*
	 * Reconfigure flash timings based on final system frequency.
	 * Note that the flash is connected to the AXI bus matrix
	 * so it runs at sys_bus_ck = sys_cpu_ck / BMPRE.
	 */
	res = configure_flash_timings(SystemCoreClock / STM32_BMPRE);
	if (res < 0) {
		__ASSERT(0, "Flash timings configuration failed");
		return res;
	}

	return 0;
}

#ifdef STM32_HSE_CSS
void __weak stm32_hse_css_callback(void) {}

/* Called by the HAL in response to HSE CSS interrupt */
hal_status_t HAL_RCC_HSE_CSSCallback(void)
{
	stm32_hse_css_callback();
	return HAL_OK;
}
#endif /* STM32_HSE_CSS */

/**
 * @brief Reset & Clock Controller device
 * Note that priority is intentionally set to 1,
 * so that RCC init runs just after SoC init.
 */
DEVICE_DT_INST_DEFINE(0,
		      &stm32_clock_control_init,
		      NULL, NULL, NULL,
		      PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &stm32_clock_control_api);
