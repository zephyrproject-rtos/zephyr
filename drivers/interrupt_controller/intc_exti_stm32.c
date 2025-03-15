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
#include <stm32_ll_bus.h>
#include <stm32_ll_system.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h> /* needed for PORTx defines */
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>

#ifdef CONFIG_SOC_SERIES_STM32F1X
#include <stm32_ll_gpio.h>  /* for EXTI GPIO lines 0 to 15 support */
#endif /* CONFIG_SOC_SERIES_STM32F1X */

#include "stm32_hsem.h"
#include "intc_exti_stm32_priv.h"

#define DT_DRV_COMPAT st_stm32_exti

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(exti_stm32, CONFIG_INTC_LOG_LEVEL);

#define EXTI_NODE_IDX	0

/* Takes elements with even indexes 0,2,4,.. with weight 0 and the others as-is */
#define SEL2ND(n, prop, idx)	(DT_PROP_BY_IDX(n, prop, idx) * (idx % 2))

#define NUM_EXTI_LINES \
	DT_INST_FOREACH_PROP_ELEM_SEP(EXTI_NODE_IDX, line_ranges, SEL2ND, (+))

#define EXTI_LINE_RANGES_INVALID_IDX 0xff

/* User callback wrapper */
struct __exti_cb {
	stm32_exti_irq_cb_t cb;
	void *data;
};

/* EXTI driver data */
struct stm32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[NUM_EXTI_LINES];
};

/** @brief EXTI lines range config mapped to a single interrupt line */
struct stm32_exti_line_range {
	/** Start of the range */
	const uint8_t start;
	/** Range length */
	const uint8_t len;
	/** IRQ number for this range */
	const IRQn_Type irqnum;

	/* User callback wrappers for each line */
	const struct __exti_cb *cb;
};

static IRQn_Type stm32_exti_linenum2irqnum(uint32_t linenum);
static uint8_t stm32_exti_linenum2exti_line_ranges_idx(uint32_t linenum);
static int stm32_exti_clear_pending(uint32_t linenum);
static inline uint32_t linenum_to_ll_exti_line(uint32_t linenum);
static void stm32_exti_remove_irq_callback(uint32_t linenum);
static int stm32_exti_set_irq_callback(uint32_t linenum, stm32_exti_irq_cb_t cb,
				       void *user);
static int stm32_exti_set_trigger_type(uint32_t linenum,
					stm32_exti_trigger_type trigger);

/**
 * @returns the LL_<PPP>_EXTI_LINE_xxx define that corresponds to specified @p linenum
 * This value can be used with the LL EXTI source configuration functions.
 */
static inline uint32_t stm32_exti_linenum_to_src_cfg_line(uint32_t linenum)
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
 * @brief Checks pending bit for specified EXTI line
 *
 * @param linenum EXTI line number
 * @returns true if pending, false otherwise
 *
 * @note This function will assert if @p linenum is invalid
 */
static inline bool stm32_exti_is_pending(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	if (linenum < 32U) {
		return EXTI_IS_ACTIVE_FLAG_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		return EXTI_IS_ACTIVE_FLAG_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		return EXTI_IS_ACTIVE_FLAG_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
	}

	return false;
}

/**
 * @brief Clears pending bit for specified EXTI line number
 *
 * @param linenum EXTI line number
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
static inline int stm32_exti_clear_pending(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	if (linenum < 32U) {
		EXTI_CLEAR_FLAG_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_CLEAR_FLAG_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_CLEAR_FLAG_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @returns the LL_EXTI_LINE_n define for EXTI line number @p linenum
 */
static inline uint32_t linenum_to_ll_exti_line(uint32_t linenum)
{
	return BIT(linenum % 32U);
}

/**
 * @returns EXTI line number for LL_EXTI_LINE_n define
 */
static inline uint32_t ll_exti_line_to_linenum(uint32_t ll_exti_line)
{
	return ll_exti_line % 32U;
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
	const struct device *dev = DEVICE_DT_INST_GET(EXTI_NODE_IDX);
	struct stm32_exti_data *data = dev->data;
	const struct stm32_exti_line_range *range = exti_range;

	/* see which bits are set */
	for (uint8_t linenum = range->start;
		linenum < range->start + range->len; linenum++) {
		/* check if interrupt is pending */
		if (stm32_exti_is_pending(linenum)) {
			/* clear pending interrupt */
			stm32_exti_clear_pending(linenum);

			/* run callback only if one is registered */
			if (!data->cb[linenum].cb) {
				continue;
			}

			data->cb[linenum].cb(linenum, data->cb[linenum].data);
		}
	}
}

