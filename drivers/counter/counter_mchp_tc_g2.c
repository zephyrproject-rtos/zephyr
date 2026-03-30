/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_tc_g2_counter

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_mchp_tc_g2, CONFIG_COUNTER_LOG_LEVEL);

#define MODE_BASE	(TC_CMR_WAVEFORM_WAVE_1 | \
			 TC_CMR_WAVEFORM_EEVT_XC0)
#define MODE_ALARM	(MODE_BASE | \
			 TC_CMR_WAVEFORM_WAVSEL_UP)
#define MODE_TOP_VALUE	(MODE_BASE | \
			 TC_CMR_WAVEFORM_WAVSEL_UP_RC)

#define ID_MSK(id) (TC_SR_CPAS_Msk << (id))

enum compare_reg_id {
	RA_ID, /* Alarm channel 0 */
	RB_ID, /* Alarm channel 1 */
	RC_ID, /* Top value or alarm channel 2 */
	TOP_ID = RC_ID,
	MAX_ID
};

struct sam_tc_config {
	struct counter_config_info info;
	tc_channel_registers_t *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct atmel_sam_pmc_config gclk_cfg;
	const struct pinctrl_dev_config *pincfg;
	uint8_t clock_selection;
	uint8_t top_alarm;
	void (*irq_config_func)(const struct device *dev);
};

struct sam_tc_alarm_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct sam_tc_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	struct k_spinlock lock;

	struct sam_tc_alarm_data alarm[MAX_ID];
};

static inline void tc_configure(tc_channel_registers_t *regs, uint32_t mode)
{
	regs->TC_CMR = mode;
}

static inline void tc_start(tc_channel_registers_t *regs)
{
	regs->TC_CCR = TC_CCR_SWTRG_1 | TC_CCR_CLKEN_1;
}

static inline void tc_stop(tc_channel_registers_t *regs)
{
	regs->TC_CCR = TC_CCR_CLKDIS_1;
}

static inline void tc_reset(tc_channel_registers_t *regs)
{
	regs->TC_CCR = TC_CCR_SWTRG_1;
}

static inline uint32_t tc_irq_status(tc_channel_registers_t *regs)
{
	return regs->TC_SR;
}

static inline void tc_irq_enable(tc_channel_registers_t *regs, uint32_t mask)
{
	regs->TC_IER = mask & TC_IER_Msk;
}

static inline void tc_irq_disable(tc_channel_registers_t *regs, uint32_t mask)
{
	regs->TC_IDR = mask & TC_IDR_Msk;
}

static inline void tc_irq_disable_all(tc_channel_registers_t *regs)
{
	tc_irq_disable(regs, TC_IDR_Msk);
}

static inline uint32_t tc_irq_mask(tc_channel_registers_t *regs)
{
	return regs->TC_IMR;
}

static inline uint32_t tc_counter_value(tc_channel_registers_t *regs)
{
	return regs->TC_CV;
}

static uint32_t tc_compare_get_value(tc_channel_registers_t *regs, uint32_t id)
{
	if (id == RA_ID) {
		return regs->TC_RA;
	}
	if (id == RB_ID) {
		return regs->TC_RB;
	}
	if (id == RC_ID) {
		return regs->TC_RC;
	}

	return 0;
}

static void tc_compare_set_value(tc_channel_registers_t *regs, uint32_t id, uint32_t value)
{
	if (id == RA_ID) {
		regs->TC_RA = TC_RA_RA(value);
	} else if (id == RB_ID) {
		regs->TC_RB = TC_RB_RB(value);
	} else if (id == RC_ID) {
		regs->TC_RC = TC_RC_RC(value);
	} else {
		return;
	}
}

static void tc_compare_clear_value(tc_channel_registers_t *regs, uint32_t id)
{
	tc_compare_set_value(regs, id, 0);
}

static int sam_tc_start(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;

	tc_start(config->regs);

	return 0;
}

static int sam_tc_stop(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;

	tc_stop(config->regs);

	return 0;
}

static int sam_tc_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct sam_tc_config *config = dev->config;

	*ticks = tc_counter_value(config->regs);

	return 0;
}

