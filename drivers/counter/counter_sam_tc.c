/*
 * Copyright (c) 2021, Piotr Mienkowski
 * Copyright (c) 2023, Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_tc

/** @file
 * @brief Atmel SAM MCU family counter (TC) driver.
 *
 * This version of the driver uses a single channel to provide a basic 16-bit
 * counter (on SAM4E series the counter is 32-bit). Remaining TC channels could
 * be used in the future to provide additional functionality, e.g. input clock
 * divider configured via DT properties.
 *
 * Remarks:
 * - The driver is not thread safe.
 * - The driver does not implement guard periods.
 * - The driver does not guarantee that short relative alarm will trigger the
 *   interrupt immediately and not after the full cycle / counter overflow.
 *
 * Use at your own risk or submit a patch.
 */

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(counter_sam_tc, CONFIG_COUNTER_LOG_LEVEL);

#define MAX_ALARMS_PER_TC_CHANNEL 2
#if defined(CONFIG_SOC_SERIES_SAM4E) || defined(CONFIG_SOC_SERIES_SAM3X)
#define COUNTER_SAM_TOP_VALUE_MAX UINT32_MAX
#else
#define COUNTER_SAM_TOP_VALUE_MAX UINT16_MAX
#define COUNTER_SAM_16_BIT
#endif

/* Device constant configuration parameters */
struct counter_sam_dev_cfg {
	struct counter_config_info info;
	Tc *regs;
	uint32_t reg_cmr;
	uint32_t reg_rc;
	void (*irq_config_func)(const struct device *dev);
	const struct atmel_sam_pmc_config clock_cfg[TCCHANNEL_NUMBER];
	const struct pinctrl_dev_config *pcfg;
	uint8_t clk_sel;
	bool nodivclk;
	uint8_t tc_chan_num;
};

struct counter_sam_alarm_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

/* Device run time data */
struct counter_sam_dev_data {
	counter_top_callback_t top_cb;
	void *top_user_data;

	struct counter_sam_alarm_data alarm[MAX_ALARMS_PER_TC_CHANNEL];
};

static const uint32_t sam_tc_input_freq_table[] = {
#if defined(CONFIG_SOC_SERIES_SAMX7X)
	USEC_PER_SEC,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 8,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 32,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 128,
	32768,
#elif defined(CONFIG_SOC_SERIES_SAM4L)
	1024,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 2,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 8,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 32,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 128,
#else
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 2,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 8,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 32,
	SOC_ATMEL_SAM_MCK_FREQ_HZ / 128,
	32768,
#endif
	USEC_PER_SEC, USEC_PER_SEC, USEC_PER_SEC,
};

static int counter_sam_tc_start(const struct device *dev)
{
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];

	tc_ch->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

	return 0;
}

static int counter_sam_tc_stop(const struct device *dev)
{
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];

	tc_ch->TC_CCR = TC_CCR_CLKDIS;

	return 0;
}

static int counter_sam_tc_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];

	*ticks = tc_ch->TC_CV;

	return 0;
}

static int counter_sam_tc_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_sam_dev_data *data = dev->data;
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];
	uint32_t top_value;
	uint32_t alarm_value;

	__ASSERT_NO_MSG(alarm_cfg->callback != NULL);

	top_value = tc_ch->TC_RC;

	if ((top_value != 0) && (alarm_cfg->ticks > top_value)) {
		return -EINVAL;
	}

#ifdef COUNTER_SAM_16_BIT
	if ((top_value == 0) && (alarm_cfg->ticks > UINT16_MAX)) {
		return -EINVAL;
	}
