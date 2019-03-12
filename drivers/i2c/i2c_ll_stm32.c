/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/stm32_clock_control.h>
#include <clock_control.h>
#include <misc/util.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <i2c.h>
#include "i2c_ll_stm32.h"

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32);

#include "i2c-priv.h"

int i2c_stm32_runtime_configure(struct device *dev, u32_t config)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	u32_t clock = 0U;
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
	clock_control_get_rate(device_get_binding(STM32_CLOCK_CONTROL_NAME),
			(clock_control_subsys_t *) &cfg->pclken, &clock);
#endif /* CONFIG_SOC_SERIES_STM32F3X) || CONFIG_SOC_SERIES_STM32F0X */

	data->dev_config = config;

	k_sem_take(&data->bus_mutex, K_FOREVER);
	LL_I2C_Disable(i2c);
	LL_I2C_SetMode(i2c, LL_I2C_MODE_I2C);
	ret = stm32_i2c_configure_timing(dev, clock);
	k_sem_give(&data->bus_mutex);

	return ret;
}

#define OPERATION(msg) (((struct i2c_msg *) msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(struct device *dev, struct i2c_msg *msg,
			      u8_t num_msgs, u16_t slave)
{
	struct i2c_stm32_data *data = DEV_DATA(dev);
#if defined(CONFIG_I2C_STM32_V1)
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	I2C_TypeDef *i2c = cfg->i2c;
#endif
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

	for (u8_t i = 1; i <= num_msgs; i++) {

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
#if defined(CONFIG_I2C_STM32_V1)
	LL_I2C_Enable(i2c);
#endif

	current = msg;

	while (num_msgs > 0) {
		u8_t *next_msg_flags = NULL;

		if (num_msgs > 1) {
			next = current + 1;
			next_msg_flags = &(next->flags);
		}
		while (current->len > 0) {
			u32_t temp_len = current->len;
			u8_t tmp_msg_flags = current->flags & ~I2C_MSG_RESTART;
			u8_t tmp_next_msg_flags = next_msg_flags ?
							*next_msg_flags : 0;

			if (current->len > 255) {
				current->len = 255U;
				current->flags &= ~I2C_MSG_STOP;
				if (next_msg_flags) {
					*next_msg_flags = current->flags &
							  ~I2C_MSG_RESTART;
				}
			}
			if ((current->flags & I2C_MSG_RW_MASK) ==
								I2C_MSG_WRITE) {
				ret = stm32_i2c_msg_write(dev, current,
							  next_msg_flags,
							  slave);
			} else {
				ret = stm32_i2c_msg_read(dev, current,
							 next_msg_flags, slave);
			}

			if (ret < 0) {
				goto exit;
			}
			if (next_msg_flags) {
				*next_msg_flags = tmp_next_msg_flags;
			}
			current->buf += current->len;
			current->flags = tmp_msg_flags;
			current->len = temp_len - current->len;
		}
		current++;
		num_msgs--;
	}
exit:
#if defined(CONFIG_I2C_STM32_V1)
	LL_I2C_Disable(i2c);
#endif
	k_sem_give(&data->bus_mutex);
	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_runtime_configure,
	.transfer = i2c_stm32_transfer,
#if defined(CONFIG_I2C_SLAVE) && defined(CONFIG_I2C_STM32_V2)
	.slave_register = i2c_stm32_slave_register,
	.slave_unregister = i2c_stm32_slave_unregister,
#endif
};

static int i2c_stm32_init(struct device *dev)
{
	struct device *clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	u32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data *data = DEV_DATA(dev);
#ifdef CONFIG_I2C_STM32_INTERRUPT
	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);
	cfg->irq_config_func(dev);
#endif

	/*
	 * initialize mutex used when multiple transfers
	 * are taking place to guarantee that each one is
	 * atomic and has exclusive access to the I2C bus.
	 */
	k_sem_init(&data->bus_mutex, 1, 1);

	__ASSERT_NO_MSG(clock);
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

	switch ((u32_t)cfg->i2c) {
#ifdef CONFIG_I2C_1
	case DT_I2C_1_BASE_ADDRESS:
		LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);
		break;
#endif /* CONFIG_I2C_1 */

#if defined(CONFIG_SOC_SERIES_STM32F3X) && defined(CONFIG_I2C_2)
	case DT_I2C_2_BASE_ADDRESS:
		LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_SYSCLK);
		break;
#endif /* CONFIG_SOC_SERIES_STM32F3X && CONFIG_I2C_2 */

#ifdef CONFIG_I2C_3
	case DT_I2C_3_BASE_ADDRESS:
		LL_RCC_SetI2CClockSource(LL_RCC_I2C3_CLKSOURCE_SYSCLK);
		break;
#endif /* CONFIG_I2C_3 */
	}
