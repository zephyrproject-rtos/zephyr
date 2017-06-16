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
#include <board.h>
#include <errno.h>
#include <i2c.h>
#include "i2c_ll_stm32.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

static int i2c_stm32_runtime_configure(struct device *dev, u32_t config)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	u32_t clock;

	if (data->dev_config.bits.is_slave_read)
		return -EINVAL;

	data->dev_config.raw = config;

	clock_control_get_rate(device_get_binding(STM32_CLOCK_CONTROL_NAME),
			(clock_control_subsys_t *) &cfg->pclken, &clock);

	LL_I2C_Disable(i2c);
	LL_I2C_SetMode(i2c, LL_I2C_MODE_I2C);

	return i2c_configure_timing(dev, clock);
}

#define OPERATION(msg) (((struct i2c_msg *) msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(struct device *dev, struct i2c_msg *msg,
			      u8_t num_msgs, u16_t slave)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_msg *current, *next;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

	LL_I2C_Enable(i2c);

	current = msg;
	while (num_msgs > 0) {
		unsigned int flags = 0;

		if (current->len > 255)
			return -EINVAL;

		/* do NOT issue the i2c stop condition at the end of transfer */
		if (num_msgs > 1) {
			next = current + 1;
			if (OPERATION(current) != OPERATION(next)) {
				flags = I2C_MSG_RESTART;
			}
		}

		if ((current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = msg_write(dev, current, flags, slave);
		} else {
			ret = msg_read(dev, current, flags, slave);
		}

		if (ret < 0) {
			break;
		}

		NEXT(current, num_msgs);
	};

	LL_I2C_Disable(i2c);

	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_runtime_configure,
	.transfer = i2c_stm32_transfer,
};

static int i2c_stm32_init(struct device *dev)
{
	struct device *clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	int ret;

	__ASSERT_NO_MSG(clock);
	clock_control_on(clock, (clock_control_subsys_t *) &cfg->pclken);

	ret = i2c_stm32_runtime_configure(dev, data->dev_config.raw);
	if (ret < 0) {
		SYS_LOG_ERR("i2c: failure initializing");
		return ret;
	}
#ifdef CONFIG_I2C_STM32_INTERRUPT
	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);
	cfg->irq_config_func(dev);
#endif
	return 0;
}

#ifdef CONFIG_I2C_1

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_1(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_1 = {
	.i2c = (I2C_TypeDef *) I2C1_BASE,
	.pclken = {
		.enr = LL_APB1_GRP1_PERIPH_I2C1,
		.bus = STM32_CLOCK_BUS_APB1,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_1,
#endif
};

static struct i2c_stm32_data i2c_stm32_dev_data_1 = {
	.dev_config.raw = CONFIG_I2C_1_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c_stm32_1, CONFIG_I2C_1_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_1, &i2c_stm32_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_1(struct device *dev)
{
	IRQ_CONNECT(I2C1_EV_IRQn, CONFIG_I2C_1_IRQ_PRI,
		   i2c_stm32_event_isr, DEVICE_GET(i2c_stm32_1), 0);
	irq_enable(I2C1_EV_IRQn);

	IRQ_CONNECT(I2C1_ER_IRQn, CONFIG_I2C_1_IRQ_PRI,
		   i2c_stm32_error_isr, DEVICE_GET(i2c_stm32_1), 0);
	irq_enable(I2C1_ER_IRQn);
}
#endif

#endif /* CONFIG_I2C_1 */

#ifdef CONFIG_I2C_2

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_2(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_2 = {
	.i2c = (I2C_TypeDef *) I2C2_BASE,
	.pclken = {
		.enr = LL_APB1_GRP1_PERIPH_I2C2,
		.bus = STM32_CLOCK_BUS_APB1,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_2,
#endif
};

static struct i2c_stm32_data i2c_stm32_dev_data_2 = {
	.dev_config.raw = CONFIG_I2C_2_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c_stm32_2, CONFIG_I2C_2_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_2, &i2c_stm32_cfg_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_2(struct device *dev)
{
	IRQ_CONNECT(I2C2_EV_IRQn, CONFIG_I2C_2_IRQ_PRI,
		   i2c_stm32_event_isr, DEVICE_GET(i2c_stm32_2), 0);
	irq_enable(I2C2_EV_IRQn);

	IRQ_CONNECT(I2C2_ER_IRQn, CONFIG_I2C_2_IRQ_PRI,
		   i2c_stm32_error_isr, DEVICE_GET(i2c_stm32_2), 0);
	irq_enable(I2C2_ER_IRQn);
}
#endif

#endif /* CONFIG_I2C_2 */

#ifdef I2C3_BASE

#ifdef CONFIG_I2C_3

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_3(struct device *port);
#endif

static const struct i2c_stm32_config i2c_stm32_cfg_3 = {
	.i2c = (I2C_TypeDef *) I2C3_BASE,
	.pclken = {
		.enr = LL_APB1_GRP1_PERIPH_I2C3,
		.bus = STM32_CLOCK_BUS_APB1,
	},
#ifdef CONFIG_I2C_STM32_INTERRUPT
	.irq_config_func = i2c_stm32_irq_config_func_3,
#endif
};

static struct i2c_stm32_data i2c_stm32_dev_data_3 = {
	.dev_config.raw = CONFIG_I2C_3_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c_stm32_3, CONFIG_I2C_3_NAME, &i2c_stm32_init,
		    &i2c_stm32_dev_data_3, &i2c_stm32_cfg_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32_INTERRUPT
static void i2c_stm32_irq_config_func_3(struct device *dev)
{
	IRQ_CONNECT(I2C3_EV_IRQn, CONFIG_I2C_3_IRQ_PRI,
		   i2c_stm32_event_isr, DEVICE_GET(i2c_stm32_3), 0);
	irq_enable(I2C3_EV_IRQn);

	IRQ_CONNECT(I2C3_ER_IRQn, CONFIG_I2C_3_IRQ_PRI,
		   i2c_stm32_error_isr, DEVICE_GET(i2c_stm32_3), 0);
	irq_enable(I2C3_ER_IRQn);
}
#endif

#endif /* CONFIG_I2C_3 */

#endif /* SOC_SERIES_STM32F1X */
