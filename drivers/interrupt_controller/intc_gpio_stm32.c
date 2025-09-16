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
 * @brief Driver for STM32 External interrupt/event controller
 */

#include <zephyr/device.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h> /* For STM32F1 series */
#include <stm32_ll_exti.h>
#include <stm32_ll_system.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h> /* For STM32L0 series */
#include <zephyr/drivers/interrupt_controller/gpio_intc_stm32.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>

#include "stm32_hsem.h"
#include "intc_exti_stm32_priv.h"

/** @brief EXTI lines range mapped to a single interrupt line */
struct stm32_exti_range {
	/* Start of the range */
	uint8_t start;
	/* Range length */
	uint8_t len;
};

#define EXTI_NUM_LINES_TOTAL	DT_PROP(EXTI_NODE, num_lines)
#define NUM_EXTI_LINES		DT_PROP(EXTI_NODE, num_gpio_lines)

BUILD_ASSERT(EXTI_NUM_LINES_TOTAL >= NUM_EXTI_LINES,
	"The total number of EXTI lines must be greater or equal than the number of GPIO lines");

static IRQn_Type exti_irq_table[NUM_EXTI_LINES] = {[0 ... NUM_EXTI_LINES - 1] = 0xFF};

/* User callback wrapper */
struct __exti_cb {
	stm32_gpio_irq_cb_t cb;
	void *data;
};

/* EXTI driver data */
struct stm32_intc_gpio_data {
	/* per-line callbacks */
	struct __exti_cb cb[NUM_EXTI_LINES];
};

static struct stm32_intc_gpio_data intc_gpio_data;

/**
 * @returns the LL_<PPP>_EXTI_LINE_xxx define that corresponds to specified @p linenum
 * This value can be used with the LL EXTI source configuration functions.
 */
static inline uint32_t stm32_exti_linenum_to_src_cfg_line(gpio_pin_t linenum)
{
#if defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X)
	return ((linenum % 4 * 4) << 16) | (linenum / 4);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) || \
	defined(CONFIG_SOC_SERIES_STM32MP2X)
	return ((linenum & 0x3) << (16 + 3)) | (linenum >> 2);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7rs_exti)
	/* Gives the LL_SBS_EXTI_LINEn corresponding to the line number */
	return (((linenum % 4 * 4) << LL_SBS_REGISTER_PINPOS_SHFT) | (linenum / 4));
#else
	return (0xF << ((linenum % 4 * 4) + 16)) | (linenum / 4);
#endif
}

/**
 * @returns EXTI line number for LL_EXTI_LINE_n define
 */
static inline gpio_pin_t ll_exti_line_to_linenum(stm32_gpio_irq_line_t line)
{
	return LOG2(line);
}

/**
 * @brief EXTI ISR handler
 *
 * Check EXTI lines in exti_range for pending interrupts
 *
 * @param exti_range Pointer to a exti_range structure
 */
static void stm32_intc_gpio_isr(const void *exti_range)
{
	struct stm32_intc_gpio_data *data = &intc_gpio_data;
	const struct stm32_exti_range *range = exti_range;
	stm32_gpio_irq_line_t line;
	uint32_t line_num;

	/* see which bits are set */
	for (uint8_t i = 0; i < range->len; i++) {
		line_num = range->start + i;

		/* check if interrupt is pending */
		if (stm32_exti_is_pending(line_num)) {
			/* clear pending interrupt */
			stm32_exti_clear_pending(line_num);

			/* run callback only if one is registered */
			if (!data->cb[line_num].cb) {
				continue;
			}

			/* `line` can be passed as-is because LL_EXTI_LINE_n is (1 << n) */
			line = exti_linenum_to_ll_exti_line(line_num);
			data->cb[line_num].cb(line, data->cb[line_num].data);
		}
	}
}

static void stm32_fill_irq_table(int8_t start, int8_t len, int32_t irqn)
{
	for (int i = 0; i < len; i++) {
		exti_irq_table[start + i] = irqn;
	}
}

