/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Counter driver for Infineon TCPWM peripheral. */

#define DT_DRV_COMPAT infineon_tcpwm_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/timer/ifx_tcpwm.h>
#include <zephyr/dt-bindings/pinctrl/ifx_cat1-pinctrl.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_tcpwm_counter, CONFIG_COUNTER_LOG_LEVEL);

#include <cy_sysclk.h>
#include <cy_tcpwm_counter.h>

struct ifx_tcpwm_counter_config {
	struct counter_config_info counter_info;
	TCPWM_GRP_CNT_Type *reg_base;
	uint32_t index;
	bool resolution_32_bits;
	IRQn_Type irq_num;
	cy_en_divider_types_t divider_type;
	uint32_t divider_sel;
	uint32_t divider_val;
	void (*irq_enable_func)(const struct device *dev);
};

struct ifx_tcpwm_counter_data {
	bool alarm_irq_flag;
	cy_israddress callback;
	/* Timer/counter comparison value */
	uint32_t compare_value;
	/* Default value of the timer/counter. */
	uint32_t value;
	struct counter_alarm_cfg alarm_cfg;
	struct counter_top_cfg top_value_cfg_counter;
	uint32_t guard_period;
	struct ifx_cat1_clock clock;
	uint8_t clock_peri_group;
};

static const cy_stc_tcpwm_counter_config_t counter_default_config = {
	.period = 32768,
	.clockPrescaler = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
	.runMode = CY_TCPWM_COUNTER_CONTINUOUS,
	.countDirection = CY_TCPWM_COUNTER_COUNT_UP,
	.compareOrCapture = CY_TCPWM_COUNTER_MODE_COMPARE,
	.compare0 = 16384,
	.compare1 = 16384,
	.enableCompareSwap = false,
	.interruptSources = CY_TCPWM_INT_NONE,
	.captureInputMode = 0x3U,
	.captureInput = CY_TCPWM_INPUT_0,
	.reloadInputMode = 0x3U,
	.reloadInput = CY_TCPWM_INPUT_0,
	.startInputMode = 0x3U,
	.startInput = CY_TCPWM_INPUT_0,
	.stopInputMode = 0x3U,
	.stopInput = CY_TCPWM_INPUT_0,
	.countInputMode = 0x3U,
	.countInput = CY_TCPWM_INPUT_1,
};

typedef enum {
	/* No interrupt handled */
	COUNTER_IRQ_NONE = 0,
	/* Interrupt when terminal count is reached */
	COUNTER_IRQ_TERMINAL_COUNT = 1 << 0,
	/* Interrupt when Compare/Capture value is reached */
	COUNTER_IRQ_CAPTURE_COMPARE = 1 << 1,
	/* Interrupt on terminal count and Compare/Capture values */
	COUNTER_IRQ_ALL = (1 << 2) - 1,
} counter_event_t;

static void counter_enable_event(const struct device *dev, counter_event_t event, bool enable)
{
	const struct ifx_tcpwm_counter_config *const config = dev->config;
	uint32_t savedIntrStatus = Cy_SysLib_EnterCriticalSection();
	uint32_t old_mask = IFX_TCPWM_GetInterruptMask(config->reg_base);
	uint32_t new_event;

	if (enable) {
		/* Clear any newly enabled events so that old IRQs don't trigger ISRs */
		IFX_TCPWM_ClearInterrupt(config->reg_base, ~old_mask & event);
	}

	new_event = enable ? (old_mask | event) : (old_mask & ~event);
	IFX_TCPWM_SetInterruptMask(config->reg_base, new_event);

	Cy_SysLib_ExitCriticalSection(savedIntrStatus);
}

