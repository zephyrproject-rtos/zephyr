/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019-23 Linaro Limited
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include "stm32_hsem.h"
#include "intc_exti_stm32_priv.h"

LOG_MODULE_REGISTER(exti_stm32, CONFIG_INTC_LOG_LEVEL);

#define IS_VALID_EXTI_LINE_NUM(line_num) ((line_num) < STM32_EXTI_TOTAL_LINES_NUM)

bool stm32_exti_is_pending(uint32_t line_num)
{
	bool ret = false;
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_DBG("No line number %d", line_num);
		return false;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (line_num < 32U) {
		ret = EXTI_IS_ACTIVE_FLAG(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		ret = EXTI_IS_ACTIVE_FLAG(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		ret = EXTI_IS_ACTIVE_FLAG(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}

int stm32_exti_clear_pending(uint32_t line_num)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_DBG("No line number %d", line_num);
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (line_num < 32U) {
		EXTI_CLEAR_FLAG(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_CLEAR_FLAG(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_CLEAR_FLAG(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}

int stm32_exti_sw_interrupt(uint32_t line_num)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_DBG("No line number %d", line_num);
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (line_num < 32U) {
		EXTI_GENERATE_SWI(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_GENERATE_SWI(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_GENERATE_SWI(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}

/** Enables the peripheral clock required to access EXTI registers */
static int stm32_exti_enable_registers(void)
{
	/* Initialize to 0 for series where there is nothing to do. */
	int ret = 0;
#if defined(CONFIG_SOC_SERIES_STM32F2X) ||     \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32H7RSX) || \
	defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	struct stm32_pclken pclken = {
#if defined(CONFIG_SOC_SERIES_STM32H7X)
		.bus = STM32_CLOCK_BUS_APB4,
		.enr = LL_APB4_GRP1_PERIPH_SYSCFG
#elif defined(CONFIG_SOC_SERIES_STM32H7RSX)
		.bus = STM32_CLOCK_BUS_APB4,
		.enr = LL_APB4_GRP1_PERIPH_SBS
#else
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
#endif /* CONFIG_SOC_SERIES_STM32H7X */
	};

	ret = clock_control_on(clk, (clock_control_subsys_t) &pclken);
#endif
	return ret;
}

/**
 * @brief Initializes the EXTI GPIO interrupt controller driver
 */
static int stm32_exti_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return stm32_exti_enable_registers();
}

/**
 * @brief Enable EXTI interrupts.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_enable_isr(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_ENABLE_IT(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_ENABLE_IT(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_ENABLE_IT(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Disable EXTI interrupts.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_isr(uint32_t line_num, uint32_t line)
{
	/* Disable requested line interrupt */
	if (line_num < 32U) {
		EXTI_DISABLE_IT(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_DISABLE_IT(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_DISABLE_IT(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Enables rising trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_enable_rising_trig(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_ENABLE_RISING_TRIG(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_ENABLE_RISING_TRIG(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_ENABLE_RISING_TRIG(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Disables rising trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_rising_trig(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_DISABLE_RISING_TRIG(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_DISABLE_RISING_TRIG(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_DISABLE_RISING_TRIG(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Enables falling trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void  stm32_exti_enable_falling_trig(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_ENABLE_FALLING_TRIG(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_ENABLE_FALLING_TRIG(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_ENABLE_FALLING_TRIG(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Disables falling trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_falling_trig(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_DISABLE_FALLING_TRIG(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_DISABLE_FALLING_TRIG(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_DISABLE_FALLING_TRIG(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Selects EXTI trigger mode
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 * @param mode	EXTI mode
 */
static inline void stm32_exti_select_line_trigger(uint32_t line_num, uint32_t line,
												  uint32_t trg)
{
	switch (trg) {
	case STM32_EXTI_TRIG_NONE:
		stm32_exti_disable_rising_trig(line_num, line);
		stm32_exti_disable_falling_trig(line_num, line);
		break;
	case STM32_EXTI_TRIG_RISING:
		stm32_exti_enable_rising_trig(line_num, line);
		stm32_exti_disable_falling_trig(line_num, line);
		break;
	case STM32_EXTI_TRIG_FALLING:
		stm32_exti_enable_falling_trig(line_num, line);
		stm32_exti_disable_rising_trig(line_num, line);
		break;
	case STM32_EXTI_TRIG_BOTH:
		stm32_exti_enable_rising_trig(line_num, line);
		stm32_exti_enable_falling_trig(line_num, line);
		break;
	default:
		LOG_ERR("Unsupported EXTI trigger %d", trg);
		break;
	}
}

/**
 * @brief Enable EXTI event.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_enable_event(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_ENABLE_EVENT(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_ENABLE_EVENT(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_ENABLE_EVENT(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Disable EXTI interrupts.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_event(uint32_t line_num, uint32_t line)
{
	if (line_num < 32U) {
		EXTI_DISABLE_EVENT(0_31, line);
#if STM32_EXTI_TOTAL_LINES_NUM_64
	} else if (line_num < 64U) {
		EXTI_DISABLE_EVENT(32_63, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_64 */
#if STM32_EXTI_TOTAL_LINES_NUM_96
	} else if (line_num < 96U) {
		EXTI_DISABLE_EVENT(64_95, line);
#endif /* STM32_EXTI_TOTAL_LINES_NUM_96 */
	} else {
		LOG_DBG("Invalid EXTI line number: %d", line_num);
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Enables external interrupt/event for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 * @param mode	EXTI mode
 */
static void stm32_exti_set_mode(uint32_t line_num, uint32_t line,
								stm32_exti_mode mode)
{
	switch (mode) {
	case STM32_EXTI_MODE_NONE:
		stm32_exti_disable_event(line_num, line);
		stm32_exti_disable_isr(line_num, line);
		break;
	case STM32_EXTI_MODE_IT:
		stm32_exti_disable_event(line_num, line);
		stm32_exti_enable_isr(line_num, line);
		break;
	case STM32_EXTI_MODE_EVENT:
		stm32_exti_disable_isr(line_num, line);
		stm32_exti_enable_event(line_num, line);
		break;
	case STM32_EXTI_MODE_BOTH:
		stm32_exti_enable_isr(line_num, line);
		stm32_exti_enable_event(line_num, line);
		break;
	default:
		LOG_ERR("Unsupported EXTI mode %d", mode);
		break;
	}
}

int stm32_exti_enable(uint32_t line_num, stm32_exti_trigger_type trigger,
					  stm32_exti_mode mode)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_ERR("No line number %d", line_num);
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_exti_select_line_trigger(line_num, line, trigger);
	stm32_exti_set_mode(line_num, line, mode);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}

int stm32_exti_disable(uint32_t line_num)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_ERR("No line number %d", line_num);
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_exti_set_mode(line_num, line, STM32_EXTI_MODE_NONE);
	stm32_exti_select_line_trigger(line_num, line, STM32_EXTI_TRIG_NONE);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}

DEVICE_DT_DEFINE(EXTI_NODE, &stm32_exti_init,
	NULL, NULL, NULL,
	PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
	NULL);
