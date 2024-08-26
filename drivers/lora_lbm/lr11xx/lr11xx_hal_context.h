/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LR11XX_HAL_CONTEXT_H
#define LR11XX_HAL_CONTEXT_H

#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <ral_lr11xx_bsp.h>
#include <lr11xx_system_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback upon firing event trigger
 *
 */
typedef void (*event_cb_t)(const struct device *dev);

struct lr11xx_hal_context_tcxo_cfg_t {
	ral_xosc_cfg_t xosc_cfg;
	lr11xx_system_tcxo_supply_voltage_t voltage;
	uint32_t wakeup_time_ms;
};

struct lr11xx_hal_context_lf_clck_cfg_t {
	lr11xx_system_lfclk_cfg_t lf_clk_cfg;
	bool wait_32k_ready;
};

typedef struct lr11xx_pa_pwr_cfg_s {
	int8_t power;
	uint8_t pa_duty_cycle;
	uint8_t pa_hp_sel;
} lr11xx_pa_pwr_cfg_t;

/**
 * @brief lr11xx context device config structure
 *
 */
struct lr11xx_hal_context_cfg_t {
	struct spi_dt_spec spi; /* spi peripheral */

	struct gpio_dt_spec reset; /* reset pin */
	struct gpio_dt_spec busy;  /* busy pin */
	struct gpio_dt_spec event; /* event pin */

	lr11xx_system_version_type_t chip_type; /* Which configured chip type in device tree */

	uint8_t lf_tx_path_options; /* LF tx path options */

	struct lr11xx_hal_context_tcxo_cfg_t tcxo_cfg;       /* TCXO/XTAL options*/
	struct lr11xx_hal_context_lf_clck_cfg_t lf_clck_cfg; /* LF Clock options */
	lr11xx_system_rfswitch_cfg_t rf_switch_cfg;          /* RF Switches options*/
	lr11xx_system_reg_mode_t reg_mode;                   /* Regulator mode */

	bool rx_boosted; /* RXBoosted option */

	uint8_t tx_offset; /* Board TX power offset */
	/* Calibration tables are stored as uint8_t because
	 * device tree provides them as flat arrays
	 */
	/* Power amplifier configuration for Low frequency / Low power */
	lr11xx_pa_pwr_cfg_t *pa_lf_lp_cfg_table;
	/* Power amplifier configuration for Low frequency / High power */
	lr11xx_pa_pwr_cfg_t *pa_lf_hp_cfg_table;
	/* Power amplifier configuration for High frequency */
	lr11xx_pa_pwr_cfg_t *pa_hf_cfg_table;
	lr11xx_radio_rssi_calibration_table_t rssi_calibration_table_below_600mhz;
	lr11xx_radio_rssi_calibration_table_t rssi_calibration_table_from_600mhz_to_2ghz;
	lr11xx_radio_rssi_calibration_table_t rssi_calibration_table_above_2ghz;
};

/* This type holds the current sleep status of the radio */
typedef enum {
	RADIO_SLEEP,
	RADIO_AWAKE
} radio_sleep_status_t;

struct lr11xx_hal_context_data_t {
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER
	const struct device *lr11xx_dev;
	struct gpio_callback event_cb; /* event callback structure */
	event_cb_t event_interrupt_cb; /* event interrupt user provided callback */
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD */
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD
	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD */
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER */
	radio_sleep_status_t radio_status;
	uint8_t tx_offset; /* Board TX power offset */
};

#ifdef __cplusplus
}
#endif

#endif /* LR11XX_HAL_CONTEXT_H */
