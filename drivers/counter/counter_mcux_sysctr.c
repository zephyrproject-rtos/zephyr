/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sysctr

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

#include <fsl_sysctr.h>

LOG_MODULE_REGISTER(mcux_sysctr, CONFIG_COUNTER_LOG_LEVEL);

/* SYS_CTR has two compare frames, each capable of generating one interrupt. */
#define SYSCTR_NUM_CHANNELS 2

/*
 * The slow clock (32 kHz LPO) is divided by 64 inside the SYS_CTR hardware
 * before it drives the counter. The clock_control driver returns the raw LPO
 * rate, so we apply this factor to get the actual counter tick rate.
 */
#define SYSCTR_SLOW_CLK_DIV 64U

/* Maximum tick value for the 56-bit SYS_CTR hardware counter. */
#define SYSCTR_MAX_TICKS ((1ULL << 56) - 1)

struct mcux_sysctr_chan_data {
	counter_alarm_callback_t    callback;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	counter_alarm_callback_64_t callback_64;
#endif
	void *user_data;
};

struct mcux_sysctr_config {
	struct counter_config_info info;
	SYS_CTR_CONTROL_Type *ctrl_base;
	SYS_CTR_READ_Type    *read_base;
	SYS_CTR_COMPARE_Type *cmp_base;
	const struct device  *clock_dev;
	clock_control_subsys_t base_clock_subsys;
	clock_control_subsys_t slow_clock_subsys;
	sysctr_clock_source_t  clock_source;
	unsigned int irqn;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_sysctr_data {
	uint32_t clock_frequency;
	uint32_t guard_period;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	uint64_t guard_period_64;
#endif
	struct k_spinlock lock;
	atomic_t irq_pending;
	struct mcux_sysctr_chan_data channels[SYSCTR_NUM_CHANNELS];
};

static int mcux_sysctr_start(const struct device *dev)
{
	const struct mcux_sysctr_config *config = dev->config;

	SYSCTR_StartCounter(config->ctrl_base);

	return 0;
}

static int mcux_sysctr_stop(const struct device *dev)
{
	const struct mcux_sysctr_config *config = dev->config;

	SYSCTR_StopCounter(config->ctrl_base);

	return 0;
}

/*
 * The SYS_CTR hardware counter is 56 bits wide. The Zephyr counter API uses
 * 32-bit tick values, so we expose the lower 32 bits of the hardware counter.
 */
static int mcux_sysctr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_sysctr_config *config = dev->config;

