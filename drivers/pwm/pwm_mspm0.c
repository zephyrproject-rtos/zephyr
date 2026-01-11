/*
 * Copyright (c) 2025, Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_timer_pwm

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Driverlib includes */
#include <ti/driverlib/dl_timera.h>
#include <ti/driverlib/dl_timerg.h>
#include <ti/driverlib/dl_timer.h>

LOG_MODULE_REGISTER(pwm_mspm0, CONFIG_PWM_LOG_LEVEL);

/* capture and compare block count per timer */
#define MSPM0_TIMER_CC_COUNT		2
#define MSPM0_TIMER_CC_MAX		4
#define MSPM0_CC_INTR_BIT_OFFSET	4

enum mspm0_capture_mode {
	CMODE_EDGE_TIME,
	CMODE_PULSE_WIDTH
};

struct pwm_mspm0_config {
	const struct mspm0_sys_clock clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	GPTIMER_Regs *base;
	DL_Timer_ClockConfig clk_config;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif
	uint8_t	cc_idx[MSPM0_TIMER_CC_MAX];
	uint8_t cc_idx_cnt;
	bool is_capture;
};

struct pwm_mspm0_data {
	uint32_t pulse_cycle[MSPM0_TIMER_CC_MAX];
	uint32_t period;
	struct k_mutex lock;

	DL_TIMER_PWM_MODE out_mode;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t last_sample;
	enum mspm0_capture_mode cmode;
	pwm_capture_callback_handler_t callback;
	pwm_flags_t flags;
	void *user_data;
	bool is_synced;
#endif
};

static void mspm0_setup_pwm_out(const struct pwm_mspm0_config *config,
				struct pwm_mspm0_data *data)
{
	int i;
	DL_Timer_PWMConfig pwmcfg = { 0 };
	uint8_t ccdir_mask = 0;

	pwmcfg.period = data->period;
	pwmcfg.pwmMode = data->out_mode;

	for (i = 0; i < config->cc_idx_cnt; i++) {
		if (config->cc_idx[i] >= MSPM0_TIMER_CC_COUNT) {
			pwmcfg.isTimerWithFourCC = true;
			break;
		}
	}

	DL_Timer_initPWMMode(config->base, &pwmcfg);

	for (i = 0; i < config->cc_idx_cnt; i++) {
		DL_Timer_setCaptureCompareValue(config->base,
						data->pulse_cycle[i],
						config->cc_idx[i]);
		ccdir_mask |= 1 << config->cc_idx[i];
	}

	DL_Timer_enableClock(config->base);
	DL_Timer_setCCPDirection(config->base, ccdir_mask);
	DL_Timer_startCounter(config->base);
}