/* This macro:
 * - populates line_range_x from line_range dt property
 * - fills exti_irq_table through stm32_fill_irq_table()
 * - calls IRQ_CONNECT for each interrupt and matching line_range
 */
#define STM32_EXTI_INIT_LINE_RANGE(node_id, interrupts, idx)			\
	static const struct stm32_exti_range line_range_##idx = {		\
		DT_PROP_BY_IDX(node_id, line_ranges, UTIL_X2(idx)),		\
		DT_PROP_BY_IDX(node_id, line_ranges, UTIL_INC(UTIL_X2(idx)))	\
	};									\
	stm32_fill_irq_table(line_range_##idx.start,				\
			     line_range_##idx.len,				\
			     DT_IRQ_BY_IDX(node_id, idx, irq));			\
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),				\
		DT_IRQ_BY_IDX(node_id, idx, priority),				\
		stm32_intc_gpio_isr, &line_range_##idx, 0);

/**
 * @brief Initializes the EXTI GPIO interrupt controller driver
 */
static int stm32_exti_gpio_intc_init(void)
{
	DT_FOREACH_PROP_ELEM(EXTI_NODE,
			     interrupt_names,
			     STM32_EXTI_INIT_LINE_RANGE);

	return 0;
}

SYS_INIT(stm32_exti_gpio_intc_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);

/**
 * @brief EXTI GPIO interrupt controller API implementation
 */

/**
 * @internal
 * STM32 EXTI driver:
 * The type @ref stm32_gpio_irq_line_t is used to hold the LL_EXTI_LINE_xxx
 * defines of the LL EXTI API that corresponds to the provided pin.
 *
 * The port is not part of these definitions because port configuration
 * is done via different APIs, which use the LL_<PPP>_EXTI_LINE_xxx defines
 * returned by @ref stm32_exti_linenum_to_src_cfg_line instead.
 * @endinternal
 */
stm32_gpio_irq_line_t stm32_gpio_intc_get_pin_irq_line(uint32_t port, gpio_pin_t pin)
{
	ARG_UNUSED(port);
	return exti_linenum_to_ll_exti_line(pin);
}

void stm32_gpio_intc_enable_line(stm32_gpio_irq_line_t line)
{
	unsigned int irqnum;
	uint32_t line_num = ll_exti_line_to_linenum(line);

	__ASSERT_NO_MSG(line_num < NUM_EXTI_LINES);

	/* Get matching exti irq provided line thanks to irq_table */
	irqnum = exti_irq_table[line_num];
	__ASSERT_NO_MSG(irqnum != 0xFF);

	/* Enable requested line interrupt */
	EXTI_ENABLE_IT(0_31, line);

	/* Enable exti irq interrupt */
	irq_enable(irqnum);
}

void stm32_gpio_intc_disable_line(stm32_gpio_irq_line_t line)
{
	EXTI_DISABLE_IT(0_31, line);
}