	*ticks = (uint32_t)SYSCTR_GetCounterlValue(config->read_base);

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int mcux_sysctr_get_value_64(const struct device *dev, uint64_t *ticks)
{
	const struct mcux_sysctr_config *config = dev->config;

	*ticks = SYSCTR_GetCounterlValue(config->read_base);

	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static uint32_t mcux_sysctr_get_top_value(const struct device *dev)
{
	const struct mcux_sysctr_config *config = dev->config;

	return config->info.max_top_value;
}

/*
 * Reset the counter to zero, stopping and restarting it if it was running.
 * Returns -ENOTSUP when the clock source does not allow counter writes.
 */
static int mcux_sysctr_reset_counter(const struct mcux_sysctr_config *config)
{
	bool was_running;

	if (config->clock_source != kSYSCTR_BaseFrequency) {
		return -ENOTSUP;
	}

	was_running = (config->ctrl_base->CNTCR & SYS_CTR_CONTROL_CNTCR_EN_MASK) != 0U;
	if (was_running) {
		SYSCTR_StopCounter(config->ctrl_base);
	}

	SYSCTR_SetCounterlValue(config->ctrl_base, 0U);

	if (was_running) {
		SYSCTR_StartCounter(config->ctrl_base);
	}

	return 0;
}

static int mcux_sysctr_set_top_value(const struct device *dev,
				     const struct counter_top_cfg *cfg)
{
	const struct mcux_sysctr_config *config = dev->config;

	/* SYS_CTR is a free-running counter; only UINT32_MAX is supported. */
	if (cfg->ticks != config->info.max_top_value) {
		return -ENOTSUP;
	}

	/*
	 * Keep the fixed top value but honor counter API reset semantics:
	 * when DONT_RESET is not set, clear the counter value.
	 */
	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		return mcux_sysctr_reset_counter(config);
	}

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static uint64_t mcux_sysctr_get_top_value_64(const struct device *dev)
{
	ARG_UNUSED(dev);

	return SYSCTR_MAX_TICKS;
}

static int mcux_sysctr_set_top_value_64(const struct device *dev,
					const struct counter_top_cfg_64 *cfg)
{
	const struct mcux_sysctr_config *config = dev->config;

	/* SYS_CTR is a free-running counter; only SYSCTR_MAX_TICKS is supported. */
	if (cfg->ticks != SYSCTR_MAX_TICKS) {
		return -ENOTSUP;
	}

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		return mcux_sysctr_reset_counter(config);
	}

	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */


static int mcux_sysctr_set_alarm(const struct device *dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_sysctr_config *config = dev->config;
	struct mcux_sysctr_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t flags = alarm_cfg->flags;
	uint32_t top_val = config->info.max_top_value;
	uint64_t current;
	uint64_t compare_val;
	uint32_t now;
	uint32_t guard;
	bool irq_on_late;
	int err = 0;
	k_spinlock_key_t key;

	if (chan_id >= SYSCTR_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %d", chan_id);
		return -EINVAL;
	}

	/*
	 * Programming compare values must only be called while the counter is
	 * running on the base frequency.
	 */
	if (config->clock_source != kSYSCTR_BaseFrequency) {
		LOG_ERR("set_alarm is not supported in alternate clock mode");
		return -ENOTSUP;
	}

	current = SYSCTR_GetCounterlValue(config->read_base);
	now = (uint32_t)current;

	if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		__ASSERT_NO_MSG(data->guard_period < top_val);
		guard = data->guard_period;
		irq_on_late = !!(flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE);
	} else {
		/*
		 * If relative value is smaller than half of the counter range
		 * it is assumed that there is a risk of setting value too late
		 * and late detection algorithm must be applied. When late
		 * setting is detected, interrupt shall be triggered for
		 * immediate expiration of the timer.
		 */
		irq_on_late = ticks < (top_val / 2U);
		guard = irq_on_late ? (top_val - top_val / 2U) : 0U;
		ticks = now + ticks;
	}

	/*
	 * Reconstruct the full 64-bit compare value from the 32-bit ticks
	 * and the upper bits of the current counter value. If ticks is behind
	 * the current lower-32-bit value, the target is in the next 32-bit
	 * cycle, so increment the upper part.
	 */
	{
		uint64_t current_high = current & ~(uint64_t)UINT32_MAX;

		compare_val = current_high | (uint64_t)ticks;
		if (ticks < now) {
			compare_val += ((uint64_t)UINT32_MAX + 1ULL);
		}
	}

	key = k_spin_lock(&data->lock);

	if (data->channels[chan_id].callback != NULL
#ifdef CONFIG_COUNTER_64BITS_TICKS
	    || data->channels[chan_id].callback_64 != NULL
#endif
	    ) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("Channel %d already in use", chan_id);
		return -EBUSY;
	}

	data->channels[chan_id].callback  = alarm_cfg->callback;
	data->channels[chan_id].user_data = alarm_cfg->user_data;

	SYSCTR_SetCompareValue(config->ctrl_base, config->cmp_base,
			       (sysctr_compare_frame_t)chan_id, compare_val);

	/* Unmask interrupt for this compare frame (IMASK = 0 → enabled). */
	SYSCTR_EnableInterrupts(config->cmp_base,
		chan_id == 0 ? kSYSCTR_Compare0InterruptEnable
			     : kSYSCTR_Compare1InterruptEnable);

	SYSCTR_EnableCompare(config->cmp_base,
			     (sysctr_compare_frame_t)chan_id, true);

	k_spin_unlock(&data->lock, key);

	/*
	 * Late detection per counter API (counter.h):
	 *
	 * Re-read the counter after programming the compare register.
	 * If the counter has already passed beyond the target and is
	 * still within the guard period, the alarm is late:
	 *
	 *   late if (now_reread - ticks) < guard   [unsigned arithmetic]
	 */
	if (guard != 0U &&
	    ((uint32_t)SYSCTR_GetCounterlValue(config->read_base) - ticks) < guard) {
		if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
			err = -ETIME;
		}

