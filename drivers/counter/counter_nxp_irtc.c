/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/pm/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(nxp_irtc_counter, CONFIG_COUNTER_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_irtc_wake_timer

/* Wake timer source oscillator and the OSC_DIV_ENA prescaler ratio. */
#define NXP_IRTC_OSC_FREQ 32768U
#define NXP_IRTC_OSC_DIV  32U

struct nxp_irtc_counter_config {
	struct counter_config_info info;
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	/* Enable the OSC_DIV_ENA /32 prescaler (1024 Hz vs raw 32768 Hz). */
	bool osc_div;
};

struct nxp_irtc_counter_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
	/* Value last written to WAKE_TIMER_CNT; get_value reports loaded - cnt. */
	uint32_t loaded;
	/* An alarm interrupt is armed (kept set after expiry so get_value still
	 * reports the full elapsed span until the alarm is cancelled or the
	 * counter is restarted). The alarm has fired once alarm_callback is
	 * cleared, which is also the condition that allows a one-shot re-arm.
	 */
	bool armed;
	struct k_spinlock lock;
};

/*
 * Force a lock then unlock of the IRTC registers, guaranteeing a window of
 * write access (the IRTC re-arms write protection automatically).
 */
static void nxp_irtc_unlock(RTC_Type *base)
{
	while ((base->STATUS & (uint16_t)RTC_STATUS_WRITE_PROT_EN_MASK) == 0U) {
		*(volatile uint8_t *)(&base->STATUS) |= RTC_STATUS_WE(0x2);
	}
	while ((base->STATUS & (uint16_t)RTC_STATUS_WRITE_PROT_EN_MASK) != 0U) {
		*(volatile uint8_t *)(&base->STATUS) = 0x00;
		*(volatile uint8_t *)(&base->STATUS) = 0x40;
		*(volatile uint8_t *)(&base->STATUS) = 0xC0;
		*(volatile uint8_t *)(&base->STATUS) = 0x80;
	}
}

/* Load the down-counter with the given start value and start it counting. */
static void nxp_irtc_wake_load(RTC_Type *base, uint32_t value, bool intr_en, bool osc_div)
{
	uint32_t ctrl = base->WAKE_TIMER_CTRL;

	base->WAKE_TIMER_CTRL = RTC_WAKE_TIMER_CTRL_CLR_WAKE_TIMER_MASK;

	if (osc_div) {
		ctrl |= RTC_WAKE_TIMER_CTRL_OSC_DIV_ENA_MASK;
	} else {
		ctrl &= ~(uint32_t)RTC_WAKE_TIMER_CTRL_OSC_DIV_ENA_MASK;
	}
	if (intr_en) {
		ctrl |= RTC_WAKE_TIMER_CTRL_INTR_EN_MASK;
	} else {
		ctrl &= ~(uint32_t)RTC_WAKE_TIMER_CTRL_INTR_EN_MASK;
	}
	ctrl &= ~(uint32_t)RTC_WAKE_TIMER_CTRL_CLR_WAKE_TIMER_MASK;
	base->WAKE_TIMER_CTRL = ctrl;

	base->WAKE_TIMER_CNT = value;
}

static void nxp_irtc_counter_isr(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	struct nxp_irtc_counter_data *data = dev->data;
	counter_alarm_callback_t callback;
	void *user_data;
	uint32_t span;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	nxp_irtc_unlock(config->base);

	/* Acknowledge the expiry (write-1-to-clear the wake flag). */
	config->base->WAKE_TIMER_CTRL |= RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK;
	/*
	 * The wake timer is one-shot: stop further interrupts but keep the
	 * loaded reference so a later counter_get_value() still reports the
	 * full elapsed time (the down-counter remains at zero until re-armed).
	 * alarm_callback is cleared, marking the alarm as fired and allowing a
	 * subsequent one-shot re-arm.
	 */
	config->base->WAKE_TIMER_CTRL &= ~(uint32_t)RTC_WAKE_TIMER_CTRL_INTR_EN_MASK;

	callback = data->alarm_callback;
	user_data = data->alarm_user_data;
	span = data->loaded;
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	k_spin_unlock(&data->lock, key);

	if (callback != NULL) {
		callback(dev, 0, span, user_data);
	}
}

