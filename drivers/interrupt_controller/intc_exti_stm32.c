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
#include <stm32_ll_gpio.h> /* For STM32F1 series */
#include <stm32_ll_exti.h>
#include <stm32_ll_system.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h> /* For STM32L0 series */
#include <zephyr/drivers/interrupt_controller/gpio_intc_stm32.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>

#include "stm32_hsem.h"
#include "intc_exti_stm32_priv.h"

LOG_MODULE_REGISTER(exti_stm32, CONFIG_INTC_LOG_LEVEL);

/** @brief EXTI lines range mapped to a single interrupt line */
struct stm32_exti_range {
	/** Start of the range */
	uint8_t start;
	/** Range length */
	uint8_t len;
};

/* The number of EXTI lines associated with GPIO's on most stm32 */
#define NUM_GPIO_EXTI_LINES	16U
#define NUM_EXTI_LINES		DT_PROP(DT_NODELABEL(exti), num_lines)

/* EXTI IRQ table assiciated with GPIO's */
static IRQn_Type exti_gpio_irq_table[NUM_GPIO_EXTI_LINES] = {
	[0 ... NUM_GPIO_EXTI_LINES - 1] = 0xFF
};

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

static inline gpio_pin_t ll_exti_line_to_linenum(stm32_gpio_irq_line_t line);

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