static void counter_isr_handler(const struct device *dev)
{
	struct ifx_tcpwm_counter_data *const data = dev->data;
	const struct ifx_tcpwm_counter_config *const config = dev->config;
	uint32_t pending_int;

	pending_int = IFX_TCPWM_GetInterruptStatusMasked(config->reg_base);
	IFX_TCPWM_ClearInterrupt(config->reg_base, pending_int);
	NVIC_ClearPendingIRQ(config->irq_num);

	/* Alarm compare/capture interrupt */
	if ((data->alarm_cfg.callback != NULL) &&
	    (((COUNTER_IRQ_CAPTURE_COMPARE & pending_int) == COUNTER_IRQ_CAPTURE_COMPARE) ||
	     data->alarm_irq_flag)) {
		/* Alarm works as one-shot, so disable interrupt */
		counter_enable_event(dev, COUNTER_IRQ_CAPTURE_COMPARE, false);

		/* Call User callback for Alarm */
		data->alarm_cfg.callback(dev, 1, IFX_TCPWM_Counter_GetCounter(config->reg_base),
					 data->alarm_cfg.user_data);
		data->alarm_irq_flag = false;
	}

	/* Top_value terminal count interrupt */
	if ((data->top_value_cfg_counter.callback != NULL) &&
	    ((COUNTER_IRQ_TERMINAL_COUNT & pending_int) == COUNTER_IRQ_TERMINAL_COUNT)) {

		/* Call User callback for top value */
		data->top_value_cfg_counter.callback(dev, data->top_value_cfg_counter.user_data);
	}
}

static int ifx_tcpwm_counter_init(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	cy_rslt_t rslt;
	struct ifx_tcpwm_counter_data *const data = dev->data;
	const struct ifx_tcpwm_counter_config *config = dev->config;
	uint32_t clk_connection;
	cy_stc_tcpwm_counter_config_t counter_config = counter_default_config;

	/* Calculate clock connection based on TCPWM index */
	if (config->resolution_32_bits) {
		clk_connection = PCLK_TCPWM0_CLOCK_COUNTER_EN0 + config->index;
	} else {
		clk_connection = PCLK_TCPWM0_CLOCK_COUNTER_EN256 + config->index;
	}
	/* Configure PWM clock */
	Cy_SysClk_PeriPclkDisableDivider((en_clk_dst_t)clk_connection, config->divider_type,
					 config->divider_sel);
	Cy_SysClk_PeriPclkSetDivider((en_clk_dst_t)clk_connection, config->divider_type,
				     config->divider_sel, config->divider_val);
	Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)clk_connection, config->divider_type,
					config->divider_sel);

	Cy_SysClk_PeriPclkAssignDivider(clk_connection, config->divider_type, config->divider_sel);

	/* Initialize counter structure */
	data->alarm_irq_flag = false;
	data->top_value_cfg_counter.ticks = config->counter_info.max_top_value;
	data->compare_value = 0;
	data->value = 0;

	/* Configure timer */
	counter_config.period = data->top_value_cfg_counter.ticks;
	counter_config.compare0 = data->compare_value;

	/* DeInit will clear the interrupt mask; save it now and restore after we re-nit */
	uint32_t old_mask = IFX_TCPWM_GetInterruptMask(config->reg_base);

	IFX_TCPWM_Counter_DeInit(config->reg_base, &counter_config);

	rslt = (cy_rslt_t)IFX_TCPWM_Counter_Init(config->reg_base, &counter_config);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	IFX_TCPWM_Counter_Enable(config->reg_base);
	IFX_TCPWM_SetInterruptMask(config->reg_base, old_mask);

	/* This must be called after IFX_TCPWM_Counter_Init */
	IFX_TCPWM_Counter_SetCounter(config->reg_base, data->value);

	/* enable the counter interrupt */
	config->irq_enable_func(dev);

	return 0;
}

static int ifx_tcpwm_counter_start(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	const struct ifx_tcpwm_counter_config *config = dev->config;

	IFX_TCPWM_Counter_Enable(config->reg_base);
	IFX_TCPWM_TriggerStart_Single(config->reg_base);

	return 0;
}

static int ifx_tcpwm_counter_stop(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	const struct ifx_tcpwm_counter_config *config = dev->config;

	IFX_TCPWM_Counter_Disable(config->reg_base);

	return 0;
}

static int ifx_tcpwm_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(ticks != NULL);

	const struct ifx_tcpwm_counter_config *config = dev->config;

	*ticks = IFX_TCPWM_Counter_GetCounter(config->reg_base);

	return 0;
}

