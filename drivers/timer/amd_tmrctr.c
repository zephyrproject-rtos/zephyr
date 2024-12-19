/*
 * Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(amd_timer);

#define DT_DRV_COMPAT amd_xps_timer_1_00_a

/* Register definitions */
#define XTC_TCSR_OFFSET 0 /* Control/Status register */
#define XTC_TLR_OFFSET	4 /* Load register */
#define XTC_TCR_OFFSET	8 /* Timer counter register */

/* Control status register mask */
#define XTC_CSR_CASC_MASK		BIT(11)
#define XTC_CSR_ENABLE_ALL_MASK		BIT(10)
#define XTC_CSR_ENABLE_PWM_MASK		BIT(9)
#define XTC_CSR_INT_OCCURRED_MASK	BIT(8)
#define XTC_CSR_ENABLE_TMR_MASK		BIT(7)
#define XTC_CSR_ENABLE_INT_MASK		BIT(6)
#define XTC_CSR_LOAD_MASK		BIT(5)
#define XTC_CSR_AUTO_RELOAD_MASK	BIT(4)
#define XTC_CSR_EXT_CAPTURE_MASK	BIT(3)
#define XTC_CSR_EXT_GENERATE_MASK	BIT(2)
#define XTC_CSR_DOWN_COUNT_MASK		BIT(1)
#define XTC_CSR_CAPTURE_MODE_MASK	BIT(0)

/* Offset of second timer */
#define TIMER_REG_OFFSET	0x10

static uint32_t last_cycles;

struct xilinx_timer_config {
	uint32_t instance;
	mem_addr_t base;
	uint32_t clock_rate;
	uint32_t cycles_per_tick;
	uint32_t irq;
	bool one_timer;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	void (*irq_config_func)(const struct device *dev);
#endif
};

/* private data */
struct xilinx_timer_data {
	uint32_t clocksource_offset;
	uint32_t clockevent_offset;
};

/* Pointing to timer instance which is system timer */
static const struct device *sys_dev;

static inline uint32_t xlnx_tmrctr_read32(const struct device *dev, uint32_t timer_offset,
					  uint32_t offset)
{
	const struct xilinx_timer_config *config = dev->config;
	uint32_t reg = (uint32_t)(config->base) + timer_offset + offset;

	LOG_DBG("%s: 0x%x (base = 0x%lx, timer_offset = 0x%x, offset = 0x%x",
		__func__, reg, config->base, timer_offset, offset);

	return sys_read32(reg);
}

static inline void xlnx_tmrctr_write32(const struct device *dev, uint32_t timer_offset,
				       uint32_t value, uint32_t offset)
{
	const struct xilinx_timer_config *config = dev->config;
	uint32_t reg = (uint32_t)(config->base) + timer_offset + offset;

	LOG_DBG("%s: 0x%x (base = 0x%lx, timer_offset = 0x%x, offset = 0x%x",
		__func__, reg, config->base, timer_offset, offset);

	sys_write32(value, reg);
}

volatile uint32_t xlnx_tmrctr_read_count(const struct device *dev)
{
	struct xilinx_timer_data *data = dev->data;

	return xlnx_tmrctr_read32(dev, data->clocksource_offset, XTC_TCR_OFFSET);
}

volatile uint32_t xlnx_tmrctr_read_hw_cycle_count(const struct device *dev)
{
	return xlnx_tmrctr_read_count(dev);
}

static void xlnx_tmrctr_clear_interrupt(const struct device *dev)
{
	struct xilinx_timer_data *data = dev->data;

	uint32_t control_status_register = xlnx_tmrctr_read32(dev, data->clockevent_offset,
							      XTC_TCSR_OFFSET);

	xlnx_tmrctr_write32(dev, data->clockevent_offset,
			    control_status_register | XTC_CSR_INT_OCCURRED_MASK, XTC_TCSR_OFFSET);
}

static void xlnx_tmrctr_irq_handler(const struct device *dev)
{
	uint32_t cycles;
	uint32_t delta_ticks;
	const struct xilinx_timer_config *config = dev->config;

	cycles = xlnx_tmrctr_read_count(dev);
	/* Calculate the number of ticks since last announcement  */
	delta_ticks = (cycles - last_cycles) / config->cycles_per_tick;
	/* Update last cycles count without the rounding error */
	last_cycles += (delta_ticks * config->cycles_per_tick);

	if (sys_dev == dev) {
		/* Announce to the kernel */
		sys_clock_announce(delta_ticks);
	}

	xlnx_tmrctr_clear_interrupt(dev);
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return xlnx_tmrctr_read_hw_cycle_count(sys_dev);
}

