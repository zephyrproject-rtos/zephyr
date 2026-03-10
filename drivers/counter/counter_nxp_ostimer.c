/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_os_timer

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nxp_ostimer_counter, CONFIG_COUNTER_LOG_LEVEL);

/* OSTIMER is a 42-bit counter; maximum binary value is 2^42 - 1 */
#define OSTIMER_COUNTER_WIDTH 42
#define OSTIMER_MAX_VALUE ((1ULL << OSTIMER_COUNTER_WIDTH) - 1ULL)
#define OSTIMER_COUNTER_MASK OSTIMER_MAX_VALUE

struct nxp_ostimer_config {
	struct counter_config_info info;
	OSTIMER_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	unsigned int irqn;
};

struct nxp_ostimer_data {
	uint32_t clock_freq;
	/* Pending alarm callback — only one of the two is active at a time */
	counter_alarm_callback_t    alarm_cb32;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	counter_alarm_callback_64_t alarm_cb64;
#endif
	void *alarm_user_data;
	uint32_t guard_period;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	uint64_t guard_period_64;
#endif
};

/* Gray-code helpers — omitted when the counter uses binary encoding */

#if !(defined(FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER) && \
	FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER)

static inline uint64_t ostimer_gray_to_binary(uint64_t gray)
{
#if defined(FSL_FEATURE_SYSCTRL_HAS_CODE_GRAY) && FSL_FEATURE_SYSCTRL_HAS_CODE_GRAY
	uint64_t binary;

	gray = gray & OSTIMER_COUNTER_MASK;
	SYSCTL->CODE_GRAY_LSB = (uint32_t)(gray & 0xFFFFFFFFU);
	SYSCTL->CODE_GRAY_MSB = (uint32_t)(gray >> 32U);
	__NOP();

	binary =  (uint64_t)(SYSCTL->CODE_BIN_MSB) << 32U;
	binary |= (uint64_t)(SYSCTL->CODE_BIN_LSB);

	return binary & OSTIMER_COUNTER_MASK;
#else
	gray ^= (gray >> 32);
	gray ^= (gray >> 16);
	gray ^= (gray >>  8);
	gray ^= (gray >>  4);
	gray ^= (gray >>  2);
	gray ^= (gray >>  1);
	return gray;
#endif
}

static inline uint64_t ostimer_binary_to_gray(uint64_t bin)
{
	return bin ^ (bin >> 1);
}

#endif /* !FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER */

/* Register-level primitives */

static uint64_t ostimer_get_count(OSTIMER_Type *base)
{
	uint64_t val;

	/*
	 * Read low word first; the hardware latches the high word when the
	 * low word is read, so this ordering gives a consistent 42-bit value.
	 */
	val  = (uint64_t)base->EVTIMERL;
	val |= (uint64_t)base->EVTIMERH << 32;

#if defined(FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER) && \
	FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER
	return val & OSTIMER_COUNTER_MASK;
#else
	return ostimer_gray_to_binary(val);
#endif
}

static inline void ostimer_wait_match_write_ready(OSTIMER_Type *base)
{
#ifdef OSTIMER_OSEVENT_CTRL_MATCH_WR_RDY_MASK
	while (base->OSEVENT_CTRL & OSTIMER_OSEVENT_CTRL_MATCH_WR_RDY_MASK) {
	}
#else
	ARG_UNUSED(base);
#endif
}

static void ostimer_set_match(OSTIMER_Type *base, uint64_t target_bin)
{
	uint64_t val;

#if defined(FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER) && \
	FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER
	val = target_bin;
#else
	val = ostimer_binary_to_gray(target_bin);
#endif

	ostimer_wait_match_write_ready(base);

	base->MATCH_L = (uint32_t)(val & 0xFFFFFFFFU);
	base->MATCH_H = (uint32_t)(val >> 32);
}

static inline void ostimer_enable_irq(OSTIMER_Type *base)
{
	base->OSEVENT_CTRL |= OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;
}

static inline void ostimer_disable_irq(OSTIMER_Type *base)
{
	base->OSEVENT_CTRL &= ~OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;
}

static inline void ostimer_clear_flag(OSTIMER_Type *base)
{
	base->OSEVENT_CTRL |= OSTIMER_OSEVENT_CTRL_OSTIMER_INTRFLAG_MASK;
}

