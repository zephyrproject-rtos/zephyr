/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_timer

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/smartbond_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <DA1469xAB.h>
#include <da1469x_pdc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_timer, CONFIG_COUNTER_LOG_LEVEL);

#define LP_CLK_OSC_RC32K	0
#define LP_CLK_OSC_RCX		1
#define LP_CLK_OSC_XTAL32K	2

#define TIMER_TOP_VALUE		0xFFFFFF

#define COUNTER_DT_DEVICE(_idx) DEVICE_DT_GET_OR_NULL(DT_NODELABEL(timer##_idx))

#define PDC_XTAL_EN (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(xtal32m)) ? \
					MCU_PDC_EN_XTAL : MCU_PDC_EN_NONE)

struct counter_smartbond_data {
	counter_alarm_callback_t callback;
	void *user_data;
	uint32_t guard_period;
	uint32_t freq;
#if defined(CONFIG_PM_DEVICE)
	uint8_t pdc_idx;
#endif
};

struct counter_smartbond_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_smartbond_config {
	struct counter_config_info info;
	/* Register set for timer */
	TIMER2_Type *timer;
	uint8_t prescaler;
	/* Timer driven by DIVn if 1 or lp_clk if 0 */
	uint8_t clock_src_divn;
	uint8_t irqn;
	void (*irq_config_func)(const struct device *dev);

	LOG_INSTANCE_PTR_DECLARE(log);
};

#if defined(CONFIG_PM_DEVICE)
static void counter_smartbond_pm_policy_state_lock_get(const struct device *dev)
{
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_device_runtime_get(dev);
}

static void counter_smartbond_pm_policy_state_lock_put(const struct device *dev)
{
	pm_device_runtime_put(dev);
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

/*
 * Routine to check whether the device is allowed to enter the sleep state or not.
 * Entering the standby mode should be allowed for TIMER1/2 that are clocked by LP
 * clock. Although, TIMER1/2 are powered by a distinct power domain,
 * namely PD_TMR which is always enabled (used to generate the sleep tick count),
 * the DIVN path which reflects the main crystal, that is XTAL32M, is turned off
 * during sleep by PDC. It's worth noting that during sleep the clock source of
 * a timer block will automatically be switched from DIVN to LP and vice versa.
 */
static inline bool counter_smartbond_is_sleep_allowed(const struct device *dev)
{
	const struct counter_smartbond_config *config = dev->config;

	return (((dev == COUNTER_DT_DEVICE(1)) ||
			 (dev == COUNTER_DT_DEVICE(2))) && !config->clock_src_divn);
}

/* Get the PDC trigger associated with the requested counter device */
static uint8_t counter_smartbond_pdc_trigger_get(const struct device *dev)
{
	const struct counter_smartbond_config *config = dev->config;

	switch ((uint32_t)config->timer) {
	case (uint32_t)TIMER:
		return MCU_PDC_TRIGGER_TIMER;
	case (uint32_t)TIMER2:
		return MCU_PDC_TRIGGER_TIMER2;
	case (uint32_t)TIMER3:
		return MCU_PDC_TRIGGER_TIMER3;
	case (uint32_t)TIMER4:
		return MCU_PDC_TRIGGER_TIMER4;
	default:
		return 0;
	}
}

/*
 * Add PDC entry so that the application core, which should be turned off during sleep,
 * can get notified upon counter events. This routine is called for counter instances
 * that are powered by PD_TMR and can operate during sleep.
 */
static void counter_smartbond_pdc_add(const struct device *dev)
{
	struct counter_smartbond_data *data = dev->data;
	uint8_t trigger = counter_smartbond_pdc_trigger_get(dev);

	data->pdc_idx = da1469x_pdc_add(trigger, MCU_PDC_MASTER_M33, PDC_XTAL_EN);
	__ASSERT_NO_MSG(data->pdc_idx >= 0);

	da1469x_pdc_set(data->pdc_idx);
	da1469x_pdc_ack(data->pdc_idx);
}

static void counter_smartbond_pdc_del(const struct device *dev)
{
	struct counter_smartbond_data *data = dev->data;

	da1469x_pdc_del(data->pdc_idx);
}
#endif

static int counter_smartbond_start(const struct device *dev)
{
	const struct counter_smartbond_config *config = dev->config;
	TIMER2_Type *timer = config->timer;

#if defined(CONFIG_PM_DEVICE)
	if (!counter_smartbond_is_sleep_allowed(dev)) {
		/*
		 * Power mode constraints should be applied as long as the device
		 * is up and running.
		 */
		counter_smartbond_pm_policy_state_lock_get(dev);
	} else {
		counter_smartbond_pdc_add(dev);
	}
#endif

	/* Enable counter in free running mode */
	timer->TIMER2_CTRL_REG |= (TIMER2_TIMER2_CTRL_REG_TIM_CLK_EN_Msk |
				  TIMER2_TIMER2_CTRL_REG_TIM_EN_Msk |
				  TIMER2_TIMER2_CTRL_REG_TIM_FREE_RUN_MODE_EN_Msk);

	return 0;
}

static int counter_smartbond_stop(const struct device *dev)
{
	const struct counter_smartbond_config *config = dev->config;
	struct counter_smartbond_data *data = dev->data;
	TIMER2_Type *timer = config->timer;

	/* disable counter */
	timer->TIMER2_CTRL_REG &= ~(TIMER2_TIMER2_CTRL_REG_TIM_EN_Msk |
				    TIMER2_TIMER2_CTRL_REG_TIM_IRQ_EN_Msk |
					TIMER2_TIMER2_CTRL_REG_TIM_CLK_EN_Msk);
	data->callback = NULL;

#if defined(CONFIG_PM_DEVICE)
	if (!counter_smartbond_is_sleep_allowed(dev)) {
		counter_smartbond_pm_policy_state_lock_put(dev);
	} else {
		counter_smartbond_pdc_del(dev);
	}
#endif

	return 0;
}

static uint32_t counter_smartbond_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return TIMER_TOP_VALUE;
}