		/*
		 * Interrupt is triggered always for relative alarm and for
		 * absolute depending on the COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE
		 * flag.
		 */
		if (irq_on_late) {
			atomic_or(&data->irq_pending, BIT(chan_id));
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			data->channels[chan_id].callback = NULL;
		}
	}

	return err;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int mcux_sysctr_set_alarm_64(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg_64 *alarm_cfg)
{
	const struct mcux_sysctr_config *config = dev->config;
	struct mcux_sysctr_data *data = dev->data;
	uint64_t ticks = alarm_cfg->ticks;
	uint32_t flags = alarm_cfg->flags;
	uint64_t top_val = SYSCTR_MAX_TICKS;
	uint64_t current;
	uint64_t compare_val;
	uint64_t guard;
	bool irq_on_late;
	int err = 0;
	k_spinlock_key_t key;

	if (chan_id >= SYSCTR_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %d", chan_id);
		return -EINVAL;
	}

	/*
	 * SYS_CTR hardware restriction: programming compare values is only
	 * allowed while running on the base frequency.
	 */
	if (config->clock_source != kSYSCTR_BaseFrequency) {
		LOG_ERR("set_alarm is not supported in alternate clock mode");
		return -ENOTSUP;
	}

	if ((flags & COUNTER_ALARM_CFG_ABSOLUTE) && ticks > top_val) {
		LOG_ERR("Absolute ticks 0x%llx exceeds 56-bit counter max",
			(unsigned long long)ticks);
		return -EINVAL;
	}

	current = SYSCTR_GetCounterlValue(config->read_base);

	if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		__ASSERT_NO_MSG(data->guard_period_64 < top_val);
		guard = data->guard_period_64;
		irq_on_late = !!(flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE);
		compare_val = ticks;
	} else {
		irq_on_late = ticks < (top_val / 2U);
		guard = irq_on_late ? (top_val - top_val / 2U) : 0U;
		ticks = (current + ticks) & top_val;
		compare_val = ticks;
	}

	key = k_spin_lock(&data->lock);

	if (data->channels[chan_id].callback_64 != NULL ||
	    data->channels[chan_id].callback != NULL) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("Channel %d already in use", chan_id);
		return -EBUSY;
	}

	data->channels[chan_id].callback    = NULL;
	data->channels[chan_id].callback_64 = alarm_cfg->callback;
	data->channels[chan_id].user_data   = alarm_cfg->user_data;

	SYSCTR_SetCompareValue(config->ctrl_base, config->cmp_base,
			       (sysctr_compare_frame_t)chan_id, compare_val);

	SYSCTR_EnableInterrupts(config->cmp_base,
		chan_id == 0 ? kSYSCTR_Compare0InterruptEnable
			     : kSYSCTR_Compare1InterruptEnable);

	SYSCTR_EnableCompare(config->cmp_base,
			     (sysctr_compare_frame_t)chan_id, true);

	k_spin_unlock(&data->lock, key);

	/*
	 * Late detection: re-read the counter after programming. If the
	 * counter has passed beyond the target within the guard period,
	 * the alarm is late: (now_reread - ticks) < guard.
	 */
	if (guard != 0U &&
	    ((SYSCTR_GetCounterlValue(config->read_base) - ticks) & top_val) < guard) {
		if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
			err = -ETIME;
		}

		if (irq_on_late) {
			atomic_or(&data->irq_pending, BIT(chan_id));
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			data->channels[chan_id].callback_64 = NULL;
		}
	}

	return err;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int mcux_sysctr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_sysctr_config *config = dev->config;
	struct mcux_sysctr_data *data = dev->data;
	k_spinlock_key_t key;

	if (chan_id >= SYSCTR_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %d", chan_id);
		return -EINVAL;
	}

	/* Disable compare and mask interrupt to prevent a stale IRQ. */
	SYSCTR_EnableCompare(config->cmp_base,
			     (sysctr_compare_frame_t)chan_id, false);
	SYSCTR_DisableInterrupts(config->cmp_base,
		chan_id == 0 ? kSYSCTR_Compare0InterruptEnable
			     : kSYSCTR_Compare1InterruptEnable);

	key = k_spin_lock(&data->lock);

	data->channels[chan_id].callback  = NULL;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	data->channels[chan_id].callback_64 = NULL;
