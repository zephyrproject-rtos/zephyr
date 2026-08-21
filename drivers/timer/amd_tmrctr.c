/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AMD Xilinx AXI Timer driver for Zephyr system clock
 *
 * Uses timer 1 as a free-running clocksource and timer 0 as a one-shot
 * clockevent when tickless operation is enabled.
 */

#include <limits.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

LOG_MODULE_REGISTER(amd_timer);

#define DT_DRV_COMPAT amd_xps_timer_1_00_a

/* Register definitions */
#define XTC_TCSR_OFFSET 0 /* Control/Status register */
#define XTC_TLR_OFFSET  4 /* Load register */
#define XTC_TCR_OFFSET  8 /* Timer counter register */

/* Control status register mask */
#define XTC_CSR_CASC_MASK               BIT(11)
#define XTC_CSR_ENABLE_ALL_MASK         BIT(10)
#define XTC_CSR_ENABLE_PWM_MASK         BIT(9)
#define XTC_CSR_INT_OCCURRED_MASK       BIT(8)
#define XTC_CSR_ENABLE_TMR_MASK         BIT(7)
#define XTC_CSR_ENABLE_INT_MASK         BIT(6)
#define XTC_CSR_LOAD_MASK               BIT(5)
#define XTC_CSR_AUTO_RELOAD_MASK        BIT(4)
#define XTC_CSR_EXT_CAPTURE_MASK        BIT(3)
#define XTC_CSR_EXT_GENERATE_MASK       BIT(2)
#define XTC_CSR_DOWN_COUNT_MASK         BIT(1)
#define XTC_CSR_CAPTURE_MODE_MASK       BIT(0)

/* Offset of second timer */
#define TIMER_REG_OFFSET        0x10

#if defined(CONFIG_TEST)
/* tests/kernel/context: TICK_IRQ for irq_enable/disable coverage */
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_INST(0, amd_xps_timer_1_00_a));
#endif

/* Max clockevent interval: INT32_MAX ticks worth of cycles, tick-aligned. */
#define CYCLES_MAX_COMPUTE(cpt) \
	((uint32_t)(INT32_MAX - (INT32_MAX % (cpt))))

static struct k_spinlock amd_timer_lock;
static uint32_t last_hw_cycles;
static uint64_t accumulated_cycles;
static uint64_t last_announce;
static uint32_t cycles_max;

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

struct xilinx_timer_data {
	uint32_t clocksource_offset;
	uint32_t clockevent_offset;
};

/* Timer instance selected as the Zephyr system timer. */
static const struct device *sys_dev;

/**
 * @brief Read a 32-bit timer register.
 *
 * @param dev Timer device.
 * @param timer_offset Base offset of the timer counter within the IP block.
 * @param offset Register offset from the timer counter base.
 *
 * @return Register value.
 */
static inline uint32_t xlnx_tmrctr_read32(const struct device *dev, uint32_t timer_offset,
					  uint32_t offset)
{
	const struct xilinx_timer_config *config = dev->config;
	uint32_t reg = (uint32_t)(config->base) + timer_offset + offset;

	LOG_DBG("%s: 0x%x (base = 0x%lx, timer_offset = 0x%x, offset = 0x%x",
		__func__, reg, config->base, timer_offset, offset);

	return sys_read32(reg);
}

/**
 * @brief Write a 32-bit timer register.
 *
 * @param dev Timer device.
 * @param timer_offset Base offset of the timer counter within the IP block.
 * @param value Value to write.
 * @param offset Register offset from the timer counter base.
 */
static inline void xlnx_tmrctr_write32(const struct device *dev, uint32_t timer_offset,
				       uint32_t value, uint32_t offset)
{
	const struct xilinx_timer_config *config = dev->config;
	uint32_t reg = (uint32_t)(config->base) + timer_offset + offset;

	LOG_DBG("%s: 0x%x (base = 0x%lx, timer_offset = 0x%x, offset = 0x%x",
		__func__, reg, config->base, timer_offset, offset);

	sys_write32(value, reg);
}

/**
 * @brief Program the timer load (reset) register.
 *
 * @param dev Timer device.
 * @param counter_number Timer counter offset within the IP block.
 * @param reset_value Value to load into the counter.
 */
static inline void xlnx_tmrctr_set_reset_value(const struct device *dev, uint8_t counter_number,
					       uint32_t reset_value)
{
	xlnx_tmrctr_write32(dev, counter_number, reset_value, XTC_TLR_OFFSET);
}

/**
 * @brief Read the current clocksource counter value.
 *
 * @param dev Timer device.
 *
 * @return Current TCR value of the clocksource timer.
 */
static uint32_t xlnx_tmrctr_read_count(const struct device *dev)
{
	struct xilinx_timer_data *data = dev->data;

	return xlnx_tmrctr_read32(dev, data->clocksource_offset, XTC_TCR_OFFSET);
}

/**
 * @brief Return the raw hardware cycle count from the clocksource timer.
 *
 * @param dev Timer device.
 *
 * @return Current hardware cycle count.
 */
static uint32_t xlnx_tmrctr_read_hw_cycle_count(const struct device *dev)
{
	return xlnx_tmrctr_read_count(dev);
}