#endif

	if (data->alarm[chan_id].callback != NULL) {
		return -EBUSY;
	}

	if (chan_id == 0) {
		tc_ch->TC_IDR = TC_IDR_CPAS;
	} else {
		tc_ch->TC_IDR = TC_IDR_CPBS;
	}

	data->alarm[chan_id].callback = alarm_cfg->callback;
	data->alarm[chan_id].user_data = alarm_cfg->user_data;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		alarm_value = alarm_cfg->ticks;
	} else {
		alarm_value = tc_ch->TC_CV + alarm_cfg->ticks;
		if (top_value != 0) {
			alarm_value %= top_value;
		}
	}

	if (chan_id == 0) {
		tc_ch->TC_RA = alarm_value;
		/* Clear interrupt status register */
		(void)tc_ch->TC_SR;
		tc_ch->TC_IER = TC_IER_CPAS;
	} else {
		tc_ch->TC_RB = alarm_value;
		/* Clear interrupt status register */
		(void)tc_ch->TC_SR;
		tc_ch->TC_IER = TC_IER_CPBS;
	}

	LOG_DBG("set alarm: channel %u, count %u", chan_id, alarm_value);

	return 0;
}

static int counter_sam_tc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_sam_dev_data *data = dev->data;
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];

	if (chan_id == 0) {
		tc_ch->TC_IDR = TC_IDR_CPAS;
		tc_ch->TC_RA = 0;
	} else {
		tc_ch->TC_IDR = TC_IDR_CPBS;
		tc_ch->TC_RB = 0;
	}

	data->alarm[chan_id].callback = NULL;
	data->alarm[chan_id].user_data = NULL;

	LOG_DBG("cancel alarm: channel %u", chan_id);

	return 0;
}

static int counter_sam_tc_set_top_value(const struct device *dev,
					const struct counter_top_cfg *top_cfg)
{
	struct counter_sam_dev_data *data = dev->data;
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];
	int ret = 0;

	for (int i = 0; i < MAX_ALARMS_PER_TC_CHANNEL; i++) {
		if (data->alarm[i].callback) {
			return -EBUSY;
		}
	}

	/* Disable the compare interrupt */
	tc_ch->TC_IDR = TC_IDR_CPCS;

	data->top_cb = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	tc_ch->TC_RC = top_cfg->ticks;

	if ((top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0) {
		if (tc_ch->TC_CV >= top_cfg->ticks) {
			ret = -ETIME;
			if ((top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0) {
				tc_ch->TC_CCR = TC_CCR_SWTRG;
			}
		}
	} else {
		tc_ch->TC_CCR = TC_CCR_SWTRG;
	}

	/* Enable the compare interrupt */
	tc_ch->TC_IER = TC_IER_CPCS;

	return ret;
}

static uint32_t counter_sam_tc_get_top_value(const struct device *dev)
{
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];

	return tc_ch->TC_RC;
}

static uint32_t counter_sam_tc_get_pending_int(const struct device *dev)
{
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];

	return tc_ch->TC_SR & tc_ch->TC_IMR;
}

static void counter_sam_tc_isr(const struct device *dev)
{
	struct counter_sam_dev_data *data = dev->data;
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];
	uint32_t status;

	status = tc_ch->TC_SR;

	if ((status & TC_SR_CPAS) != 0) {
		tc_ch->TC_IDR = TC_IDR_CPAS;
		if (data->alarm[0].callback) {
			counter_alarm_callback_t cb = data->alarm[0].callback;

			data->alarm[0].callback = NULL;
			cb(dev, 0, tc_ch->TC_RA, data->alarm[0].user_data);
		}
	}

	if ((status & TC_SR_CPBS) != 0) {
		tc_ch->TC_IDR = TC_IDR_CPBS;
		if (data->alarm[1].callback) {
			counter_alarm_callback_t cb = data->alarm[1].callback;

			data->alarm[1].callback = NULL;
			cb(dev, 1, tc_ch->TC_RB, data->alarm[1].user_data);
		}
	}

	if ((status & TC_SR_CPCS) != 0) {
		if (data->top_cb) {
			data->top_cb(dev, data->top_user_data);
		}
	}
}

static int counter_sam_initialize(const struct device *dev)
{
	const struct counter_sam_dev_cfg *const dev_cfg = dev->config;
	Tc *const tc = dev_cfg->regs;
	TcChannel *tc_ch = &tc->TcChannel[dev_cfg->tc_chan_num];
	int retval;

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0 && retval != -ENOENT) {
		return retval;
	}

	/* Enable channel's clock */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&dev_cfg->clock_cfg[dev_cfg->tc_chan_num]);

	/* Clock and Mode Selection */
	tc_ch->TC_CMR = dev_cfg->reg_cmr;
	tc_ch->TC_RC = dev_cfg->reg_rc;

