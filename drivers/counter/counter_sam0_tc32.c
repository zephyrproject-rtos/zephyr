/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <device.h>
#include <soc.h>
#include <clock_control.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(counter_sam0_tc32, CONFIG_COUNTER_LOG_LEVEL);

struct counter_sam0_tc32_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_sam0_tc32_data {
	counter_top_callback_t top_cb;
	void *top_user_data;

	struct counter_sam0_tc32_ch_data ch;
};

struct counter_sam0_tc32_config {
	struct counter_config_info info;
	TcCount32 *regs;

	const char *clk_dev;
	clock_control_subsys_t clk_sys;
#ifdef MCLK
	volatile u32_t *mclk;
	u32_t mclk_mask;
#else
	u32_t pm_apbcmask;
#endif

	void (*irq_config_func)(struct device *dev);
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) ((const struct counter_sam0_tc32_config *const) \
		      (dev)->config->config_info)
#define DEV_DATA(dev) ((struct counter_sam0_tc32_data *const) \
		       (dev)->driver_data)


static void wait_synchronization(TcCount32 *regs)
{
#if defined(TC_SYNCBUSY_MASK)
	/* SYNCBUSY is a register */
	while ((regs->SYNCBUSY.reg & TC_SYNCBUSY_MASK) != 0) {
	}
#elif defined(TC_STATUS_SYNCBUSY)
	/* SYNCBUSY is a bit */
	while ((regs->STATUS.reg & TC_STATUS_SYNCBUSY) != 0) {
	}
#else
#error Unsupported device
#endif
}

static void read_synchronize_count(TcCount32 *regs)
{
#if defined(TC_READREQ_RREQ)
	regs->READREQ.reg = TC_READREQ_RREQ |
			    TC_READREQ_ADDR(TC_COUNT32_COUNT_OFFSET);
	wait_synchronization(regs);
#elif defined(TC_CTRLBSET_CMD_READSYNC)
	regs->CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;
	wait_synchronization(regs);
#else
	ARG_UNUSED(regs);
#endif
}

static int counter_sam0_tc32_start(struct device *dev)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	/*
	 * This will also reset the current counter value if it's
	 * already running.
	 */
	tc->CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
	wait_synchronization(tc);
	return 0;
}

static int counter_sam0_tc32_stop(struct device *dev)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	/*
	 * The older (pre SAML1x) manuals claim the counter retains its
	 * value on stop, but this doesn't actually seem to happen.
	 * The SAML1x manual says it resets, which is what the SAMD21
	 * counter actually appears to do.
	 */
	tc->CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
	wait_synchronization(tc);
	return 0;
}

static u32_t counter_sam0_tc32_read(struct device *dev)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	read_synchronize_count(tc);
	return tc->COUNT.reg;
}

static void counter_sam0_tc32_relative_alarm(struct device *dev, u32_t ticks)
{
	struct counter_sam0_tc32_data *data = DEV_DATA(dev);
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;
	u32_t before;
	u32_t target;
	u32_t after;
	u32_t max;

	read_synchronize_count(tc);
	before = tc->COUNT.reg;

	target = before + ticks;
	max = tc->CC[0].reg;
	if (target > max) {
		target -= max;
	}

	tc->CC[1].reg = target;
	wait_synchronization(tc);
	tc->INTFLAG.reg = TC_INTFLAG_MC1;

	read_synchronize_count(tc);
	after = tc->COUNT.reg;

	/* Pending now, so no further checking required */
	if (tc->INTFLAG.bit.MC1) {
		goto out_future;
	}

	/*
	 * Check if we missed the interrupt and call the handler
	 * immediately if we did.
	 */
	if (after < target) {
		goto out_future;
	}

	/* Check wrapped */
	if (target < before && after >= before) {
		goto out_future;
	}

	counter_alarm_callback_t cb = data->ch.callback;

	tc->INTENCLR.reg = TC_INTENCLR_MC1;
	tc->INTFLAG.reg = TC_INTFLAG_MC1;
	data->ch.callback = NULL;

	cb(dev, 0, target, data->ch.user_data);

	return;

out_future:
	tc->INTENSET.reg = TC_INTFLAG_MC1;
}

static int counter_sam0_tc32_set_alarm(struct device *dev, u8_t chan_id,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_sam0_tc32_data *data = DEV_DATA(dev);
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	ARG_UNUSED(chan_id);

	if (alarm_cfg->ticks > tc->CC[0].reg) {
		return -EINVAL;
	}

	int key = irq_lock();

	if (data->ch.callback) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->ch.callback = alarm_cfg->callback;
	data->ch.user_data = alarm_cfg->user_data;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		tc->CC[1].reg = alarm_cfg->ticks;
		wait_synchronization(tc);
		tc->INTFLAG.reg = TC_INTFLAG_MC1;
		tc->INTENSET.reg = TC_INTFLAG_MC1;
	} else {
		counter_sam0_tc32_relative_alarm(dev, alarm_cfg->ticks);
	}

	irq_unlock(key);

	return 0;
}

