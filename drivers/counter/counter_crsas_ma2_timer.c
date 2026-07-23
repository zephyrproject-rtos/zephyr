/*
 * Copyright (c) 2026 Google LLC
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Note that whenever System Timer is mentioned in this file, it refers to
 * ARM CRSAS-MA2 System Timer, and not Zephyr system timer.
 */

#define DT_DRV_COMPAT arm_crsas_ma2_timer

/*
 * The ARM CRSAS-MA2 System Timer supports two modes:
 * 1. Normal one-shot mode:
 *  1a. 64-bit upcounter: Use directly the 64-bit CompareValue register (two 32-bit
 *                        writes).
 *  1b. 32-bit downcounter: Write to the 32-bit TimerValue (TVAL) register, which
 *                          when written causes an update to the CompareValue register.
 *                          where CompareValue = CounterValue + SignExtend(TimerValue).
 *                          Reading TimerValue register returns the count down value:
 *                            TimerValue = CompareValue[31:0] - CounterValue[31:0]
 *
 *  An interrupt is generated when:
 *       CounterValue >= CompareValue
 *
 * 2. Auto-increment mode: Allows generating an interrupt at regular intervals without
 *    needing to reprogram any registers after each interrupt.
 *    Operates as a 64-bit up-counter with a 32-bit offset using the 32-bit ReloadValue
 *    (AIVAL_RELOAD) register.
 *        When AIVAL_RELOAD is written, the 64-bit AutoIncrValue register is loaded
 *        with the value:
 *          AutoIncrValue = ZeroExtend(Reload[31:0])[63:0] + Counter[63:0]
 *
 *    The documentation is unclear/buggy but it seems like the timer condition is met
 *    when CounterValue >= AutoIncrValue.
 *    When the timer condition is met:
 *      * an interrupt is generated, reflected as a bit in the AIVAL_CTL register
 *      * AutoIncrValue is reloaded using same rule as above
 *    When the auto-increment feature is enabled, the normal mode using the CompareValue
 *    and TimerValue registers is disabled. However, though not mentioned in the manual,
 *    TimerValue reads still returns a value and CompareValue retains its last value.
 *    So auto-increment enable likely just disables the interrupt based on the
 *    CompareValue.
 *    Probably a better name for this mode is auto-reload, rather than auto-increment.
 *
 * So how does this map to the Zephyr counter API? The counter API uses set_top_value()
 * for repeating timers, which is best implemented using the auto-increment feature.
 * We could use the one-shot mode, and reprogram the TimerValue at the interrupt, but this
 * would introduce a slight drift with every interrupt, and the slower the processor,
 * the greater the drift will be over time.
 *
 * However, the Zephyr counter API allows a one-shot alarm to be scheduled while
 * a repeating top callback is already scheduled (but not the inverse). Since the
 * HW doesn't allow both the one-shot and auto-reload features to run at the same
 * time, we need to be a bit creative.
 *
 * In top only mode, we use AicrReloadValue and auto-increment/reload mode.
 * In alarm only mode, we disable auto-increment/reload mode and just set TimerValue.
 *
 * If while in top only-mode, a one-shot alarm is requested, we read and save the
 * value of the AicrReloadValue (e.g. the match value of when the reload was supposed
 * to happen next). If it is too soon from now, return an error.
 * Then disable auto-increment/reload mode and set TimerValue to alarm_cfg->ticks.
 * When the isr fires, invoke the alarm callback, then check if a new alarm
 * has been scheduled. If so, just return. If not, then compute the difference
 * between CurrentValue and AicrValue and write that into AicrReload register.
 * This is a temporary reload value. When the next isr fires, we check a
 * flag if we need to reset the AicrReloadValue with the last set top value.
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(crsas_ma2_timer, CONFIG_COUNTER_LOG_LEVEL);

/* Register Offsets within CNTBase */
#define CNTPCT_LO_OFFSET         0x000 /* Physical counter value Low 32-bits */
#define CNTPCT_HI_OFFSET         0x004 /* Physical counter value High 32-bits */
#define CNTFRQ_OFFSET            0x010 /* Counter Frequency register */
#define CNTP_CVAL_LO_OFFSET      0x020 /* Timer CompareValue Low 32-bits */
#define CNTP_CVAL_HI_OFFSET      0x024 /* Timer CompareValue High 32-bits */
#define CNTP_TVAL_OFFSET         0x028 /* Timer Value register */
#define CNTP_CTL_OFFSET          0x02C /* Timer Control register */
#define CNTP_AIVAL_LO_OFFSET     0x040 /* Timer AutoIncr value Low 32-bits */
#define CNTP_AIVAL_HI_OFFSET     0x044 /* Timer AutoIncr value High 32-bits */
#define CNTP_AIVAL_RELOAD_OFFSET 0x048 /* Timer AutoIncrValue reload */
#define CNTP_AIVAL_CTL_OFFSET    0x04C /* Timer AutoIncr Control register */

