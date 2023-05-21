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
#ifdef CONFIG_I2C_STM32_V1
	uint16_t slave_address;
#endif
	uint16_t addr;
	uint32_t msg;
	uint32_t msg_buf_pos;
	struct i2c_msg *msgs;
	uint32_t num_msgs;
#ifdef CONFIG_I2C_CALLBACK
	i2c_callback_t cb;
	void *userdata;
#endif
	struct {
#ifdef CONFIG_I2C_STM32_V1
		unsigned int is_restart;
#endif
		unsigned int is_write;
		unsigned int is_arlo;
		unsigned int is_nack;
		unsigned int is_err;
		struct i2c_msg msg;
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

int stm32_i2c_check_error(struct i2c_stm32_data *data, bool is_timeout);
int stm32_i2c_transfer_next(const struct device *dev);
int32_t stm32_i2c_configure_timing(const struct device *dev, uint32_t clk);
int i2c_stm32_runtime_configure(const struct device *dev, uint32_t config);
int i2c_stm32_get_config(const struct device *dev, uint32_t *config);

void stm32_i2c_event_isr(void *arg);
void stm32_i2c_error_isr(void *arg);
#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void stm32_i2c_combined_isr(void *arg);
#endif

#ifdef CONFIG_I2C_TARGET
int i2c_stm32_target_register(const struct device *dev, struct i2c_target_config *config);
int i2c_stm32_target_unregister(const struct device *dev, struct i2c_target_config *config);
#endif

#endif	/* ZEPHYR_DRIVERS_I2C_I2C_LL_STM32_H_ */