static int mspm0_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;

	if (channel >= MSPM0_TIMER_CC_MAX) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles > UINT16_MAX) {
		LOG_ERR("period cycles exceeds 16-bit timer limit");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->pulse_cycle[channel] = pulse_cycles;
	data->period = period_cycles;

	if (data->out_mode == DL_TIMER_PWM_MODE_CENTER_ALIGN) {
		data->period = period_cycles >> 1;
	}

	DL_Timer_setLoadValue(config->base, data->period);
	DL_Timer_setCaptureCompareValue(config->base,
					data->pulse_cycle[channel],
					config->cc_idx[channel]);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mspm0_pwm_get_cycles_per_sec(const struct device *dev,
					uint32_t channel, uint64_t *cycles)
{
	const struct pwm_mspm0_config *config = dev->config;
	DL_Timer_ClockConfig clkcfg;
	uint32_t clock_rate;
	int ret;

	if (cycles == NULL) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(config->clock_dev,
				(clock_control_subsys_t)&config->clock_subsys,
				&clock_rate);
	if (ret != 0) {
		LOG_ERR("clk get rate err %d", ret);
		return ret;
	}

	DL_Timer_getClockConfig(config->base, &clkcfg);
	*cycles = clock_rate /
			((clkcfg.divideRatio + 1) * (clkcfg.prescale + 1));

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
#define MSPM0_CTRCTL_CAC_CCCTL_ACOND(x) (x << 10)

static void mspm0_set_combined_mode(const struct pwm_mspm0_config *config,
				    struct pwm_mspm0_data *data)
{
	DL_Timer_setLoadValue(config->base, data->period);
	DL_Timer_setCaptureCompareInput(config->base, 0,
					((config->cc_idx[0] & 0x1) ?
					 DL_TIMER_CC_IN_SEL_CCPX : DL_TIMER_CC_IN_SEL_CCP0),
					config->cc_idx[0]);

	DL_Timer_setCaptureCompareInput(config->base, 0,
					GPTIMER_IFCTL_01_ISEL_CCPX_INPUT_PAIR,
					(config->cc_idx[0] ^ 1));

	DL_Timer_setCaptureCompareCtl(config->base,
				      DL_TIMER_CC_MODE_CAPTURE,
				      DL_TIMER_CC_CCOND_TRIG_FALL,
				      config->cc_idx[0]);

	DL_Timer_setCaptureCompareCtl(config->base,
				      DL_TIMER_CC_MODE_CAPTURE,
				      DL_TIMER_CC_CCOND_TRIG_RISE,
				      (config->cc_idx[0] ^ 1));

	DL_Timer_setCCPDirection(config->base, DL_TIMER_CC0_INPUT);

	DL_Timer_setCounterControl(config->base,
				   DL_TIMER_CZC_CCCTL0_ZCOND,
				   MSPM0_CTRCTL_CAC_CCCTL_ACOND(config->cc_idx[0]),
				   DL_TIMER_CLC_CCCTL0_LCOND);

	DL_Timer_setCounterRepeatMode(config->base, DL_TIMER_REPEAT_MODE_ENABLED);
	DL_Timer_setCounterMode(config->base, DL_TIMER_COUNT_MODE_DOWN);
}

static void mspm0_setup_capture(const struct device *dev,
				const struct pwm_mspm0_config *config,
				struct pwm_mspm0_data *data)
{
	if (data->cmode == CMODE_EDGE_TIME) {
		DL_Timer_CaptureConfig cc_cfg = { 0 };

		cc_cfg.inputChan = config->cc_idx[0];
		cc_cfg.period = data->period;
		cc_cfg.edgeCaptMode = DL_TIMER_CAPTURE_EDGE_DETECTION_MODE_RISING;

		DL_Timer_initCaptureMode(config->base, &cc_cfg);
	} else {
		mspm0_set_combined_mode(config, data);
	}

	DL_Timer_enableClock(config->base);
	config->irq_config_func(dev);
}

static int mspm0_capture_configure(const struct device *dev,
				   uint32_t channel,
				   pwm_flags_t flags,
				   pwm_capture_callback_handler_t cb,
				   void *user_data)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	uint32_t intr_mask;

	if (config->is_capture != true ||
	    channel != 0) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	switch (flags & PWM_CAPTURE_TYPE_MASK) {
	case PWM_CAPTURE_TYPE_PULSE:
	case PWM_CAPTURE_TYPE_BOTH:
	case PWM_CAPTURE_TYPE_PERIOD:
		/* CCD1/CCD0 event for capture index 0/1 respectively */
		intr_mask = BIT(!(config->cc_idx[0]) + MSPM0_CC_INTR_BIT_OFFSET) |
			    DL_TIMER_INTERRUPT_ZERO_EVENT;
		break;

	default:
		/* edge time event */
		intr_mask = BIT(config->cc_idx[0] + MSPM0_CC_INTR_BIT_OFFSET);
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* If interrupt is enabled --> channel is on-going */
	if (DL_Timer_getEnabledInterrupts(config->base, intr_mask)) {
		LOG_ERR("Channel %d is busy", channel);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	data->flags = flags;
	data->callback = cb;
	data->user_data = user_data;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mspm0_capture_enable(const struct device *dev, uint32_t channel)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	uint32_t intr_mask;

	if (config->is_capture != true ||
	    channel != 0) {
		LOG_ERR("Invalid capture mode or channel");
		return -EINVAL;
	}

	if (!data->callback) {
		LOG_ERR("Callback is not configured");
		return -EINVAL;
	}

	switch (data->flags & PWM_CAPTURE_TYPE_MASK) {
	case PWM_CAPTURE_TYPE_PULSE:
	case PWM_CAPTURE_TYPE_BOTH:
	case PWM_CAPTURE_TYPE_PERIOD:
		/* CCD1/CCD0 event for capture index 0/1 respectively */
		intr_mask = BIT(!(config->cc_idx[0]) + MSPM0_CC_INTR_BIT_OFFSET) |
			    DL_TIMER_INTERRUPT_ZERO_EVENT;
		break;

	default:
		/* edge time event */
		intr_mask = BIT(config->cc_idx[0] + MSPM0_CC_INTR_BIT_OFFSET);
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* If interrupt is enabled --> channel is on-going */
	if (DL_Timer_getEnabledInterrupts(config->base, intr_mask)) {
		LOG_ERR("Channel %d is busy", channel);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	DL_Timer_setTimerCount(config->base, data->period);
	DL_Timer_startCounter(config->base);
	DL_Timer_clearInterruptStatus(config->base, intr_mask);
	DL_Timer_enableInterrupt(config->base, intr_mask);

	k_mutex_unlock(&data->lock);
	return 0;
}

static int mspm0_capture_disable(const struct device *dev, uint32_t channel)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	uint32_t intr_mask;

	if (config->is_capture != true ||
	    channel != 0) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	switch (data->flags & PWM_CAPTURE_TYPE_MASK) {
	case PWM_CAPTURE_TYPE_PULSE:
	case PWM_CAPTURE_TYPE_BOTH:
	case PWM_CAPTURE_TYPE_PERIOD:
		/* CCD1/CCD0 event for capture index 0/1 respectively */
		intr_mask = BIT(!(config->cc_idx[0]) + MSPM0_CC_INTR_BIT_OFFSET) |
			    DL_TIMER_INTERRUPT_ZERO_EVENT;
		break;

	default:
		/* edge time event */
		intr_mask = BIT(config->cc_idx[0] + MSPM0_CC_INTR_BIT_OFFSET);
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	DL_Timer_disableInterrupt(config->base, intr_mask);
	DL_Timer_stopCounter(config->base);
	data->is_synced = false;
	k_mutex_unlock(&data->lock);

	return 0;
}
#endif

static int pwm_mspm0_init(const struct device *dev)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	DL_Timer_reset(config->base);
	if (!DL_Timer_isPowerEnabled(config->base)) {
		DL_Timer_enablePower(config->base);
	}

	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);
	DL_Timer_setClockConfig(config->base,
				(DL_Timer_ClockConfig *)&config->clk_config);
	if (config->is_capture) {
#ifdef CONFIG_PWM_CAPTURE
		mspm0_setup_capture(dev, config, data);
#endif
	} else {
		mspm0_setup_pwm_out(config, data);
	}

	return 0;
}

static DEVICE_API(pwm, pwm_mspm0_driver_api) = {
	.set_cycles = mspm0_pwm_set_cycles,
	.get_cycles_per_sec = mspm0_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mspm0_capture_configure,
	.enable_capture = mspm0_capture_enable,
	.disable_capture = mspm0_capture_disable,
#endif
};

#ifdef CONFIG_PWM_CAPTURE
static void mspm0_cc_isr(const struct device *dev)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	uint32_t status;
	uint32_t cc1 = 0;
	uint32_t cc0 = 0;
	uint32_t period = 0;
	uint32_t pulse = 0;

	status = DL_Timer_getPendingInterrupt(config->base);

	switch (status) {
	case DL_TIMER_IIDX_CC0_DN:
	case DL_TIMER_IIDX_CC1_DN:
		break;

	/* Timer reached zero no pwm signal is detected */
	case DL_TIMERG_IIDX_ZERO:
		if (data->callback &&
		    !(data->flags & PWM_CAPTURE_MODE_CONTINUOUS)) {
			data->callback(dev, 0, 0, 0, -ERANGE, data->user_data);
			DL_Timer_stopCounter(config->base);
		}
		__fallthrough;

	default:
		return;
	}

	if (data->flags & PWM_CAPTURE_TYPE_PERIOD) {
		cc1 =  DL_Timer_getCaptureCompareValue(config->base,
						       config->cc_idx[0] ^ 0x1);
	}

	/* ignore the unsynced counter value for pwm mode */
	if (data->is_synced == false &&
	    data->cmode != CMODE_EDGE_TIME) {
		data->last_sample = cc1;
		data->is_synced = true;
		return;
	}

	if (data->flags & PWM_CAPTURE_TYPE_PULSE ||
	    data->cmode == CMODE_EDGE_TIME) {
		cc0 = DL_Timer_getCaptureCompareValue(config->base,
						      config->cc_idx[0]);
	}

	if (!(data->flags & PWM_CAPTURE_MODE_CONTINUOUS)) {
		DL_Timer_stopCounter(config->base);
		data->is_synced = false;
	}


	period = ((data->last_sample - cc1 + UINT16_MAX) % UINT16_MAX);
	pulse = ((data->last_sample - cc0 + UINT16_MAX) % UINT16_MAX);

	/* fixme: random intermittent cc0 is greater than cc1 due to capture block error */
	if (pulse > period) {
		pulse -= period;
	}

	if (data->callback && period) {
		data->callback(dev, 0, period, pulse, 0, data->user_data);
	}

	data->last_sample = cc1;
}

#define MSP_CC_IRQ_REGISTER(n)							\
	static void mspm0_cc_## n ##_irq_register(const struct device *dev)	\
	{									\
		const struct pwm_mspm0_config *config = dev->config;		\
		if (!config->is_capture) {					\
			return;							\
		}								\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)),				\
			    DT_IRQ(DT_INST_PARENT(n), priority), mspm0_cc_isr,	\
			    DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));				\
	}