/* Bits in the CNTP_CTL register */
#define CNTP_CTL_ENABLE_BIT  0 /* Timer enable */
#define CNTP_CTL_IMASK_BIT   1 /* Timer interrupt mask */
#define CNTP_CTL_ISTATUS_BIT 2 /* Timer status: 1 is condition met and
				* interrupt will be generated if IMASK is not set
				*/

/* Bits in the CNTP_AIVAL_CTL register */
#define CNTP_AIVAL_CTL_EN_BIT  0 /* Timer AutoIncrValue enable */
#define CNTP_AIVAL_CTL_CLR_BIT 1 /* Timer AutoIncr interrupt clear bit.
				  * Write 0 to clear AutoIncr interrupt
				  */


struct timer_crsas_ma2_config {
	struct counter_config_info info;
	uintptr_t base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t irqn;
};

struct timer_crsas_ma2_data {
	counter_alarm_callback_t alarm_cb;
	counter_top_callback_t top_cb;
	void *user_data;
	void *top_user_data;
	uint64_t saved_aival;
	uint32_t counter_value_at_start;
	uint32_t top_value;
	uint32_t guard_period;
	uint32_t alarm_after_top;
	bool reload_top;
	bool free_running;
};

static int timer_crsas_ma2_start(const struct device *dev)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;
	const struct device *const sys_counter = DEVICE_DT_GET_ONE(arm_crsas_ma2_counter);

	/* Check that System Counter is initialized.
	 * This is the only time this is checked. It is assumed that System Counter driver
	 * blocks all attempts to stop the counter if System Timer is enabled.
	 */
	if (!device_is_ready(sys_counter)) {
		LOG_WRN_ONCE("ARM CRSAS-MA2 System Counter must be initialized for System Timer "
			     "to be started");
		return -ENODEV;
	}

	/* Start but with interrupts masked since this API is for a
	 * free running timer with no alarm.
	 */
	sys_write32(BIT(CNTP_CTL_ENABLE_BIT) | BIT(CNTP_CTL_IMASK_BIT),
		    config->base + CNTP_CTL_OFFSET);
	data->counter_value_at_start = sys_read32(config->base + CNTPCT_LO_OFFSET);
	LOG_DBG("start: counter_value_at_start = %u", data->counter_value_at_start);
	data->free_running = true;

	return 0;
}

static bool timer_enabled(const struct timer_crsas_ma2_config *config)
{
	uint32_t ctl = sys_read32(config->base + CNTP_CTL_OFFSET);

	return ((ctl & BIT(CNTP_CTL_ENABLE_BIT)) == BIT(CNTP_CTL_ENABLE_BIT));
}

static int timer_crsas_ma2_stop(const struct device *dev)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;

	sys_write32(0, config->base + CNTP_CTL_OFFSET);
	data->free_running = false;

	return 0;
}

