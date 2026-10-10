/*
 * Copyright (c) 2024 BayLibre, SAS
 * Copyright (c) 2026 Texas Instruments Incorporated
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

/* Prescaler register value - hardware divides by (TICKDIV + 1) */
#define LGPT_CLK_PRESCALE(pres) ((pres) << 8)

/* Number of capture/compare channels available per LGPT instance */
#define LGPT_NUM_CHANNELS 3

#define LGPT_CCFG_COMPARE_MODE (LGPT_C0CFG_CCACT_SET_ON_CMP | LGPT_C0CFG_EDGE_RISE)

static void counter_cc23x0_lgpt_isr(const struct device *dev);

struct counter_cc23x0_lgpt_config {
	struct counter_config_info counter_info;
	uint32_t base;
	uint32_t clk_idx;
	uint32_t prescale;
};

struct counter_cc23x0_lgpt_data {
	struct counter_alarm_cfg alarm_cfg[LGPT_NUM_CHANNELS];
	struct counter_top_cfg target_cfg;
	bool is_running;
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

	/* Clear interrupt flags - use ICLR only, not IMCLR which disables interrupts */
	HWREG(config->base + LGPT_O_ICLR) = reg_mis;

	LOG_DBG("ISR -> LGPT[%x] RIS[%x] MIS[%x] ISR[%x]", config->base, reg_ris, reg_mis, isr);

	/* Target interrupt (top value reached) - used by counter_set_top_value() */
	if (isr & LGPT_RIS_TGT) {
		LOG_DBG("LGPT_RIS_TGT");
		if (data->target_cfg.callback) {
			data->target_cfg.callback(dev, data->target_cfg.user_data);
		}
	}

	if (isr & LGPT_RIS_ZERO) {
		LOG_DBG("LGPT_RIS_ZERO");
	}
	if (isr & LGPT_RIS_DBLTRANS) {
		LOG_DBG("LGPT_RIS_DBLTRANS");
	}
	if (isr & LGPT_RIS_CNTRCHNG) {
		LOG_DBG("LGPT_RIS_CNTRCHNG");
	}
	if (isr & LGPT_RIS_DIRCHNG) {
		LOG_DBG("LGPT_RIS_DIRCHNG");
	}
	if (isr & LGPT_RIS_IDX) {
		LOG_DBG("LGPT_RIS_IDX");
	}
	if (isr & LGPT_RIS_FAULT) {
		LOG_DBG("LGPT_RIS_FAULT");
	}

	/* Channel 0 capture/compare interrupt - used by counter_set_alarm() */
	if (isr & LGPT_RIS_C0CC) {
		LOG_DBG("LGPT_RIS_C0CC");
		/* Disable channel interrupt for one-shot behavior */
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_IMASK_C0CC;
		if (data->alarm_cfg[0].callback) {
			data->alarm_cfg[0].callback(dev, 0, HWREG(config->base + LGPT_O_CNTR),
						    data->alarm_cfg[0].user_data);
		}
	}

	/* Channel 1 capture/compare interrupt */
	if (isr & LGPT_RIS_C1CC) {
		LOG_DBG("LGPT_RIS_C1CC");
		/* Disable channel interrupt for one-shot behavior */
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_IMASK_C1CC;
		if (data->alarm_cfg[1].callback) {
			data->alarm_cfg[1].callback(dev, 1, HWREG(config->base + LGPT_O_CNTR),
						    data->alarm_cfg[1].user_data);
		}
	}

	/* Channel 2 capture/compare interrupt */
	if (isr & LGPT_RIS_C2CC) {
		LOG_DBG("LGPT_RIS_C2CC");
		/* Disable channel interrupt for one-shot behavior */
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_IMASK_C2CC;
		if (data->alarm_cfg[2].callback) {
			data->alarm_cfg[2].callback(dev, 2, HWREG(config->base + LGPT_O_CNTR),
						    data->alarm_cfg[2].user_data);
		}
	}
}

static uint32_t counter_cc23x0_lgpt_get_freq(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	/* Hardware divides by (prescale + 1) */
	return (DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) / (config->prescale + 1));
}

static int counter_cc23x0_lgpt_set_alarm(const struct device *dev, uint8_t chan_id,
					 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;
	uint32_t compare_val;
	uint32_t top_val;

	/* Get current top value for bounds checking */
	top_val = HWREG(config->base + LGPT_O_TGT);

	/* Validate ticks against current top value */
	if (alarm_cfg->ticks > top_val) {
		LOG_ERR("Ticks out of range");
		return -EINVAL;
	}

	/* Calculate compare value based on absolute/relative flag */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		compare_val = alarm_cfg->ticks;
	} else {
		/* Relative alarm - add current counter value with wrap */
		uint32_t current = HWREG(config->base + LGPT_O_CNTR);

		compare_val = (current + alarm_cfg->ticks) % (top_val + 1);
	}

	/* Configure channel based on channel ID */
	switch (chan_id) {
	case 0:
		HWREG(config->base + LGPT_O_C0CC) = compare_val;
		HWREG(config->base + LGPT_O_C0CFG) = LGPT_CCFG_COMPARE_MODE;
		HWREG(config->base + LGPT_O_IMSET) = LGPT_IMASK_C0CC;
		break;
	case 1:
		HWREG(config->base + LGPT_O_C1CC) = compare_val;
		HWREG(config->base + LGPT_O_C1CFG) = LGPT_CCFG_COMPARE_MODE;
		HWREG(config->base + LGPT_O_IMSET) = LGPT_IMASK_C1CC;
		break;
	case 2:
		HWREG(config->base + LGPT_O_C2CC) = compare_val;
		HWREG(config->base + LGPT_O_C2CFG) = LGPT_CCFG_COMPARE_MODE;
		HWREG(config->base + LGPT_O_IMSET) = LGPT_IMASK_C2CC;
		break;
	default:
		return -ENOTSUP;
	}

	/* Save alarm configuration */
	data->alarm_cfg[chan_id].flags = alarm_cfg->flags;
	data->alarm_cfg[chan_id].ticks = alarm_cfg->ticks;
	data->alarm_cfg[chan_id].callback = alarm_cfg->callback;
	data->alarm_cfg[chan_id].user_data = alarm_cfg->user_data;

	LOG_DBG("Set alarm ch%d: ticks=%u compare=%u top=%u", chan_id,
		alarm_cfg->ticks, compare_val, top_val);

	return 0;
}

