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

#include <zephyr/drivers/dma.h>

typedef void (*irq_config_func_t)(const struct device *port);

#ifdef CONFIG_I2C_STM32_V2
/*  Private I2C_MSG_* flags for STM32 I2C */
#define I2C_MSG_STM32_USE_RELOAD_MODE	BIT(7)
#endif

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

#ifdef CONFIG_I2C_STM32_V2_DMA
struct stream {
	const struct device *dev_dma;
	int32_t dma_channel;
};
#endif /* CONFIG_I2C_STM32_V2_DMA */

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
#ifdef CONFIG_I2C_STM32_V2_DMA
	struct stream tx_dma;
	struct stream rx_dma;
#endif /* CONFIG_I2C_STM32_V2_DMA */
};

struct i2c_stm32_data {
#ifdef CONFIG_I2C_RTIO
	struct i2c_rtio *ctx;
	uint32_t dev_config;
	uint8_t *xfer_buf;
	size_t xfer_len;
	uint8_t xfer_flags;
#ifdef CONFIG_I2C_STM32_V1
	size_t msg_len;
	uint8_t is_restart;
	uint16_t slave_address;
#else
	uint8_t burst_flags;
	uint8_t burst_len;
#endif /* CONFIG_I2C_STM32_V1 */
#else /* CONFIG_I2C_RTIO */
#ifdef CONFIG_I2C_STM32_INTERRUPT
	struct k_sem device_sync_sem;
#endif /* CONFIG_I2C_STM32_INTERRUPT */
	struct k_sem bus_mutex;
	uint32_t dev_config;
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
	/* Store the current timing structure set by runtime config */
	struct i2c_config_timing current_timing;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2) */
#ifdef CONFIG_I2C_STM32_V1
	uint16_t slave_address;
#endif /* CONFIG_I2C_STM32_V1 */
	struct {
#ifdef CONFIG_I2C_STM32_V1
		unsigned int is_restart;
		unsigned int flags;
#endif /* CONFIG_I2C_STM32_V1 */
		unsigned int is_write;
		unsigned int is_arlo;
		unsigned int is_nack;
		unsigned int is_err;
		bool continue_in_next;
		struct i2c_msg *msg;
		unsigned int len;
		uint8_t *buf;
	} current;
	bool is_configured;
	bool smbalert_active;
	enum i2c_stm32_mode mode;
#ifdef CONFIG_SMBUS_STM32_SMBALERT
	i2c_stm32_smbalert_cb_func_t smbalert_cb_func;
	const struct device *smbalert_cb_dev;
#endif /* CONFIG_SMBUS_STM32_SMBALERT */
#ifdef CONFIG_I2C_STM32_V2_DMA
	struct dma_config dma_tx_cfg;
	struct dma_config dma_rx_cfg;
	struct dma_block_config dma_blk_cfg;
#endif /* CONFIG_I2C_STM32_V2_DMA */
#endif /* CONFIG_I2C_RTIO */

#ifdef CONFIG_I2C_TARGET
	bool master_active;
	bool slave_attached;
	struct i2c_target_config *slave_cfg;
#ifdef CONFIG_I2C_STM32_V2
	struct i2c_target_config *slave2_cfg;
#endif /* CONFIG_I2C_STM32_V2 */
#endif /* CONFIG_I2C_TARGET */
};

#ifdef CONFIG_I2C_RTIO
bool i2c_stm32_start(const struct device *dev);
int i2c_stm32_msg_start(const struct device *dev, uint8_t flags,
			uint8_t *buf, size_t buf_len, uint16_t i2c_addr);
#else /* CONFIG_I2C_RTIO */
int i2c_stm32_transaction(const struct device *dev,
			  struct i2c_msg msg, uint8_t *next_msg_flags,
			  uint16_t periph);
#endif /* CONFIG_I2C_RTIO */

int i2c_stm32_runtime_configure(const struct device *dev, uint32_t config);

#ifdef CONFIG_I2C_TARGET
int i2c_stm32_target_register(const struct device *dev, struct i2c_target_config *config);
int i2c_stm32_target_unregister(const struct device *dev, struct i2c_target_config *config);
#endif /* CONFIG_I2C_TARGET */

int i2c_stm32_activate(const struct device *dev);
int i2c_stm32_configure_timing(const struct device *dev, uint32_t clk);

#ifdef CONFIG_PM_DEVICE
int i2c_stm32_pm_action(const struct device *dev, enum pm_device_action action);
int i2c_stm32_suspend(const struct device *dev);
#endif /* CONFIG_PM_DEVICE */

int i2c_stm32_error(const struct device *dev);
void i2c_stm32_event(const struct device *dev);

#ifdef CONFIG_I2C_STM32_INTERRUPT
#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void i2c_stm32_combined_isr(void *arg);
#define I2C_STM32_IRQ_CONNECT_AND_ENABLE(index)							\
	do {											\
		IRQ_CONNECT(DT_INST_IRQN(index),						\
			    DT_INST_IRQ(index, priority),					\
			    i2c_stm32_combined_isr,						\
			    DEVICE_DT_INST_GET(index), 0);					\
		irq_enable(DT_INST_IRQN(index));						\
	} while (false)
#else /* CONFIG_I2C_STM32_COMBINED_INTERRUPT */
void i2c_stm32_event_isr(void *arg);
void i2c_stm32_error_isr(void *arg);
#define I2C_STM32_IRQ_CONNECT_AND_ENABLE(index)							\
	do {											\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, event, irq),				\
			    DT_INST_IRQ_BY_NAME(index, event, priority),			\
			    i2c_stm32_event_isr,						\
			    DEVICE_DT_INST_GET(index), 0);					\
		irq_enable(DT_INST_IRQ_BY_NAME(index, event, irq));				\
												\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, error, irq),				\
			    DT_INST_IRQ_BY_NAME(index, error, priority),			\
			    i2c_stm32_error_isr,						\
			    DEVICE_DT_INST_GET(index), 0);					\
		irq_enable(DT_INST_IRQ_BY_NAME(index, error, irq));				\
	} while (false)
#endif /* CONFIG_I2C_STM32_COMBINED_INTERRUPT */

#define I2C_STM32_IRQ_HANDLER_DECL(index)							\
static void i2c_stm32_irq_config_func_##index(const struct device *dev)
#define I2C_STM32_IRQ_HANDLER_FUNCTION(index)							\
	.irq_config_func = i2c_stm32_irq_config_func_##index,
#define I2C_STM32_IRQ_HANDLER(index)								\
static void i2c_stm32_irq_config_func_##index(const struct device *dev)				\
{												\
	I2C_STM32_IRQ_CONNECT_AND_ENABLE(index);						\
}

#else /* CONFIG_I2C_STM32_INTERRUPT */
#define I2C_STM32_IRQ_HANDLER_DECL(index)
#define I2C_STM32_IRQ_HANDLER_FUNCTION(index)
#define I2C_STM32_IRQ_HANDLER(index)
#endif /* CONFIG_I2C_STM32_INTERRUPT */

#endif	/* ZEPHYR_DRIVERS_I2C_I2C_LL_STM32_H_ */