static uint32_t ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}

	/* top is not 2^n-1 */
	if (val >= old) {
		/* Both values are <= top, so this won't underflow and
		 * the result is between 0 and top, inclusive.
		 */
		return val - old;
	}

	/* Both values are <= top, and old is strictly bigger than val,
	 * so this won't underflow. It also won't overflow because top
	 * is limited to INT32_MAX through max_top_value.
	 * The result is between 1 and top, inclusive.
	 */
	return val + top + 1U - old;
}

/* Note: physical counter is always incrementing as long as
 * the System Counter is enabled, even if the timer is disabled.
 * the CNTPCT registers are just shadows of the System Counter.
 */
static uint32_t timer_get_value(const struct device *dev)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	uint32_t val = sys_read32(config->base + CNTPCT_LO_OFFSET);
	struct timer_crsas_ma2_data *data = dev->data;
	uint32_t old = data->counter_value_at_start;
	uint32_t top = data->top_value;

	return ticks_sub(val, old, top);
}

static uint64_t get_physical_counter(const struct timer_crsas_ma2_config *config)
{
	uint32_t hi, lo, temp;

	hi = sys_read32(config->base + CNTPCT_HI_OFFSET);
	do {
		temp = hi;
		lo = sys_read32(config->base + CNTPCT_LO_OFFSET);
		hi = sys_read32(config->base + CNTPCT_HI_OFFSET);
	} while (hi != temp);

	return ((uint64_t)hi << 32) | lo;
}

static int timer_crsas_ma2_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct timer_crsas_ma2_config *config = dev->config;

	if (!timer_enabled(config)) {
		/* Timer is not running */
		*ticks = 0;
		LOG_ERR("timer is not running");
		return -ENOTSUP;
	}

	*ticks = timer_get_value(dev);
	LOG_DBG("get_value API %u", *ticks);
	return 0;
}

static uint32_t timer_crsas_ma2_get_top_value(const struct device *dev)
{
	struct timer_crsas_ma2_data *data = dev->data;

	return data->top_value;
}

static uint32_t timer_crsas_ma2_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct timer_crsas_ma2_data *data = dev->data;

	return data->guard_period;
}

static int timer_crsas_ma2_set_guard_period(const struct device *dev, uint32_t guard,
					    uint32_t flags)
{
	struct timer_crsas_ma2_data *data = dev->data;

	__ASSERT_NO_MSG(guard < data->top_value);

	data->guard_period = guard;
	return 0;
}

static int timer_crsas_ma2_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;
	uint32_t relative_ticks;
	uint32_t now = timer_get_value(dev);

	if (chan_id != 0) {
		return -ENOTSUP;
	}

	/* The test for counters expects -EINVAL to be returned if the alarm_cfg
	 * ticks is > top.
	 * The value written to the register is 32-bit number that is sign-extended
	 * and added to 64-bit counter value, so ticks can't be > INT32_MAX,
	 * which is why max_top_value isn't set to UINT32_MAX, even though
	 * set_top_value() supports it.
	 */
	if (alarm_cfg->ticks > data->top_value) {
		return -EINVAL;
	}

	if (data->alarm_cb != NULL) {
		return -EBUSY;
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;
	relative_ticks = alarm_cfg->ticks;

	if (!timer_enabled(config)) {
		/* Record counter value when we started timer */
		data->counter_value_at_start = sys_read32(config->base + CNTPCT_LO_OFFSET);
		LOG_DBG("set_alarm: counter_value_at_start = %u", data->counter_value_at_start);
	}

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		/* Compute relative_ticks by computing the difference
		 * between the alarm_cfg->ticks and current timer value.
		 */

		relative_ticks = ticks_sub(alarm_cfg->ticks, now, data->top_value);
		LOG_DBG("abs alarm ticks %u, now %u, relative_ticks = %u, top %u, guard %u",
			alarm_cfg->ticks, now, relative_ticks, data->top_value, data->guard_period);
		if (relative_ticks > (data->top_value - data->guard_period) ||
		    (alarm_cfg->ticks == now && data->guard_period > 0)) {
			if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
				/* Late, trigger interrupt */
				NVIC_SetPendingIRQ(config->irqn);
			} else {
				data->alarm_cb = NULL;
			}
			return -ETIME;
		}
	}

	if (data->top_cb != NULL) {
		/* Let ISR handle a case where alarm is set to happen after next turnaround */
		if (relative_ticks > (data->top_value + 1 - now)) {
			data->alarm_after_top = relative_ticks - (data->top_value + 1 - now);
			return 0;
		}

		/* top_cb being set means we're in auto-increment mode.
		 * we have to save the AutoIncrementValue and then
		 * disable auto-increment mode and switch to normal mode.
		 */
		uint32_t aival_lo = sys_read32(config->base + CNTP_AIVAL_LO_OFFSET);
		uint32_t aival_hi = sys_read32(config->base + CNTP_AIVAL_HI_OFFSET);

		data->saved_aival = ((uint64_t)aival_hi << 32) | (uint64_t)aival_lo;
		sys_write32(0, config->base + CNTP_AIVAL_CTL_OFFSET);
	}

	/* Enable timer and clear CTL.IMASK to enable interrupt. If already
	 * enabled, it's still faster to just write than read-modify write
	 * to clear IMASK.
	 */
	sys_write32(relative_ticks, config->base + CNTP_TVAL_OFFSET);
	sys_write32(BIT(CNTP_CTL_ENABLE_BIT), config->base + CNTP_CTL_OFFSET);

	return 0;
}