static uint32_t counter_smartbond_read(const struct device *dev)
{
	const struct counter_smartbond_config *config = dev->config;
	TIMER2_Type *timer = config->timer;

	return timer->TIMER2_TIMER_VAL_REG;
}

static int counter_smartbond_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = counter_smartbond_read(dev);

	return 0;
}

static int counter_smartbond_set_alarm(const struct device *dev, uint8_t chan,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_smartbond_config *config = dev->config;
	struct counter_smartbond_data *data = dev->data;
	TIMER2_Type *timer = config->timer;
	volatile uint32_t *timer_clear_irq_reg = ((TIMER_Type *)timer) == TIMER ?
					&((TIMER_Type *)timer)->TIMER_CLEAR_IRQ_REG :
					&timer->TIMER2_CLEAR_IRQ_REG;
	bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	uint32_t flags = alarm_cfg->flags;
	uint32_t val = alarm_cfg->ticks;
	bool irq_on_late;
	int err = 0;
	uint32_t max_rel_val;
	uint32_t now;
	uint32_t diff;

	if (chan != 0 || alarm_cfg->ticks > counter_smartbond_get_top_value(dev)) {
		return -EINVAL;
	}

	if (data->callback) {
		return -EBUSY;
	}

	now = counter_smartbond_read(dev);
	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	__ASSERT_NO_MSG(data->guard_period < TIMER_TOP_VALUE);

	if (absolute) {
		max_rel_val = TIMER_TOP_VALUE - data->guard_period;
		irq_on_late = flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		/* If relative value is smaller than half of the counter range
		 * it is assumed that there is a risk of setting value too late
		 * and late detection algorithm must be applied. When late
		 * setting is detected, interrupt shall be triggered for
		 * immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CC and counter.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = val < (TIMER_TOP_VALUE / 2U);
		/* limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? TIMER_TOP_VALUE / 2U : TIMER_TOP_VALUE;
		val = (now + val) & TIMER_TOP_VALUE;
	}
	timer->TIMER2_RELOAD_REG = val;
	*timer_clear_irq_reg = 1;
	/* decrement value to detect also case when val == counter_smartbond_read(dev). Otherwise,
	 * condition would need to include comparing diff against 0.
	 */
	diff = ((val - 1U) - counter_smartbond_read(dev)) & TIMER_TOP_VALUE;
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			data->callback = NULL;
		}
	} else {
		if (diff == 0) {
			/* RELOAD value could be set just in time for interrupt
			 * trigger or too late. In any case time is interrupt
			 * should be triggered. No need to enable interrupt
			 * on TIMER just make sure interrupt is pending.
			 */
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			timer->TIMER2_CTRL_REG |= TIMER2_TIMER2_CTRL_REG_TIM_IRQ_EN_Msk;
		}
	}

	return err;
}

