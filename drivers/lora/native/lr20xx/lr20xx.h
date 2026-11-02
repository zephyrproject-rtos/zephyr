/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LR20XX_LR20XX_INTERNAL_H_
#define ZEPHYR_DRIVERS_LORA_LR20XX_LR20XX_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/atomic.h>

#include "lr20xx_hal.h"
#include "lr20xx_regs.h"

enum lr20xx_variants {
	LR20XX_INVALID = 0,
	LR20XX_LR2021 = 1,
	LR20XX_LR2022 = 2,
	LR20XX_LR2012 = 3,
};

enum lr20xx_tcxo_voltages {
	LR20XX_TCXO_1600 = 0,
	LR20XX_TCXO_1700 = 1,
	LR20XX_TCXO_1800 = 2,
	LR20XX_TCXO_2200 = 3,
	LR20XX_TCXO_2400 = 4,
	LR20XX_TCXO_2700 = 5,
	LR20XX_TCXO_3000 = 6,
	LR20XX_TCXO_3300 = 7,
	LR20XX_NO_TCXO = 8,
};

enum lr20xx_lf_clock {
	LR20XX_LF_CLOCK_RC = 0,
	LR20XX_LF_CLOCK_RESERVED = 1,
	LR20XX_LF_CLOCK_SIGNAL = 2,
};

#define LR20XX_DIO_FUNCTION_IRQ_PREP	1

enum lr20xx_dio_function {
	LR20XX_DIO_FUNCTION_NONE = 0,
	LR20XX_DIO_FUNCTION_IRQ = LR20XX_DIO_FUNCTION_IRQ_PREP,
	LR20XX_DIO_FUNCTION_RFSW = 2,
	LR20XX_DIO_FUNCTION_RESERVED_0 = 3,
	LR20XX_DIO_FUNCTION_RESERVED_1 = 4,
	LR20XX_DIO_FUNCTION_GPIO_LOW = 5,
	LR20XX_DIO_FUNCTION_GPIO_HIGH = 6,
	LR20XX_DIO_FUNCTION_HF_OUT = 7,
	LR20XX_DIO_FUNCTION_LF_OUT = 8,
	LR20XX_DIO_FUNCTION_TX_TRIGGER = 9,
	LR20XX_DIO_FUNCTION_RX_TRIGGER = 10,
};

enum lr20xx_dio_sleep_pull {
	LR20XX_DIO_SLEEP_PULL_NONE = 0,
	LR20XX_DIO_SLEEP_PULL_DOWN = 1,
	LR20XX_DIO_SLEEP_PULL_UP = 2,
	LR20XX_DIO_SLEEP_PULL_AUTO = 3,
};

struct lr20xx_rfsw_config {
	uint8_t standby;
	uint8_t rx_lf;
	uint8_t tx_lf;
	uint8_t rx_hf;
	uint8_t tx_hf;
};

struct lr20xx_config {
	enum lr20xx_variants variant;
	struct spi_dt_spec spi;
	struct spi_dt_spec spi_wake;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec irq;
	enum lr20xx_lf_clock lf_clk;
	enum lr20xx_tcxo_voltages tcxo_voltage;
	uint16_t tcxo_startup_delay_ms;
	struct lr20xx_rfsw_config rfsw;
	enum lr20xx_dio_function dio_functions[LR20XX_DIO_CNT];
	enum lr20xx_dio_sleep_pull dio_sleep_pulls[LR20XX_DIO_CNT];
	uint8_t irq_dio;
	bool use_simo;
	bool force_ldro;
	bool rx_boosted;
};

enum lr20xx_state {
	LR20XX_STATE_SLEEP = 0,
	LR20XX_STATE_IDLE,
	LR20XX_STATE_TX,
	LR20XX_STATE_RX,
	LR20XX_STATE_RX_DUTY_CYCLE,
};

struct lr20xx_tx_result {
	int status;
};

struct lr20xx_rx_result {
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	int status;
};

struct lr20xx_data {
	struct lr20xx_hal_data hal;

	/* Current state (atomic for lock-free state transitions) */
	atomic_t state;
	struct k_mutex lock;

	/* Current configuration */
	struct lora_modem_config config;
	bool config_valid;

	/* TX completion via message queue */
	struct k_msgq tx_msgq;
	struct lr20xx_tx_result tx_result;

	/* RX completion via message queue */
	struct k_msgq rx_msgq;
	struct lr20xx_rx_result rx_result;

	/* RX data buffer (shared between IRQ handler and recv) */
	uint8_t rx_buf[LR20XX_LORA_MAX_PAYLOAD_LENGTH];

	/* Async RX callback */
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;

	/* RX duty cycle parameters (in LR20xx timeout ticks)
	 * Unlike LR11xx the second argument is the total time instead of the sleep time.
	 */
	struct {
		uint32_t rx_period;
		uint32_t sleep_rx_period;
	} duty_cycle;

	/* Async TX signal */
	struct k_poll_signal *tx_async_signal;

	/* Deferred work for interrupt handling */
	struct k_work irq_work;
	const struct device *dev;
};

#endif /* ZEPHYR_DRIVERS_LORA_LR20XX_LR20XX_INTERNAL_H_ */
