/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include "i2c_ll_stm32.h"

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_ll_stm32);

#include "i2c-priv.h"

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
#define DT_DRV_COMPAT st_stm32_i2c_v2
#else
#define DT_DRV_COMPAT st_stm32_i2c_v1
#endif

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_I2C_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_I2C_DOMAIN_CLOCK_SUPPORT 0
#endif

int i2c_stm32_runtime_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t clock = 0U;
	int ret;

	if (IS_ENABLED(STM32_I2C_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					   (clock_control_subsys_t)&cfg->pclken[1],
					   &clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[1])");
			return -EIO;
		}
	} else {
		if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					   (clock_control_subsys_t *) &cfg->pclken[0],
					   &clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[0])");
			return -EIO;
		}
	}

	data->dev_config = config;

	k_sem_take(&data->bus_mutex, K_FOREVER);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_get(dev);
#else
	pm_device_busy_set(dev);
#endif

	LL_I2C_Disable(i2c);
	LL_I2C_SetMode(i2c, LL_I2C_MODE_I2C);
	ret = stm32_i2c_configure_timing(dev, clock);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_put(dev);
#else
	pm_device_busy_clear(dev);
#endif

	k_sem_give(&data->bus_mutex);

	return ret;
}

static inline int
i2c_stm32_transaction(const struct device *dev,
		      struct i2c_msg msg, uint8_t *next_msg_flags,
		      uint16_t periph)
{
	/*
	 * Perform a I2C transaction, while taking into account the STM32 I2C
	 * peripheral has a limited maximum chunk size. Take appropriate action
	 * if the message to send exceeds that limit.
	 *
	 * The last chunk of a transmission uses this function's next_msg_flags
	 * parameter for its backend calls (_write/_read). Any previous chunks
	 * use a copy of the current message's flags, with the STOP and RESTART
	 * bits turned off. This will cause the backend to use reload-mode,
	 * which will make the combination of all chunks to look like one big
	 * transaction on the wire.
	 */
	const uint32_t i2c_stm32_maxchunk = 255U;
	const uint8_t saved_flags = msg.flags;
	uint8_t combine_flags =
		saved_flags & ~(I2C_MSG_STOP | I2C_MSG_RESTART);
	uint8_t *flagsp = NULL;
	uint32_t rest = msg.len;
	int ret = 0;

	do { /* do ... while to allow zero-length transactions */
		if (msg.len > i2c_stm32_maxchunk) {
			msg.len = i2c_stm32_maxchunk;
			msg.flags &= ~I2C_MSG_STOP;
			flagsp = &combine_flags;
		} else {
			msg.flags = saved_flags;
			flagsp = next_msg_flags;
		}
		if ((msg.flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = stm32_i2c_msg_write(dev, &msg, flagsp, periph);
		} else {
			ret = stm32_i2c_msg_read(dev, &msg, flagsp, periph);
		}
		if (ret < 0) {
			break;
		}
		rest -= msg.len;
		msg.buf += msg.len;
		msg.len = rest;
	} while (rest > 0U);

	return ret;
}

#define OPERATION(msg) (((struct i2c_msg *) msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(const struct device *dev, struct i2c_msg *msg,
			      uint8_t num_msgs, uint16_t slave)
{
	struct i2c_stm32_data *data = dev->data;
	struct i2c_msg *current, *next;
	int ret = 0;

	/* Check for validity of all messages, to prevent having to abort
	 * in the middle of a transfer
	 */
	current = msg;

	/*
	 * Set I2C_MSG_RESTART flag on first message in order to send start
	 * condition
	 */
	current->flags |= I2C_MSG_RESTART;

	for (uint8_t i = 1; i <= num_msgs; i++) {

		if (i < num_msgs) {
			next = current + 1;

			/*
			 * Restart condition between messages
			 * of different directions is required
			 */
			if (OPERATION(current) != OPERATION(next)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					ret = -EINVAL;
					break;
				}
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				ret = -EINVAL;
				break;
			}
		} else {
			/* Stop condition is required for the last message */
			current->flags |= I2C_MSG_STOP;
		}

		current++;
	}

	if (ret) {
		return ret;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	/* Send out messages */
	k_sem_take(&data->bus_mutex, K_FOREVER);

	/* Prevent driver from being suspended by PM until I2C transaction is complete */
#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_get(dev);
#endif

	/* Prevent the clocks to be stopped during the i2c transaction */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	current = msg;

	while (num_msgs > 0) {
		uint8_t *next_msg_flags = NULL;

		if (num_msgs > 1) {
			next = current + 1;
			next_msg_flags = &(next->flags);
		}
		ret = i2c_stm32_transaction(dev, *current, next_msg_flags, slave);
		if (ret < 0) {
			break;
		}
		current++;
		num_msgs--;
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_put(dev);
#endif

	k_sem_give(&data->bus_mutex);

	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_runtime_configure,
	.transfer = i2c_stm32_transfer,
#if defined(CONFIG_I2C_TARGET)
	.target_register = i2c_stm32_target_register,
	.target_unregister = i2c_stm32_target_unregister,
#endif
};

#ifdef CONFIG_PM_DEVICE

static int i2c_stm32_suspend(const struct device *dev)
{
	int ret;
	const struct i2c_stm32_config *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Disable device clock. */
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure disabling I2C clock");
		return ret;
	}

	/* Move pins to sleep state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
	if (ret == -ENOENT) {
		/* Warn but don't block suspend */
		LOG_WRN("I2C pinctrl sleep state not available ");
	} else if (ret < 0) {
		return ret;
	}

	return 0;
}

