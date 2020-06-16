/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT st_stm32_stepperctl

#include "drivers/stepper.h"

#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/clock_control/stm32_clock_control.h>

#include "stepper_context.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(stepper_stm32, CONFIG_STEPPER_LOG_LEVEL);

/** GPIO information. */
struct gpio_info {
	/** Port. */
	const char *port;
	/** Pin. */
	gpio_pin_t pin;
	/** Flags. */
	gpio_flags_t flags;
};

/** Stepper motor configuration. */
struct stepper_stm32_mconfig {
	/** Microsteps. */
	uint32_t microsteps;
	/** Enable GPIO. */
	struct gpio_info enable;
	/** Direction GPIO. */
	struct gpio_info direction;
	/** Master channel. */
	uint32_t channel;
	/** Master LL channel. */
	uint32_t ll_channel;
	/** Master timer set compare routine. */
	void (*master_set_compare)(TIM_TypeDef *TIMx, uint32_t CompareValue);
};

/** Stepper controller configuration. */
struct stepper_stm32_config {
	/** Master timer instance. */
	TIM_TypeDef *master_timer;
	/** Master timer clock subsystem. */
	struct stm32_pclken master_pclken;
	/** Slave timer instance. */
	TIM_TypeDef *slave_timer;
	/** Slave timer clock subsystem. */
	struct stm32_pclken slave_pclken;
	/** Master/Slave ITR. */
	uint32_t itr;
	/** Slave IRQ configuration routine. */
	void (*slave_irq_config)(void);
	/** Stepper motors configuration. */
	const struct stepper_stm32_mconfig *motors;
	/** Number of stepper motors. */
	size_t n_motors;
};

/** Stepper motor data. */
struct stepper_stm32_motor_data {
	/** Enable GPIO controller. */
	struct device *enable;
	/** Direction GPIO controller. */
	struct device *direction;
};

/** Stepper data. */
struct stepper_stm32_data {
	/** Context. */
	struct stepper_context ctx;
	/** STM32 Clock controller. */
	struct device *clk;
	/** Enabled status flag. */
	bool enabled;
	/** Maximum number of successive steps. */
	uint32_t max_steps;
	/** Pending number of steps. */
	uint32_t pending_steps;
	/** PWM timer frequency (Hz). */
	uint32_t f_tim;
	/** Stepper motors data. */
	struct stepper_stm32_motor_data *motors;
};

#define LL_TIM_CHANNEL_CH(n) UTIL_CAT(LL_TIM_CHANNEL_CH, n)
#define LL_TIM_OC_SetCompareCH(n) UTIL_CAT(LL_TIM_OC_SetCompareCH, n)

static inline struct stepper_stm32_data *to_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct stepper_stm32_config *to_config(struct device *dev)
{
	return dev->config_info;
}

static inline int32_t get_motor_index(const struct stepper_stm32_config *config,
				      uint32_t channel)
{
	for (size_t i = 0u; i < config->n_motors; i++) {
		if (config->motors[i].channel == channel) {
			return (int32_t)i;
		}
	}

	return -ENODEV;
}

/**
 * @brief Obtain timer clock.
 *
 * @param clk	 Clock control device.
 * @param pclken Timer clock details.
 *
 * @return Timer clock value (Hz).
 */
static uint32_t get_timer_clock(struct device *clk,
				const struct stm32_pclken *pclken)
{
	int err;
	uint32_t bus_clk, apb_psc, tim_clk;

	err = clock_control_get_rate(clk, (clock_control_subsys_t *)pclken,
				     &bus_clk);
	__ASSERT_NO_MSG(err == 0);

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_D2PPRE1;
	} else {
		apb_psc = CONFIG_CLOCK_STM32_D2PPRE2;
	}

	/*
	 * Depending on pre-scaler selection (TIMPRE), timer clock frequency
	 * is defined as follows:
	 *
	 * - TIMPRE=0: If the APB prescaler (PPRE1, PPRE2) is configured to a
	 *   division factor of 1 or 2 then the timer clock equals to HCLK.
	 *   Otherwise the timer clock is set to twice the frequency of APB bus
	 *   clock.
	 * - TIMPRE=1: If the APB prescaler (PPRE1, PPRE2) is configured to a
	 *   division factor of 1, 2 or 4, then the timer clock equals to HCLK.
	 *   Otherwise, the timer clock frequencies are set to four times to
	 *   the frequency of the APB domain.
	 *
	 * Ref. RM0433 Rev. 7, Table 56.
	 */
	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
		if (apb_psc == 1U || apb_psc == 2U) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			tim_clk = clocks.HCLK_Frequency;
		} else {
			tim_clk = bus_clk * 2U;
		}
	} else {
		if (apb_psc == 1U || apb_psc == 2U || apb_psc == 4U) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			tim_clk = clocks.HCLK_Frequency;
		} else {
			tim_clk = bus_clk * 4U;
		}
	}