#else
#define MSP_CC_IRQ_REGISTER(n)
#endif

#define MSPM0_PWM_MODE(mode)		DT_CAT(DL_TIMER_PWM_MODE_, mode)
#define MSPM0_CAPTURE_MODE(mode)	DT_CAT(CMODE_, mode)
#define MSPM0_CLK_DIV(div)		DT_CAT(DL_TIMER_CLOCK_DIVIDE_, div)

#define MSPM0_CC_IDX_ARRAY(node_id, prop, idx)	\
				 DT_PROP_BY_IDX(node_id, prop, idx),

#define MSPM0_PWM_DATA(n)	\
	.out_mode = MSPM0_PWM_MODE(DT_STRING_TOKEN(DT_DRV_INST(n),		\
				   ti_pwm_mode)),

#define MSPM0_CAPTURE_DATA(n)		\
	IF_ENABLED(CONFIG_PWM_CAPTURE,	\
	(.cmode = MSPM0_CAPTURE_MODE(DT_STRING_TOKEN(DT_DRV_INST(n), ti_cc_mode)),))

#define PWM_DEVICE_INIT_MSPM0(n)						\
	static struct pwm_mspm0_data pwm_mspm0_data_ ## n = {			\
		.period = DT_PROP(DT_DRV_INST(n), ti_period),			\
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_pwm_mode),	\
			    (MSPM0_PWM_DATA(n)), ())				\
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),	\
			    (MSPM0_CAPTURE_DATA(n)), ())			\
	};									\
	PINCTRL_DT_INST_DEFINE(n);						\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode), (MSP_CC_IRQ_REGISTER(n)), ())	\
										\
	static const struct pwm_mspm0_config pwm_mspm0_config_ ## n = {		\
		.base = (GPTIMER_Regs *)DT_REG_ADDR(DT_INST_PARENT(n)),		\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(		\
						DT_INST_PARENT(n), 0)),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.clock_subsys = {						\
			.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),\
		},								\
		.cc_idx = {							\
			DT_INST_FOREACH_PROP_ELEM(n, ti_cc_index,		\
						  MSPM0_CC_IDX_ARRAY)		\
		},								\
		.cc_idx_cnt = DT_INST_PROP_LEN(n, ti_cc_index),			\
		.clk_config = {							\
			.clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(		\
				DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n),	\
						      0, clk)),			\
			.divideRatio = MSPM0_CLK_DIV(DT_PROP(DT_INST_PARENT(n),	\
						     ti_clk_div)),		\
			.prescale = DT_PROP(DT_INST_PARENT(n), ti_clk_prescaler),\
		},								\
		.is_capture = DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),	\
		IF_ENABLED(CONFIG_PWM_CAPTURE,					\
		(.irq_config_func = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),	\
						(mspm0_cc_## n ##_irq_register), (NULL))))	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      pwm_mspm0_init,					\
			      NULL,						\
			      &pwm_mspm0_data_ ## n,				\
			      &pwm_mspm0_config_ ## n,				\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,		\
			      &pwm_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MSPM0)