static int counter_sam0_tc32_cancel_alarm(struct device *dev, u8_t chan_id)
{
	struct counter_sam0_tc32_data *data = DEV_DATA(dev);
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	int key = irq_lock();

	ARG_UNUSED(chan_id);

	data->ch.callback = NULL;
	tc->INTENCLR.reg = TC_INTENCLR_MC1;
	tc->INTFLAG.reg = TC_INTFLAG_MC1;

	irq_unlock(key);
	return 0;
}

static int counter_sam0_tc32_set_top_value(struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	struct counter_sam0_tc32_data *data = DEV_DATA(dev);
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;
	int err = 0;
	int key = irq_lock();

	if (data->ch.callback) {
		irq_unlock(key);
		return -EBUSY;
	}

	if (top_cfg->callback) {
		data->top_cb = top_cfg->callback;
		data->top_user_data = top_cfg->user_data;
		tc->INTENSET.reg = TC_INTFLAG_MC0;
	} else {
		tc->INTENCLR.reg = TC_INTFLAG_MC0;
	}

	tc->CC[0].reg = top_cfg->ticks;

	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		/*
		 * Top trigger is on equality of the rising edge only, so
		 * manually reset it if the counter has missed the new top.
		 */
		if (counter_sam0_tc32_read(dev) >= top_cfg->ticks) {
			err = -ETIME;
			if (top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				tc->CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
			}
		}
	} else {
		tc->CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
	}

	wait_synchronization(tc);

	tc->INTFLAG.reg = TC_INTFLAG_MC0;
	irq_unlock(key);
	return err;
}

static u32_t counter_sam0_tc32_get_pending_int(struct device *dev)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	return tc->INTFLAG.reg & (TC_INTFLAG_MC0 | TC_INTFLAG_MC1);
}

static u32_t counter_sam0_tc32_get_top_value(struct device *dev)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;

	/*
	 * Unsync read is safe here because we're not using
	 * capture mode, so things are only set from the CPU
	 * end.
	 */
	return tc->CC[0].reg;
}

static u32_t counter_sam0_tc32_get_max_relative_alarm(struct device *dev)
{
	return counter_sam0_tc32_get_top_value(dev) - 1;
}

static void counter_sam0_tc32_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct counter_sam0_tc32_data *data = DEV_DATA(dev);
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;
	u8_t status = tc->INTFLAG.reg;

	/* Acknowledge all interrupts */
	tc->INTFLAG.reg = status;

	if (status & TC_INTFLAG_MC1) {
		if (data->ch.callback) {
			counter_alarm_callback_t cb = data->ch.callback;

			tc->INTENCLR.reg = TC_INTENCLR_MC1;
			data->ch.callback = NULL;

			cb(dev, 0, tc->CC[1].reg, data->ch.user_data);
		}
	}

	if (status & TC_INTFLAG_MC0) {
		if (data->top_cb) {
			data->top_cb(dev, data->top_user_data);
		}
	}
}

static int  counter_sam0_calculate_prescaler(struct device *dev,
					     u32_t clk_freq)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;
	u32_t reg;
	u32_t div;


	/*
	 * In 32 bit mode, NFRQ mode always uses MAX as the counter top, so
	 * use MFRQ mode which uses CC0 as the top at the expense of only
	 * having CC1 available for alarms.
	 */
	reg = TC_CTRLA_MODE_COUNT32;
#ifdef TC_CTRLA_WAVEGEN_MFRQ
	reg |= TC_CTRLA_WAVEGEN_MFRQ;
#endif

	div = clk_freq / cfg->info.freq;
	switch (div) {
	case 1:
		reg |= TC_CTRLA_PRESCALER_DIV1;
		break;
	case 2:
		reg |= TC_CTRLA_PRESCALER_DIV2;
		break;
	case 4:
		reg |= TC_CTRLA_PRESCALER_DIV4;
		break;
	case 16:
		reg |= TC_CTRLA_PRESCALER_DIV16;
		break;
	case 64:
		reg |= TC_CTRLA_PRESCALER_DIV64;
		break;
	case 256:
		reg |= TC_CTRLA_PRESCALER_DIV256;
		break;
	case 1024:
		reg |= TC_CTRLA_PRESCALER_DIV1024;
		break;
	default:
		LOG_ERR("Unable to match counter %s requested frequency (%u/%u)",
			DEV_NAME(dev), clk_freq, cfg->info.freq);
		return -EINVAL;
	}

	clk_freq /= div;
	if (clk_freq != cfg->info.freq) {
		LOG_ERR("Inexact frequency match for counter %s (%u vs %u)",
			DEV_NAME(dev), clk_freq, cfg->info.freq);
		return -EINVAL;
	}


	tc->CTRLA.reg = reg;
	wait_synchronization(tc);

	return 0;
}

