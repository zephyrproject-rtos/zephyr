/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _STM32_I2C_H_
#define _STM32_I2C_H_

typedef void (*irq_config_func_t)(struct device *port);

struct i2c_stm32_config {
	struct stm32_pclken pclken;
	I2C_TypeDef *i2c;
#ifdef CONFIG_I2C_STM32_INTERRUPT
	irq_config_func_t irq_config_func;
#endif
};

struct i2c_stm32_data {
	union dev_config dev_config;
#ifdef CONFIG_I2C_STM32_INTERRUPT
	struct k_sem device_sync_sem;
#endif
	struct {
		struct i2c_msg *msg;
		unsigned int len;
		u8_t *buf;
		unsigned int is_err;
		unsigned int is_nack;
		unsigned int is_write;
#ifdef CONFIG_I2C_STM32_V1
		unsigned int is_restart;
		unsigned int flags;
#endif
	} current;
#ifdef CONFIG_I2C_STM32_V1
	u16_t slave_address;
#endif
};


#define DEV_CFG(dev)						\
((const struct i2c_stm32_config * const)(dev)->config->config_info)

#define DEV_DATA(dev)						\
((struct i2c_stm32_data * const)(dev)->driver_data)

#define NEXT(p, len)						\
do { p++; len--; } while (0)

s32_t msg_write(struct device *dev, struct i2c_msg *msg, u32_t flags,
		u16_t saddr);
s32_t msg_read(struct device *dev, struct i2c_msg *msg, u32_t flags,
	       u16_t saddr);
s32_t i2c_configure_timing(struct device *dev, u32_t clock);
void i2c_stm32_event_isr(void *args);
void i2c_stm32_error_isr(void *args);
#endif	/* _STM32_I2C_H_ */