static int counter_cc23x0_lgpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;

	if (chan_id >= config->counter_info.channels) {
		LOG_ERR("Invalid chan ID");
		return -ENOTSUP;
	}

	/* Disable interrupt and clear channel config */
	switch (chan_id) {
	case 0:
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_IMASK_C0CC;
		HWREG(config->base + LGPT_O_C0CC) = 0x0;
		HWREG(config->base + LGPT_O_C0CFG) = 0x0;
		break;
	case 1:
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_IMASK_C1CC;
		HWREG(config->base + LGPT_O_C1CC) = 0x0;
		HWREG(config->base + LGPT_O_C1CFG) = 0x0;
		break;
	case 2:
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_IMASK_C2CC;
		HWREG(config->base + LGPT_O_C2CC) = 0x0;
		HWREG(config->base + LGPT_O_C2CFG) = 0x0;
		break;
	default:
		return -ENOTSUP;
	}

	/* Clear saved configuration */
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

	/* Validate ticks parameter */
	if (cfg->ticks > config->counter_info.max_top_value || cfg->ticks == 0) {
		return -EINVAL;
	}

	/* Set new top value - LGPT hardware supports this whether running or not */
	HWREG(config->base + LGPT_O_TGT) = cfg->ticks;

	/* Enable interrupt if callback provided */
	if (cfg->callback) {
		HWREG(config->base + LGPT_O_IMSET) = LGPT_RIS_TGT;
	} else {
		HWREG(config->base + LGPT_O_IMCLR) = LGPT_RIS_TGT;
	}

	/* Reset counter if COUNTER_TOP_CFG_DONT_RESET is NOT set */
	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		HWREG(config->base + LGPT_O_CNTR) = 0x0;
	}

	/* Save configuration */
	data->target_cfg.flags = cfg->flags;
	data->target_cfg.ticks = cfg->ticks;
	data->target_cfg.callback = cfg->callback;
	data->target_cfg.user_data = cfg->user_data;

	return 0;
}

static uint32_t counter_cc23x0_lgpt_get_pending_int(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	return HWREG(config->base + LGPT_O_RIS) & HWREG(config->base + LGPT_O_MIS) ? 1 : 0;
}

static int counter_cc23x0_lgpt_start(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;
	uint32_t reg_val;

	/* Prevent double start - only acquire lock if not already running */
	if (data->is_running) {
		return 0;
	}

	lgpt_cc23x0_pm_policy_state_lock_get();
	data->is_running = true;

	LOG_DBG("[START] LGPT base[%x]", config->base);

	/* Read current CTL value, clear mode and channel reset bits, then set UP_PER mode */
	reg_val = HWREG(config->base + LGPT_O_CTL);
	reg_val &= ~(LGPT_CTL_MODE_M | LGPT_CTL_C2RST | LGPT_CTL_C1RST | LGPT_CTL_C0RST);
	reg_val |= LGPT_CTL_MODE_UP_PER;
	HWREG(config->base + LGPT_O_CTL) = reg_val;

	return 0;
}

static int counter_cc23x0_lgpt_stop(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;
	struct counter_cc23x0_lgpt_data *data = dev->data;
	uint32_t reg_val;

	/* Prevent double stop - only release lock if actually running */
	if (!data->is_running) {
		return 0;
	}

	LOG_DBG("[STOP] LGPT base[%x]", config->base);

	/* Stop timer by setting mode to DIS */
	reg_val = HWREG(config->base + LGPT_O_CTL);
	reg_val &= ~LGPT_CTL_MODE_M;
	reg_val |= LGPT_CTL_MODE_DIS;
	HWREG(config->base + LGPT_O_CTL) = reg_val;

	data->is_running = false;
	lgpt_cc23x0_pm_policy_state_lock_put();

	return 0;
}

static void counter_cc23x0_lgpt_init_common(const struct device *dev)
{
	const struct counter_cc23x0_lgpt_config *config = dev->config;

	/* Reset counter to 0 */
	HWREG(config->base + LGPT_O_CNTR) = 0x0;
	/* Set target value */
	HWREG(config->base + LGPT_O_TGT) = config->counter_info.max_top_value;
	/* Configure prescaler */
	HWREG(config->base + LGPT_O_PRECFG) = LGPT_CLK_PRESCALE(config->prescale);
	/* Configure sync select */
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
			.channels = LGPT_NUM_CHANNELS,						\
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
