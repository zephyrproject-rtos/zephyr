/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_lgpt

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <driverlib/clkctl.h>
#include <inc/hw_lgpt.h>
#include <inc/hw_lgpt1.h>
#include <inc/hw_lgpt3.h>
#include <inc/hw_types.h>
#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>

LOG_MODULE_REGISTER(counter_cc23x0_lgpt, CONFIG_COUNTER_LOG_LEVEL);

#define LGPT_CLK_PRESCALE(pres) (((pres) + 1) << 8)

static void counter_cc23x0_lgpt_isr(const struct device *dev);

struct counter_cc23x0_lgpt_config {
	struct counter_config_info counter_info;
	uint32_t base;
	uint32_t clk_idx;
	uint32_t prescale;
};

struct counter_cc23x0_lgpt_data {
	struct counter_alarm_cfg alarm_cfg[3];
	struct counter_top_cfg target_cfg;
};

static inline void lgpt_cc23x0_pm_policy_state_lock_get(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static inline void lgpt_cc23x0_pm_policy_state_lock_put(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

static int counter_cc23x0_lgpt_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	*ticks = HWREG(config->base + LGPT_O_CNTR);

	return 0;
}

static void counter_cc23x0_lgpt_isr(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	const struct counter_cc23x0_lgpt_data *data = dev->data;
	uint32_t reg_ris = HWREG(config->base + LGPT_O_RIS);
	uint32_t reg_mis = HWREG(config->base + LGPT_O_MIS);
	uint32_t isr = reg_ris & reg_mis;

	HWREG(config->base + LGPT_O_ICLR) |= reg_mis;
	HWREG(config->base + LGPT_O_IMCLR) |= reg_mis;

	LOG_DBG("\nISR -> LGPT[%x] RIS[%x] MIS[%x] ISR[%x]\n", config->base, reg_ris, reg_mis, isr);

	if (isr & LGPT_RIS_TGT) {
		LOG_DBG("LGPT_RIS_TGT\n");
		if (data->target_cfg.callback) {
			data->target_cfg.callback(dev, data->target_cfg.user_data);
		}
	}
	if (isr & LGPT_RIS_ZERO) {
		LOG_DBG("LGPT_RIS_ZERO\n");
	}
	if (isr & LGPT_RIS_DBLTRANS) {
		LOG_DBG("LGPT_RIS_DBLTRANS\n");
	}
	if (isr & LGPT_RIS_CNTRCHNG) {
		LOG_DBG("LGPT_RIS_CNTRCHNG\n");
	}
	if (isr & LGPT_RIS_DIRCHNG) {
		LOG_DBG("LGPT_RIS_DIRCHNG\n");
	}
	if (isr & LGPT_RIS_IDX) {
		LOG_DBG("LGPT_RIS_IDX\n");
	}
	if (isr & LGPT_RIS_FAULT) {
		LOG_DBG("LGPT_RIS_FAULT\n");
	}
	if (isr & LGPT_RIS_C0CC) {
		LOG_DBG("LGPT_RIS_C0CC\n");
		if (data->alarm_cfg[0].callback) {
			data->alarm_cfg[0].callback(dev, 0, HWREG(config->base + LGPT_O_CNTR),
						    data->alarm_cfg[0].user_data);
		}
	}
	if (isr & LGPT_RIS_C1CC) {
		LOG_DBG("LGPT_RIS_C1CC\n");
		if (data->alarm_cfg[1].callback) {
			data->alarm_cfg[1].callback(dev, 1, HWREG(config->base + LGPT_O_CNTR),
						    data->alarm_cfg[1].user_data);
		}
	}
	if (isr & LGPT_RIS_C2CC) {
		LOG_DBG("LGPT_RIS_C2CC\n");
		if (data->alarm_cfg[2].callback) {
			data->alarm_cfg[2].callback(dev, 2, HWREG(config->base + LGPT_O_CNTR),
						    data->alarm_cfg[2].user_data);
		}
	}
}

static uint32_t counter_cc23x0_lgpt_get_freq(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	return (DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) / config->prescale);
}