static int ifx_tcpwm_counter_set_top_value(const struct device *dev,
					   const struct counter_top_cfg *cfg)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	struct ifx_tcpwm_counter_data *const data = dev->data;
	const struct ifx_tcpwm_counter_config *const config = dev->config;

	data->top_value_cfg_counter = *cfg;

	/* Check new top value limit */
	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	/* Checks if new period value is not less then old period value */
	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		data->value = 0u;
	} else {
		/* timer_configure resets timer counter register to value
		 * defined in config structure 'data->value', so update
		 * counter value with current value of counter (read by
		 * IFX_TCPWM_Counter_GetCounter function).
		 */
		data->value = IFX_TCPWM_Counter_GetCounter(config->reg_base);
	}

	IFX_TCPWM_Block_SetPeriod(config->reg_base, cfg->ticks);

	/* Register an top_value terminal count event callback handler if
	 * callback is not NULL.
	 */
	if (cfg->callback != NULL) {
		counter_enable_event(dev, COUNTER_IRQ_TERMINAL_COUNT, true);
	}

	return 0;
}

static uint32_t ifx_tcpwm_counter_get_top_value(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_tcpwm_counter_data *const data = dev->data;

	return data->top_value_cfg_counter.ticks;
}

static inline bool counter_is_bit_mask(uint32_t val)
{
	/* Return true if value equals 2^n - 1 */
	return !(val & (val + 1U));
}

static uint32_t counter_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;

	/* refer to https://tbrindus.ca/how-builtin-expect-works/ for 'likely' usage */
	if (likely(counter_is_bit_mask(top))) {
		return (val1 + val2) & top;
	}

	to_top = top - val1;

	return (val2 <= to_top) ? (val1 + val2) : (val2 - to_top - 1U);
}

static uint32_t counter_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	/* refer to https://tbrindus.ca/how-builtin-expect-works/ for 'likely' usage */
	if (likely(counter_is_bit_mask(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : (val + top + 1U - old);
}

static int ifx_tcpwm_counter_set_alarm(const struct device *dev, uint8_t chan_id,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(alarm_cfg != NULL);

	struct ifx_tcpwm_counter_data *const data = dev->data;
	const struct ifx_tcpwm_counter_config *const config = dev->config;

	uint32_t compare_value = alarm_cfg->ticks;
	uint32_t top_val = ifx_tcpwm_counter_get_top_value(dev);
	uint32_t flags = alarm_cfg->flags;
	uint32_t max_rel_val;
	bool absolute = ((flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) ? false : true;
	bool irq_on_late;

	data->alarm_cfg = *alarm_cfg;

	/* Checks if compare value is not less then period value */
	if (alarm_cfg->ticks > top_val) {
		return -EINVAL;
	}

	if (absolute) {
		max_rel_val = top_val - data->guard_period;
		irq_on_late = ((flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) == 0) ? false : true;
	} else {
		/* If relative value is smaller than half of the counter range it is assumed
		 * that there is a risk of setting value too late and late detection algorithm
		 * must be applied. When late setting is detected, interrupt shall be
		 * triggered for immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CC and counter.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = compare_value < (top_val / 2U);

		/* limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? (top_val / 2U) : top_val;
		compare_value = counter_ticks_add(IFX_TCPWM_Counter_GetCounter(config->reg_base),
						  compare_value, top_val);
	}

	/* Decrement value to detect the case when compare_value == counter_read(dev). Otherwise,
	 * condition would need to include comparing diff against 0.
	 */
	uint32_t curr = IFX_TCPWM_Counter_GetCounter(config->reg_base);
	uint32_t diff = counter_ticks_sub((compare_value - 1), curr, top_val);

	if ((absolute && (compare_value < curr)) || (diff > max_rel_val)) {

		/* Interrupt is triggered always for relative alarm and for absolute depending
		 * on the flag.
		 */
		if (irq_on_late) {
			data->alarm_irq_flag = true;
			counter_enable_event(dev, COUNTER_IRQ_CAPTURE_COMPARE, true);
			IFX_TCPWM_SetInterrupt(config->reg_base, COUNTER_IRQ_CAPTURE_COMPARE);
		}

		if (absolute) {
			return -ETIME;
		}
	} else {
		/* Setting new compare value
		 *
		 * timer_configure resets timer counter register to value
		 * defined in config structure 'data->value', so update
		 * counter value with current value of counter (read by
		 * IFX_TCPWM_Counter_GetCounter function).
		 */
		data->value = IFX_TCPWM_Counter_GetCounter(config->reg_base);
		data->compare_value = compare_value;

		/* Reconfigure timer */
		IFX_TCPWM_Block_SetCC0Val(config->reg_base, compare_value);

		counter_enable_event(dev, COUNTER_IRQ_CAPTURE_COMPARE, true);
	}

	return 0;
}

static int ifx_tcpwm_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	__ASSERT_NO_MSG(dev != NULL);

	counter_enable_event(dev, COUNTER_IRQ_CAPTURE_COMPARE, false);
	return 0;
}

static uint32_t ifx_tcpwm_counter_get_pending_int(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	const struct ifx_tcpwm_counter_config *const config = dev->config;

	return NVIC_GetPendingIRQ(config->irq_num);
}

static uint32_t ifx_tcpwm_counter_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_tcpwm_counter_data *const data = dev->data;

	return data->guard_period;
}

static int ifx_tcpwm_counter_set_guard_period(const struct device *dev, uint32_t guard,
					      uint32_t flags)
{
	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(guard < ifx_tcpwm_counter_get_top_value(dev));

	struct ifx_tcpwm_counter_data *const data = dev->data;

	data->guard_period = guard;
	return 0;
}

static DEVICE_API(counter, counter_api) = {
	.start = ifx_tcpwm_counter_start,
	.stop = ifx_tcpwm_counter_stop,
	.get_value = ifx_tcpwm_counter_get_value,
	.set_alarm = ifx_tcpwm_counter_set_alarm,
	.cancel_alarm = ifx_tcpwm_counter_cancel_alarm,
	.set_top_value = ifx_tcpwm_counter_set_top_value,
	.get_pending_int = ifx_tcpwm_counter_get_pending_int,
	.get_top_value = ifx_tcpwm_counter_get_top_value,
	.get_guard_period = ifx_tcpwm_counter_get_guard_period,
	.set_guard_period = ifx_tcpwm_counter_set_guard_period,
};

#define DT_INST_GET_CYHAL_GPIO_OR(inst, gpios_prop, default)                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, gpios_prop),                                       \
		    (DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_INST(inst, DT_DRV_COMPAT), gpios_prop)),   \
		    (default))

