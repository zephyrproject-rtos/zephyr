/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_HAL_H_
#define ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_HAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

/* Matches GetVersion 'Use Case' */
enum lr11xx_variants {
	LR11XX_INVALID = 0,
	LR11XX_LR1110 = 0x1,
	LR11XX_LR1120 = 0x2,
	LR11XX_LR1121 = 0x3,
	LR11XX_VARIANT_BOOTLOADER = 0xdf,
};

/* Matches DTS enum */
enum lr11xx_tcxo_voltages {
	LR11XX_TCXO_1600 = 0,
	LR11XX_TCXO_1700 = 1,
	LR11XX_TCXO_1800 = 2,
	LR11XX_TCXO_2200 = 3,
	LR11XX_TCXO_2400 = 4,
	LR11XX_TCXO_2700 = 5,
	LR11XX_TCXO_3000 = 6,
	LR11XX_TCXO_3300 = 7,
	LR11XX_NO_TCXO = 8,
};

enum lr11xx_lf_clock {
	LR11XX_LF_CLOCK_RC = 0,
	LR11XX_LF_CLOCK_CRYSTAL = 1,
	LR11XX_LF_CLOCK_SIGNAL = 2,
	LR11XX_LF_CLOCK_MAX = 3,
};

struct lr11xx_hal_rfsw_config {
	uint8_t enable;
	uint8_t standby;
	uint8_t rx;
	uint8_t tx;
	uint8_t tx_hp;
	uint8_t tx_hf;
	uint8_t gnss;
	uint8_t wifi;
};

struct lr11xx_hal_config {
	enum lr11xx_variants variant;
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec irq;
	enum lr11xx_lf_clock lf_clk;
	enum lr11xx_tcxo_voltages tcxo_voltage;
	uint16_t tcxo_startup_delay_ms;
	struct lr11xx_hal_rfsw_config rfsw;
	bool use_dcdc;
	bool force_ldro;
	bool rx_boosted;
};

struct lr11xx_hal_data {
	struct gpio_callback irq_cb;
	void (*irq_callback)(const struct device *dev);
	const struct device *dev;
};

int lr11xx_hal_init(const struct device *dev);

int lr11xx_hal_reset(const struct device *dev);

int lr11xx_hal_wait_busy(const struct device *dev, uint32_t timeout_ms);

bool lr11xx_hal_is_busy(const struct device *dev);

int lr11xx_hal_write_cmd(const struct device *dev, uint16_t opcode,
			 const uint8_t *data, size_t len);

int lr11xx_hal_read_cmd(const struct device *dev, uint16_t opcode, uint8_t *data, size_t len);

int lr11xx_hal_read_status(const struct device *dev, uint8_t stats[2], uint32_t *irq_status);

int lr11xx_hal_write_buffer(const struct device *dev, const uint8_t *data, size_t len);

int lr11xx_hal_read_buffer(const struct device *dev, uint8_t offset, uint8_t *data, size_t len);

int lr11xx_hal_set_irq_callback(const struct device *dev,
				 void (*callback)(const struct device *dev));

void lr11xx_hal_irq_enable(const struct device *dev);

int lr11xx_hal_wakeup(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_HAL_H_ */