/**
 * @brief Clear the clockevent timer interrupt status bit.
 *
 * @param dev Timer device.
 */
static void xlnx_tmrctr_clear_interrupt(const struct device *dev)
{
	struct xilinx_timer_data *data = dev->data;

	uint32_t control_status_register = xlnx_tmrctr_read32(dev, data->clockevent_offset,
							      XTC_TCSR_OFFSET);

	xlnx_tmrctr_write32(dev, data->clockevent_offset,
			    control_status_register | XTC_CSR_INT_OCCURRED_MASK, XTC_TCSR_OFFSET);
}

/**
 * @brief Program timer 0 as a down-counting one-shot clockevent.
 *
 * The next interrupt fires after @p load_cycles clocksource cycles elapse.
 *
 * @param dev Timer device.
 * @param load_cycles Number of clock cycles until the next interrupt.
 */
static void amd_clockevent_program(const struct device *dev, uint32_t load_cycles)
{
	struct xilinx_timer_data *data = dev->data;
	const uint32_t off = data->clockevent_offset;
	uint32_t tcsr;

	load_cycles = MAX(load_cycles, 1U);
	load_cycles = MIN(load_cycles, UINT32_MAX);

	tcsr = xlnx_tmrctr_read32(dev, off, XTC_TCSR_OFFSET);
	tcsr &= ~(uint32_t)XTC_CSR_ENABLE_TMR_MASK;
	xlnx_tmrctr_write32(dev, off, tcsr, XTC_TCSR_OFFSET);

	xlnx_tmrctr_clear_interrupt(dev);

	xlnx_tmrctr_set_reset_value(dev, off, load_cycles);

	/* One-shot down counter: no auto-reload (tickless reprogramming). */
	tcsr = XTC_CSR_ENABLE_INT_MASK | XTC_CSR_DOWN_COUNT_MASK;
	xlnx_tmrctr_write32(dev, off, tcsr | XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, off, tcsr | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);
}

/**
 * @brief Return the accumulated 64-bit hardware cycle count.
 *
 * Handles 32-bit counter wrap by accumulating deltas from the clocksource.
 *
 * @param dev Timer device.
 *
 * @return Accumulated cycle count.
 */
static uint64_t amd_tmrctr_cycles_get_64(const struct device *dev)
{
	uint32_t now = xlnx_tmrctr_read_hw_cycle_count(dev);

	accumulated_cycles += (uint32_t)(now - last_hw_cycles);
	last_hw_cycles = now;

	return accumulated_cycles;
}

/**
 * @brief Timer interrupt service routine.
 *
 * Updates accumulated cycle count, announces elapsed ticks to the kernel,
 * and clears the clockevent interrupt.
 *
 * @param dev Timer device.
 */
static void xlnx_tmrctr_irq_handler(const struct device *dev)
{
	uint64_t cycles;
	uint32_t delta_ticks;
	const struct xilinx_timer_config *config = dev->config;
	k_spinlock_key_t key = k_spin_lock(&amd_timer_lock);

	cycles = amd_tmrctr_cycles_get_64(dev);

	delta_ticks = (uint32_t)((cycles - last_announce) / config->cycles_per_tick);

	last_announce += (uint64_t)delta_ticks * config->cycles_per_tick;

	xlnx_tmrctr_clear_interrupt(dev);

	k_spin_unlock(&amd_timer_lock, key);

	if (sys_dev == dev && delta_ticks > 0U) {
		sys_clock_announce((int32_t)delta_ticks);
	}
}

/**
 * @brief Set the next kernel tick timeout (tickless mode).
 *
 * @param ticks Number of ticks until the next timeout, or K_TICKS_FOREVER.
 * @param idle True if the CPU is entering idle state.
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) || sys_dev == NULL) {
		return;
	}

	const struct xilinx_timer_config *cfg = sys_dev->config;
	const uint32_t cpt = cfg->cycles_per_tick;
	uint32_t cycles;
	int64_t t = ticks;

	if (t < 0) {
		t = 0;
	}

	k_spinlock_key_t key = k_spin_lock(&amd_timer_lock);

	if (ticks == K_TICKS_FOREVER) {
		cycles = cycles_max;
	} else if (t == 0) {
		/*
		 * Kernel: "ASAP". A single AXI cycle is usually << one tick; timer1
		 * may not move a full cpt → sys_clock_announce(0). Arm one tick.
		 */
		cycles = MIN(cpt, cycles_max);
	} else if (t >= (int64_t)(cycles_max / cpt)) {
		cycles = cycles_max;
	} else {
		cycles = (uint32_t)((uint64_t)t * (uint64_t)cpt);
		cycles = MAX(cycles, 1U);
		cycles = MIN(cycles, cycles_max);
	}
	amd_clockevent_program(sys_dev, cycles);

	k_spin_unlock(&amd_timer_lock, key);
}

/**
 * @brief Return the number of ticks elapsed since the last announce.
 *
 * @return Elapsed ticks, or 0 if tickless mode is disabled.
 */