static int counter_sam0_tc32_initialize(struct device *dev)
{
	const struct counter_sam0_tc32_config *const cfg = DEV_CFG(dev);
	TcCount32 *tc = cfg->regs;
	struct device *clk = device_get_binding(cfg->clk_dev);
	u32_t clk_freq;
	int retval;

	if (!clk) {
		return -EINVAL;
	}

	clock_control_on(clk, cfg->clk_sys);
	clock_control_get_rate(clk, cfg->clk_sys, &clk_freq);

#ifdef MCLK
	/* Enable TC clock in MCLK */
	*cfg->mclk |= cfg->mclk_mask;
#else
	/* Enable clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;
#endif

	retval = counter_sam0_calculate_prescaler(dev, clk_freq);
	if (retval != 0) {
		return retval;
	}

#ifdef TC_WAVE_WAVEGEN_MFRQ
	tc->WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
#endif

	/* Disable all interrupts */
	tc->INTENCLR.reg = TC_INTENCLR_MASK;

	/* Set the initial top as the maximum */
	tc->CC[0].reg = UINT32_MAX;

	cfg->irq_config_func(dev);

	tc->CTRLA.bit.ENABLE = 1;
	wait_synchronization(tc);

	/* Stop the counter initially */
	tc->CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
	wait_synchronization(tc);

	return 0;
}

static const struct counter_driver_api counter_sam0_tc32_driver_api = {
	.start = counter_sam0_tc32_start,
	.stop = counter_sam0_tc32_stop,
	.read = counter_sam0_tc32_read,
	.set_alarm = counter_sam0_tc32_set_alarm,
	.cancel_alarm = counter_sam0_tc32_cancel_alarm,
	.set_top_value = counter_sam0_tc32_set_top_value,
	.get_pending_int = counter_sam0_tc32_get_pending_int,
	.get_top_value = counter_sam0_tc32_get_top_value,
	.get_max_relative_alarm = counter_sam0_tc32_get_max_relative_alarm,
};


#ifdef MCLK
#define COUNTER_SAM0_TC32_CLOCK_CONTROL(n) \
	.mclk = MCLK_TC##n,		   \
	.mclk_mask = MCLK_TC##n##_MASK,
#else
#define COUNTER_SAM0_TC32_CLOCK_CONTROL(n) \
	.pm_apbcmask = PM_APBCMASK_TC##n,
#endif

#define COUNTER_SAM0_TC32_DEVICE(n)					      \
	static void counter_sam0_tc32_config_##n(struct device *dev);	      \
	static const struct counter_sam0_tc32_config			      \
	counter_sam0_tc32_dev_config_##n = {				      \
		.info = {						      \
			.max_top_value = UINT32_MAX,			      \
			.freq = DT_ATMEL_SAM0_TC32_TC_##n##_CLOCK_FREQUENCY,  \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		      \
			.channels = 1					      \
		},							      \
		.regs = (TcCount32 *)DT_ATMEL_SAM0_TC32_TC_##n##_BASE_ADDRESS,\
		.clk_dev = DT_ATMEL_SAM0_TC32_TC_##n##_CLOCK_CONTROLLER,      \
		.clk_sys = (clock_control_subsys_t)TC##n##_GCLK_ID,	      \
		COUNTER_SAM0_TC32_CLOCK_CONTROL(n)			      \
		.irq_config_func = &counter_sam0_tc32_config_##n,	      \
	};								      \
	static struct counter_sam0_tc32_data counter_sam0_tc32_dev_data_##n;  \
	DEVICE_AND_API_INIT(counter_sam0_tc32_##n,			      \
			    DT_ATMEL_SAM0_TC32_TC_##n##_LABEL,		      \
			    &counter_sam0_tc32_initialize,		      \
			    &counter_sam0_tc32_dev_data_##n,		      \
			    &counter_sam0_tc32_dev_config_##n, PRE_KERNEL_1,  \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		      \
			    &counter_sam0_tc32_driver_api);		      \
	static void counter_sam0_tc32_config_##n(struct device *dev)	      \
	{								      \
		IRQ_CONNECT(DT_ATMEL_SAM0_TC32_TC_##n##_IRQ_0,		      \
			    DT_ATMEL_SAM0_TC32_TC_##n##_IRQ_0_PRIORITY,	      \
			    counter_sam0_tc32_isr,			      \
			    DEVICE_GET(counter_sam0_tc32_##n),		      \
			    0);						      \
		irq_enable(DT_ATMEL_SAM0_TC32_TC_##n##_IRQ_0);		      \
	}

#if DT_ATMEL_SAM0_TC32_TC_0_BASE_ADDRESS
COUNTER_SAM0_TC32_DEVICE(0);
#endif

#if DT_ATMEL_SAM0_TC32_TC_2_BASE_ADDRESS
COUNTER_SAM0_TC32_DEVICE(2);
#endif

#if DT_ATMEL_SAM0_TC32_TC_4_BASE_ADDRESS
COUNTER_SAM0_TC32_DEVICE(4);
#endif

#if DT_ATMEL_SAM0_TC32_TC_6_BASE_ADDRESS
COUNTER_SAM0_TC32_DEVICE(6);
#endif