static int timer_crsas_ma2_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;

	/* This API is used for periodic callbacks, as opposed to the
	 * alarm API which is a oneshot.
	 */

	if (data->alarm_cb) {
		LOG_ERR("Alarm currently active");
		return -EBUSY;
	}

	data->reload_top = false;

	if (!(top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		data->counter_value_at_start = sys_read32(config->base + CNTPCT_LO_OFFSET);
		LOG_DBG("set_top: reset counter_value_at_start = %u", data->counter_value_at_start);
	} else if (timer_get_value(dev) >= top_cfg->ticks) {
		if (top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			data->counter_value_at_start = sys_read32(config->base + CNTPCT_LO_OFFSET);
			LOG_ERR("set_top: reset when late counter_value_at_start = %u",
				data->counter_value_at_start);
		}
		return -ETIME;
	}

	data->top_value = top_cfg->ticks;

	if (top_cfg->callback == NULL) {
		/* Stopping periodic timer by disabling auto-increment.
		 * Clear callback first. In the unlikely event of interrupt being generated
		 * between the next few lines, the ISR checks the callback pointer and re-enables
		 * auto-increment operation if it is not NULL. If it is NULL and auto-increment
		 * interrupt is generated, the ISR simply does nothing.
		 * User data pointer can be cleared as last, it isn't used without a callback.
		 */
		data->top_cb = NULL;
		sys_write32(0, config->base + CNTP_AIVAL_CTL_OFFSET);
		data->top_user_data = NULL;

		if (!data->free_running) {
			/* Disable timer to possibly save power */
			sys_write32(0, config->base + CNTP_CTL_OFFSET);
		}

		return 0;
	}

	/* Starting/changing periodic timer */

	/* Disable timer */
	sys_write32(0, config->base + CNTP_CTL_OFFSET);
	/* Disable auto-increment */
	sys_write32(0, config->base + CNTP_AIVAL_CTL_OFFSET);

	data->top_cb = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	/* Set the auto-increment reload value */
	sys_write32(data->top_value, config->base + CNTP_AIVAL_RELOAD_OFFSET);

	/* Enable auto-increment mode */
	sys_write32(BIT(CNTP_AIVAL_CTL_EN_BIT), config->base + CNTP_AIVAL_CTL_OFFSET);

	/* Enable timer and unmask interrupt */
	sys_write32(BIT(CNTP_CTL_ENABLE_BIT), config->base + CNTP_CTL_OFFSET);

	return 0;
}

static uint32_t timer_crsas_ma2_get_pending_int(const struct device *dev)
{
	const struct timer_crsas_ma2_config *config = dev->config;

	uint32_t ctl = sys_read32(config->base + CNTP_CTL_OFFSET);

	return (ctl & BIT(CNTP_CTL_ISTATUS_BIT)) != 0;
}

