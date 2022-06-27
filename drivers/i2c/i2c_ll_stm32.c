/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
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
LOG_MODULE_REGISTER(i2c_ll_stm32);

#include "i2c-priv.h"

int i2c_stm32_runtime_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t clock = 0U;
	int ret;

#if defined(CONFIG_SOC_SERIES_STM32F3X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	LL_RCC_ClocksTypeDef rcc_clocks;

	/*
	 * STM32F0/3 I2C's independent clock source supports only
	 * HSI and SYSCLK, not APB1. We force clock variable to
	 * SYSCLK frequency.
	 */
	LL_RCC_GetSystemClocksFreq(&rcc_clocks);
	clock = rcc_clocks.SYSCLK_Frequency;
#else
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			(clock_control_subsys_t *) &cfg->pclken, &clock) < 0) {
		LOG_ERR("Failed call clock_control_get_rate");
		return -EIO;
	}

#endif /* CONFIG_SOC_SERIES_STM32F3X) || CONFIG_SOC_SERIES_STM32F0X */

	data->dev_config = config;

	k_sem_take(&data->bus_mutex, K_FOREVER);
	LL_I2C_Disable(i2c);
	LL_I2C_SetMode(i2c, LL_I2C_MODE_I2C);
	ret = stm32_i2c_configure_timing(dev, clock);
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

	/* Send out messages */
	k_sem_take(&data->bus_mutex, K_FOREVER);

	current = msg;

	while (num_msgs > 0) {
		uint8_t *next_msg_flags = NULL;

		if (num_msgs > 1) {
			next = current + 1;
			next_msg_flags = &(next->flags);
		}
		ret = i2c_stm32_transaction(dev, *current, next_msg_flags, slave);
		if (ret < 0)
			break;
		current++;
		num_msgs--;
	}

	k_sem_give(&data->bus_mutex);
	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_runtime_configure,
	.transfer = i2c_stm32_transfer,
#if defined(CONFIG_I2C_SLAVE)
	.slave_register = i2c_stm32_slave_register,
	.slave_unregister = i2c_stm32_slave_unregister,
#endif
};

static int i2c_stm32_init(const struct device *dev)
{
	const struct device *clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct i2c_stm32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data *data = dev->data;
#ifdef CONFIG_I2C_STM32_INTERRUPT
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	cfg->irq_config_func(dev);
#endif

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	/*
	 * initialize mutex used when multiple transfers
	 * are taking place to guarantee that each one is
	 * atomic and has exclusive access to the I2C bus.
	 */
	k_sem_init(&data->bus_mutex, 1, 1);

	if (clock_control_on(clock,
		(clock_control_subsys_t *) &cfg->pclken) != 0) {
		LOG_ERR("i2c: failure enabling clock");
		return -EIO;
	}

#if defined(CONFIG_SOC_SERIES_STM32F3X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	/*
	 * STM32F0/3 I2C's independent clock source supports only
	 * HSI and SYSCLK, not APB1. We force I2C clock source to SYSCLK.
	 * I2C2 on STM32F0 uses APB1 clock as I2C clock source
	 */

	switch ((uint32_t)cfg->i2c) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	case DT_REG_ADDR(DT_NODELABEL(i2c1)):
		LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);
		break;
#endif

#if defined(CONFIG_SOC_SERIES_STM32F3X) &&	\
	DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
	case DT_REG_ADDR(DT_NODELABEL(i2c2)):
		LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_SYSCLK);
		break;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
	case DT_REG_ADDR(DT_NODELABEL(i2c3)):
		LL_RCC_SetI2CClockSource(LL_RCC_I2C3_CLKSOURCE_SYSCLK);
		break;
#endif
	}
#endif /* CONFIG_SOC_SERIES_STM32F3X) || CONFIG_SOC_SERIES_STM32F0X */

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	/*
	 * Force i2c reset for STM32F1 series.
	 * So that they can enter master mode properly.
	 * Issue described in ES096 2.14.7
	 */
	I2C_TypeDef * i2c = cfg->i2c;

	LL_I2C_EnableReset(i2c);
	LL_I2C_DisableReset(i2c);
#endif

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	return 0;
}

/* Macros for I2C instance declaration */