static int xlnx_tmrctr_initialize(const struct device *dev)
{
	const struct xilinx_timer_config *config = dev->config;
	uint32_t num_counters = config->one_timer ? 1 : 2;

	for (uint8_t counter_number = 0; counter_number < num_counters; counter_number++) {
		uint32_t reg_offset = counter_number * TIMER_REG_OFFSET;

		/* Set the compare register to 0. */
		xlnx_tmrctr_write32(dev, reg_offset, 0, XTC_TLR_OFFSET);
		/* Reset the timer and the interrupt. */
		xlnx_tmrctr_write32(dev, reg_offset, XTC_CSR_INT_OCCURRED_MASK | XTC_CSR_LOAD_MASK,
						XTC_TCSR_OFFSET);
		/* Release the reset. */
		xlnx_tmrctr_write32(dev, reg_offset, 0, XTC_TCSR_OFFSET);
	}

	return 0;
}

static inline void xlnx_tmrctr_set_reset_value(const struct device *dev, uint8_t counter_number,
					       uint32_t reset_value)
{
	xlnx_tmrctr_write32(dev, counter_number, reset_value, XTC_TLR_OFFSET);
}

static inline void xlnx_tmrctr_set_options(const struct device *dev, uint8_t counter_number,
					   uint32_t options)
{
	xlnx_tmrctr_write32(dev, counter_number, options, XTC_TCSR_OFFSET);
}

static int xlnx_tmrctr_start(const struct device *dev)
{
	struct xilinx_timer_data *data = dev->data;
	uint32_t control_status_register;

	control_status_register = xlnx_tmrctr_read32(dev, data->clockevent_offset,
						     XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, data->clockevent_offset, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, data->clockevent_offset,
			    control_status_register | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);


	control_status_register = xlnx_tmrctr_read32(dev, data->clocksource_offset,
						     XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, data->clocksource_offset, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, data->clocksource_offset,
			    control_status_register | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);

	return 0;
}

static int xilinx_timer_init(const struct device *dev)
{
	int status;
	const struct xilinx_timer_config *config = dev->config;
	struct xilinx_timer_data *data = dev->data;

	LOG_DBG("%s: %d: Timer init at base 0x%lx, IRQ %d, clock %d, one_timer %d",
		__func__, config->instance, config->base, config->irq, config->clock_rate,
		config->one_timer);

	/* Initialize both timers - pretty much timer reset */
	status = xlnx_tmrctr_initialize(dev);
	if (status != 0) {
		return status;
	}

	if (!sys_dev && !config->one_timer) {
		/* Doing assignment which timer is clockevent/clocksource by it's offset in IP */
		data->clockevent_offset = 0;
		data->clocksource_offset = TIMER_REG_OFFSET;

		xlnx_tmrctr_set_reset_value(dev, data->clockevent_offset, config->cycles_per_tick);
		xlnx_tmrctr_set_options(dev, data->clockevent_offset,
					XTC_CSR_ENABLE_INT_MASK |
					XTC_CSR_AUTO_RELOAD_MASK |
					XTC_CSR_DOWN_COUNT_MASK);

		xlnx_tmrctr_set_options(dev, data->clocksource_offset, XTC_CSR_AUTO_RELOAD_MASK);

		status = xlnx_tmrctr_start(dev);
		if (status != 0) {
			return status;
		}

		last_cycles = xlnx_tmrctr_read_hw_cycle_count(dev);

		/* Assigning this instance as default timer */
		sys_dev = dev;
	}

	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
	}

	return 0;
}

#define XILINX_TIMER_INIT(inst)									\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupts),					\
		   (static void xilinx_timer_##inst##_irq_config(const struct device *dev)	\
		   {										\
			ARG_UNUSED(dev);							\
			IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),		\
				    xlnx_tmrctr_irq_handler, DEVICE_DT_INST_GET(inst), 0);	\
			irq_enable(DT_INST_IRQN(inst));						\
		   }))										\
												\
	static struct xilinx_timer_data xilinx_timer_##inst##_data;				\
												\
	static const struct xilinx_timer_config xilinx_timer_##inst##_cfg = {			\
		.instance = inst,								\
		.base = DT_INST_REG_ADDR(inst),							\
		.clock_rate = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency),		\
		.cycles_per_tick = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency) /	\
							   CONFIG_SYS_CLOCK_TICKS_PER_SEC,	\
		.irq = DT_INST_IRQN(inst),							\
		.one_timer = DT_INST_PROP_OR(inst, xlnx_one_timer_only, 0),			\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupts),				\
			   (.irq_config_func = xilinx_timer_##inst##_irq_config))		\
	};											\
	DEVICE_DT_INST_DEFINE(inst, &xilinx_timer_init, NULL, &xilinx_timer_##inst##_data,	\
			&xilinx_timer_##inst##_cfg, PRE_KERNEL_2,				\
			CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);				\
												\
	BUILD_ASSERT(DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency) >=			\
		     CONFIG_SYS_CLOCK_TICKS_PER_SEC,						\
		     "Timer clock frequency must be greater than the system tick frequency");	\
	BUILD_ASSERT((DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency) %			\
		     CONFIG_SYS_CLOCK_TICKS_PER_SEC) == 0,					\
		     "Timer clock frequency is not divisible by the system tick frequency");	\
	BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC %					\
		     DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency)) == 0,		\
		     "CPU clock frequency is not divisible by the Timer clock frequency");

DT_INST_FOREACH_STATUS_OKAY(XILINX_TIMER_INIT)