#else
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_APB1_PRESCALER;
	}
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
		apb_psc = CONFIG_CLOCK_STM32_APB2_PRESCALER;
	}
#endif

	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1U) {
		tim_clk = bus_clk;
	} else {
		tim_clk = bus_clk * 2U;
	}
#endif

	return tim_clk;
}

static void stepper_stm32_slave_timer_irq(void *arg)
{
	const struct stepper_stm32_config *cfg = to_config(arg);
	struct stepper_stm32_data *data = to_data(arg);

	if (LL_TIM_IsActiveFlag_UPDATE(cfg->slave_timer)) {
		LL_TIM_ClearFlag_UPDATE(cfg->slave_timer);

		if (data->pending_steps > data->max_steps) {
			data->pending_steps -= data->max_steps;
			LL_TIM_SetAutoReload(cfg->slave_timer,
					     MIN(data->pending_steps,
						 data->max_steps));
		} else {
			data->pending_steps = 0;

			LL_TIM_DisableCounter(cfg->master_timer);

			LL_TIM_DisableIT_UPDATE(cfg->slave_timer);
			LL_TIM_DisableCounter(cfg->slave_timer);

			stepper_context_complete(&data->ctx, 0);
		}
	}
}

static int start_move(struct device *dev, uint32_t channel, int32_t steps)
{
	const struct stepper_stm32_config *cfg = to_config(dev);
	struct stepper_stm32_data *data = to_data(dev);

	int32_t index;
	const struct stepper_stm32_mconfig *mcfg;
	struct stepper_stm32_motor_data *mdata;

	index = get_motor_index(cfg, channel);
	if (index < 0) {
		LOG_ERR("Invalid channel: %d", channel);
		return -EINVAL;
	}

	if (steps == 0) {
		return 0;
	}

	mcfg = &cfg->motors[index];
	mdata = &data->motors[index];

	steps *= mcfg->microsteps;
	if (steps > 0) {
		data->pending_steps = steps;
		gpio_pin_set(mdata->direction, mcfg->direction.pin, 0);
	} else {
		data->pending_steps = -steps;
		gpio_pin_set(mdata->direction, mcfg->direction.pin, 1);
	}

	/* slave timer keeps track of executed steps */
	LL_TIM_SetAutoReload(cfg->slave_timer,
			     MIN(data->pending_steps, data->max_steps));
	LL_TIM_SetCounter(cfg->slave_timer, 0);
	LL_TIM_EnableCounter(cfg->slave_timer);

	LL_TIM_ClearFlag_UPDATE(cfg->slave_timer);
	LL_TIM_EnableIT_UPDATE(cfg->slave_timer);

	LL_TIM_SetCounter(cfg->master_timer, 0);
	LL_TIM_EnableCounter(cfg->master_timer);

	return stepper_context_wait_for_completion(&data->ctx);
}

static int stepper_stm32_set_enabled(struct device *dev, uint32_t channel,
				     bool enabled)
{
	const struct stepper_stm32_config *cfg = to_config(dev);
	struct stepper_stm32_data *data = to_data(dev);

	int32_t index;
	const struct stepper_stm32_mconfig *mcfg;
	struct stepper_stm32_motor_data *mdata;

	index = get_motor_index(cfg, channel);
	if (index < 0) {
		LOG_ERR("Invalid channel: %d", channel);
		return -EINVAL;
	}

	mcfg = &cfg->motors[index];
	mdata = &data->motors[index];

	data->enabled = enabled;

	if (data->enabled) {
		if (IS_TIM_BREAK_INSTANCE(cfg->master_timer)) {
			LL_TIM_EnableAllOutputs(cfg->master_timer);
		}

		LL_TIM_CC_EnableChannel(cfg->master_timer, mcfg->ll_channel);
		gpio_pin_set(mdata->enable, mcfg->enable.pin, true);
	} else {
		gpio_pin_set(mdata->enable, mcfg->enable.pin, false);
		LL_TIM_CC_DisableChannel(cfg->master_timer, mcfg->ll_channel);
	}

	return 0;
}