static int counter_smartbond_cancel_alarm(const struct device *dev, uint8_t chan)
{
	const struct counter_smartbond_config *config = dev->config;
	TIMER2_Type *timer = config->timer;
	struct counter_smartbond_data *data = dev->data;

	ARG_UNUSED(chan);

	timer->TIMER2_CTRL_REG &= ~TIMER2_TIMER2_CTRL_REG_TIM_IRQ_EN_Msk;
	data->callback = NULL;

	return 0;
}

static int counter_smartbond_set_top_value(const struct device *dev,
					   const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (cfg->ticks != 0xFFFFFF) {
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t counter_smartbond_get_pending_int(const struct device *dev)
{
	const struct counter_smartbond_config *config = dev->config;

	/* There is no register to check TIMER peripheral to check for interrupt
	 * pending, check directly in NVIC.
	 */
	return NVIC_GetPendingIRQ(config->irqn);
}

static int counter_smartbond_init_timer(const struct device *dev)
{
	const struct counter_smartbond_config *cfg = dev->config;
	struct counter_smartbond_data *data = dev->data;
	TIMER2_Type *timer = cfg->timer;
	TIMER_Type *timer0 = ((TIMER_Type *)cfg->timer) == TIMER ? TIMER : NULL;
	const struct device *osc_dev;
	uint32_t osc_freq;
	enum smartbond_clock osc;

	if (cfg->clock_src_divn) {
		/* Timer clock source is DIVn 32MHz */
		timer->TIMER2_CTRL_REG = TIMER2_TIMER2_CTRL_REG_TIM_SYS_CLK_EN_Msk;
		data->freq = DT_PROP(DT_NODELABEL(divn_clk), clock_frequency) /
			     (cfg->prescaler + 1);
	} else {
		osc_dev = DEVICE_DT_GET(DT_NODELABEL(osc));
		timer->TIMER2_CTRL_REG = 0;
		switch ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			 CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) {
		case LP_CLK_OSC_RC32K:
			osc = SMARTBOND_CLK_RC32K;
			break;
		case LP_CLK_OSC_RCX:
			osc = SMARTBOND_CLK_RCX;
			break;
		default:
		case LP_CLK_OSC_XTAL32K:
			osc = SMARTBOND_CLK_XTAL32K;
			break;
		}
		clock_control_get_rate(osc_dev, (clock_control_subsys_t)osc, &osc_freq);
		data->freq = osc_freq / (cfg->prescaler + 1);
	}
	timer->TIMER2_PRESCALER_REG = cfg->prescaler;
	timer->TIMER2_RELOAD_REG = counter_get_max_top_value(dev);
	timer->TIMER2_GPIO1_CONF_REG = 0;
	timer->TIMER2_GPIO2_CONF_REG = 0;
	timer->TIMER2_SHOTWIDTH_REG = 0;
	timer->TIMER2_CAPTURE_GPIO1_REG = 0;
	timer->TIMER2_CAPTURE_GPIO2_REG = 0;
	timer->TIMER2_PWM_FREQ_REG = 0;
	timer->TIMER2_PWM_DC_REG = 0;
	if (timer0) {
		timer0->TIMER_CAPTURE_GPIO3_REG = 0;
		timer0->TIMER_CAPTURE_GPIO4_REG = 0;
	}

	/* config/enable IRQ */
	cfg->irq_config_func(dev);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Make sure device state is marked as suspended */
	pm_device_init_suspended(dev);

	return pm_device_runtime_enable(dev);
#endif

	return 0;
}

static uint32_t counter_smartbond_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_smartbond_data *data = dev->data;

	ARG_UNUSED(flags);
	return data->guard_period;
}

static int counter_smartbond_set_guard_period(const struct device *dev, uint32_t guard,
					      uint32_t flags)
{
	struct counter_smartbond_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(guard < counter_smartbond_get_top_value(dev));

	data->guard_period = guard;

	return 0;
}

static uint32_t counter_smartbond_get_freq(const struct device *dev)
{
	struct counter_smartbond_data *data = dev->data;

	return data->freq;
}

