/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_LL_STM32_H_
#define ZEPHYR_DRIVERS_I2C_I2C_LL_STM32_H_

#include <zephyr/drivers/i2c/stm32.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_I2C_STM32_BUS_RECOVERY
#include <zephyr/drivers/gpio.h>
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */

typedef void (*irq_config_func_t)(const struct device *port);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
/**
 * @brief structure to convey optional i2c timings settings
 */
struct i2c_config_timing {
	/* i2c peripheral clock in Hz */
	uint32_t periph_clock;
	/* i2c bus speed in Hz */
	uint32_t i2c_speed;
	/* I2C_TIMINGR register value of i2c v2 peripheral */
	uint32_t timing_setting;
};
#endif

struct i2c_stm32_config {
#ifdef CONFIG_I2C_STM32_INTERRUPT
	irq_config_func_t irq_config_func;
#endif
#ifdef CONFIG_I2C_STM32_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif /* CONFIG_I2C_STM32_BUS_RECOVERY */
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	I2C_TypeDef *i2c;
	uint32_t bitrate;
	const struct pinctrl_dev_config *pcfg;
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
	const struct i2c_config_timing *timings;
	size_t n_timings;
#endif
};

struct i2c_stm32_data {
#ifdef CONFIG_I2C_STM32_INTERRUPT
	struct k_sem device_sync_sem;
#endif
	struct k_sem bus_mutex;
	uint32_t dev_config;
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
	/* Store the current timing structure set by runtime config */
	struct i2c_config_timing current_timing;
#endif
#ifdef CONFIG_I2C_STM32_V1
	uint16_t slave_address;
#endif
	struct {
#ifdef CONFIG_I2C_STM32_V1
		unsigned int is_restart;
		unsigned int flags;
#endif
		unsigned int is_write;
		unsigned int is_arlo;
		unsigned int is_nack;
		unsigned int is_err;
		struct i2c_msg *msg;
		unsigned int len;
		uint8_t *buf;
	} current;
#ifdef CONFIG_I2C_TARGET
	bool master_active;
	struct i2c_target_config *slave_cfg;
#ifdef CONFIG_I2C_STM32_V2
	struct i2c_target_config *slave2_cfg;
#endif
	bool slave_attached;
#endif
	bool is_configured;
	bool smbalert_active;
	enum i2c_stm32_mode mode;
#ifdef CONFIG_SMBUS_STM32_SMBALERT
	i2c_stm32_smbalert_cb_func_t smbalert_cb_func;
	const struct device *smbalert_cb_dev;
#endif
};

int32_t stm32_i2c_transaction(const struct device *dev,
			    struct i2c_msg msg, uint8_t *next_msg_flags,
			    uint16_t periph);
int32_t stm32_i2c_configure_timing(const struct device *dev, uint32_t clk);
int i2c_stm32_runtime_configure(const struct device *dev, uint32_t config);
int i2c_stm32_get_config(const struct device *dev, uint32_t *config);

#ifdef CONFIG_I2C_TARGET
int i2c_stm32_target_register(const struct device *dev, struct i2c_target_config *config);
int i2c_stm32_target_unregister(const struct device *dev, struct i2c_target_config *config);
#endif

int i2c_stm32_activate(const struct device *dev);

#ifdef CONFIG_PM_DEVICE
int i2c_stm32_pm_action(const struct device *dev, enum pm_device_action action);
int i2c_stm32_suspend(const struct device *dev)
#endif


int i2c_stm32_error(const struct device *dev);
void i2c_stm32_event(const struct device *dev);

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void i2c_stm32_combined_isr(void *arg);
#else
void i2c_stm32_event_isr(void *arg);
void i2c_stm32_error_isr(void *arg);
#endif

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
#define STM32_I2C_IRQ_CONNECT_AND_ENABLE(index)				\
	do {								\
		IRQ_CONNECT(DT_INST_IRQN(index),			\
			    DT_INST_IRQ(index, priority),		\
			    i2c_stm32_combined_isr,			\
			    DEVICE_DT_INST_GET(index), 0);		\
		irq_enable(DT_INST_IRQN(index));			\
	} while (false)
#else
#define STM32_I2C_IRQ_CONNECT_AND_ENABLE(index)				\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, event, irq),	\
			    DT_INST_IRQ_BY_NAME(index, event, priority),\
			    i2c_stm32_event_isr,			\
			    DEVICE_DT_INST_GET(index), 0);		\
		irq_enable(DT_INST_IRQ_BY_NAME(index, event, irq));	\
									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, error, irq),	\
			    DT_INST_IRQ_BY_NAME(index, error, priority),\
			    i2c_stm32_error_isr,			\
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

#endif	/* ZEPHYR_DRIVERS_I2C_I2C_LL_STM32_H_ */
