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

#define EXTI_NODE DT_INST(0, st_stm32_exti)

#include <zephyr/device.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_system.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/gpio_intc_stm32.h> /* TODO: gpio includes shall be dropped here */
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/irq.h>

#include "stm32_hsem.h"


#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)

#define EXTI_IS_ACTIVE_RISING_FLAG_0_31(line)  LL_EXTI_IsActiveRisingFlag_0_31(line)
#define EXTI_IS_ACTIVE_FALLING_FLAG_0_31(line) LL_EXTI_IsActiveFallingFlag_0_31(line)
#define LL_EXTI_IsActiveFlag_0_31(line)  (EXTI_IS_ACTIVE_RISING_FLAG_0_31(line) || EXTI_IS_ACTIVE_FALLING_FLAG_0_31(line))

#define EXTI_CLEAR_RISING_FLAG_0_31(line)  LL_EXTI_ClearRisingFlag_0_31(line)
#define EXTI_CLEAR_FALLING_FLAG_0_31(line) LL_EXTI_ClearFallingFlag_0_31(line)
#define LL_EXTI_ClearFlag_0_31(line) { EXTI_CLEAR_RISING_FLAG_0_31(line); EXTI_CLEAR_FALLING_FLAG_0_31(line); }

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp) && defined(CONFIG_CPU_CORTEX_M4)
#define CPU_NR _C2
#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp) && defined(CONFIG_CPU_CORTEX_M4) */
#define CPU_NR
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp) && defined(CONFIG_CPU_CORTEX_M4) */

#define EXTI_ENABLE_IT_0_31(line)  CONCAT(LL, CPU_NR, _EXTI_EnableIT_0_31)(line)
#define EXTI_DISABLE_IT_0_31(line) CONCAT(LL, CPU_NR, _EXTI_DisableIT_0_31)(line)

#define EXTI_ENABLE_EVENT_0_31(line)  CONCAT(LL, CPU_NR, _EXTI_EnableEvent_0_31)(line)
#define EXTI_DISABLE_EVENT_0_31(line) CONCAT(LL, CPU_NR, _EXTI_DisableEvent_0_31)(line)

#define EXTI_CLEAR_FLAG_0_31(line)  CONCAT(LL, CPU_NR, _EXTI_ClearFlag_0_31)(line)
#define EXTI_CLEAR_FLAG_32_63(line) CONCAT(LL, CPU_NR, _EXTI_ClearFlag_32_63)(line)

#define EXTI_IS_ACTIVE_FLAG_0_31(line)  CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_0_31)(line)
#define EXTI_IS_ACTIVE_FLAG_32_63(line) CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_32_63)(line)
#define EXTI_IS_ACTIVE_FLAG_64_95(line) CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_64_95)(line)

#define EXTI_ENABLE_RISING_TRIG_0_31(line)  CONCAT(LL, _EXTI_EnableRisingTrig_0_31)(line)
#define EXTI_ENABLE_FALLING_TRIG_0_31(line) CONCAT(LL, _EXTI_EnableFallingTrig_0_31)(line)

#define EXTI_DISABLE_FALLING_TRIG_0_31(line) CONCAT(LL, _EXTI_DisableFallingTrig_0_31)(line)
#define EXTI_DISABLE_RISING_TRIG_0_31(line)  CONCAT(LL, _EXTI_DisableRisingTrig_0_31)(line)

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
 * @returns the LL_EXTI_LINE_n define for EXTI line number @p linenum
 */
static inline uint32_t linenum_to_ll_exti_line(uint32_t linenum)
{
	/* TODO: This imlementation will not work for lines above 31 */
	return BIT(linenum);
}

/**
 * @returns EXTI line number for LL_EXTI_LINE_n define
 */
static inline uint32_t ll_exti_line_to_linenum(uint32_t linenum)
{
	return LOG2(linenum);
}

/**
 * @returns the LL_<PPP>_EXTI_LINE_xxx define that corresponds to specified @p linenum
 * This value can be used with the LL EXTI source configuration functions.
 */