int stm32_exti_is_pending(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (line_num < 32U) {
		ret = EXTI_IS_ACTIVE_FLAG_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		ret = EXTI_IS_ACTIVE_FLAG_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		ret = EXTI_IS_ACTIVE_FLAG_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}

int stm32_exti_clear_pending(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (line_num < 32U) {
		EXTI_CLEAR_FLAG_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_CLEAR_FLAG_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_CLEAR_FLAG_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
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
		if (stm32_exti_is_pending(line) > 0) {
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
		exti_gpio_irq_table[start + i] = irqn;
	}
}

/* This macro:
 * - populates line_range_x from line_range dt property
 * - fill exti_gpio_irq_table through stm32_fill_irq_table()
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

	__ASSERT_NO_MSG(line_num < NUM_GPIO_EXTI_LINES);

	/* Get matching exti irq provided line thanks to irq_table */
	irqnum = exti_gpio_irq_table[line_num];
	__ASSERT_NO_MSG(irqnum != 0xFF);

	/* Enable requested line interrupt */
	EXTI_ENABLE_IT_0_31(line);

	/* Enable exti irq interrupt */
	irq_enable(irqnum);
}

void stm32_gpio_intc_disable_line(stm32_gpio_irq_line_t line)
{
	EXTI_DISABLE_IT_0_31(line);
}

/**
 * @brief Enable EXTI interrupts.
 *
 * @param line	EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_enable_isr(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_ENABLE_IT_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_ENABLE_IT_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_ENABLE_IT_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Disable EXTI interrupts.
 *
 * @param line	EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_disable_isr(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	/* Disable requested line interrupt */
	if (line_num < 32U) {
		EXTI_DISABLE_IT_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_DISABLE_IT_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_DISABLE_IT_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Enables rising trigger for specified EXTI line
 *
 * @param line EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_enable_rising_trig(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_ENABLE_RISING_TRIG_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_ENABLE_RISING_TRIG_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_ENABLE_RISING_TRIG_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Disables rising trigger for specified EXTI line
 *
 * @param line EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_disable_rising_trig(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_DISABLE_RISING_TRIG_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_DISABLE_RISING_TRIG_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_DISABLE_RISING_TRIG_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Enables falling trigger for specified EXTI line
 *
 * @param line EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_enable_falling_trig(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_ENABLE_FALLING_TRIG_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_ENABLE_FALLING_TRIG_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_ENABLE_FALLING_TRIG_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Disables falling trigger for specified EXTI line
 *
 * @param line EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_disable_falling_trig(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_DISABLE_FALLING_TRIG_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_DISABLE_FALLING_TRIG_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_DISABLE_FALLING_TRIG_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

static inline void stm32_exti_intc_select_line_trigger(uint32_t line, uint32_t trg)
{
	switch (trg) {
	case STM32_GPIO_IRQ_TRIG_NONE:
		stm32_exti_disable_rising_trig(line);
		stm32_exti_disable_falling_trig(line);
		break;
	case STM32_GPIO_IRQ_TRIG_RISING:
		stm32_exti_enable_rising_trig(line);
		stm32_exti_disable_falling_trig(line);
		break;
	case STM32_GPIO_IRQ_TRIG_FALLING:
		stm32_exti_enable_falling_trig(line);
		stm32_exti_disable_rising_trig(line);
		break;
	case STM32_GPIO_IRQ_TRIG_BOTH:
		stm32_exti_enable_rising_trig(line);
		stm32_exti_enable_falling_trig(line);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
}

void stm32_gpio_intc_select_line_trigger(stm32_gpio_irq_line_t line, uint32_t trg)
{
	stm32_exti_intc_select_line_trigger(line, trg);
}

static inline int stm32_exti_intc_set_irq_callback(stm32_gpio_irq_line_t line,
						stm32_gpio_irq_cb_t cb, void *user)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;
	uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num >= ARRAY_SIZE(data->cb)) {
		LOG_ERR("Invalid line: %d", line);
		return -EINVAL;
	}

	if ((data->cb[line_num].cb == cb) && (data->cb[line_num].data == user)) {
		return 0;
	}

	/* if callback already exists/maybe-running return busy */
	if (data->cb[line_num].cb != NULL) {
		LOG_ERR("Callback already exists for line %d", line);
		return -EBUSY;
	}

	data->cb[line_num].cb = cb;
	data->cb[line_num].data = user;

	return 0;
}

static inline void stm32_exti_intc_remove_irq_callback(stm32_gpio_irq_line_t line)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;
	uint32_t line_num = ll_exti_line_to_linenum(line);

	data->cb[line_num].cb = NULL;
	data->cb[line_num].data = NULL;
}

int stm32_gpio_intc_set_irq_callback(stm32_gpio_irq_line_t line, stm32_gpio_irq_cb_t cb, void *user)
{
	return stm32_exti_intc_set_irq_callback(line, cb, user);
}

void stm32_gpio_intc_remove_irq_callback(stm32_gpio_irq_line_t line)
{
	stm32_exti_intc_remove_irq_callback(line);
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

/**
 * @brief Enable EXTI event.
 *
 * @param line	EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_enable_event(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_ENABLE_EVENT_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_ENABLE_EVENT_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_ENABLE_EVENT_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Disable EXTI interrupts.
 *
 * @param line	EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
static int stm32_exti_disable_event(uint32_t line)
{
	int ret = 0;
	const uint32_t line_num = ll_exti_line_to_linenum(line);

	if (line_num < 32U) {
		EXTI_DISABLE_EVENT_0_31(line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (line_num < 64U) {
		EXTI_DISABLE_EVENT_32_63(line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (line_num < 96U) {
		EXTI_DISABLE_EVENT_64_95(line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line: %d", line);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Enables external interrupt/event for specified EXTI line
 *
 * @param line	EXTI line
 * @param mode	EXTI mode
 * @returns 0 on success
 * @returns -ENOSYS if @p mode is not supported
 */
static int stm32_exti_set_mode(uint32_t line, stm32_exti_mode mode)
{
	int ret = 0;

	switch (mode) {
	case STM32_EXTI_MODE_NONE:
		stm32_exti_disable_event(line);
		stm32_exti_disable_isr(line);
		break;
	case STM32_EXTI_MODE_IT:
		stm32_exti_disable_event(line);
		stm32_exti_enable_isr(line);
		break;
	case STM32_EXTI_MODE_EVENT:
		stm32_exti_disable_isr(line);
		stm32_exti_enable_event(line);
		break;
	case STM32_EXTI_MODE_BOTH:
		stm32_exti_enable_isr(line);
		stm32_exti_enable_event(line);
		break;
	default:
		LOG_ERR("Unsupported EXTI mode type: %d", mode);
		ret = -ENOSYS;
		break;
	}

	return ret;
}

int stm32_exti_enable(uint32_t line, stm32_exti_trigger_type trigger,
		stm32_exti_mode mode, stm32_exti_irq_cb_t cb, void *user)
{
	int ret = 0;

	if (cb != NULL) {
		ret = stm32_exti_intc_set_irq_callback(line, cb, user);
		if (ret != 0) {
			return ret;
		}
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_exti_intc_select_line_trigger(line, trigger);
	ret = stm32_exti_set_mode(line, mode);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}

int stm32_exti_disable(uint32_t line)
{
	int ret = 0;

	ret = stm32_exti_set_mode(line, STM32_EXTI_MODE_NONE);
	if (ret != 0) {
		return ret;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_exti_intc_select_line_trigger(line, STM32_EXTI_TRIG_NONE);
	stm32_exti_intc_remove_irq_callback(line);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}