static void restore_auto_increment_after_alarm(const struct timer_crsas_ma2_config *config,
					       struct timer_crsas_ma2_data *data)
{
	/* Restore auto_increment, but set AIVAL_RELOAD with the time remaining */
	uint32_t ticks_until_top = data->saved_aival - get_physical_counter(config);

	sys_write32(ticks_until_top, config->base + CNTP_AIVAL_RELOAD_OFFSET);
	/* Request the isr to restore original top value */
	data->reload_top = true;
	/* Enable auto-increment mode */
	sys_write32(BIT(CNTP_AIVAL_CTL_EN_BIT), config->base + CNTP_AIVAL_CTL_OFFSET);
	/* Enable timer and unmask interrupt */
	sys_write32(BIT(CNTP_CTL_ENABLE_BIT), config->base + CNTP_CTL_OFFSET);
}

static int timer_crsas_ma2_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;

	if (chan_id != 0) {
		return -ENOTSUP;
	}

	if (data->alarm_cb == NULL) {
		/* Already done, API test says to return 0 */
		return 0;
	}

	/* Disable timer and mask interrupt to prevent race as we modify data->alarm_cb */
	sys_write32(BIT(CNTP_CTL_IMASK_BIT), config->base + CNTP_CTL_OFFSET);

	data->alarm_cb = NULL;
	data->user_data = NULL;
	if (data->top_cb) {
		restore_auto_increment_after_alarm(config, data);
	} else if (data->free_running) {
		/* Enable timer but in freee running (interrupt masked) mode */
		sys_write32(BIT(CNTP_CTL_ENABLE_BIT) | BIT(CNTP_CTL_IMASK_BIT),
			    config->base + CNTP_CTL_OFFSET);
	}

	return 0;
}

static uint32_t timer_crsas_ma2_get_freq(const struct device *dev)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	uint32_t clk_freq = 0;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clk_freq)) {
		LOG_ERR("unable to get clock frequency");
		return 0;
	}
	return clk_freq;
}

static void timer_crsas_ma2_isr(const struct device *dev)
{
	const struct timer_crsas_ma2_config *config = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;

	if (data->reload_top) {
		sys_write32(data->top_value, config->base + CNTP_AIVAL_RELOAD_OFFSET);
		data->reload_top = false;
	}

	if (data->free_running) {
		if (data->top_cb != NULL) {
			/* Keep timer "running" but leave interrupt enabled for top_cb. */
			sys_write32(BIT(CNTP_CTL_ENABLE_BIT), config->base + CNTP_CTL_OFFSET);
		} else {
			/* Keep timer "running" but clear and mask interrupt. */
			sys_write32(BIT(CNTP_CTL_ENABLE_BIT) | BIT(CNTP_CTL_IMASK_BIT),
				    config->base + CNTP_CTL_OFFSET);
		}
	} else if (data->top_cb == NULL) {
		/* Disable timer. This also clears normal interrupts. */
		sys_write32(0, config->base + CNTP_CTL_OFFSET);
	}

	if (data->alarm_cb && data->alarm_after_top == 0) {
		counter_alarm_callback_t alarm_cb = data->alarm_cb;
		void *alarm_user_data = data->user_data;
		uint32_t now = timer_get_value(dev);

		data->alarm_cb = NULL;
		data->user_data = NULL;

		/* Invoke alarm_callback. Callback can set another alarm. */
		alarm_cb(dev, 0, now, alarm_user_data);

		if ((data->alarm_cb == NULL) && (data->top_cb != NULL)) {
			/* If no new alarm, but we have a top_cb, have
			 * to restore auto_increment mode.
			 */
			restore_auto_increment_after_alarm(config, data);
		}
	} else if (data->top_cb) {
		/* No alarm, but we have a top_cb, this should be the
		 * auto-increment interrupt.
		 */

		if (data->alarm_after_top == 0) {
			/* Clear the AIVAL interrupt by writing to register. Faster
			 * to just write entire register with the enable bit set
			 * and 0 to the CLR bit, instead of read-modify-write.
			 */
			sys_write32(BIT(CNTP_AIVAL_CTL_EN_BIT),
				    config->base + CNTP_AIVAL_CTL_OFFSET);
		} else {
			/* alarm_after_top being set means the alarm will expire in this turnaround.
			 * We have to save the AutoIncrementValue and then disable auto-increment
			 * mode and switch to normal mode.
			 */
			uint32_t aival_lo = sys_read32(config->base + CNTP_AIVAL_LO_OFFSET);
			uint32_t aival_hi = sys_read32(config->base + CNTP_AIVAL_HI_OFFSET);

			data->saved_aival = ((uint64_t)aival_hi << 32) | (uint64_t)aival_lo;
			sys_write32(0, config->base + CNTP_AIVAL_CTL_OFFSET);

			/* Enable timer and clear CTL.IMASK to enable interrupt. If already
			 * enabled, it's still faster to just write than read-modify write
			 * to clear IMASK.
			 */
			sys_write32(data->alarm_after_top, config->base + CNTP_TVAL_OFFSET);
			sys_write32(BIT(CNTP_CTL_ENABLE_BIT), config->base + CNTP_CTL_OFFSET);
			data->alarm_after_top = 0;
		}

		/* Invoke top callback but don't disable. It's supposed to be periodic.
		 * Auto-increment mode in the System Timer will use the reload register
		 * to set the next match.
		 */
		data->top_cb(dev, data->top_user_data);
	}
}