static inline uint32_t stm32_exti_linenum_to_src_cfg_line(gpio_pin_t linenum)
{
#if defined(CONFIG_SOC_SERIES_STM32L0X) || defined(CONFIG_SOC_SERIES_STM32F0X)
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

uint32_t stm32_exti_is_pending(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	/* TODO: shall work for all available lines */
	return EXTI_IS_ACTIVE_FLAG_0_31(ll_exti_line);
}

void stm32_exti_clear_pending(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	/* TODO: shall work for all available lines */
	EXTI_CLEAR_FLAG_0_31(ll_exti_line);
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
	uint32_t line;
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

static void stm32_fill_irq_table(int8_t start, int8_t len, int32_t irqn)
{
	for (int i = 0; i < len; i++) {
		exti_irq_table[start + i] = irqn;
	}
}

/* TODO Review STM32_EXTI_INIT_LINE_RANGE() macro regarding:
 *  - idx - there is only one EXTI. Why idx is needed then?
 *  - IRQ_CONNECT() - peripherals shall define own EXTI irqs and reset corresponding flags
*/
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

	/* TODO: Verify EXTI clock is always enabled */
	return 0;
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
 * The type @ref uint32_t is used to hold the LL_EXTI_LINE_xxx
 * defines of the LL EXTI API that corresponds to the provided pin.
 *
 * The port is not part of these definitions because port configuration
 * is done via different APIs, which use the LL_<PPP>_EXTI_LINE_xxx defines
 * returned by @ref stm32_exti_linenum_to_src_cfg_line instead.
 * @endinternal
 */
uint32_t stm32_gpio_intc_get_pin_irq_line(uint32_t port, gpio_pin_t pin)
{
	ARG_UNUSED(port);
	return linenum_to_ll_exti_line(pin);
}

void stm32_exti_enable_irq(uint32_t linenum)
{
	unsigned int irqnum;
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	__ASSERT_NO_MSG(linenum < NUM_EXTI_LINES);

	/* Get matching exti irq provided line number thanks to irq_table */
	irqnum = exti_irq_table[linenum];
	__ASSERT_NO_MSG(irqnum != 0xFF);

	/* Enable requested line interrupt */
	EXTI_ENABLE_IT_0_31(ll_exti_line);

	/* Enable exti irq interrupt */
	// irq_enable(irqnum);
}

void stm32_exti_disable_irq(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	EXTI_DISABLE_IT_0_31(ll_exti_line);
}

void stm32_exti_set_trigger_type(uint32_t linenum, uint32_t trigger)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* TODO: compare with intc_gpio_stm32wb0.c implementation */
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	switch (trigger) {
	case STM32_EXTI_TRIG_NONE:
		EXTI_DISABLE_RISING_TRIG_0_31(ll_exti_line);
		EXTI_DISABLE_FALLING_TRIG_0_31(ll_exti_line);
		break;
	case STM32_EXTI_TRIG_RISING:
		EXTI_ENABLE_RISING_TRIG_0_31(ll_exti_line);
		EXTI_DISABLE_FALLING_TRIG_0_31(ll_exti_line);
		break;
	case STM32_EXTI_TRIG_FALLING:
		EXTI_ENABLE_FALLING_TRIG_0_31(ll_exti_line);
		EXTI_DISABLE_RISING_TRIG_0_31(ll_exti_line);
		break;
	case STM32_EXTI_TRIG_BOTH:
		EXTI_ENABLE_RISING_TRIG_0_31(ll_exti_line);
		EXTI_ENABLE_FALLING_TRIG_0_31(ll_exti_line);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

void stm32_exti_set_mode(uint32_t linenum, uint32_t mode)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* TODO: implement mode selector */
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	switch (mode) {
	case STM32_EXTI_MODE_IT:
		EXTI_DISABLE_EVENT_0_31(ll_exti_line);
		EXTI_ENABLE_IT_0_31(ll_exti_line);
		break;
	case STM32_EXTI_MODE_EVENT:
		EXTI_DISABLE_IT_0_31(ll_exti_line);
		EXTI_ENABLE_EVENT_0_31(ll_exti_line);
		break;
	case STM32_EXTI_MODE_BOTH:
		EXTI_ENABLE_IT_0_31(ll_exti_line);
		EXTI_ENABLE_EVENT_0_31(ll_exti_line);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

int stm32_gpio_intc_set_irq_callback(uint32_t linenum, stm32_gpio_irq_cb_t cb, void *user)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;

	if ((data->cb[linenum].cb == cb) && (data->cb[linenum].data == user)) {
		return 0;
	}

	/* if callback already exists/maybe-running return busy */
	if (data->cb[linenum].cb != NULL) {
		return -EBUSY;
	}

	data->cb[linenum].cb = cb;
	data->cb[linenum].data = user;

	return 0;
}

void stm32_gpio_intc_remove_irq_callback(uint32_t linenum)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;

	data->cb[linenum].cb = NULL;
	data->cb[linenum].data = NULL;
}

void stm32_exti_set_line_src_port(uint32_t linenum, uint32_t port)
{
	uint32_t ll_line = stm32_exti_linenum_to_src_cfg_line(linenum);

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

uint32_t stm32_exti_get_line_src_port(uint32_t linenum)
{
	uint32_t ll_line = stm32_exti_linenum_to_src_cfg_line(linenum);
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