int stm32_exti_enable_event(uint32_t linenum)
{
	const uint8_t exti_line_ranges_idx =
		stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (exti_line_ranges_idx == EXTI_LINE_RANGES_INVALID_IDX) {
		return -EINVAL;
	}

	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	if (linenum < 32U) {
		EXTI_ENABLE_EVENT_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_ENABLE_EVENT_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_ENABLE_EVENT_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

int stm32_exti_disable_event(uint32_t linenum)
{
	const uint8_t exti_line_ranges_idx =
		stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (exti_line_ranges_idx == EXTI_LINE_RANGES_INVALID_IDX) {
		return -EINVAL;
	}

	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	if (linenum < 32U) {
		EXTI_DISABLE_EVENT_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_DISABLE_EVENT_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_DISABLE_EVENT_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Enables rising trigger for specified EXTI line
 *
 * @param linenum EXTI line number
 * @param ll_exti_line LL EXTI line
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
static int stm32_exti_enable_rising_trig(const uint32_t linenum,
					  const uint32_t ll_exti_line)
{
	if (linenum < 32U) {
		EXTI_ENABLE_RISING_TRIG_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_ENABLE_RISING_TRIG_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_ENABLE_RISING_TRIG_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Disables rising trigger for specified EXTI line
 *
 * @param linenum EXTI line number
 * @param ll_exti_line LL EXTI line
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
static int stm32_exti_disable_rising_trig(const uint32_t linenum,
					   const uint32_t ll_exti_line)
{
	if (linenum < 32U) {
		EXTI_DISABLE_RISING_TRIG_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_DISABLE_RISING_TRIG_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_DISABLE_RISING_TRIG_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Enables falling trigger for specified EXTI line
 *
 * @param linenum EXTI line number
 * @param ll_exti_line LL EXTI line
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
static int stm32_exti_enable_falling_trig(const uint32_t linenum,
					   const uint32_t ll_exti_line)
{
	if (linenum < 32U) {
		EXTI_ENABLE_FALLING_TRIG_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_ENABLE_FALLING_TRIG_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_ENABLE_FALLING_TRIG_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Disables falling trigger for specified EXTI line
 *
 * @param linenum EXTI line number
 * @param ll_exti_line LL EXTI line
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
static int stm32_exti_disable_falling_trig(const uint32_t linenum,
					    const uint32_t ll_exti_line)
{
	if (linenum < 32U) {
		EXTI_DISABLE_FALLING_TRIG_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_DISABLE_FALLING_TRIG_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_DISABLE_FALLING_TRIG_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Enables external interrupt/event for specified EXTI line
 *
 * @param linenum EXTI line number
 * @param mode EXTI mode
 * @returns 0 on success
 * @returns -EINVAL if @p linenum is invalid
 * @returns -ENOSYS if @p mode is not implemented
 */
static int stm32_exti_set_mode(uint32_t linenum, stm32_exti_mode mode)
{
	int ret = 0;

	switch (mode) {
	case STM32_EXTI_MODE_NONE:
		ret = stm32_exti_disable_event(linenum);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_disable_irq(linenum);
		if (ret != 0) {
			return ret;
		}
		break;
	case STM32_EXTI_MODE_IT:
		ret = stm32_exti_disable_event(linenum);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_enable_irq(linenum);
		if (ret != 0) {
			return ret;
		}
		break;
	case STM32_EXTI_MODE_EVENT:
		ret = stm32_exti_disable_irq(linenum);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_enable_event(linenum);
		if (ret != 0) {
			return ret;
		}
		break;
	case STM32_EXTI_MODE_BOTH:
		ret = stm32_exti_enable_irq(linenum);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_enable_event(linenum);
		if (ret != 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("Not supported EXTI mode: %d", mode);
		ret = -ENOSYS;
		break;
	}

	return ret;
}

int stm32_exti_enable(uint32_t linenum, stm32_exti_trigger_type trigger,
		      stm32_exti_mode mode, stm32_exti_irq_cb_t cb, void *user)
{
	int ret = 0;

	const uint8_t exti_line_ranges_idx =
		stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (exti_line_ranges_idx == EXTI_LINE_RANGES_INVALID_IDX) {
		ret = -EINVAL;
		return ret;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	ret = stm32_exti_set_irq_callback(linenum, cb, user);
	if (ret != 0) {
		return ret;
	}

	stm32_exti_set_trigger_type(linenum, trigger);
	stm32_exti_set_mode(linenum, mode);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}

int stm32_exti_disable(uint32_t linenum)
{
	const uint8_t exti_line_ranges_idx =
		stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (exti_line_ranges_idx == EXTI_LINE_RANGES_INVALID_IDX) {
		return -EINVAL;
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_exti_set_mode(linenum, STM32_EXTI_MODE_NONE);
	stm32_exti_set_trigger_type(linenum, STM32_EXTI_TRIG_NONE);
	stm32_exti_remove_irq_callback(linenum);

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

/* TODO: https://docs.zephyrproject.org/latest/build/dts/howtos.html */
static struct stm32_exti_data exti_data;

#define STM32_EXTI_INIT_LINE_RANGE(n, interrupts, idx)		\
	{								\
		.start = DT_PROP_BY_IDX(n, line_ranges, UTIL_X2(idx)),	\
		.len = DT_PROP_BY_IDX(n, line_ranges, UTIL_INC(UTIL_X2(idx))),	\
		.irqnum = DT_IRQ_BY_IDX(n, idx, irq),				\
		.cb = NULL,	\
	},								\

static struct stm32_exti_line_range exti_lines_range[] = {
	DT_INST_FOREACH_PROP_ELEM(EXTI_NODE_IDX, interrupt_names, STM32_EXTI_INIT_LINE_RANGE)
};

BUILD_ASSERT(ARRAY_SIZE(exti_lines_range) <= EXTI_LINE_RANGES_INVALID_IDX,
	"Expected exti_lines_range length to be less than 0xff");
BUILD_ASSERT(2*ARRAY_SIZE(exti_lines_range) == DT_INST_PROP_LEN(EXTI_NODE_IDX, line_ranges),
	"The number of EXTI line ranges shall match the number of interrupt names");
BUILD_ASSERT(ARRAY_SIZE(exti_lines_range) == DT_INST_NUM_IRQS(EXTI_NODE_IDX),
	"The number of EXTI line ranges shall match the number of IRQs");

#define STM32_EXTI_INIT_IRQ_CONNECT(n, interrupts, idx)		\
	IRQ_CONNECT(DT_IRQ_BY_IDX(n, idx, irq),				\
		DT_IRQ_BY_IDX(n, idx, priority),				\
		stm32_exti_isr, &exti_lines_range[idx], 0);

/**
 * @brief Convert EXTI line number to exti_lines_range array index
 *
 * @note This function will skim over all available EXTI line number
 *       ranges and NOT over all available EXTI line numbers.
 *       EXTI line ranges number is much smaller than number EXTI lines.
 *
 * @param linenum EXTI line number
 * @return uint8_t exti_lines_range index for the given EXTI line number
 *                 or EXTI_LINE_RANGES_INVALID_IDX if not found
 */
static uint8_t stm32_exti_linenum2exti_line_ranges_idx(uint32_t linenum)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(exti_lines_range); i++) {
		if (linenum >= exti_lines_range[i].start &&
			linenum < exti_lines_range[i].start + exti_lines_range[i].len) {
			return i;
		}
	}
	return EXTI_LINE_RANGES_INVALID_IDX;
}

/**
 * @brief Convert EXTI line number to IRQ number
 *
 * @param linenum EXTI line number
 * @return IRQn_Type interrupt number for the given EXTI line number
 *                   or EXTI_LINE_RANGES_INVALID_IDX if not found
 */
static IRQn_Type stm32_exti_linenum2irqnum(uint32_t linenum)
{
	const uint8_t idx = stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (idx != EXTI_LINE_RANGES_INVALID_IDX) {
		return exti_lines_range[idx].irqnum;
	}
	return idx;
}

/**
 * @brief Initializes the EXTI device interrupt controller driver
 */
static int stm32_exti_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	DT_INST_FOREACH_PROP_ELEM(EXTI_NODE_IDX,
				  interrupt_names,
				  STM32_EXTI_INIT_IRQ_CONNECT);

	/* Map exti_lines_range to exti_data */
	for (size_t i = 0; i < ARRAY_SIZE(exti_lines_range); i++) {
		size_t idx = 0;

		for (size_t j = 0; j < i; j++) {
			idx += exti_lines_range[j].len;
		}
		exti_lines_range[i].cb = exti_data.cb + idx;
	}

	return stm32_exti_enable_registers();
}

DEVICE_DT_INST_DEFINE(EXTI_NODE_IDX, &stm32_exti_init,
		      NULL,
		      &exti_data, &exti_lines_range,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		      NULL);

int stm32_exti_enable_irq(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	/* Get matching exti irq provided line number thanks to irq_table */
	const IRQn_Type irqnum = stm32_exti_linenum2irqnum(linenum);

	if (irqnum == 0xff) {
		LOG_ERR("Invalid IRQ number: %d", irqnum);
		return -EINVAL;
	}

	if (linenum < 32U) {
		EXTI_ENABLE_IT_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_ENABLE_IT_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_ENABLE_IT_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	/* Enable exti irq interrupt */
	irq_enable(irqnum);

	return 0;
}

int stm32_exti_disable_irq(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	/* Get matching exti irq provided line number thanks to irq_table */
	const IRQn_Type irqnum = stm32_exti_linenum2irqnum(linenum);

	if (irqnum == 0xff) {
		LOG_ERR("Invalid IRQ number: %d", irqnum);
		return -EINVAL;
	}

	/* Disable exti irq interrupt */
	irq_disable(irqnum);

	/* Disable requested line interrupt */
	if (linenum < 32U) {
		EXTI_DISABLE_IT_0_31(ll_exti_line);
#ifdef CONFIG_EXTI_STM32_HAS_64_LINES
	} else if (linenum < 64U) {
		EXTI_DISABLE_IT_32_63(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */
#ifdef CONFIG_EXTI_STM32_HAS_96_LINES
	} else if (linenum < 96U) {
		EXTI_DISABLE_IT_64_95(ll_exti_line);
#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */
	} else {
		LOG_ERR("Invalid line number: %d", linenum);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Configures the trigger type for the specified EXTI line
 *
 * @param linenum EXTI line number
 * @param mode EXTI trigger type
 * @returns 0 on success
 * @returns -EINVAL if @p linenum is invalid
 * @returns -ENOSYS if @p mode is not implemented
 */
static int stm32_exti_set_trigger_type(uint32_t linenum,
					stm32_exti_trigger_type trigger)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);
	int ret = 0;

	switch (trigger) {
	case STM32_EXTI_TRIG_NONE:
		ret = stm32_exti_disable_rising_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_disable_falling_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		break;
	case STM32_EXTI_TRIG_RISING:
		ret = stm32_exti_enable_rising_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_disable_falling_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		break;
	case STM32_EXTI_TRIG_FALLING:
		ret = stm32_exti_enable_falling_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_disable_rising_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		break;
	case STM32_EXTI_TRIG_BOTH:
		ret = stm32_exti_enable_rising_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		ret = stm32_exti_enable_falling_trig(linenum, ll_exti_line);
		if (ret != 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("Not supported EXTI trigger type: %d", trigger);
		ret = -ENOSYS;
		break;
	}

	return ret;
}

/**
 * @brief Set callback invoked when an interrupt occurs on specified EXTI line number
 *
 * @param linenum	EXTI interrupt line number
 * @param cb	Interrupt callback function
 * @param user	Custom user data for usage by the callback
 * @returns 0 on success, -EBUSY if a callback is already set for @p line
 */
static int stm32_exti_set_irq_callback(uint32_t linenum, stm32_exti_irq_cb_t cb,
				       void *user)
{
	const struct device *const dev = DEVICE_DT_INST_GET(EXTI_NODE_IDX);
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

/**
 * @brief Removes the interrupt callback of specified EXTI line number
 *
 * @param linenum	EXTI interrupt line number
 */
static void stm32_exti_remove_irq_callback(uint32_t linenum)
{
	const struct device *const dev = DEVICE_DT_INST_GET(EXTI_NODE_IDX);
	struct stm32_exti_data *data = dev->data;

	data->cb[linenum].cb = NULL;
	data->cb[linenum].data = NULL;
}

int stm32_exti_set_line_src_port(uint32_t linenum, uint32_t port)
{
	const uint8_t exti_line_ranges_idx =
		stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (exti_line_ranges_idx == EXTI_LINE_RANGES_INVALID_IDX) {
		return -EINVAL;
	}

	const uint32_t ll_line = stm32_exti_linenum_to_src_cfg_line(linenum);

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

	return 0;
}

int stm32_exti_get_line_src_port(uint32_t linenum, uint32_t *port)
{
	const uint8_t exti_line_ranges_idx =
		stm32_exti_linenum2exti_line_ranges_idx(linenum);

	if (exti_line_ranges_idx == EXTI_LINE_RANGES_INVALID_IDX) {
		return -EINVAL;
	}

	uint32_t ll_line = stm32_exti_linenum_to_src_cfg_line(linenum);

#ifdef CONFIG_SOC_SERIES_STM32F1X
	*port = LL_GPIO_AF_GetEXTISource(ll_line);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	*port = LL_EXTI_GetEXTISource(ll_line);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7rs_exti)
	*port = LL_SBS_GetEXTISource(ll_line);
#else
	*port = LL_SYSCFG_GetEXTISource(ll_line);
#endif

#if defined(CONFIG_SOC_SERIES_STM32L0X) && defined(LL_SYSCFG_EXTI_PORTH)
	/*
	 * Ports F and G are not present on some STM32L0 parts, so
	 * for these parts port H external interrupt is enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (*port == LL_SYSCFG_EXTI_PORTH) {
		*port = STM32_PORTH;
	}
#endif

	return 0;
}