static int nxp_irtc_counter_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	struct nxp_irtc_counter_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/*
	 * Free-run the wake timer from the top value with its interrupt masked.
	 * counter_get_value() then reports (loaded - cnt), an up-counting value
	 * that advances until an alarm reloads the reference.
	 */
	nxp_irtc_unlock(config->base);
	config->base->WAKE_TIMER_CTRL |= RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK;
	nxp_irtc_wake_load(config->base, info->max_top_value, false, config->osc_div);
	data->loaded = info->max_top_value;
	data->armed = false;
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int nxp_irtc_counter_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	struct nxp_irtc_counter_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	nxp_irtc_unlock(config->base);
	/*
	 * Hold the wake timer cleared (CLR_WAKE_TIMER) to halt counting, mask
	 * its interrupt and acknowledge any pending flag in a single write.
	 * counter_get_value() then reports zero until the counter is restarted.
	 */
	config->base->WAKE_TIMER_CTRL =
		RTC_WAKE_TIMER_CTRL_CLR_WAKE_TIMER_MASK | RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK;
	data->armed = false;
	data->loaded = 0U;
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int nxp_irtc_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	struct nxp_irtc_counter_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Up-counter view: ticks elapsed since the reference was last loaded. */
	*ticks = data->loaded - config->base->WAKE_TIMER_CNT;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int nxp_irtc_counter_set_alarm(const struct device *dev, uint8_t chan_id,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	struct nxp_irtc_counter_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;

	/* The wake timer has a single hardware alarm channel. */
	if (chan_id >= 1U) {
		return -ENOTSUP;
	}

	/* Only relative alarms are supported. */
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U) {
		return -ENOTSUP;
	}

	if (ticks > info->max_top_value) {
		return -EINVAL;
	}

	/* A zero-tick relative alarm would expire immediately; arm the minimum. */
	if (ticks == 0U) {
		ticks = 1U;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/*
	 * Reject only a live (not-yet-fired) alarm. Once the one-shot alarm has
	 * fired the ISR clears alarm_callback, so a fired alarm can be re-armed.
	 */
	if (data->armed && (data->alarm_callback != NULL)) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->loaded = ticks;
	data->armed = true;

	nxp_irtc_unlock(config->base);
	config->base->WAKE_TIMER_CTRL |= RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK;
	nxp_irtc_wake_load(config->base, ticks, true, config->osc_div);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int nxp_irtc_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	struct nxp_irtc_counter_data *data = dev->data;

	/* The wake timer has a single hardware alarm channel. */
	if (chan_id >= 1U) {
		return -ENOTSUP;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Drop the alarm interrupt and resume free-running from the top value. */
	nxp_irtc_unlock(config->base);
	config->base->WAKE_TIMER_CTRL |= RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK;
	nxp_irtc_wake_load(config->base, info->max_top_value, false, config->osc_div);
	data->loaded = info->max_top_value;
	data->armed = false;
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int nxp_irtc_counter_set_top_value(const struct device *dev,
					  const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static uint32_t nxp_irtc_counter_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}

static uint32_t nxp_irtc_counter_get_pending_int(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);

	return ((config->base->WAKE_TIMER_CTRL & RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK) != 0U) ? 1U
											    : 0U;
}

/*
 * A stub action is provided only so the device gets a pm_base and can be selected
 * as a wakeup source via pm_device_wakeup_enable() (which sets the WS_ENABLED flag
 * this driver reads).
 */
static int nxp_irtc_counter_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static int nxp_irtc_counter_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct nxp_irtc_counter_config *config =
		CONTAINER_OF(info, struct nxp_irtc_counter_config, info);
	RTC_Type *base = config->base;

	nxp_irtc_unlock(base);

	/* Hold the wake timer cleared and its interrupt disabled until armed. */
	base->WAKE_TIMER_CTRL =
		RTC_WAKE_TIMER_CTRL_CLR_WAKE_TIMER_MASK | RTC_WAKE_TIMER_CTRL_WAKE_FLAG_MASK;

	config->irq_config_func(dev);

	return pm_device_driver_init(dev, nxp_irtc_counter_pm_action);
}

static DEVICE_API(counter, nxp_irtc_counter_driver_api) = {
	.start = nxp_irtc_counter_start,
	.stop = nxp_irtc_counter_stop,
	.get_value = nxp_irtc_counter_get_value,
	.set_alarm = nxp_irtc_counter_set_alarm,
	.cancel_alarm = nxp_irtc_counter_cancel_alarm,
	.set_top_value = nxp_irtc_counter_set_top_value,
	.get_pending_int = nxp_irtc_counter_get_pending_int,
	.get_top_value = nxp_irtc_counter_get_top_value,
};

#define NXP_IRTC_COUNTER_INIT(n)                                                                   \
	static void nxp_irtc_counter_irq_config_##n(const struct device *dev);                     \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, nxp_irtc_counter_pm_action);                                   \
                                                                                                   \
	static const struct nxp_irtc_counter_config nxp_irtc_counter_config_##n = {                \
		.base = (RTC_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),                                \
		.irq_config_func = nxp_irtc_counter_irq_config_##n,                                \
		.osc_div = DT_INST_PROP(n, osc_div),                                               \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = DT_INST_PROP(n, osc_div)                                   \
						? (NXP_IRTC_OSC_FREQ / NXP_IRTC_OSC_DIV)           \
						: NXP_IRTC_OSC_FREQ,                               \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1U,                                                    \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct nxp_irtc_counter_data nxp_irtc_counter_data_##n;                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nxp_irtc_counter_init, PM_DEVICE_DT_INST_GET(n),                  \
			      &nxp_irtc_counter_data_##n, &nxp_irtc_counter_config_##n.info,       \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,                           \
			      &nxp_irtc_counter_driver_api);                                       \
                                                                                                   \
	static void nxp_irtc_counter_irq_config_##n(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), nxp_irtc_counter_isr,       \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(NXP_IRTC_COUNTER_INIT)