static DEVICE_API(counter, timer_crsas_ma2_api) = {
	.start = timer_crsas_ma2_start,
	.stop = timer_crsas_ma2_stop,
	.get_value = timer_crsas_ma2_get_value,
	.set_alarm = timer_crsas_ma2_set_alarm,
	.cancel_alarm = timer_crsas_ma2_cancel_alarm,
	.set_top_value = timer_crsas_ma2_set_top_value,
	.get_pending_int = timer_crsas_ma2_get_pending_int,
	.get_top_value = timer_crsas_ma2_get_top_value,
	.get_guard_period = timer_crsas_ma2_get_guard_period,
	.set_guard_period = timer_crsas_ma2_set_guard_period,
	.get_freq = timer_crsas_ma2_get_freq,
};

static int timer_crsas_ma2_init(const struct device *dev)
{
	const struct timer_crsas_ma2_config *cfg = dev->config;
	struct timer_crsas_ma2_data *data = dev->data;

	/* Make sure timer and auto-increment mode are disabled */
	sys_write32(0, cfg->base + CNTP_CTL_OFFSET);
	sys_write32(0, cfg->base + CNTP_AIVAL_CTL_OFFSET);

	cfg->irq_config_func(dev);

	data->top_value = cfg->info.max_top_value;

	return 0;
}

#define TIMER_CRSAS_MA2_DEVICE(n)                                                                  \
	static void timer_crsas_ma2_irq_config_##n(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), timer_crsas_ma2_isr,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct timer_crsas_ma2_config timer_crsas_ma2_config_##n = {                  \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = INT32_MAX,                                        \
				.freq = 0, /* not used, we implement the get_freq API */           \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)(DT_INST_CLOCKS_CELL(n, clkid)),           \
		.irq_config_func = timer_crsas_ma2_irq_config_##n,                                 \
		.irqn = DT_INST_IRQN(n),                                                           \
	};                                                                                         \
	static struct timer_crsas_ma2_data timer_crsas_ma2_data_##n;                               \
	DEVICE_DT_INST_DEFINE(n, timer_crsas_ma2_init, NULL, &timer_crsas_ma2_data_##n,            \
			      &timer_crsas_ma2_config_##n, POST_KERNEL,                            \
			      CONFIG_COUNTER_INIT_PRIORITY, &timer_crsas_ma2_api);

DT_INST_FOREACH_STATUS_OKAY(TIMER_CRSAS_MA2_DEVICE)