#endif /* CONFIG_SOC_SERIES_STM32F3X) || CONFIG_SOC_SERIES_STM32F0X */

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_I2C_1

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_1(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_1 = {
	.i2c = (I2C_TypeDef *)DT_I2C_1_BASE_ADDRESS,
	.pclken = {
		.enr = DT_I2C_1_CLOCK_BITS,
		.bus = DT_I2C_1_CLOCK_BUS,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_1,
#endif
	.bitrate = DT_I2C_1_BITRATE,
};

static struct i2c_stm32_data i2c_stm32_dev_data_1;

DEVICE_AND_API_INIT(i2c_stm32_1, CONFIG_I2C_1_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_1, &i2c_stm32_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_1(struct device *dev)
{
#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
	IRQ_CONNECT(DT_I2C_1_COMBINED_IRQ, DT_I2C_1_COMBINED_IRQ_PRI,
		   stm32_i2c_combined_isr, DEVICE_GET(i2c_stm32_1), 0);
	irq_enable(DT_I2C_1_COMBINED_IRQ);
#else
	IRQ_CONNECT(DT_I2C_1_EVENT_IRQ, DT_I2C_1_EVENT_IRQ_PRI,
		   stm32_i2c_event_isr, DEVICE_GET(i2c_stm32_1), 0);
	irq_enable(DT_I2C_1_EVENT_IRQ);

	IRQ_CONNECT(DT_I2C_1_ERROR_IRQ, DT_I2C_1_ERROR_IRQ_PRI,
		   stm32_i2c_error_isr, DEVICE_GET(i2c_stm32_1), 0);
	irq_enable(DT_I2C_1_ERROR_IRQ);
#endif
}
#endif

#endif /* CONFIG_I2C_1 */

#ifdef CONFIG_I2C_2

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_2(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_2 = {
	.i2c = (I2C_TypeDef *)DT_I2C_2_BASE_ADDRESS,
	.pclken = {
		.enr = DT_I2C_2_CLOCK_BITS,
		.bus = DT_I2C_2_CLOCK_BUS,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_2,
#endif
	.bitrate = DT_I2C_2_BITRATE,
};

static struct i2c_stm32_data i2c_stm32_dev_data_2;

DEVICE_AND_API_INIT(i2c_stm32_2, CONFIG_I2C_2_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_2, &i2c_stm32_cfg_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_2(struct device *dev)
{
#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
	IRQ_CONNECT(DT_I2C_2_COMBINED_IRQ, DT_I2C_2_COMBINED_IRQ_PRI,
		   stm32_i2c_combined_isr, DEVICE_GET(i2c_stm32_2), 0);
	irq_enable(DT_I2C_2_COMBINED_IRQ);
#else
	IRQ_CONNECT(DT_I2C_2_EVENT_IRQ, DT_I2C_2_EVENT_IRQ_PRI,
		   stm32_i2c_event_isr, DEVICE_GET(i2c_stm32_2), 0);
	irq_enable(DT_I2C_2_EVENT_IRQ);

	IRQ_CONNECT(DT_I2C_2_ERROR_IRQ, DT_I2C_2_ERROR_IRQ_PRI,
		   stm32_i2c_error_isr, DEVICE_GET(i2c_stm32_2), 0);
	irq_enable(DT_I2C_2_ERROR_IRQ);
#endif
}
#endif

#endif /* CONFIG_I2C_2 */

#ifdef CONFIG_I2C_3

#ifndef I2C3_BASE
#error "I2C_3 is not available on the platform that you selected"
#endif /* I2C3_BASE */

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_3(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_3 = {
	.i2c = (I2C_TypeDef *)DT_I2C_3_BASE_ADDRESS,
	.pclken = {
		.enr = DT_I2C_3_CLOCK_BITS,
		.bus = DT_I2C_3_CLOCK_BUS,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_3,
#endif
	.bitrate = DT_I2C_3_BITRATE,
};

static struct i2c_stm32_data i2c_stm32_dev_data_3;

DEVICE_AND_API_INIT(i2c_stm32_3, CONFIG_I2C_3_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_3, &i2c_stm32_cfg_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_3(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_3_EVENT_IRQ, DT_I2C_3_EVENT_IRQ_PRI,
		   stm32_i2c_event_isr, DEVICE_GET(i2c_stm32_3), 0);
	irq_enable(DT_I2C_3_EVENT_IRQ);

	IRQ_CONNECT(DT_I2C_3_ERROR_IRQ, DT_I2C_3_ERROR_IRQ_PRI,
		   stm32_i2c_error_isr, DEVICE_GET(i2c_stm32_3), 0);
	irq_enable(DT_I2C_3_ERROR_IRQ);
}
#endif

#endif /* CONFIG_I2C_3 */

#ifdef CONFIG_I2C_4

#ifndef I2C4_BASE
#error "I2C_4 is not available on the platform that you selected"
#endif /* I2C4_BASE */

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_4(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_4 = {
	.i2c = (I2C_TypeDef *)DT_I2C_4_BASE_ADDRESS,
	.pclken = {
		.enr = DT_I2C_4_CLOCK_BITS,
		.bus = DT_I2C_4_CLOCK_BUS,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_4,
#endif
	.bitrate = DT_I2C_4_BITRATE,
};

static struct i2c_stm32_data i2c_stm32_dev_data_4;

DEVICE_AND_API_INIT(i2c_stm32_4, CONFIG_I2C_4_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_4, &i2c_stm32_cfg_4,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_4(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_4_EVENT_IRQ, DT_I2C_4_EVENT_IRQ_PRI,
		   stm32_i2c_event_isr, DEVICE_GET(i2c_stm32_4), 0);
	irq_enable(DT_I2C_4_EVENT_IRQ);

	IRQ_CONNECT(DT_I2C_4_ERROR_IRQ, DT_I2C_4_ERROR_IRQ_PRI,
		   stm32_i2c_error_isr, DEVICE_GET(i2c_stm32_4), 0);
	irq_enable(DT_I2C_4_ERROR_IRQ);
}
#endif

#endif /* CONFIG_I2C_4 */