static int sam_tc_set_alarm(const struct device *dev, uint8_t chan_id,
			    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct sam_tc_config *config = dev->config;
	const struct counter_config_info *info = &config->info;
	struct sam_tc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t top_value = 0;
	uint32_t alarm_value;

	__ASSERT_NO_MSG(alarm_cfg->callback);

	if (chan_id >= info->channels) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm[chan_id].callback) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	top_value = tc_compare_get_value(config->regs, TOP_ID);

	if (config->top_alarm) {
		/* Check if top value is running */
		if ((top_value) && (!data->alarm[RC_ID].callback)) {
			/* Top value is running, the RC cannot be reused for alarm */
			if (chan_id == RC_ID) {
				k_spin_unlock(&data->lock, key);
				return -ENOTSUP;
			}
		} else {
			/* Top value is not running, clear top_value. */
			top_value = 0;
		}
	}

	if ((top_value) && (alarm_cfg->ticks > top_value)) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	data->alarm[chan_id].callback = alarm_cfg->callback;
	data->alarm[chan_id].user_data = alarm_cfg->user_data;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		alarm_value = alarm_cfg->ticks;
	} else {
		/* Overflow safety */
		alarm_value = tc_counter_value(config->regs) + alarm_cfg->ticks;
		if (top_value) {
			alarm_value %= top_value;
		}
	}

	tc_compare_set_value(config->regs, chan_id, alarm_value);
	tc_irq_enable(config->regs, ID_MSK(chan_id));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int sam_tc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct sam_tc_config *config = dev->config;
	const struct counter_config_info *info = &config->info;
	struct sam_tc_data *data = dev->data;
	k_spinlock_key_t key;

	if (chan_id >= info->channels) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm[chan_id].callback) {
		tc_irq_disable(config->regs, ID_MSK(chan_id));
		tc_compare_clear_value(config->regs, chan_id);

		data->alarm[chan_id].callback = NULL;
		data->alarm[chan_id].user_data = NULL;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int sam_tc_set_top_value(const struct device *dev,
				const struct counter_top_cfg *top_cfg)
{
	const struct sam_tc_config *config = dev->config;
	const struct counter_config_info *info = &config->info;
	struct sam_tc_data *data = dev->data;
	k_spinlock_key_t key;
	int i, ret = 0;

	if (!top_cfg->ticks) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	for (i = 0; i < info->channels; i++) {
		if (data->alarm[i].callback) {
			k_spin_unlock(&data->lock, key);
			return -EBUSY;
		}
	}

	tc_irq_disable(config->regs, ID_MSK(TOP_ID));

	if (config->top_alarm) {
		tc_configure(config->regs, MODE_TOP_VALUE | TC_CMR_TCCLKS(config->clock_selection));
	}
	tc_compare_set_value(config->regs, TOP_ID, top_cfg->ticks);

	if (top_cfg->callback) {
		data->top_cb = top_cfg->callback;
		data->top_user_data = top_cfg->user_data;
	}

	if ((top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0) {
		if (tc_counter_value(config->regs) >= top_cfg->ticks) {
			ret = -ETIME;
			if ((top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0) {
				tc_reset(config->regs);
			}
		}
	} else {
		tc_reset(config->regs);
	}

	tc_irq_enable(config->regs, ID_MSK(TOP_ID));

	k_spin_unlock(&data->lock, key);

	return ret;
}

static uint32_t sam_tc_get_top_value(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;
	struct sam_tc_data *data = dev->data;
	k_spinlock_key_t key;

	if (config->top_alarm) {
		key = k_spin_lock(&data->lock);

		if (data->alarm[RC_ID].callback) {
			k_spin_unlock(&data->lock, key);
			return 0;
		}

		k_spin_unlock(&data->lock, key);
	}

	return tc_compare_get_value(config->regs, TOP_ID);
}

static uint32_t sam_tc_get_pending_int(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;

	return tc_irq_status(config->regs) & tc_irq_mask(config->regs);
}

static uint32_t sam_tc_get_freq(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;
	uint32_t rate = 0;

	if (config->clock_selection == 0) {
		clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				       (clock_control_subsys_t)&config->gclk_cfg,
				       &rate);
	} else if ((config->clock_selection >= 1) && (config->clock_selection <= 3)) {
		clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				       (clock_control_subsys_t)&config->clock_cfg,
				       &rate);
		if (rate) {
			rate >>= 3 + ((config->clock_selection - 1) * 2);
		}
	} else if (config->clock_selection == 4) {
		rate = DT_PROP(DT_PATH(clocks, slow_xtal), clock_frequency);
	} else {
		return 0;
	}

	return rate;
}

static void sam_tc_isr(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;
	const struct counter_config_info *info = &config->info;
	struct sam_tc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t status;
	int i;

	status = tc_irq_status(config->regs);

	key = k_spin_lock(&data->lock);

	for (i = 0; i < info->channels; i++) {
		if ((status & ID_MSK(i)) && (data->alarm[i].callback)) {
			counter_alarm_callback_t cb = data->alarm[i].callback;
			void *user_data = data->alarm[i].user_data;
			uint32_t ticks = tc_counter_value(config->regs);

			tc_irq_disable(config->regs, ID_MSK(i));
			tc_compare_clear_value(config->regs, i);
			data->alarm[i].callback = NULL;
			data->alarm[i].user_data = NULL;

			cb(dev, i, ticks, user_data);

			status &= ~ID_MSK(i);
		}
	}

	if (status & ID_MSK(TOP_ID)) {
		if (data->top_cb) {
			data->top_cb(dev, data->top_user_data);
		}
	}

	k_spin_unlock(&data->lock, key);
}

static int sam_tc_init(const struct device *dev)
{
	const struct sam_tc_config *config = dev->config;
	int ret;

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	/* Enable channel's clock */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&config->clock_cfg);

	/* Configure TC */
	tc_stop(config->regs);
	tc_irq_disable_all(config->regs);
	tc_irq_status(config->regs);
	if (config->top_alarm) {
		tc_configure(config->regs, MODE_ALARM | TC_CMR_TCCLKS(config->clock_selection));
	} else {
		tc_configure(config->regs, MODE_TOP_VALUE | TC_CMR_TCCLKS(config->clock_selection));
	}

	config->irq_config_func(dev);

	LOG_INF("Device %s initialized, reg:0x%08x cs:%d channels:%u top_alarm:%u",
		dev->name,
		(unsigned int)config->regs,
		(unsigned int)config->clock_selection,
		(unsigned int)config->info.channels,
		(unsigned int)config->top_alarm);

	return 0;
}