#endif
	data->channels[chan_id].user_data = NULL;

	atomic_and(&data->irq_pending, ~BIT(chan_id));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static uint32_t mcux_sysctr_get_guard_period(const struct device *dev,
					     uint32_t flags)
{
	struct mcux_sysctr_data *data = dev->data;

	return data->guard_period;
}

static int mcux_sysctr_set_guard_period(const struct device *dev,
					uint32_t ticks, uint32_t flags)
{
	struct mcux_sysctr_data *data = dev->data;

	data->guard_period = ticks;
	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static uint64_t mcux_sysctr_get_guard_period_64(const struct device *dev,
						uint32_t flags)
{
	struct mcux_sysctr_data *data = dev->data;

	return data->guard_period_64;
}

static int mcux_sysctr_set_guard_period_64(const struct device *dev,
					   uint64_t ticks, uint32_t flags)
{
	struct mcux_sysctr_data *data = dev->data;

	if (ticks > SYSCTR_MAX_TICKS) {
		return -EINVAL;
	}

	data->guard_period_64 = ticks;
	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static uint32_t mcux_sysctr_get_pending_int(const struct device *dev)
{
	const struct mcux_sysctr_config *config = dev->config;

	return (SYSCTR_GetStatusFlags(config->cmp_base) != 0U) ? 1U : 0U;
}

static uint32_t mcux_sysctr_get_freq(const struct device *dev)
{
	struct mcux_sysctr_data *data = dev->data;

	return data->clock_frequency;
}

static void mcux_sysctr_isr(const struct device *dev)
{
	const struct mcux_sysctr_config *config = dev->config;
	struct mcux_sysctr_data *data = dev->data;
	uint32_t status = SYSCTR_GetStatusFlags(config->cmp_base);

	for (uint8_t chan = 0; chan < SYSCTR_NUM_CHANNELS; chan++) {
		uint32_t chan_flag = (chan == 0U) ? (uint32_t)kSYSCTR_Compare0Flag
						 : (uint32_t)kSYSCTR_Compare1Flag;
		bool pending = !!(status & chan_flag);
		bool sw_pending = atomic_test_bit(&data->irq_pending, chan);

		if (!pending && !sw_pending) {
			continue;
		}

		/*
		 * Disable compare and mask interrupt for this channel.
		 * Compare Interrupt Status field is cleared when disabling compare.
		 */
		SYSCTR_EnableCompare(config->cmp_base,
				     (sysctr_compare_frame_t)chan, false);
		SYSCTR_DisableInterrupts(config->cmp_base,
			chan == 0U ? kSYSCTR_Compare0InterruptEnable
				   : kSYSCTR_Compare1InterruptEnable);
		atomic_and(&data->irq_pending, ~BIT(chan));

		if (data->channels[chan].callback != NULL) {
			counter_alarm_callback_t cb = data->channels[chan].callback;
			void *ud = data->channels[chan].user_data;

			data->channels[chan].callback  = NULL;
			data->channels[chan].user_data = NULL;

			cb(dev, chan, (uint32_t)SYSCTR_GetCounterlValue(
				config->read_base), ud);
#ifdef CONFIG_COUNTER_64BITS_TICKS
		} else if (data->channels[chan].callback_64 != NULL) {
			counter_alarm_callback_64_t cb = data->channels[chan].callback_64;
			void *ud = data->channels[chan].user_data;

			data->channels[chan].callback_64 = NULL;
			data->channels[chan].user_data   = NULL;

			cb(dev, chan, SYSCTR_GetCounterlValue(config->read_base), ud);
#endif
		}
	}
}

static DEVICE_API(counter, mcux_sysctr_driver_api) = {
	.start             = mcux_sysctr_start,
	.stop              = mcux_sysctr_stop,
	.get_value         = mcux_sysctr_get_value,
	.set_alarm         = mcux_sysctr_set_alarm,
	.cancel_alarm      = mcux_sysctr_cancel_alarm,
	.set_top_value     = mcux_sysctr_set_top_value,
	.get_pending_int   = mcux_sysctr_get_pending_int,
	.get_top_value     = mcux_sysctr_get_top_value,
	.get_freq          = mcux_sysctr_get_freq,
	.get_guard_period  = mcux_sysctr_get_guard_period,
	.set_guard_period  = mcux_sysctr_set_guard_period,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64      = mcux_sysctr_get_value_64,
	.set_alarm_64      = mcux_sysctr_set_alarm_64,
	.get_top_value_64  = mcux_sysctr_get_top_value_64,
	.set_top_value_64  = mcux_sysctr_set_top_value_64,
	.get_guard_period_64 = mcux_sysctr_get_guard_period_64,
	.set_guard_period_64 = mcux_sysctr_set_guard_period_64,
#endif /* CONFIG_COUNTER_64BITS_TICKS */
};

static int mcux_sysctr_init(const struct device *dev)
{
	const struct mcux_sysctr_config *config = dev->config;
	struct mcux_sysctr_data *data = dev->data;
	clock_control_subsys_t active_subsys;
	sysctr_config_t sysctr_cfg;
	uint32_t raw_rate;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	active_subsys = (config->clock_source == kSYSCTR_BaseFrequency)
			? config->base_clock_subsys
			: config->slow_clock_subsys;

	if (clock_control_get_rate(config->clock_dev, active_subsys, &raw_rate)) {
		LOG_ERR("could not get clock frequency");
		return -EINVAL;
	}

	/*
	 * The slow clock path has a fixed /64 prescaler inside SYS_CTR.
	 * Adjust the reported tick rate accordingly.
	 */
	data->clock_frequency = (config->clock_source == kSYSCTR_BaseFrequency)
				? raw_rate
				: raw_rate / SYSCTR_SLOW_CLK_DIV;

	SYSCTR_GetDefaultConfig(&sysctr_cfg);
	SYSCTR_Init(config->ctrl_base, config->cmp_base, &sysctr_cfg);
	SYSCTR_SetCounterClockSource(config->ctrl_base, config->clock_source);

	config->irq_config_func(dev);

	return 0;
}

#define COUNTER_MCUX_SYSCTR_INIT(n)							\
	static struct mcux_sysctr_data mcux_sysctr_data_##n;				\
	static void mcux_sysctr_irq_config_##n(const struct device *dev);		\
											\
	static const struct mcux_sysctr_config mcux_sysctr_config_##n = {		\
		.info = {								\
			COND_CODE_1(CONFIG_COUNTER_64BITS_TICKS,			\
				(.max_top_value_64 = SYSCTR_MAX_TICKS,),		\
				(.max_top_value = UINT32_MAX,))				\
			.channels      = SYSCTR_NUM_CHANNELS,				\
			.flags         = COUNTER_CONFIG_INFO_COUNT_UP,			\
		},									\
		.ctrl_base = (SYS_CTR_CONTROL_Type *)					\
				DT_INST_REG_ADDR_BY_NAME(n, control),			\
		.read_base = (SYS_CTR_READ_Type *)					\
				DT_INST_REG_ADDR_BY_NAME(n, read),			\
		.cmp_base  = (SYS_CTR_COMPARE_Type *)					\
				DT_INST_REG_ADDR_BY_NAME(n, compare),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, base)),	\
		.base_clock_subsys =							\
			(clock_control_subsys_t)					\
			DT_INST_CLOCKS_CELL_BY_NAME(n, base, name),			\
		.slow_clock_subsys =							\
			(clock_control_subsys_t)					\
			DT_INST_CLOCKS_CELL_BY_NAME(n, slow, name),			\
		.clock_source = DT_INST_PROP(n, clock_source),				\
		.irqn = DT_INST_IRQN(n),						\
		.irq_config_func = mcux_sysctr_irq_config_##n,				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, mcux_sysctr_init, NULL,				\
			      &mcux_sysctr_data_##n,					\
			      &mcux_sysctr_config_##n,					\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			      &mcux_sysctr_driver_api);					\
											\
	static void mcux_sysctr_irq_config_##n(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    mcux_sysctr_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));						\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_SYSCTR_INIT)
