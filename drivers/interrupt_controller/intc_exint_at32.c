/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019-23 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in AT32 MCUs
 */

#define EXINT_NODE DT_INST(0, at_at32_exint)

#include <zephyr/device.h>
#include <soc.h>
#include <at32_exint.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/intc_at32.h>
#include <zephyr/drivers/clock_control/at32_clock_control.h>
#include <zephyr/irq.h>

#define EXINT_NOTSUP 0xFFU

/** @brief EXINT lines range mapped to a single interrupt line */
struct at32_exint_range {
	/** Start of the range */
	uint8_t start;
	/** Range length */
	uint8_t len;
};

#define NUM_EXINT_LINES DT_PROP(DT_NODELABEL(exint), num_lines)

static IRQn_Type exint_irq_table[NUM_EXINT_LINES] = {[0 ... NUM_EXINT_LINES - 1] = 0xFF};

/* User callback wrapper */
struct __exti_cb {
	at32_exint_irq_cb_t cb;
	void *data;
};

/* EXINT driver data */
struct at32_exint_data {
	/* per-line callbacks */
	struct __exti_cb cb[NUM_EXINT_LINES];
};
static scfg_port_source_type get_source_port(uint32_t port)
{
	uint32_t port_index;
	port_index = (port - (uint32_t)GPIOA) / ((uint32_t)GPIOB - (uint32_t)GPIOA);
	return (scfg_port_source_type)port_index;
}

static uint32_t scfg_get_exint_port(scfg_pins_source_type pin_source)
{
    uint32_t tmp = 0x00;
    tmp = ((uint32_t)0x0F) << (0x04 * (pin_source & (uint8_t)0x03));

    switch (pin_source >> 0x02)
    {
        case 0:
            return  (SCFG->exintc1 >> (0x04 * (pin_source & (uint8_t)0x03))) & 0x0F;
        case 1:
            return  (SCFG->exintc2 >> (0x04 * (pin_source & (uint8_t)0x03))) & 0x0F;
        case 2:
            return  (SCFG->exintc3 >> (0x04 * (pin_source & (uint8_t)0x03))) & 0x0F;
        case 3:
            return  (SCFG->exintc4 >> (0x04 * (pin_source & (uint8_t)0x03))) & 0x0F;
        default:
	        return 0;
  }
	return 0;
}

/**
 * @returns the LL_<PPP>_EXINT_LINE_xxx define that corresponds to specified @p linenum
 * This value can be used with the EXUBT source configuration functions.
 */
static inline uint32_t at32_exint_linenum_to_src_cfg_line(gpio_pin_t linenum)
{
  return (0xF << ((linenum % 4 * 4) + 16)) | (linenum / 4);
}

/**
 * @brief Checks interrupt pending bit for specified EXINT line
 *
 * @param line EXINT line number
 */
static inline int at32_exint_is_pending(at32_irq_line_t line)
{
  return exint_flag_get(line);
}

/**
 * @brief Clears interrupt pending bit for specified EXINT line
 *
 * @param line EXINT line number
 */
static inline void at32_exint_clear_pending(at32_irq_line_t line)
{
  exint_flag_clear(line);
}

/**
 * @returns the EXINT_LINE_x define for EXINT line number @p linenum
 */
static inline at32_irq_line_t linenum_to_exint_line(gpio_pin_t linenum)
{
	return BIT(linenum);
}

/**
 * @returns EXINT line number for EXINT_LINE_n define
 */
static inline gpio_pin_t exint_line_to_linenum(at32_irq_line_t line)
{
	return LOG2(line);
}

/**
 * @brief EXINT ISR handler
 *
 * Check EXINT lines in exti_range for pending interrupts
 *
 * @param EXINT_range Pointer to a exti_range structure
 */
static void at32_exint_isr(const void *exint_range)
{
	const struct device *dev = DEVICE_DT_GET(EXINT_NODE);
	struct at32_exint_data *data = dev->data;
	const struct at32_exint_range *range = exint_range;
	at32_irq_line_t line;
	uint32_t line_num;

	/* see which bits are set */
	for (uint8_t i = 0; i <= range->len; i++) {
		line_num = range->start + i;
		line = linenum_to_exint_line(line_num);
		/* check if interrupt is pending */
		if (at32_exint_is_pending(line) != 0) {
			/* clear pending interrupt */
			at32_exint_clear_pending(line);
			/* run callback only if one is registered */
			if (!data->cb[line_num].cb) {
				continue;
			}
			/* `line` can be passed as-is because EXINT_LINE_x is (1 << x) */
			data->cb[line_num].cb(line, data->cb[line_num].data);
		}
	}
}

/** Enables the peripheral clock required to access EXINT registers */
static int at32_exint_enable_registers(void)
{
	/* Initialize to 0 for series where there is nothing to do. */
	int ret = 0;
	return ret;
}

static void at32_fill_irq_table(int8_t start, int8_t len, int32_t irqn)
{
	for (int i = 0; i < len; i++) {
		exint_irq_table[start + i] = irqn;
	}
}

