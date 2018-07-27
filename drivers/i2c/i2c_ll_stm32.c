/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2018 Bobby Noelte
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

#if defined(CONFIG_SOC_SERIES_STM32F3X) || defined(CONFIG_SOC_SERIES_STM32F0X)
/*
 * STM32F0/3 I2C's independent clock source supports only
 * HSI and SYSCLK, not APB1. We force I2C clock source to SYSCLK.
 * I2C2 on STM32F0 uses APB1 clock as I2C clock source
 */
#define LL_I2C_NO_CLOCKSOURCE 0x12345678 /* dummy value */
#define LL_I2C_1_CLOCKSOURCE LL_RCC_I2C1_CLKSOURCE_SYSCLK
#if defined(CONFIG_SOC_SERIES_STM32F3X)
#define LL_I2C_2_CLOCKSOURCE LL_RCC_I2C2_CLKSOURCE_SYSCLK
#else
/* I2C2 on STM32F0 uses APB1 clock - can't set clock source */
#define LL_I2C_2_CLOCKSOURCE LL_I2C_NO_CLOCKSOURCE
#endif
#define LL_I2C_3_CLOCKSOURCE LL_RCC_I2C3_CLKSOURCE_SYSCLK
#endif

int i2c_stm32_runtime_configure(struct device *dev, u32_t config)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	u32_t clock = 0;

#if defined(CONFIG_SOC_SERIES_STM32F3X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	if (cfg->ll_clock_source != LL_I2C_NO_CLOCKSOURCE) {
		LL_RCC_ClocksTypeDef rcc_clocks;

		/*
		 * STM32F0/3 I2C's independent clock source supports only
		 * HSI and SYSCLK, not APB1. We force clock variable to
		 * SYSCLK frequency.
		 */
		LL_RCC_GetSystemClocksFreq(&rcc_clocks);
		clock = rcc_clocks.SYSCLK_Frequency;
	} else
		/* I2C2 on STM32F0 uses APB1 clock as I2C clock source */
#else
	{
		clock_control_get_rate(
			device_get_binding(STM32_CLOCK_CONTROL_NAME),
			(clock_control_subsys_t *) &cfg->pclken, &clock);
	}
#endif /* CONFIG_SOC_SERIES_STM32F3X) || CONFIG_SOC_SERIES_STM32F0X */

	data->dev_config = config;

	LL_I2C_Disable(i2c);
	LL_I2C_SetMode(i2c, LL_I2C_MODE_I2C);

	return stm32_i2c_configure_timing(dev, clock);
}

#define OPERATION(msg) (((struct i2c_msg *) msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(struct device *dev, struct i2c_msg *msg,
			      u8_t num_msgs, u16_t slave)
{
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
		/* Maximum length of a single message is 255 Bytes */
		if (current->len > 255) {
			ret = -EINVAL;
			break;
		}

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

		if ((current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = stm32_i2c_msg_write(dev, current, next_msg_flags,
						  slave);
		} else {
			ret = stm32_i2c_msg_read(dev, current, next_msg_flags,
						 slave);
		}

		if (ret < 0) {
			break;
		}

		current++;
		num_msgs--;
	}
#if defined(CONFIG_I2C_STM32_V1)
	LL_I2C_Disable(i2c);
#endif
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
#ifdef CONFIG_I2C_STM32_INTERRUPT
	struct i2c_stm32_data *data = DEV_DATA(dev);

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);
	cfg->irq_config_func(dev);
#endif

	__ASSERT_NO_MSG(clock);
	clock_control_on(clock, (clock_control_subsys_t *) &cfg->pclken);

#if defined(CONFIG_SOC_SERIES_STM32F3X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	if (cfg->ll_clock_source != LL_I2C_NO_CLOCKSOURCE) {
		LL_RCC_SetI2CClockSource(cfg->ll_clock_source);
	}
#endif

	bitrate_cfg = _i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	return 0;
}

/**
 * @code{.cogeno.py}
 * cogeno.import_module('devicedeclare')
 *
 * device_configs = ['CONFIG_I2C_{}'.format(x) for x in range(0, 5)]
 * driver_names = ['I2C_{}'.format(x) for x in range(0, 5)]
 * device_inits = 'i2c_stm32_init'
 * device_levels = 'POST_KERNEL'
 * device_prios = 'CONFIG_KERNEL_INIT_PRIORITY_DEVICE'
 * device_api = 'api_funcs'
 * device_info = \
 * """
 * #if CONFIG_I2C_STM32_INTERRUPT
 * DEVICE_DECLARE(${device-name});
 * static void ${device-config-irq}(struct device *dev)
 * {
 * #ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
 *         IRQ_CONNECT(${interrupts/combined/irq}, \\
 *                     ${interrupts/combined/priority}, \\
 *                     stm32_i2c_combined_isr, \\
 *                     DEVICE_GET(${device-name}), 0);
 *         irq_enable(${interrupts/combined/irq});
 * #else
 *         IRQ_CONNECT(${interrupts/event/irq}, \\
 *                     ${interrupts/event/priority}, \\
 *                     stm32_i2c_event_isr, \\
 *                     DEVICE_GET(${device-name}), 0);
 *         irq_enable(${interrupts/event/irq});
 *         IRQ_CONNECT(${interrupts/error/irq}, \\
 *                     ${interrupts/error/priority}, \\
 *                     stm32_i2c_error_isr, \\
 *                     DEVICE_GET(${device-name}), 0);
 *         irq_enable(${interrupts/error/irq});
 * #endif
 * }
 * #endif
 * static const struct i2c_stm32_config ${device-config-info} = {
 *         .i2c = (I2C_TypeDef *)${reg/0/address/0},
 *         .pclken.bus = ${clocks/0/bus},
 *         .pclken.enr = ${clocks/0/bits},
 * #if CONFIG_I2C_STM32_INTERRUPT
 *         .irq_config_func = ${device-config-irq},
 * #endif
 *         .bitrate = ${clock-frequency},
 * #if defined(CONFIG_SOC_SERIES_STM32F3X) \\
 *             || defined(CONFIG_SOC_SERIES_STM32F0X)
 *         .ll_clock_source = LL_${driver-name}_CLOCKSOURCE,
 * #endif
 * };
 * static struct i2c_stm32_data ${device-data};
 * """
 *
 * devicedeclare.device_declare_multi( \
 *     device_configs,
 *     driver_names,
 *     device_inits,
 *     device_levels,
 *     device_prios,
 *     device_api,
 *     device_info)
 * @endcode{.cogeno.py}
 */
/** @code{.cogeno.ins}@endcode */
