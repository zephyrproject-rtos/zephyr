/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019-23 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 */

#define EXTI_NODE DT_INST(0, st_stm32_exti)

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
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>

#include "stm32_hsem.h"

/** @brief EXTI lines range mapped to a single interrupt line */
struct stm32_exti_range {
	/** Start of the range */
	uint8_t start;
	/** Range length */
	uint8_t len;
};

#define NUM_EXTI_LINES DT_PROP(DT_NODELABEL(exti), num_lines)

static IRQn_Type exti_irq_table[NUM_EXTI_LINES] = {[0 ... NUM_EXTI_LINES - 1] = 0xFF};

/* User callback wrapper */
struct __exti_cb {
	stm32_gpio_irq_cb_t cb;
	void *data;
};

/* EXTI driver data */
struct stm32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[NUM_EXTI_LINES];
};

/**
 * @returns the LL_<PPP>_EXTI_LINE_xxx define that corresponds to specified @p linenum
 * This value can be used with the LL EXTI source configuration functions.
 */
static inline uint32_t stm32_exti_linenum_to_src_cfg_line(gpio_pin_t linenum)
{
#if defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X)
	return ((linenum % 4 * 4) << 16) | (linenum / 4);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	return ((linenum & 0x3) << (16 + 3)) | (linenum >> 2);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7rs_exti)
	/* Gives the LL_SBS_EXTI_LINEn corresponding to the line number */
	return (((linenum % 4 * 4) << LL_SBS_REGISTER_PINPOS_SHFT) | (linenum / 4));
#else
	return (0xF << ((linenum % 4 * 4) + 16)) | (linenum / 4);
#endif
}

/**
 * @brief Checks interrupt pending bit for specified EXTI line
 *
 * @param line EXTI line number
 */
static inline int stm32_exti_is_pending(stm32_gpio_irq_line_t line)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	return (LL_EXTI_IsActiveRisingFlag_0_31(line) ||
		LL_EXTI_IsActiveFallingFlag_0_31(line));
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	return LL_C2_EXTI_IsActiveFlag_0_31(line);
#else
	return LL_EXTI_IsActiveFlag_0_31(line);
#endif
}

/**
 * @brief Clears interrupt pending bit for specified EXTI line
 *
 * @param line EXTI line number
 */
static inline void stm32_exti_clear_pending(stm32_gpio_irq_line_t line)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	LL_EXTI_ClearRisingFlag_0_31(line);
	LL_EXTI_ClearFallingFlag_0_31(line);
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_ClearFlag_0_31(line);
#else
	LL_EXTI_ClearFlag_0_31(line);
#endif
}

/**
 * @returns the LL_EXTI_LINE_n define for EXTI line number @p linenum
 */
static inline stm32_gpio_irq_line_t linenum_to_ll_exti_line(gpio_pin_t linenum)
{
	return BIT(linenum);
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
static void stm32_exti_isr(const void *exti_range)
{
	const struct device *dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;
	const struct stm32_exti_range *range = exti_range;
	stm32_gpio_irq_line_t line;
	uint32_t line_num;

	/* see which bits are set */
	for (uint8_t i = 0; i <= range->len; i++) {
		line_num = range->start + i;
		line = linenum_to_ll_exti_line(line_num);

		/* check if interrupt is pending */
		if (stm32_exti_is_pending(line) != 0) {
			/* clear pending interrupt */
			stm32_exti_clear_pending(line);

			/* run callback only if one is registered */
			if (!data->cb[line_num].cb) {
				continue;
			}

			/* `line` can be passed as-is because LL_EXTI_LINE_n is (1 << n) */
			data->cb[line_num].cb(line, data->cb[line_num].data);
		}
	}
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

static void stm32_fill_irq_table(int8_t start, int8_t len, int32_t irqn)
{
	for (int i = 0; i < len; i++) {
		exti_irq_table[start + i] = irqn;
	}
}

/* This macro:
 * - populates line_range_x from line_range dt property
 * - fill exti_irq_table through stm32_fill_irq_table()
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
		stm32_exti_isr, &line_range_##idx, 0);

/**
 * @brief Initializes the EXTI GPIO interrupt controller driver
 */
static int stm32_exti_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	DT_FOREACH_PROP_ELEM(DT_NODELABEL(exti),
			     interrupt_names,
			     STM32_EXTI_INIT_LINE_RANGE);

	return stm32_exti_enable_registers();
}

static struct stm32_exti_data exti_data;
DEVICE_DT_DEFINE(EXTI_NODE, &stm32_exti_init,
		 NULL,
		 &exti_data, NULL,
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		 NULL);

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
	return linenum_to_ll_exti_line(pin);
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
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_EnableIT_0_31(line);
#else
	LL_EXTI_EnableIT_0_31(line);
#endif

	/* Enable exti irq interrupt */
	irq_enable(irqnum);
}

void stm32_gpio_intc_disable_line(stm32_gpio_irq_line_t line)
{
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_DisableIT_0_31(line);
#else
	LL_EXTI_DisableIT_0_31(line);
#endif
}

void stm32_gpio_intc_select_line_trigger(stm32_gpio_irq_line_t line, uint32_t trg)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	switch (trg) {
	case STM32_GPIO_IRQ_TRIG_NONE:
		LL_EXTI_DisableRisingTrig_0_31(line);
		LL_EXTI_DisableFallingTrig_0_31(line);
		break;
	case STM32_GPIO_IRQ_TRIG_RISING:
		LL_EXTI_EnableRisingTrig_0_31(line);
		LL_EXTI_DisableFallingTrig_0_31(line);
		break;
	case STM32_GPIO_IRQ_TRIG_FALLING:
		LL_EXTI_EnableFallingTrig_0_31(line);
		LL_EXTI_DisableRisingTrig_0_31(line);
		break;
	case STM32_GPIO_IRQ_TRIG_BOTH:
		LL_EXTI_EnableRisingTrig_0_31(line);
		LL_EXTI_EnableFallingTrig_0_31(line);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

int stm32_gpio_intc_set_irq_callback(stm32_gpio_irq_line_t line, stm32_gpio_irq_cb_t cb, void *user)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;
	uint32_t line_num = ll_exti_line_to_linenum(line);

	if ((data->cb[line_num].cb == cb) && (data->cb[line_num].data == user)) {
		return 0;
	}

	/* if callback already exists/maybe-running return busy */
	if (data->cb[line_num].cb != NULL) {
		return -EBUSY;
	}

	data->cb[line_num].cb = cb;
	data->cb[line_num].data = user;

	return 0;
}

void stm32_gpio_intc_remove_irq_callback(stm32_gpio_irq_line_t line)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;
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