/* This macro:
 * - populates line_range_x from line_range dt property
 * - fill exti_irq_table through at32_fill_irq_table()
 * - calls IRQ_CONNECT for each interrupt and matching line_range
 */
#define AT32_EXINT_INIT_LINE_RANGE(node_id, interrupts, idx)			\
	static const struct at32_exint_range line_range_##idx = {		\
		DT_PROP_BY_IDX(node_id, line_ranges, UTIL_X2(idx)),		\
		DT_PROP_BY_IDX(node_id, line_ranges, UTIL_INC(UTIL_X2(idx)))	\
	};									\
	at32_fill_irq_table(line_range_##idx.start,				\
			     line_range_##idx.len,				\
			     DT_IRQ_BY_IDX(node_id, idx, irq));			\
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),				\
		DT_IRQ_BY_IDX(node_id, idx, priority),				\
		at32_exint_isr, &line_range_##idx, 0);
/**
 * @brief Initializes the EXINT interrupt controller driver
 */
static int at32_exint_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	DT_FOREACH_PROP_ELEM(DT_NODELABEL(exint),
			     interrupt_names,
			     AT32_EXINT_INIT_LINE_RANGE);
	return at32_exint_enable_registers();
	return 0;
}

static struct at32_exint_data exint_data;
DEVICE_DT_DEFINE(EXINT_NODE, &at32_exint_init,
		 NULL,
		 &exint_data, NULL,
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		 NULL);

/**
 * @brief EXINT GPIO interrupt controller API implementation
 */

/**
 * @internal
 * AT32 EXINT driver:
 * The type @ref at32_exint_irq_line_t is used to hold the EXINT_LINE_x
 * defines of the EXINT API that corresponds to the provided pin.
 *
 * @endinternal
 */
at32_irq_line_t at32_exint_intc_get_pin_irq_line(uint32_t port, gpio_pin_t pin)
{
	ARG_UNUSED(port);
	return linenum_to_exint_line(pin);
}

void at32_exint_intc_enable_line(at32_irq_line_t line)
{
    unsigned int irqnum;
    uint32_t line_num = exint_line_to_linenum(line);
  
    __ASSERT_NO_MSG(line_num < NUM_EXINT_LINES);
    /* Get matching exint irq provided line thanks to irq_table */
    irqnum = exint_irq_table[line_num];
    __ASSERT_NO_MSG(irqnum != 0xFF);

    exint_interrupt_enable(line, TRUE);
    /* Enable exint irq interrupt */
    irq_enable(irqnum);
}

void at32_exint_intc_disable_line(at32_irq_line_t line)
{
    exint_interrupt_enable(line, FALSE);
}

void at32_exint_intc_select_line_trigger(at32_irq_line_t line, uint32_t trigger)
{
	switch (trigger) {
        case AT32_GPIO_IRQ_TRIG_NONE:
            EXINT->polcfg1 &= ~line;
            EXINT->polcfg2 &= ~line;
            break;
        case AT32_GPIO_IRQ_TRIG_RISING:
            EXINT->polcfg1 |= line;
            EXINT->polcfg2 &= ~line;
            break;
        case AT32_GPIO_IRQ_TRIG_FALLING:
            EXINT->polcfg1 &= ~line;
            EXINT->polcfg2 |= line;
            break;
        case AT32_GPIO_IRQ_TRIG_BOTH:
            EXINT->polcfg1 |= line;
            EXINT->polcfg2 |= line;
            break;
        default:
            __ASSERT_NO_MSG(0);
            break;
	}
}

int at32_exint_intc_set_irq_callback(at32_irq_line_t line, at32_exint_irq_cb_t cb, void *arg)
{
	const struct device *const dev = DEVICE_DT_GET(EXINT_NODE);
	struct at32_exint_data *data = dev->data;
	uint32_t line_num = exint_line_to_linenum(line);

	if ((data->cb[line_num].cb == cb) && (data->cb[line_num].data == arg)) {
		return 0;
	}

	/* if callback already exists/maybe-running return busy */
	if (data->cb[line_num].cb != NULL) {
		return -EBUSY;
	}

	data->cb[line_num].cb = cb;
	data->cb[line_num].data = arg;

	return 0;
}

void at32_exint_intc_remove_irq_callback(at32_irq_line_t line)
{
    const struct device *const dev = DEVICE_DT_GET(EXINT_NODE);
    struct at32_exint_data *data = dev->data;
    uint32_t line_num = exint_line_to_linenum(line);

    data->cb[line_num].cb = NULL;
    data->cb[line_num].data = NULL;
}

void at32_exint_set_line_src_port(gpio_pin_t pin, uint32_t port)
{
    scfg_exint_line_config(get_source_port(port), pin);
}

uint32_t at32_exint_get_line_src_port(gpio_pin_t pin)
{
    uint32_t port;
    port = scfg_get_exint_port((scfg_pins_source_type)pin);
	  return port;
}