void stm32_gpio_intc_select_line_trigger(stm32_gpio_irq_line_t line, uint32_t trg)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	switch (trg) {
#if defined(CONFIG_SOC_SERIES_STM32MP2X)
	case STM32_GPIO_IRQ_TRIG_NONE:
		LL_EXTI_DisableRisingTrig_0_31(EXTI2, line);
		LL_EXTI_DisableFallingTrig_0_31(EXTI2, line);
		break;
	case STM32_GPIO_IRQ_TRIG_RISING:
		LL_EXTI_EnableRisingTrig_0_31(EXTI2, line);
		LL_EXTI_DisableFallingTrig_0_31(EXTI2, line);
		break;
	case STM32_GPIO_IRQ_TRIG_FALLING:
		LL_EXTI_EnableFallingTrig_0_31(EXTI2, line);
		LL_EXTI_DisableRisingTrig_0_31(EXTI2, line);
		break;
	case STM32_GPIO_IRQ_TRIG_BOTH:
		LL_EXTI_EnableRisingTrig_0_31(EXTI2, line);
		LL_EXTI_EnableFallingTrig_0_31(EXTI2, line);
		break;
#else /* CONFIG_SOC_SERIES_STM32MP2X */
	case STM32_GPIO_IRQ_TRIG_NONE:
		EXTI_DISABLE_RISING_TRIG(0_31, line);
		EXTI_DISABLE_FALLING_TRIG(0_31, line);
		break;
	case STM32_GPIO_IRQ_TRIG_RISING:
		EXTI_ENABLE_RISING_TRIG(0_31, line);
		EXTI_DISABLE_FALLING_TRIG(0_31, line);
		break;
	case STM32_GPIO_IRQ_TRIG_FALLING:
		EXTI_ENABLE_FALLING_TRIG(0_31, line);
		EXTI_DISABLE_RISING_TRIG(0_31, line);
		break;
	case STM32_GPIO_IRQ_TRIG_BOTH:
		EXTI_ENABLE_RISING_TRIG(0_31, line);
		EXTI_ENABLE_FALLING_TRIG(0_31, line);
		break;
#endif /* CONFIG_SOC_SERIES_STM32MP2X */
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

int stm32_gpio_intc_set_irq_callback(stm32_gpio_irq_line_t line, stm32_gpio_irq_cb_t cb, void *user)
{
	struct stm32_intc_gpio_data *data = &intc_gpio_data;
	uint32_t line_num = ll_exti_line_to_linenum(line);

	if ((data->cb[line_num].cb == cb) && (data->cb[line_num].data == user)) {
		return 0;
	}

	/* if callback already exists/is running, return busy */
	if (data->cb[line_num].cb != NULL) {
		return -EBUSY;
	}

	data->cb[line_num].cb = cb;
	data->cb[line_num].data = user;

	return 0;
}

void stm32_gpio_intc_remove_irq_callback(stm32_gpio_irq_line_t line)
{
	struct stm32_intc_gpio_data *data = &intc_gpio_data;
	uint32_t line_num = ll_exti_line_to_linenum(line);

	data->cb[line_num].cb = NULL;
	data->cb[line_num].data = NULL;
}

void stm32_exti_set_line_src_port(gpio_pin_t line, uint32_t port)
{
	uint32_t ll_line = stm32_exti_linenum_to_src_cfg_line(line);

#if defined(CONFIG_SOC_SERIES_STM32L0X) && defined(LL_SYSCFG_EXTI_PORTH)
	/*
	 * Ports F and G are not present on some STM32L0 parts, so
	 * for these parts port H external interrupt should be enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (port == STM32_PORTH) {
		port = LL_SYSCFG_EXTI_PORTH;
	}
#endif

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

#ifdef CONFIG_SOC_SERIES_STM32F1X
	LL_GPIO_AF_SetEXTISource(port, ll_line);

#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	LL_EXTI_SetEXTISource(port, ll_line);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7rs_exti)
	LL_SBS_SetEXTISource(port, ll_line);
#elif defined(CONFIG_SOC_SERIES_STM32MP2X)
	LL_EXTI_SetEXTISource(EXTI2, port, ll_line);
#else
	LL_SYSCFG_SetEXTISource(port, ll_line);
#endif
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

uint32_t stm32_exti_get_line_src_port(gpio_pin_t line)
{
	uint32_t ll_line = stm32_exti_linenum_to_src_cfg_line(line);
	uint32_t port;

#ifdef CONFIG_SOC_SERIES_STM32F1X
	port = LL_GPIO_AF_GetEXTISource(ll_line);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	port = LL_EXTI_GetEXTISource(ll_line);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7rs_exti)
	port = LL_SBS_GetEXTISource(ll_line);
#elif defined(CONFIG_SOC_SERIES_STM32MP2X)
	port = LL_EXTI_GetEXTISource(EXTI2, ll_line);
#else
	port = LL_SYSCFG_GetEXTISource(ll_line);
#endif

#if defined(CONFIG_SOC_SERIES_STM32L0X) && defined(LL_SYSCFG_EXTI_PORTH)
	/*
	 * Ports F and G are not present on some STM32L0 parts, so
	 * for these parts port H external interrupt is enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (port == LL_SYSCFG_EXTI_PORTH) {
		port = STM32_PORTH;
	}
#endif

	return port;
}
