/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019-23 Linaro Limited
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief STM32 External Interrupt/Event Controller (EXTI) Driver
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include "stm32_hsem.h"
#include "intc_exti_stm32_priv.h"

LOG_MODULE_REGISTER(exti_stm32, CONFIG_INTC_LOG_LEVEL);

#define IS_VALID_EXTI_LINE_NUM(line_num) ((line_num) < STM32_EXTI_TOTAL_LINES_NUM)

/*
 * The boilerplate for COND_CODE_x is needed because the values are not 0/1
 */
#if STM32_EXTI_TOTAL_LINES_NUM > 32
#define HAS_LINES_32_63	1
#if STM32_EXTI_TOTAL_LINES_NUM > 64
#define HAS_LINES_64_95	1
#endif /* STM32_EXTI_TOTAL_LINES_NUM > 64 */
#endif /* STM32_EXTI_TOTAL_LINES_NUM > 32 */

#define EXTI_FN_HANDLER(_fn, line_num, line)					\
	if (line_num < 32U) {							\
		_fn(0_31, line);						\
IF_ENABLED(HAS_LINES_32_63, (							\
	} else if (line_num < 64U) {						\
		_fn(32_63, line);						\
))										\
IF_ENABLED(HAS_LINES_64_95, (							\
	} else if (line_num < 96U) {						\
		_fn(64_95, line);						\
))										\
	} else {								\
		LOG_ERR("Invalid line number %u", line_num);			\
		__ASSERT_NO_MSG(0);						\
	}

#define EXTI_FN_RET_HANDLER(_fn, ret, line_num, line)				\
	if (line_num < 32U) {							\
		*ret = _fn(0_31, line);						\
IF_ENABLED(HAS_LINES_32_63, (							\
	} else if (line_num < 64U) {						\
		*ret = _fn(32_63, line);					\
))										\
IF_ENABLED(HAS_LINES_64_95, (							\
	} else if (line_num < 96U) {						\
		*ret = _fn(64_95, line);					\
))										\
	} else {								\
		LOG_ERR("Invalid line number %u", line_num);			\
		__ASSERT_NO_MSG(0);						\
	}


bool stm32_exti_is_pending(uint32_t line_num)
{
	bool ret = false;
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_ERR("Invalid line number %u", line_num);
		return false;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	/*
	 * Note: we can't use EXTI_FN_HANDLER here because we care
	 * about the return value of EXTI_IS_ACTIVE_FLAG.
	 */
	EXTI_FN_RET_HANDLER(EXTI_IS_ACTIVE_FLAG, &ret, line_num, line);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}

int stm32_exti_clear_pending(uint32_t line_num)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_ERR("Invalid line number %u", line_num);
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	EXTI_FN_HANDLER(EXTI_CLEAR_FLAG, line_num, line);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}

int stm32_exti_sw_interrupt(uint32_t line_num)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_ERR("Invalid line number %u", line_num);
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	EXTI_FN_HANDLER(EXTI_GENERATE_SWI, line_num, line);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}

/** Enables the peripheral clock required to access EXTI registers */
static int stm32_exti_enable_clocks(void)
{
	/* Initialize to 0 for series where there is nothing to do. */
	int ret = 0;

#if DT_NODE_HAS_PROP(EXTI_NODE, clocks)
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	const struct stm32_pclken pclken = STM32_CLOCK_INFO(0, EXTI_NODE);

	ret = clock_control_on(clk, (clock_control_subsys_t) &pclken);
#endif
	return ret;
}

/**
 * @brief Initializes the EXTI interrupt controller driver
 */
static int stm32_exti_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return stm32_exti_enable_clocks();
}

/**
 * @brief Enable EXTI interrupts.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_enable_it(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_ENABLE_IT, line_num, line);
}

/**
 * @brief Disable EXTI interrupts.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_it(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_DISABLE_IT, line_num, line);
}

/**
 * @brief Enables rising trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_enable_rising_trig(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_ENABLE_RISING_TRIG, line_num, line);
}

/**
 * @brief Disables rising trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_rising_trig(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_DISABLE_RISING_TRIG, line_num, line);
}

/**
 * @brief Enables falling trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void  stm32_exti_enable_falling_trig(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_ENABLE_FALLING_TRIG, line_num, line);
}

/**
 * @brief Disables falling trigger for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_falling_trig(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_DISABLE_FALLING_TRIG, line_num, line);
}

/**
 * @brief Selects EXTI trigger mode
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 * @param mode	EXTI mode
 */
static void stm32_exti_select_line_trigger(uint32_t line_num, uint32_t line,
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
		LOG_ERR("Unsupported EXTI trigger 0x%X", trg);
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
	EXTI_FN_HANDLER(EXTI_ENABLE_EVENT, line_num, line);
}

/**
 * @brief Disable EXTI interrupts.
 *
 * @param line_num	EXTI line number
 * @param line	LL EXTI line
 */
static void stm32_exti_disable_event(uint32_t line_num, uint32_t line)
{
	EXTI_FN_HANDLER(EXTI_DISABLE_EVENT, line_num, line);
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
		stm32_exti_disable_it(line_num, line);
		break;
	case STM32_EXTI_MODE_IT:
		stm32_exti_disable_event(line_num, line);
		stm32_exti_enable_it(line_num, line);
		break;
	case STM32_EXTI_MODE_EVENT:
		stm32_exti_disable_it(line_num, line);
		stm32_exti_enable_event(line_num, line);
		break;
	case STM32_EXTI_MODE_BOTH:
		stm32_exti_enable_it(line_num, line);
		stm32_exti_enable_event(line_num, line);
		break;
	default:
		LOG_ERR("Unsupported EXTI mode %u", mode);
		break;
	}
}

int stm32_exti_enable(uint32_t line_num, stm32_exti_trigger_type trigger,
		      stm32_exti_mode mode)
{
	const uint32_t line = exti_linenum_to_ll_exti_line(line_num);

	if (!IS_VALID_EXTI_LINE_NUM(line_num)) {
		LOG_ERR("Invalid line number %u", line_num);
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
		LOG_ERR("Invalid line number %u", line_num);
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