static int stepper_stm32_set_speed(struct device *dev, uint32_t channel,
				   int32_t speed)
{
	const struct stepper_stm32_config *cfg = to_config(dev);
	struct stepper_stm32_data *data = to_data(dev);

	int32_t index;
	const struct stepper_stm32_mconfig *mcfg;
	uint32_t psc, arr;

	index = get_motor_index(cfg, channel);
	if (index < 0) {
		LOG_ERR("Invalid channel: %d", channel);
		return -EINVAL;
	}

	mcfg = &cfg->motors[index];

	/*
	 * We have that speed (steps/s) corresponds to the actual PWM frequency,
	 * f_pwm, given by:
	 *
	 * speed = f_pwm = f_tim / ((arr + 1) * (psc + 1)).                  (1)
	 *
	 * Duty cycle, d, is given by the ratio between the compare register
	 * (ccr) and the auto-reload value (arr), that is:
	 *
	 * d = ccr / (arr + 1).
	 *
	 * As we always want a duty of 50%, we need to set ccr to:
	 *
	 * ccr = (arr + 1) / 2.
	 *
	 * In case we have a 32-bit timer, we can set psc=0 and set arr as:
	 *
	 * arr = (f_tim / speed) - 1
	 *
	 * For 16-bit timers the above calculation could overflow the arr
	 * registers, so prescaler is needed. Maximum resolution is achieved
	 * when using the full scale of arr (up to 65535). In such case,
	 * the value of psc that will maximize the resolution is then:
	 *
	 * psc = ((f_tim / speed) / (arr + 1)) - 1
	 *     = ((f_tim / speed) / 65536) - 1
	 *     = ((f_tim / speed) >> 16) - 1.
	 *
	 * Note that the waveform can be generated for arr >= 1, so the maximum
	 * speed we can generate is given by:
	 *
	 * speed_max = f_tim / 2.
	 *
	 * NOTE: Thanks to @ABOST for the psc calculation suggestion.
	 */

	speed *= mcfg->microsteps;
	if (speed > (data->f_tim / 2U)) {
		LOG_ERR("Speed out of range: %d", speed);
		return -EINVAL;
	}

	psc = 0U;
	arr = (data->f_tim / speed) - 1U;

	if (!IS_TIM_32B_COUNTER_INSTANCE(cfg->master_timer) && arr > UINT16_MAX) {
		psc = ((data->f_tim / speed) >> 16U) - 1U;
		arr = UINT16_MAX;
	}

	LL_TIM_SetPrescaler(cfg->master_timer, psc);
	LL_TIM_SetAutoReload(cfg->master_timer, arr);

	mcfg->master_set_compare(cfg->master_timer, (arr + 1U) / 2U);

	return 0;
}

static int stepper_stm32_move(struct device *dev, uint32_t channel,
			      int32_t steps)
{
	struct stepper_stm32_data *data = to_data(dev);

	int r;

	stepper_context_lock(&data->ctx, false, NULL);
	r = start_move(dev, channel, steps);
	stepper_context_release(&data->ctx, r);

	return r;
}

#ifdef CONFIG_STEPPER_ASYNC
static int stepper_stm32_move_async(struct device *dev, uint32_t channel,
				    int32_t steps, struct k_poll_signal *async)
{
	struct stepper_stm32_data *data = to_data(dev);

	int r;

	stepper_context_lock(&data->ctx, true, async);
	r = start_move(dev, channel, steps);
	stepper_context_release(&data->ctx, r);

	return r;
}
#endif

static int stepper_stm32_stop(struct device *dev, uint32_t channel)
{
	const struct stepper_stm32_config *cfg = to_config(dev);
	struct stepper_stm32_data *data = to_data(dev);

	LL_TIM_DisableIT_UPDATE(cfg->slave_timer);

	if (data->pending_steps > 0u) {
		data->pending_steps = 0u;

		LL_TIM_DisableCounter(cfg->master_timer);
		LL_TIM_DisableCounter(cfg->slave_timer);

		stepper_context_complete(&data->ctx, 0);
	}

	return 0;
}

static const struct stepper_driver_api stepper_stm32_driver_api = {
	.set_enabled = stepper_stm32_set_enabled,
	.set_speed = stepper_stm32_set_speed,
	.move = stepper_stm32_move,
#ifdef CONFIG_STEPPER_ASYNC
	.move_async = stepper_stm32_move_async,
#endif
	.stop = stepper_stm32_stop,
};