static DEVICE_API(counter, sam_tc_driver_api) = {
	.start = sam_tc_start,
	.stop = sam_tc_stop,
	.get_value = sam_tc_get_value,
	.set_alarm = sam_tc_set_alarm,
	.cancel_alarm = sam_tc_cancel_alarm,
	.set_top_value = sam_tc_set_top_value,
	.get_top_value = sam_tc_get_top_value,
	.get_pending_int = sam_tc_get_pending_int,
	.get_freq = sam_tc_get_freq,
};

#define SAM_TC_ALARM_CHANNELS(n) \
		COND_CODE_1(DT_INST_PROP(n, top_alarm), \
			    (MAX_ID), \
			    (MAX_ID - 1))

#define COUNTER_SAM_TC_INIT(n)						\
PINCTRL_DT_INST_DEFINE(n);						\
									\
static void counter_##n##_sam_config_func(const struct device *dev);	\
									\
static const struct sam_tc_config counter_##n##_sam_config = {		\
	.info = {							\
		.max_top_value = UINT32_MAX,				\
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
		.channels = SAM_TC_ALARM_CHANNELS(n)			\
	},								\
	.regs = (tc_channel_registers_t *)DT_INST_REG_ADDR(n),		\
	.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),			\
	.gclk_cfg = SAM_DT_CLOCK_PMC_CFG(1, DT_DRV_INST(n)),		\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	.clock_selection = DT_INST_PROP(n, clock_selection),		\
	.top_alarm = DT_INST_PROP(n, top_alarm),			\
	.irq_config_func = &counter_##n##_sam_config_func,		\
};									\
									\
static struct sam_tc_data counter_##n##_sam_data;			\
									\
DEVICE_DT_INST_DEFINE(n, sam_tc_init, NULL,				\
		&counter_##n##_sam_data, &counter_##n##_sam_config,	\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
		&sam_tc_driver_api);					\
									\
static void counter_##n##_sam_config_func(const struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
		    sam_tc_isr, DEVICE_DT_INST_GET(n), 0);		\
	irq_enable(DT_INST_IRQN(n));					\
}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_SAM_TC_INIT)