/* Counter driver init macros */
#define INFINEON_TCPWM_COUNTER_INIT(n)                                                             \
                                                                                                   \
	static void ifx_cat1_spi_irq_enable_func_##n(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    counter_isr_handler, DEVICE_DT_INST_GET(n), 0);                        \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}                                                                                          \
                                                                                                   \
	static struct ifx_tcpwm_counter_data ifx_tcpwm_counter##n##_data = {                       \
		.clock =                                                                           \
			{                                                                          \
				.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                         \
					DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), clk_dst, 1),    \
					DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),             \
				.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, div_num),            \
			},                                                                         \
		.clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), clk_dst, 1),        \
	};                                                                                         \
                                                                                                   \
	static const struct ifx_tcpwm_counter_config ifx_tcpwm_counter##n##_config = {             \
		.counter_info = {.max_top_value = (DT_PROP(DT_INST_PARENT(n), resolution) == 32)   \
							  ? UINT32_MAX                             \
							  : UINT16_MAX,                            \
				 .freq = DT_INST_PROP(n, clock_frequency),                         \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
		.reg_base = (TCPWM_GRP_CNT_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),                  \
		.index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                         \
			  DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                             \
			 DT_REG_SIZE(DT_INST_PARENT(n)),                                           \
		.irq_num = DT_IRQN(DT_INST_PARENT(n)),                                             \
		.resolution_32_bits =                                                              \
			(DT_PROP(DT_INST_PARENT(n), resolution) == 32) ? true : false,             \
		.divider_type = DT_PROP(DT_INST_PARENT(n), divider_type),                          \
		.divider_sel = DT_PROP(DT_INST_PARENT(n), divider_sel),                            \
		.divider_val = DT_PROP(DT_INST_PARENT(n), divider_val),                            \
		.irq_enable_func = ifx_cat1_spi_irq_enable_func_##n,                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_counter_init, NULL, &ifx_tcpwm_counter##n##_data,       \
			      &ifx_tcpwm_counter##n##_config, PRE_KERNEL_1,                        \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_TCPWM_COUNTER_INIT);