static int counter_cc23x0_lgpt_set_alarm(const struct device *dev, uint8_t chan_id,
					 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;

	if (alarm_cfg->ticks > config->counter_info.max_top_value) {
		LOG_ERR("Ticks out of range\n");
		return -EINVAL;
	}

	if (chan_id == 0) {
		HWREG(config->base + LGPT_O_IMASK) |= 0x100;
		HWREG(config->base + LGPT_O_C0CC) = alarm_cfg->ticks;
		HWREG(config->base + LGPT_O_C0CFG) = 0x9D;
	} else if (chan_id == 1) {
		HWREG(config->base + LGPT_O_IMASK) |= 0x200;
		HWREG(config->base + LGPT_O_C1CC) = alarm_cfg->ticks;
		HWREG(config->base + LGPT_O_C1CFG) = 0x9D;
	} else if (chan_id == 2) {
		HWREG(config->base + LGPT_O_IMASK) |= 0x400;
		HWREG(config->base + LGPT_O_C2CC) = alarm_cfg->ticks;
		HWREG(config->base + LGPT_O_C2CFG) = 0x9D;
	} else {
		LOG_ERR("Invalid chan ID\n");
		return -ENOTSUP;
	}

	data->alarm_cfg[chan_id].flags = 0;
	data->alarm_cfg[chan_id].ticks = alarm_cfg->ticks;
	data->alarm_cfg[chan_id].callback = alarm_cfg->callback;
	data->alarm_cfg[chan_id].user_data = alarm_cfg->user_data;

	return 0;
}

static int counter_cc23x0_lgpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;

	if (chan_id == 0) {
		HWREG(config->base + LGPT_O_IMCLR) |= 0x100;
		HWREG(config->base + LGPT_O_C0CC) = 0x0;
		HWREG(config->base + LGPT_O_C0CFG) = 0x0;
	} else if (chan_id == 1) {
		HWREG(config->base + LGPT_O_IMCLR) |= 0x200;
		HWREG(config->base + LGPT_O_C1CC) = 0;
		HWREG(config->base + LGPT_O_C1CFG) = 0;
	} else if (chan_id == 2) {
		HWREG(config->base + LGPT_O_IMCLR) |= 0x400;
		HWREG(config->base + LGPT_O_C2CC) = 0;
		HWREG(config->base + LGPT_O_C2CFG) = 0;
	} else {
		LOG_ERR("Invalid chan ID\n");
		return -ENOTSUP;
	}

	data->alarm_cfg[chan_id].flags = 0;
	data->alarm_cfg[chan_id].ticks = 0;
	data->alarm_cfg[chan_id].callback = NULL;
	data->alarm_cfg[chan_id].user_data = NULL;

	return 0;
}

static uint32_t counter_cc23x0_lgpt_get_top_value(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	return HWREG(config->base + LGPT_O_TGT);
}

static int counter_cc23x0_lgpt_set_top_value(const struct device *dev,
					     const struct counter_top_cfg *cfg)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;
	int ret = 0;

	/* If not running set new top value */
	if (HWREG(config->base + LGPT_O_STARTCFG) == 0) {
		HWREG(config->base + LGPT_O_TGT) = config->counter_info.max_top_value;

		HWREG(config->base + LGPT_O_IMASK) |= 0x001;
		HWREG(config->base + LGPT_O_TGT) = cfg->ticks;

		data->target_cfg.flags = 0;
		data->target_cfg.ticks = cfg->ticks;
		data->target_cfg.callback = cfg->callback;
		data->target_cfg.user_data = cfg->user_data;
	} else {
		ret = -EBUSY;
	}

	return ret;
}

static uint32_t counter_cc23x0_lgpt_get_pending_int(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	return HWREG(config->base + LGPT_O_RIS) & HWREG(config->base + LGPT_O_MIS) ? 1 : 0;
}

