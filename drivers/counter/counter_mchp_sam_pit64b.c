/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sam_pit64b_counter

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_mchp_sam, CONFIG_COUNTER_LOG_LEVEL);

#define PERIOD_MAX UINT64_MAX

struct sam_pit_config {
	struct counter_config_info info;
	pit64b_registers_t *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct atmel_sam_pmc_config gclk_cfg;
	uint8_t clock_selection;
	uint8_t prescaler_period;
	uint8_t top_alarm;
	void (*irq_config_func)(const struct device *dev);
};

struct sam_pit_alarm_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct sam_pit_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	struct k_spinlock lock;

	struct sam_pit_alarm_data alarm;
};

static inline void pit_configure(pit64b_registers_t *regs, uint32_t mode)
{
	regs->PIT64B_MR = mode;
}

static inline void pit_start(pit64b_registers_t *regs)
{
	regs->PIT64B_CR = PIT64B_CR_START_1;
}

static inline void pit_stop(pit64b_registers_t *regs)
{
	regs->PIT64B_CR = PIT64B_CR_SWRST_1;
}

static inline uint32_t pit_irq_status(pit64b_registers_t *regs)
{
	return regs->PIT64B_ISR;
}

static inline void pit_irq_enable(pit64b_registers_t *regs, uint32_t mask)
{
	regs->PIT64B_IER = mask & PIT64B_IER_Msk;
}

static inline void pit_irq_disable(pit64b_registers_t *regs, uint32_t mask)
{
	regs->PIT64B_IDR = mask & PIT64B_IDR_Msk;
}

static inline void pit_irq_disable_all(pit64b_registers_t *regs)
{
	regs->PIT64B_IDR = PIT64B_IDR_Msk;
}

static inline uint32_t pit_irq_mask(pit64b_registers_t *regs)
{
	return regs->PIT64B_IMR;
}

static inline uint64_t pit_counter_value(pit64b_registers_t *regs)
{
	uint64_t ret;

	ret = regs->PIT64B_TLSBR;
	ret |= (uint64_t)regs->PIT64B_TMSBR << 32;

	return ret;
}

static inline uint64_t pit_period_get_value(pit64b_registers_t *regs)
{
	return (uint64_t)regs->PIT64B_MSBPR << 32 | regs->PIT64B_LSBPR;
}

static inline void pit_period_set_value(pit64b_registers_t *regs, uint64_t value)
{
	regs->PIT64B_MSBPR = (uint32_t)(value >> 32);
	regs->PIT64B_LSBPR = (uint32_t)value;
}

static int sam_pit_start(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;

	pit_start(config->regs);

	return 0;
}

static int sam_pit_stop(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;

	pit_stop(config->regs);

	return 0;
}