static int stepper_stm32_init(struct device *dev)
{
	const struct stepper_stm32_config *cfg = to_config(dev);
	struct stepper_stm32_data *data = to_data(dev);

	int err;
	LL_TIM_InitTypeDef init;

	data->clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(clk);

	data->f_tim = get_timer_clock(data->clk, &cfg->master_pclken);

	/* master timer */
	if (!IS_TIM_MASTER_INSTANCE(cfg->master_timer)) {
		LOG_ERR("Selected master timer is not master capable");
		return -EINVAL;
	}

	err = clock_control_on(data->clk,
			       (clock_control_subsys_t *)&cfg->master_pclken);
	if (err < 0) {
		return err;
	}

	LL_TIM_StructInit(&init);
	init.Autoreload = 0;
	if (LL_TIM_Init(cfg->master_timer, &init) != SUCCESS) {
		LOG_ERR("Failed to initialize master timer");
		return -EIO;
	}

	LL_TIM_SetTriggerOutput(cfg->master_timer, LL_TIM_TRGO_UPDATE);
	LL_TIM_EnableMasterSlaveMode(cfg->master_timer);

	/* slave timer */
	if (!IS_TIM_SLAVE_INSTANCE(cfg->slave_timer)) {
		LOG_ERR("Selected slave timer is not slave capable");
		return -EINVAL;
	}

	if (IS_TIM_32B_COUNTER_INSTANCE(cfg->slave_timer)) {
		data->max_steps = UINT32_MAX;
	} else {
		data->max_steps = UINT16_MAX;
	}

	err = clock_control_on(data->clk,
			       (clock_control_subsys_t *)&cfg->slave_pclken);
	if (err < 0) {
		return err;
	}

	LL_TIM_StructInit(&init);
	init.Autoreload = 0U;
	if (LL_TIM_Init(cfg->slave_timer, &init) != SUCCESS) {
		LOG_ERR("Failed to initialize slave timer");
		return -EIO;
	}

	LL_TIM_SetClockSource(cfg->slave_timer, LL_TIM_CLOCKSOURCE_EXT_MODE1);
	LL_TIM_SetTriggerInput(cfg->slave_timer, cfg->itr);

	cfg->slave_irq_config();

	/* initialize motors */
	for (size_t i = 0u; i < cfg->n_motors; i++) {
		const struct stepper_stm32_mconfig *mcfg = &cfg->motors[i];
		struct stepper_stm32_motor_data *mdata = &data->motors[i];
		LL_TIM_OC_InitTypeDef oc_init;

		/* enable GPIO */
		mdata->enable = device_get_binding(mcfg->enable.port);
		if (!mdata->enable) {
			LOG_ERR("Could not obtain enable GPIO device");
			return -ENODEV;
		}

		err = gpio_pin_configure(mdata->enable, mcfg->enable.pin,
					 GPIO_OUTPUT_INACTIVE |
						 mcfg->enable.flags);
		if (err < 0) {
			LOG_ERR("Failed to configure enable GPIO pin (%d)",
				err);
			return err;
		}

		/* direction GPIO */
		mdata->direction = device_get_binding(mcfg->direction.port);
		if (!mdata->direction) {
			LOG_ERR("Could not obtain direction GPIO device");
			return -ENODEV;
		}

		err = gpio_pin_configure(mdata->direction, mcfg->direction.pin,
					 GPIO_OUTPUT_INACTIVE |
						 mcfg->direction.flags);
		if (err < 0) {
			LOG_ERR("Failed to configure direction GPIO pin (%d)",
				err);
			return err;
		}

		/* initialize timer output channel */
		LL_TIM_OC_StructInit(&oc_init);
		oc_init.OCMode = LL_TIM_OCMODE_PWM1;
		oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
		oc_init.CompareValue = 0U;
		oc_init.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
		oc_init.OCIdleState = LL_TIM_OCIDLESTATE_LOW;

		if (LL_TIM_OC_Init(cfg->master_timer, mcfg->ll_channel,
				   &oc_init) != SUCCESS) {
			LOG_ERR("Failed to configure master output-compare unit");
			return -EIO;
		}

		LL_TIM_OC_EnablePreload(cfg->master_timer, mcfg->ll_channel);
	}

	stepper_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define DT_GPIO(node_id, gpio_pha)                                             \
	{                                                                      \
		DT_GPIO_LABEL(node_id, gpio_pha),                              \
		DT_GPIO_PIN(node_id, gpio_pha),                                \
		DT_GPIO_FLAGS(node_id, gpio_pha),                              \
	}

#define DT_INST_CLK(index, inst)                                               \
	{                                                                      \
		.bus = DT_CLOCKS_CELL(DT_INST_PHANDLE(index, inst), bus),      \
		.enr = DT_CLOCKS_CELL(DT_INST_PHANDLE(index, inst), bits)      \
	}

#define CONFIGURE_SLAVE_IRQ(index, irqn)                                       \
	(IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PHANDLE(index, slave_timer), irqn, \
				    irq),                                      \
		     DT_IRQ_BY_NAME(DT_INST_PHANDLE(index, slave_timer), irqn, \
				    priority),                                 \
		     stepper_stm32_slave_timer_irq,                            \
		     DEVICE_GET(stepper_stm32_##index), 0);                    \
									       \
	 irq_enable(DT_IRQ_BY_NAME(DT_INST_PHANDLE(index, slave_timer), irqn,  \
				   irq));)

#define STEPPER_MOTOR_INIT(child)                                              \
	{                                                                      \
		.microsteps = DT_PROP(child, microsteps),                      \
		.enable = DT_GPIO(child, enable_gpios),                        \
		.direction = DT_GPIO(child, direction_gpios),                  \
		.master_set_compare =                                          \
			LL_TIM_OC_SetCompareCH(DT_REG_ADDR(child)),            \
		.ll_channel = LL_TIM_CHANNEL_CH(DT_REG_ADDR(child)),           \
		.channel = DT_REG_ADDR(child),                                 \
	},

#define STEPPER_STM32_INIT(index)                                              \
	static void stepper_stm32_irq_config_##index(void);                    \
									       \
	static const struct stepper_stm32_mconfig                              \
		stepper_stm32_mconfig_##index[] = { DT_INST_FOREACH_CHILD(     \
			index, STEPPER_MOTOR_INIT) };                          \
									       \
	static const struct stepper_stm32_config stepper_stm32_cfg_##index = { \
		.master_timer = (TIM_TypeDef *)DT_REG_ADDR(                    \
			DT_INST_PHANDLE(index, master_timer)),                 \
		.master_pclken = DT_INST_CLK(index, master_timer),             \
		.slave_timer = (TIM_TypeDef *)DT_REG_ADDR(                     \
			DT_INST_PHANDLE(index, slave_timer)),                  \
		.slave_pclken = DT_INST_CLK(index, slave_timer),               \
		.itr = DT_INST_PROP(index, itr),                               \
		.slave_irq_config = stepper_stm32_irq_config_##index,          \
		.motors = stepper_stm32_mconfig_##index,                       \
		.n_motors = ARRAY_SIZE(stepper_stm32_mconfig_##index)          \
	};                                                                     \
									       \
	static struct stepper_stm32_motor_data                                 \
		stepper_stm32_motor_data_##index[ARRAY_SIZE(                   \
			stepper_stm32_mconfig_##index)];                       \
									       \
	static struct stepper_stm32_data stepper_stm32_data_##index = {        \
		.motors = stepper_stm32_motor_data_##index,                    \
		STEPPER_CONTEXT_INIT_LOCK(stepper_stm32_data_##index, ctx),    \
		STEPPER_CONTEXT_INIT_SYNC(stepper_stm32_data_##index, ctx),    \
	};                                                                     \
									       \
	DEVICE_AND_API_INIT(stepper_stm32_##index, DT_INST_LABEL(index),       \
			    &stepper_stm32_init, &stepper_stm32_data_##index,  \
			    &stepper_stm32_cfg_##index, POST_KERNEL,           \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                \
			    &stepper_stm32_driver_api);                        \
									       \
	static void stepper_stm32_irq_config_##index(void)                     \
	{                                                                      \
		COND_CODE_1(DT_IRQ_HAS_NAME(                                   \
				    DT_INST_PHANDLE(index, slave_timer), up),  \
			    CONFIGURE_SLAVE_IRQ(index, up),                    \
			    CONFIGURE_SLAVE_IRQ(index, global));               \
	}

DT_INST_FOREACH_STATUS_OKAY(STEPPER_STM32_INIT)