#ifdef TC_EMR_NODIVCLK
	if (dev_cfg->nodivclk) {
		tc_ch->TC_EMR = TC_EMR_NODIVCLK;
	}
#endif
	dev_cfg->irq_config_func(dev);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(counter, counter_sam_driver_api) = {
	.start = counter_sam_tc_start,
	.stop = counter_sam_tc_stop,
	.get_value = counter_sam_tc_get_value,
	.set_alarm = counter_sam_tc_set_alarm,
	.cancel_alarm = counter_sam_tc_cancel_alarm,
	.set_top_value = counter_sam_tc_set_top_value,
	.get_top_value = counter_sam_tc_get_top_value,
	.get_pending_int = counter_sam_tc_get_pending_int,
};

#define COUNTER_SAM_TC_CMR(n)	\
		 (TC_CMR_TCCLKS(DT_INST_PROP_OR(n, clk, 0)) \
		| TC_CMR_WAVEFORM_WAVSEL_UP_RC \
		| TC_CMR_WAVE)

#define COUNTER_SAM_TC_REG_CMR(n) \
		DT_INST_PROP_OR(n, reg_cmr, COUNTER_SAM_TC_CMR(n))

#define COUNTER_SAM_TC_INPUT_FREQUENCY(n)	\
		COND_CODE_1(DT_INST_PROP(n, nodivclk), \
			    (SOC_ATMEL_SAM_MCK_FREQ_HZ), \
			    (sam_tc_input_freq_table[COUNTER_SAM_TC_REG_CMR(n) \
						     & TC_CMR_TCCLKS_Msk]))

#define COUNTER_SAM_TC_INIT(n)					\
PINCTRL_DT_INST_DEFINE(n);					\
								\
static void counter_##n##_sam_config_func(const struct device *dev); \
								\
static const struct counter_sam_dev_cfg counter_##n##_sam_config = { \
	.info = {						\
		.max_top_value = COUNTER_SAM_TOP_VALUE_MAX,	\
		.freq = COUNTER_SAM_TC_INPUT_FREQUENCY(n),	\
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
		.channels = MAX_ALARMS_PER_TC_CHANNEL		\
	},							\
	.regs = (Tc *)DT_INST_REG_ADDR(n),			\
	.reg_cmr = COUNTER_SAM_TC_REG_CMR(n),			\
	.reg_rc = DT_INST_PROP_OR(n, reg_rc, 0),		\
	.irq_config_func = &counter_##n##_sam_config_func,	\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	.nodivclk = DT_INST_PROP(n, nodivclk),			\
	.tc_chan_num = DT_INST_PROP_OR(n, channel, 0),		\
	.clock_cfg = SAM_DT_INST_CLOCKS_PMC_CFG(n),		\
};								\
								\
static struct counter_sam_dev_data counter_##n##_sam_data;	\
								\
DEVICE_DT_INST_DEFINE(n, counter_sam_initialize, NULL, \
		&counter_##n##_sam_data, &counter_##n##_sam_config, \
		PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,	\
		&counter_sam_driver_api);			\
								\
static void counter_##n##_sam_config_func(const struct device *dev) \
{								\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),		\
		    DT_INST_IRQ_BY_IDX(n, 0, priority),		\
		    counter_sam_tc_isr,				\
		    DEVICE_DT_INST_GET(n), 0);			\
	irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));		\
								\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),		\
		    DT_INST_IRQ_BY_IDX(n, 1, priority),		\
		    counter_sam_tc_isr,				\
		    DEVICE_DT_INST_GET(n), 0);			\
	irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));		\
								\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 2, irq),		\
		    DT_INST_IRQ_BY_IDX(n, 2, priority),		\
		    counter_sam_tc_isr,				\
		    DEVICE_DT_INST_GET(n), 0);			\
	irq_enable(DT_INST_IRQ_BY_IDX(n, 2, irq));		\
}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_SAM_TC_INIT)