static int sam_pit_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct sam_pit_config *config = dev->config;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	*ticks = (uint32_t)pit_counter_value(config->regs);

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int sam_pit_get_value_64(const struct device *dev, uint64_t *ticks)
{
	const struct sam_pit_config *config = dev->config;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	*ticks = pit_counter_value(config->regs);

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int sam_pit_set_alarm(const struct device *dev, uint8_t chan_id,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct sam_pit_config *config = dev->config;
	const struct counter_config_info *info = &config->info;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;
	uint64_t top_value;

	if (chan_id >= info->channels) {
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(alarm_cfg->callback);

	/* PIT64B only support absolute value */
	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm.callback) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/* Top-alarm is enabled, Check if the top value is running */
	top_value = pit_period_get_value(config->regs);
	if ((top_value) && (top_value != PERIOD_MAX)) {
		/* Top value is running, cannot be reused for alarm */
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->alarm.callback = alarm_cfg->callback;
	data->alarm.user_data = alarm_cfg->user_data;

	pit_period_set_value(config->regs, alarm_cfg->ticks);
	pit_irq_status(config->regs);
	pit_irq_enable(config->regs, PIT64B_IER_OVRE_Msk | PIT64B_IER_PERIOD_Msk);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int sam_pit_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct sam_pit_config *config = dev->config;
	const struct counter_config_info *info = &config->info;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;

	if (chan_id >= info->channels) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	pit_irq_disable(config->regs, PIT64B_IDR_OVRE_Msk | PIT64B_IDR_PERIOD_Msk);
	pit_period_set_value(config->regs, PERIOD_MAX);

	data->alarm.callback = NULL;
	data->alarm.user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int sam_pit_set_top_value(const struct device *dev,
				 const struct counter_top_cfg *top_cfg)
{
	const struct sam_pit_config *config = dev->config;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (!top_cfg->ticks) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if ((config->top_alarm) && (data->alarm.callback)) {
		/* Alarm is running, cannot be reused for top value */
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	pit_irq_disable(config->regs, PIT64B_IDR_OVRE_Msk | PIT64B_IDR_PERIOD_Msk);

	if (top_cfg->callback) {
		data->top_cb = top_cfg->callback;
		data->top_user_data = top_cfg->user_data;
	}

	/* PIT64B will reset the counter automatically */
	if ((top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0) {
		ret = -ENOTSUP;
	}

	pit_period_set_value(config->regs, top_cfg->ticks);
	pit_irq_status(config->regs);
	pit_irq_enable(config->regs, PIT64B_IER_OVRE_Msk | PIT64B_IER_PERIOD_Msk);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static uint32_t sam_pit_get_top_value(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;
	uint64_t ret;

	key = k_spin_lock(&data->lock);

	if ((config->top_alarm) && (data->alarm.callback)) {
		/* Alarm is running, top value is stopped */
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	ret = pit_period_get_value(config->regs);
	if (ret == PERIOD_MAX) {
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

	return (uint32_t)ret;
}

static uint32_t sam_pit_get_pending_int(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;

	return pit_irq_status(config->regs) & pit_irq_mask(config->regs);
}

static uint32_t sam_pit_get_freq(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;
	uint32_t rate = 0;

	if (config->clock_selection == 0) {
		clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				       (clock_control_subsys_t)&config->clock_cfg,
				       &rate);
	} else if (config->clock_selection == 1) {
		clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				       (clock_control_subsys_t)&config->gclk_cfg,
				       &rate);
	} else {
		return 0;
	}

	if (rate) {
		rate /= (config->prescaler_period + 1);
	}

	return rate;
}

static void sam_pit_isr(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;
	struct sam_pit_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t status;

	status = pit_irq_status(config->regs);

	key = k_spin_lock(&data->lock);

	if (status & PIT64B_ISR_OVRE_Msk) {
		LOG_ERR("%s: More than 1 rollover occurred since the last read\n\r",
			dev->name);
	}

	if (status & PIT64B_ISR_PERIOD_Msk) {
		if (data->alarm.callback) {
			counter_alarm_callback_t cb = data->alarm.callback;
			void *user_data = data->alarm.user_data;
			uint32_t ticks = (uint32_t)pit_counter_value(config->regs);

			pit_irq_disable(config->regs, PIT64B_IDR_OVRE_Msk | PIT64B_IDR_PERIOD_Msk);
			pit_period_set_value(config->regs, PERIOD_MAX);
			data->alarm.callback = NULL;
			data->alarm.user_data = NULL;

			cb(dev, 0, ticks, user_data);
		} else {
			if (data->top_cb) {
				data->top_cb(dev, data->top_user_data);
			}
		}
	}

	k_spin_unlock(&data->lock, key);
}

static int sam_pit_init(const struct device *dev)
{
	const struct sam_pit_config *config = dev->config;

	/* Enable channel's clock */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&config->clock_cfg);

	/* Configure PIT64B */
	pit_stop(config->regs);
	pit_irq_disable_all(config->regs);
	pit_irq_status(config->regs);
	pit_period_set_value(config->regs, PERIOD_MAX);
	pit_configure(config->regs, PIT64B_MR_PRESCALER(config->prescaler_period) |
				    PIT64B_MR_SMOD_1 |
				    PIT64B_MR_SGCLK(config->clock_selection) |
				    PIT64B_MR_CONT_1);

	config->irq_config_func(dev);

	LOG_INF("Device %s initialized, reg:0x%08x cs:%d pc:%d channels:%u top_alarm:%u",
		dev->name,
		(unsigned int)config->regs,
		(unsigned int)config->clock_selection,
		(unsigned int)config->prescaler_period,
		(unsigned int)config->info.channels,
		(unsigned int)config->top_alarm);

	return 0;
}

static DEVICE_API(counter, sam_pit_driver_api) = {
	.start = sam_pit_start,
	.stop = sam_pit_stop,
	.get_value = sam_pit_get_value,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64 = sam_pit_get_value_64,
#endif /* CONFIG_COUNTER_64BITS_TICKS */
	.set_alarm = sam_pit_set_alarm,
	.cancel_alarm = sam_pit_cancel_alarm,
	.set_top_value = sam_pit_set_top_value,
	.get_top_value = sam_pit_get_top_value,
	.get_pending_int = sam_pit_get_pending_int,
	.get_freq = sam_pit_get_freq,
};

#define SAM_PIT_ALARM_CHANNELS(n) \
		COND_CODE_1(DT_INST_PROP(n, top_alarm), \
			    (1), \
			    (0))

#define COUNTER_SAM_PIT64B_INIT(n)						\
static void counter_##n##_sam_config_func(const struct device *dev);		\
										\
static const struct sam_pit_config counter_##n##_sam_config = {			\
	.info = {								\
		.max_top_value = UINT32_MAX,					\
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
		.channels = SAM_PIT_ALARM_CHANNELS(n),				\
	},									\
	.regs = (pit64b_registers_t *)DT_INST_REG_ADDR(n),			\
	.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),				\
	.gclk_cfg = SAM_DT_CLOCK_PMC_CFG(1, DT_DRV_INST(n)),			\
	.clock_selection = DT_ENUM_IDX(DT_DRV_INST(n), clock_selection),	\
	.prescaler_period = DT_INST_PROP(n, prescaler_period),			\
	.top_alarm = DT_INST_PROP(n, top_alarm),				\
	.irq_config_func = &counter_##n##_sam_config_func,			\
};										\
										\
static struct sam_pit_data counter_##n##_sam_data;				\
										\
DEVICE_DT_INST_DEFINE(n, sam_pit_init, NULL,					\
	&counter_##n##_sam_data,						\
	&counter_##n##_sam_config,						\
	POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,				\
	&sam_pit_driver_api);							\
										\
static void counter_##n##_sam_config_func(const struct device *dev)		\
{										\
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
		    sam_pit_isr, DEVICE_DT_INST_GET(n), 0);			\
	irq_enable(DT_INST_IRQN(n));						\
}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_SAM_PIT64B_INIT)