uint32_t sys_clock_elapsed(void)
{
	uint64_t cycles;
	uint32_t elapsed;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) || sys_dev == NULL) {
		return 0;
	}

	const struct xilinx_timer_config *cfg = sys_dev->config;
	const uint32_t cpt = cfg->cycles_per_tick;
	k_spinlock_key_t key = k_spin_lock(&amd_timer_lock);

	cycles = amd_tmrctr_cycles_get_64(sys_dev);
	elapsed = (uint32_t)((cycles - last_announce) / cpt);

	k_spin_unlock(&amd_timer_lock, key);

	return elapsed;
}

/**
 * @brief Return the current system cycle count (32-bit).
 *
 * @return Lower 32 bits of the accumulated cycle count.
 */
uint32_t sys_clock_cycle_get_32(void)
{
	uint64_t cycles;
	k_spinlock_key_t key;

	if (sys_dev == NULL) {
		return 0;
	}

	key = k_spin_lock(&amd_timer_lock);
	cycles = amd_tmrctr_cycles_get_64(sys_dev);
	k_spin_unlock(&amd_timer_lock, key);

	return (uint32_t)cycles;
}

/**
 * @brief Return the current system cycle count (64-bit).
 *
 * @return Accumulated hardware cycle count.
 */
uint64_t sys_clock_cycle_get_64(void)
{
	uint64_t cycles;
	k_spinlock_key_t key;

	if (sys_dev == NULL) {
		return 0;
	}

	key = k_spin_lock(&amd_timer_lock);
	cycles = amd_tmrctr_cycles_get_64(sys_dev);
	k_spin_unlock(&amd_timer_lock, key);

	return cycles;
}

/**
 * @brief Reset all timer counters in the IP block.
 *
 * @param dev Timer device.
 *
 * @retval 0 Always succeeds.
 */
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

/**
 * @brief Write the timer control/status register options.
 *
 * @param dev Timer device.
 * @param counter_number Timer counter offset within the IP block.
 * @param options TCSR value to write.
 */
static inline void xlnx_tmrctr_set_options(const struct device *dev, uint8_t counter_number,
					   uint32_t options)
{
	xlnx_tmrctr_write32(dev, counter_number, options, XTC_TCSR_OFFSET);
}

/**
 * @brief Start both clockevent and clocksource timers (non-tickless mode).
 *
 * @param dev Timer device.
 *
 * @retval 0 Always succeeds.
 */
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

/**
 * @brief Start the free-running clocksource timer only.
 *
 * @param dev Timer device.
 *
 * @retval 0 Always succeeds.
 */
static int xlnx_tmrctr_start_clocksource(const struct device *dev)
{
	struct xilinx_timer_data *data = dev->data;
	uint32_t control_status_register;

	control_status_register = xlnx_tmrctr_read32(dev, data->clocksource_offset,
						     XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, data->clocksource_offset, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(dev, data->clocksource_offset,
			    control_status_register | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);

	return 0;
}

/**
 * @brief Initialize a timer instance as the Zephyr system timer.
 *
 * Timer 0 is used as clockevent and timer 1 as clocksource when two counters
 * are available. The first suitable instance becomes @p sys_dev.
 *
 * @param dev Timer device.
 *
 * @retval 0 Success.
 * @retval negative Error from a sub-initialization step.
 */
static int xilinx_timer_init(const struct device *dev)
{
	int status;
	const struct xilinx_timer_config *config = dev->config;
	struct xilinx_timer_data *data = dev->data;

	LOG_DBG("%s: %d: Timer init at base 0x%lx, IRQ %d, clock %d, one_timer %d",
		__func__, config->instance, config->base, config->irq, config->clock_rate,
		config->one_timer);

	status = xlnx_tmrctr_initialize(dev);
	if (status != 0) {
		return status;
	}

	if (!sys_dev && !config->one_timer) {
		data->clockevent_offset = 0;
		data->clocksource_offset = TIMER_REG_OFFSET;

		xlnx_tmrctr_set_options(dev, data->clocksource_offset, XTC_CSR_AUTO_RELOAD_MASK);

		if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			status = xlnx_tmrctr_start_clocksource(dev);
			if (status != 0) {
				return status;
			}

			amd_clockevent_program(dev, config->cycles_per_tick);
		} else {
			xlnx_tmrctr_set_reset_value(dev, data->clockevent_offset,
						    config->cycles_per_tick);
			xlnx_tmrctr_set_options(dev, data->clockevent_offset,
						XTC_CSR_ENABLE_INT_MASK |
						XTC_CSR_AUTO_RELOAD_MASK |
						XTC_CSR_DOWN_COUNT_MASK);

			status = xlnx_tmrctr_start(dev);
			if (status != 0) {
				return status;
			}
		}

		cycles_max = CYCLES_MAX_COMPUTE(config->cycles_per_tick);

		last_hw_cycles = xlnx_tmrctr_read_hw_cycle_count(dev);
		accumulated_cycles = last_hw_cycles;
		last_announce = (accumulated_cycles / config->cycles_per_tick) *
				config->cycles_per_tick;

		sys_dev = dev;
	}

	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
	}

	return 0;
}

/* Instantiate one timer device per devicetree node. */
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