static int counter_cc23x0_lgpt_start(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	lgpt_cc23x0_pm_policy_state_lock_get();

	LOG_DBG("[START] LGPT base[%x]\n", config->base);

	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_UP_PER;

	/* Set to 1 to start timer */
	HWREG(config->base + LGPT_O_STARTCFG) = 0x1;

	return 0;
}

static int counter_cc23x0_lgpt_stop(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	LOG_DBG("[STOP] LGPT base[%x]\n", config->base);

	/* Set to 0 to stop timer */
	HWREG(config->base + LGPT_O_STARTCFG) = 0x0;

	lgpt_cc23x0_pm_policy_state_lock_put();

	return 0;
}

static void counter_cc23x0_lgpt_init_common(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	HWREG(config->base + LGPT_O_TGT) = config->counter_info.max_top_value;
	HWREG(config->base + LGPT_O_PRECFG) = LGPT_CLK_PRESCALE(config->prescale);
	HWREG(EVTSVT_BASE + EVTSVT_O_LGPTSYNCSEL) = EVTSVT_LGPTSYNCSEL_PUBID_SYSTIM0;
}

#ifdef CONFIG_PM_DEVICE

static int lgpt_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		CLKCTLDisable(CLKCTL_BASE, config->clk_idx);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		CLKCTLEnable(CLKCTL_BASE, config->clk_idx);
		counter_cc23x0_lgpt_init_common(dev);
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(counter, cc23x0_lgpt_api) = {
	.start = counter_cc23x0_lgpt_start,
	.stop = counter_cc23x0_lgpt_stop,
	.get_value = counter_cc23x0_lgpt_get_value,
	.set_alarm = counter_cc23x0_lgpt_set_alarm,
	.cancel_alarm = counter_cc23x0_lgpt_cancel_alarm,
	.get_top_value = counter_cc23x0_lgpt_get_top_value,
	.set_top_value = counter_cc23x0_lgpt_set_top_value,
	.get_pending_int = counter_cc23x0_lgpt_get_pending_int,
	.get_freq = counter_cc23x0_lgpt_get_freq,
};

#define LGPT_CC23X0_INIT_FUNC(inst)								\
	static int counter_cc23x0_lgpt_init##inst(const struct device *dev)			\
	{											\
		const struct counter_cc23x0_lgpt_config *config = dev->config;			\
												\
		CLKCTLEnable(CLKCTL_BASE, config->clk_idx);					\
												\
		IRQ_CONNECT(DT_INST_IRQN(inst),							\
			    DT_INST_IRQ(inst, priority),					\
			    counter_cc23x0_lgpt_isr,						\
			    DEVICE_DT_INST_GET(inst),						\
			    0);									\
												\
		irq_enable(DT_INST_IRQN(inst));							\
												\
		counter_cc23x0_lgpt_init_common(dev);						\
												\
		return 0;									\
	}

#define CC23X0_LGPT_INIT(inst)									\
												\
	LGPT_CC23X0_INIT_FUNC(inst);								\
	PM_DEVICE_DT_INST_DEFINE(inst, lgpt_cc23x0_pm_action);					\
												\
	static const struct counter_cc23x0_lgpt_config cc23x0_lgpt_config_##inst = {		\
		.counter_info = {								\
			.max_top_value = DT_INST_PROP(inst, max_top_value),			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,					\
			.channels = 3,								\
		},										\
		.base = DT_INST_REG_ADDR(inst),							\
		.clk_idx = CLKCTL_LGPT##inst,							\
		.prescale = DT_INST_PROP(inst, clk_prescale),					\
	};											\
												\
	static struct counter_cc23x0_lgpt_data cc23x0_lgpt_data_##inst;				\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &counter_cc23x0_lgpt_init##inst,					\
			      PM_DEVICE_DT_INST_GET(inst),					\
			      &cc23x0_lgpt_data_##inst,						\
			      &cc23x0_lgpt_config_##inst,					\
			      POST_KERNEL,							\
			      CONFIG_COUNTER_INIT_PRIORITY,					\
			      &cc23x0_lgpt_api);

DT_INST_FOREACH_STATUS_OKAY(CC23X0_LGPT_INIT);