#endif

static int i2c_stm32_activate(const struct device *dev)
{
	int ret;
	const struct i2c_stm32_config *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Move pins to active/default state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Enable device clock. */
	if (clock_control_on(clk,
			     (clock_control_subsys_t *) &cfg->pclken[0]) != 0) {
		LOG_ERR("i2c: failure enabling clock");
		return -EIO;
	}

	return 0;
}


static int i2c_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct i2c_stm32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data *data = dev->data;
#ifdef CONFIG_I2C_STM32_INTERRUPT
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	cfg->irq_config_func(dev);
#endif

	/*
	 * initialize mutex used when multiple transfers
	 * are taking place to guarantee that each one is
	 * atomic and has exclusive access to the I2C bus.
	 */
	k_sem_init(&data->bus_mutex, 1, 1);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	i2c_stm32_activate(dev);

	if (IS_ENABLED(STM32_I2C_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		/* Enable I2C clock source */
		ret = clock_control_configure(clk,
					(clock_control_subsys_t *) &cfg->pclken[1],
					NULL);
		if (ret < 0) {
			return -EIO;
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	/*
	 * Force i2c reset for STM32F1 series.
	 * So that they can enter master mode properly.
	 * Issue described in ES096 2.14.7
	 */
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableReset(i2c);
	LL_I2C_DisableReset(i2c);
#endif

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	i2c_stm32_suspend(dev);
	pm_device_init_suspended(dev);
	(void)pm_device_runtime_enable(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int i2c_stm32_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = i2c_stm32_activate(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		err = i2c_stm32_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}

#endif

/* Macros for I2C instance declaration */

#ifdef CONFIG_I2C_STM32_INTERRUPT

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
#define STM32_I2C_IRQ_CONNECT_AND_ENABLE(index)				\
	do {								\
		IRQ_CONNECT(DT_INST_IRQN(index),			\
			    DT_INST_IRQ(index, priority),		\
			    stm32_i2c_combined_isr,			\
			    DEVICE_DT_INST_GET(index), 0);		\
		irq_enable(DT_INST_IRQN(index));			\
	} while (false)
#else
#define STM32_I2C_IRQ_CONNECT_AND_ENABLE(index)				\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, event, irq),	\
			    DT_INST_IRQ_BY_NAME(index, event, priority),\
			    stm32_i2c_event_isr,			\
			    DEVICE_DT_INST_GET(index), 0);		\
		irq_enable(DT_INST_IRQ_BY_NAME(index, event, irq));	\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, error, irq),	\
			    DT_INST_IRQ_BY_NAME(index, error, priority),\
			    stm32_i2c_error_isr,			\
			    DEVICE_DT_INST_GET(index), 0);		\
		irq_enable(DT_INST_IRQ_BY_NAME(index, error, irq));	\
	} while (false)
#endif /* CONFIG_I2C_STM32_COMBINED_INTERRUPT */

#define STM32_I2C_IRQ_HANDLER_DECL(index)				\
static void i2c_stm32_irq_config_func_##index(const struct device *dev)
#define STM32_I2C_IRQ_HANDLER_FUNCTION(index)				\
	.irq_config_func = i2c_stm32_irq_config_func_##index,
#define STM32_I2C_IRQ_HANDLER(index)					\
static void i2c_stm32_irq_config_func_##index(const struct device *dev)	\
{									\
	STM32_I2C_IRQ_CONNECT_AND_ENABLE(index);			\
}
#else

#define STM32_I2C_IRQ_HANDLER_DECL(index)
#define STM32_I2C_IRQ_HANDLER_FUNCTION(index)
#define STM32_I2C_IRQ_HANDLER(index)

#endif /* CONFIG_I2C_STM32_INTERRUPT */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
#define DEFINE_TIMINGS(index)						\
	static const uint32_t i2c_timings_##index[] =			\
		DT_INST_PROP_OR(index, timings, {});
#define USE_TIMINGS(index)						\
	.timings = (const struct i2c_config_timing *) i2c_timings_##index, \
	.n_timings = ARRAY_SIZE(i2c_timings_##index),
#else /* V2 */
#define DEFINE_TIMINGS(index)
#define USE_TIMINGS(index)
#endif /* V2 */

#define STM32_I2C_INIT(index)						\
STM32_I2C_IRQ_HANDLER_DECL(index);					\
									\
DEFINE_TIMINGS(index)							\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct stm32_pclken pclken_##index[] =			\
				 STM32_DT_INST_CLOCKS(index);		\
									\
static const struct i2c_stm32_config i2c_stm32_cfg_##index = {		\
	.i2c = (I2C_TypeDef *)DT_INST_REG_ADDR(index),			\
	.pclken = pclken_##index,					\
	.pclk_len = DT_INST_NUM_CLOCKS(index),				\
	STM32_I2C_IRQ_HANDLER_FUNCTION(index)				\
	.bitrate = DT_INST_PROP(index, clock_frequency),		\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	USE_TIMINGS(index)						\
};									\
									\
static struct i2c_stm32_data i2c_stm32_dev_data_##index;		\
									\
PM_DEVICE_DT_INST_DEFINE(index, i2c_stm32_pm_action);			\
									\
I2C_DEVICE_DT_INST_DEFINE(index, i2c_stm32_init,			\
			 PM_DEVICE_DT_INST_GET(index),			\
			 &i2c_stm32_dev_data_##index,			\
			 &i2c_stm32_cfg_##index,			\
			 POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,		\
			 &api_funcs);					\
									\
STM32_I2C_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_I2C_INIT)
