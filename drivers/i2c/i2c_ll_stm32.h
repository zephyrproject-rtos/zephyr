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
#ifdef CONFIG_I2C_STM32_INTERRUPT
	irq_config_func_t irq_config_func;
#endif
	struct stm32_pclken pclken;
	I2C_TypeDef *i2c;
	u32_t bitrate;
};

struct i2c_stm32_data {
#ifdef CONFIG_I2C_STM32_INTERRUPT
	struct k_sem device_sync_sem;
#endif
	u32_t dev_config;
#ifdef CONFIG_I2C_STM32_V1
	u16_t slave_address;
#endif
	struct {
#ifdef CONFIG_I2C_STM32_V1
		unsigned int is_restart;
		unsigned int flags;
#endif
		unsigned int is_write;
		unsigned int is_nack;
		unsigned int is_err;
		struct i2c_msg *msg;
		unsigned int len;
		u8_t *buf;
	} current;
};

s32_t stm32_i2c_msg_write(struct device *dev, struct i2c_msg *msg, u8_t *flg,
			  u16_t sadr);
s32_t stm32_i2c_msg_read(struct device *dev, struct i2c_msg *msg, u8_t *flg,
			 u16_t sadr);
s32_t stm32_i2c_configure_timing(struct device *dev, u32_t clk);

void stm32_i2c_event_isr(void *arg);
void stm32_i2c_error_isr(void *arg);
#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void stm32_i2c_combined_isr(void *arg);
#endif

#define DEV_DATA(dev) ((struct i2c_stm32_data * const)(dev)->driver_data)
#define DEV_CFG(dev)	\
((const struct i2c_stm32_config * const)(dev)->config->config_info)

#endif	/* _STM32_I2C_H_ */