#ifdef CONFIG_I2C_STM32_INTERRUPT

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
#define STM32_I2C_IRQ_CONNECT_AND_ENABLE(name)				\
	do {								\
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(name)),		\
			    DT_IRQ(DT_NODELABEL(name), priority),	\
			    stm32_i2c_combined_isr,			\
			    DEVICE_DT_GET(DT_NODELABEL(name)), 0);	\
		irq_enable(DT_IRQN(DT_NODELABEL(name)));		\
	} while (0)
#else
#define STM32_I2C_IRQ_CONNECT_AND_ENABLE(name)				\
	do {								\
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(name), event, irq),\
			    DT_IRQ_BY_NAME(DT_NODELABEL(name), event,	\
								priority),\
			    stm32_i2c_event_isr,			\
			    DEVICE_DT_GET(DT_NODELABEL(name)), 0);	\
		irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(name), event, irq));\
									\
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(name), error, irq),\
			    DT_IRQ_BY_NAME(DT_NODELABEL(name), error,	\
								priority),\
			    stm32_i2c_error_isr,			\
			    DEVICE_DT_GET(DT_NODELABEL(name)), 0);	\
		irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(name), error, irq));\
	} while (0)
#endif /* CONFIG_I2C_STM32_COMBINED_INTERRUPT */

#define STM32_I2C_IRQ_HANDLER_DECL(name)				\
static void i2c_stm32_irq_config_func_##name(const struct device *dev)
#define STM32_I2C_IRQ_HANDLER_FUNCTION(name)				\
	.irq_config_func = i2c_stm32_irq_config_func_##name,
#define STM32_I2C_IRQ_HANDLER(name)					\
static void i2c_stm32_irq_config_func_##name(const struct device *dev)	\
{									\
	STM32_I2C_IRQ_CONNECT_AND_ENABLE(name);				\
}
#else

#define STM32_I2C_IRQ_HANDLER_DECL(name)
#define STM32_I2C_IRQ_HANDLER_FUNCTION(name)
#define STM32_I2C_IRQ_HANDLER(name)

#endif /* CONFIG_I2C_STM32_INTERRUPT */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
#define DEFINE_TIMINGS(name)						\
	static const uint32_t i2c_timings_##name[] =			\
		DT_PROP_OR(DT_NODELABEL(name), timings, {});
#define USE_TIMINGS(name)						\
	.timings = (const struct i2c_config_timing *) i2c_timings_##name, \
	.n_timings = ARRAY_SIZE(i2c_timings_##name),
#else /* V2 */
#define DEFINE_TIMINGS(name)
#define USE_TIMINGS(name)
#endif /* V2 */

#define STM32_I2C_INIT(name)						\
STM32_I2C_IRQ_HANDLER_DECL(name);					\
									\
DEFINE_TIMINGS(name)							\
									\
PINCTRL_DT_DEFINE(DT_NODELABEL(name));					\
									\
static const struct i2c_stm32_config i2c_stm32_cfg_##name = {		\
	.i2c = (I2C_TypeDef *)DT_REG_ADDR(DT_NODELABEL(name)),		\
	.pclken = {							\
		.enr = DT_CLOCKS_CELL(DT_NODELABEL(name), bits),	\
		.bus = DT_CLOCKS_CELL(DT_NODELABEL(name), bus),		\
	},								\
	STM32_I2C_IRQ_HANDLER_FUNCTION(name)				\
	.bitrate = DT_PROP(DT_NODELABEL(name), clock_frequency),	\
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(name)),		\
	USE_TIMINGS(name)						\
};									\
									\
static struct i2c_stm32_data i2c_stm32_dev_data_##name;			\
									\
I2C_DEVICE_DT_DEFINE(DT_NODELABEL(name), i2c_stm32_init,		\
		    NULL, &i2c_stm32_dev_data_##name,			\
		    &i2c_stm32_cfg_##name,				\
		    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,		\
		    &api_funcs);					\
									\
STM32_I2C_IRQ_HANDLER(name)

/* I2C instances declaration */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
STM32_I2C_INIT(i2c1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
STM32_I2C_INIT(i2c2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
STM32_I2C_INIT(i2c3);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c4), okay)
STM32_I2C_INIT(i2c4);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c5), okay)
STM32_I2C_INIT(i2c5);
#endif