static inline bool ostimer_is_flag_set(OSTIMER_Type *base)
{
	return (base->OSEVENT_CTRL & OSTIMER_OSEVENT_CTRL_OSTIMER_INTRFLAG_MASK) != 0U;
}

/* Helpers: check / clear pending alarm */

static inline bool ostimer_alarm_pending(const struct nxp_ostimer_data *data)
{
#ifdef CONFIG_COUNTER_64BITS_TICKS
	return (data->alarm_cb32 != NULL) || (data->alarm_cb64 != NULL);
#else
	return data->alarm_cb32 != NULL;
#endif
}

static inline void ostimer_clear_alarm_cb(struct nxp_ostimer_data *data)
{
	data->alarm_cb32      = NULL;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	data->alarm_cb64      = NULL;
#endif
	data->alarm_user_data = NULL;
}

static inline void ostimer_set_alarm_cb_32(struct nxp_ostimer_data *data,
				counter_alarm_callback_t cb,
				void *user_data)
{
	data->alarm_cb32 = cb;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	data->alarm_cb64 = NULL;
#endif
	data->alarm_user_data = user_data;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static inline void ostimer_set_alarm_cb_64(struct nxp_ostimer_data *data,
				counter_alarm_callback_64_t cb,
				void *user_data)
{
	data->alarm_cb32 = NULL;
	data->alarm_cb64 = cb;
	data->alarm_user_data = user_data;
}
#endif

static inline void ostimer_set_match_irq(OSTIMER_Type *base, uint64_t target_bin)
{
	/*
	 * Write match first, then clear any stale INTRFLAG for the previous match,
	 * then enable interrupts.
	 */
	ostimer_clear_flag(base);
	ostimer_set_match(base, target_bin);
	ostimer_enable_irq(base);
}

static inline bool ostimer_is_late_32(uint32_t now, uint32_t target, uint32_t guard)
{
	return target <= (now + guard);
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static inline bool ostimer_is_late_64(uint64_t now, uint64_t target, uint64_t guard)
{
	return target <= ((now + guard) & OSTIMER_COUNTER_MASK);
}
#endif

/* Counter API */

static int nxp_ostimer_start(const struct device *dev)
{
	/* OSTIMER is free-running; clock and input mux are configured by soc.c */
	ARG_UNUSED(dev);
	return 0;
}

static int nxp_ostimer_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* OSTIMER is free-running; stop is not supported */
	return -ENOTSUP;
}

static int nxp_ostimer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct nxp_ostimer_config *config = dev->config;

	*ticks = (uint32_t)ostimer_get_count(config->base);
	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int nxp_ostimer_get_value_64(const struct device *dev, uint64_t *ticks)
{
	const struct nxp_ostimer_config *config = dev->config;

	*ticks = ostimer_get_count(config->base);
	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static uint32_t nxp_ostimer_get_top_value(const struct device *dev)
{
	const struct nxp_ostimer_config *config = dev->config;

	return config->info.max_top_value;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static uint64_t nxp_ostimer_get_top_value_64(const struct device *dev)
{
	ARG_UNUSED(dev);
	return OSTIMER_MAX_VALUE;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int nxp_ostimer_set_top_value(const struct device *dev,
				     const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	/* OSTIMER is free-running; top value is not configurable */
	return -ENOTSUP;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int nxp_ostimer_set_top_value_64(const struct device *dev,
					const struct counter_top_cfg_64 *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	/* OSTIMER is free-running; top value is not configurable */
	return -ENOTSUP;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int nxp_ostimer_set_alarm(const struct device *dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct nxp_ostimer_config *config = dev->config;
	struct nxp_ostimer_data *data = dev->data;
	uint64_t now;
	uint64_t target;

	ARG_UNUSED(chan_id);

	if (ostimer_alarm_pending(data)) {
		LOG_ERR("alarm already set");
		return -EBUSY;
	}

	now = ostimer_get_count(config->base);

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		/*
		 * Absolute 32-bit tick: extend to 64 bits using the upper bits
		 * of the current counter, handling 32-bit wrap.
		 */
		uint32_t now32   = (uint32_t)now;
		uint32_t ticks32 = alarm_cfg->ticks;
		uint64_t upper   = now & ~(uint64_t)UINT32_MAX;

		if (ticks32 < now32) {
			upper += ((uint64_t)1 << 32);
		}
		target = upper | ticks32;
	} else {
		target = now + alarm_cfg->ticks;
	}

	target &= OSTIMER_COUNTER_MASK;

	if (ostimer_is_late_32((uint32_t)now, (uint32_t)target, data->guard_period)) {
		if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
			/*
			 * Trigger via NVIC so the callback fires from ISR context,
			 * as required by the counter API contract.
			 */
			ostimer_set_alarm_cb_32(data, alarm_cfg->callback, alarm_cfg->user_data);
			NVIC_SetPendingIRQ(config->irqn);
		}
		return -ETIME;
	}

	ostimer_set_alarm_cb_32(data, alarm_cfg->callback, alarm_cfg->user_data);

	ostimer_set_match_irq(config->base, target);

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int nxp_ostimer_set_alarm_64(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg_64 *alarm_cfg)
{
	const struct nxp_ostimer_config *config = dev->config;
	struct nxp_ostimer_data *data = dev->data;
	uint64_t now;
	uint64_t target;

	ARG_UNUSED(chan_id);

	if (ostimer_alarm_pending(data)) {
		LOG_ERR("alarm already set");
		return -EBUSY;
	}

	now = ostimer_get_count(config->base);

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		target = alarm_cfg->ticks;
	} else {
		target = now + alarm_cfg->ticks;
	}

	target &= OSTIMER_COUNTER_MASK;
	if (ostimer_is_late_64(now, target, data->guard_period_64)) {
		if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
			ostimer_set_alarm_cb_64(data, alarm_cfg->callback, alarm_cfg->user_data);
			NVIC_SetPendingIRQ(config->irqn);
		}
		return -ETIME;
	}

	ostimer_set_alarm_cb_64(data, alarm_cfg->callback, alarm_cfg->user_data);

	ostimer_set_match_irq(config->base, target);

	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int nxp_ostimer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct nxp_ostimer_config *config = dev->config;
	struct nxp_ostimer_data *data = dev->data;

	ARG_UNUSED(chan_id);

	ostimer_wait_match_write_ready(config->base);
	ostimer_disable_irq(config->base);
	ostimer_clear_flag(config->base);
	ostimer_clear_alarm_cb(data);

	return 0;
}

static uint32_t nxp_ostimer_get_pending_int(const struct device *dev)
{
	const struct nxp_ostimer_config *config = dev->config;

	return ostimer_is_flag_set(config->base) ? 1U : 0U;
}

static uint32_t nxp_ostimer_get_freq(const struct device *dev)
{
	const struct nxp_ostimer_data *data = dev->data;

	return data->clock_freq;
}

static uint32_t nxp_ostimer_get_guard_period(const struct device *dev, uint32_t flags)
{
	const struct nxp_ostimer_data *data = dev->data;

	ARG_UNUSED(flags);
	return data->guard_period;
}

static int nxp_ostimer_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	struct nxp_ostimer_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(ticks < UINT32_MAX);

	data->guard_period = ticks;
	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static uint64_t nxp_ostimer_get_guard_period_64(const struct device *dev, uint32_t flags)
{
	struct nxp_ostimer_data *data = dev->data;

	ARG_UNUSED(flags);
	return data->guard_period_64;
}

static int nxp_ostimer_set_guard_period_64(const struct device *dev, uint64_t ticks,
					   uint32_t flags)
{
	struct nxp_ostimer_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(ticks < OSTIMER_MAX_VALUE);

	data->guard_period_64 = ticks;
	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

/* ISR */

static void nxp_ostimer_isr(const struct device *dev)
{
	const struct nxp_ostimer_config *config = dev->config;
	struct nxp_ostimer_data *data = dev->data;
	counter_alarm_callback_t    cb32;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	counter_alarm_callback_64_t cb64;
#endif
	void *user_data;
	uint64_t now64;

	ostimer_clear_flag(config->base);
	ostimer_disable_irq(config->base);

	now64     = ostimer_get_count(config->base);
	cb32      = data->alarm_cb32;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	cb64      = data->alarm_cb64;
#endif
	user_data = data->alarm_user_data;

	ostimer_clear_alarm_cb(data);

	if (cb32 != NULL) {
		cb32(dev, 0, (uint32_t)now64, user_data);
	}
#ifdef CONFIG_COUNTER_64BITS_TICKS
	else if (cb64 != NULL) {
		cb64(dev, 0, now64, user_data);
	}
#endif
	else {
		/* Intentional empty. */
	}
}

/* Init / PM */

static int nxp_ostimer_init_hw(const struct device *dev)
{
	const struct nxp_ostimer_config *config = dev->config;
	struct nxp_ostimer_data *data = dev->data;

	ostimer_clear_alarm_cb(data);

	/* Disable interrupt and clear any pending flag */
	ostimer_disable_irq(config->base);
	ostimer_clear_flag(config->base);

	config->irq_config_func(dev);

	return 0;
}

static int nxp_ostimer_pm_action(const struct device *dev,
				 enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		nxp_ostimer_init_hw(dev);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int nxp_ostimer_init(const struct device *dev)
{
	int ret;
	const struct nxp_ostimer_config *config = dev->config;
	struct nxp_ostimer_data *data = dev->data;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);

	if (ret) {
		LOG_ERR("Device clock turn on failed");
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &data->clock_freq);
	if (ret) {
		LOG_ERR("Failed to get clock rate");
		return ret;
	}

	return pm_device_driver_init(dev, nxp_ostimer_pm_action);
}

/* Driver API table */

static DEVICE_API(counter, nxp_ostimer_driver_api) = {
	.start            = nxp_ostimer_start,
	.stop             = nxp_ostimer_stop,
	.get_value        = nxp_ostimer_get_value,
	.set_alarm        = nxp_ostimer_set_alarm,
	.cancel_alarm     = nxp_ostimer_cancel_alarm,
	.set_top_value    = nxp_ostimer_set_top_value,
	.get_pending_int  = nxp_ostimer_get_pending_int,
	.get_top_value    = nxp_ostimer_get_top_value,
	.get_freq         = nxp_ostimer_get_freq,
	.get_guard_period = nxp_ostimer_get_guard_period,
	.set_guard_period = nxp_ostimer_set_guard_period,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64     = nxp_ostimer_get_value_64,
	.set_alarm_64     = nxp_ostimer_set_alarm_64,
	.get_top_value_64 = nxp_ostimer_get_top_value_64,
	.set_top_value_64 = nxp_ostimer_set_top_value_64,
	.get_guard_period_64 = nxp_ostimer_get_guard_period_64,
	.set_guard_period_64 = nxp_ostimer_set_guard_period_64,
#endif
};

/* Device instantiation */

#define NXP_OSTIMER_DEVICE(id)                                                                     \
	static void nxp_ostimer_irq_config_##id(const struct device *dev);                         \
	static const struct nxp_ostimer_config nxp_ostimer_config_##id = {                         \
		.info = {                                                                          \
			COND_CODE_1(CONFIG_COUNTER_64BITS_TICKS,                                   \
				(.max_top_value_64 = OSTIMER_MAX_VALUE,),                          \
				(.max_top_value = UINT32_MAX,))                                    \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                     \
			.channels = 1,                                                             \
		},                                                                                 \
		.base = (OSTIMER_Type *)DT_INST_REG_ADDR(id),                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),             \
		.irq_config_func = nxp_ostimer_irq_config_##id,                                    \
		.irqn = DT_INST_IRQN(id),                                                          \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(id, nxp_ostimer_pm_action);                                       \
	static struct nxp_ostimer_data nxp_ostimer_data_##id;                                      \
	DEVICE_DT_INST_DEFINE(id, &nxp_ostimer_init, PM_DEVICE_DT_INST_GET(id),                    \
			      &nxp_ostimer_data_##id, &nxp_ostimer_config_##id, POST_KERNEL,       \
			      CONFIG_COUNTER_INIT_PRIORITY, &nxp_ostimer_driver_api);              \
	static void nxp_ostimer_irq_config_##id(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), nxp_ostimer_isr,          \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

#define NXP_OSTIMER_DEVICE_IF_COUNTER(id) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(id, role, counter), (NXP_OSTIMER_DEVICE(id)), ())

DT_INST_FOREACH_STATUS_OKAY(NXP_OSTIMER_DEVICE_IF_COUNTER)