#if defined(CONFIG_PM_DEVICE)
static void counter_smartbond_resume(const struct device *dev)
{
	const struct counter_smartbond_config *cfg = dev->config;
	TIMER2_Type *timer = cfg->timer;

	/*
	 * Resume only for block instances that are powered by PD_SYS
	 * and so their register contents should reset after sleep.
	 */
	if (!counter_smartbond_is_sleep_allowed(dev)) {
		if (cfg->clock_src_divn) {
			timer->TIMER2_CTRL_REG = TIMER2_TIMER2_CTRL_REG_TIM_SYS_CLK_EN_Msk;
		} else {
			timer->TIMER2_CTRL_REG = 0;
		}
		timer->TIMER2_PRESCALER_REG = cfg->prescaler;
		timer->TIMER2_RELOAD_REG = counter_get_max_top_value(dev);
	}
}

static int counter_smartbond_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		counter_smartbond_resume(dev);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif

static const struct counter_driver_api counter_smartbond_driver_api = {
	.start = counter_smartbond_start,
	.stop = counter_smartbond_stop,
	.get_value = counter_smartbond_get_value,
	.set_alarm = counter_smartbond_set_alarm,
	.cancel_alarm = counter_smartbond_cancel_alarm,
	.set_top_value = counter_smartbond_set_top_value,
	.get_pending_int = counter_smartbond_get_pending_int,
	.get_top_value = counter_smartbond_get_top_value,
	.get_guard_period = counter_smartbond_get_guard_period,
	.set_guard_period = counter_smartbond_set_guard_period,
	.get_freq = counter_smartbond_get_freq,
};

void counter_smartbond_irq_handler(const struct device *dev)
{
	const struct counter_smartbond_config *cfg = dev->config;
	struct counter_smartbond_data *data = dev->data;
	counter_alarm_callback_t alarm_callback = data->callback;
	TIMER2_Type *timer = cfg->timer;
	/* Timer0 has interrupt clear register in other offset */
	__IOM uint32_t *timer_clear_irq_reg = ((TIMER_Type *)timer) == TIMER ?
					      &((TIMER_Type *)timer)->TIMER_CLEAR_IRQ_REG :
					      &timer->TIMER2_CLEAR_IRQ_REG;

	timer->TIMER2_CTRL_REG &= ~TIMER2_TIMER2_CTRL_REG_TIM_IRQ_EN_Msk;
	*timer_clear_irq_reg = 1;

	if (alarm_callback != NULL) {
		data->callback = NULL;
		alarm_callback(dev, 0, timer->TIMER2_TIMER_VAL_REG,
			       data->user_data);
	}
}

#define TIMERN(idx)              DT_DRV_INST(idx)

/** TIMERn instance from DT */
#define TIM(idx) ((TIMER2_Type *)DT_REG_ADDR(TIMERN(idx)))

#define COUNTER_DEVICE_INIT(idx)						\
	BUILD_ASSERT(DT_PROP(TIMERN(idx), prescaler) <= 32 &&			\
		     DT_PROP(TIMERN(idx), prescaler) > 0,			\
		     "TIMER prescaler out of range (1..32)");			\
										\
	static struct counter_smartbond_data counter##idx##_data;		\
										\
	static void counter##idx##_smartbond_irq_config(const struct device *dev)\
	{									\
		IRQ_CONNECT(DT_IRQN(TIMERN(idx)),				\
			    DT_IRQ(TIMERN(idx), priority),			\
			    counter_smartbond_irq_handler,			\
			    DEVICE_DT_INST_GET(idx),				\
			    0);							\
		irq_enable(DT_IRQN(TIMERN(idx)));				\
	}									\
										\
	static const struct counter_smartbond_config counter##idx##_config = {	\
		.info = {							\
			.max_top_value = 0x00FFFFFF,				\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = 1,						\
		},								\
		.timer = TIM(idx),						\
		.prescaler = DT_PROP(TIMERN(idx), prescaler) - 1,		\
		.clock_src_divn = DT_SAME_NODE(DT_PROP(TIMERN(idx), clock_src), \
					       DT_NODELABEL(divn_clk)) ? 1 : 0,	\
		.irq_config_func = counter##idx##_smartbond_irq_config,		\
		.irqn = DT_IRQN(TIMERN(idx)),					\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(idx, counter_smartbond_pm_action);	\
	DEVICE_DT_INST_DEFINE(idx,						\
			      counter_smartbond_init_timer,			\
			      PM_DEVICE_DT_INST_GET(idx),	\
			      &counter##idx##_data,				\
			      &counter##idx##_config,				\
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,	\
			      &counter_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT)
