/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>

#include "ralf.h"

#define STATE_FREE    0
#define STATE_BUSY    1
#define STATE_CLEANUP 2

/* LoRa sync words, taken from the legacy loramac-node sx1272.h/sx1276.h */
enum lbm_modem_lora_sync_word {
	LBM_LORA_SYNC_WORD_PRIVATE = 0x12,
	LBM_LORA_SYNC_WORD_PUBLIC = 0x34,
};

/* Current operation mode of the LBM modem */
enum lbm_modem_mode {
	MODE_SLEEP = 0,
	MODE_TX = 1,
	MODE_CW = 2,
	MODE_RX = 3,
	MODE_RX_ASYNC = 4,
	MODE_CAD = 5,
};

/* Common LBM modem configuration, must be first element of device config */
struct lbm_lora_config_common {
	/* LBM radio abstration layer structure */
	ralf_t ralf;
};

/* Common LBM modem data, must be first element of device data */
struct lbm_lora_data_common {
	/* Reference back to parent device */
	const struct device *dev;
	/* Current LoRa parameters */
	ral_lora_mod_params_t mod_params;
	ral_lora_pkt_params_t pkt_params;
	/* Operation complete worker */
	struct k_work_delayable op_done_work;
	/* RX state storage */
	union {
		struct {
			/* Sync RX params */
			uint8_t *msg;
			uint16_t msg_len;
			int16_t rssi_dbm;
			int8_t snr_db;
		} sync;
		struct {
			/* Async RX params */
			lora_recv_cb rx_cb;
			void *user_data;
		} async;
	} rx_state;
	/* User signal */
	struct k_poll_signal *operation_done;
	/* Current modem state */
	atomic_t modem_state;
	enum lbm_modem_mode modem_mode;
};

/**
 * @brief Initialise common LBM data structures
 *
 * @param dev Modem to initialise
 *
 * @retval 0 On success
 * @retval -errno On failure
 */
int lbm_lora_common_init(const struct device *dev);

/**
 * @brief Configure modem for a given mode
 *
 * Expected to be implemented by individual drivers
 *
 * @param dev Modem to configure
 * @param mode Mode to configure for
 */
void lbm_driver_antenna_configure(const struct device *dev, enum lbm_modem_mode mode);

/**
 * @brief Control a GPIO pin if it has been configured
 *
 * @param spec GPIO specification from devicetree
 * @param value Value assigned to the pin.
 *
 * @return a value from gpio_pin_set_dt()
 */
static inline int lbm_optional_gpio_set_dt(const struct gpio_dt_spec *spec, int value)
{
	if (spec->port != NULL) {
		return gpio_pin_set_dt(spec, value);
	}
	return 0;
}

/* Common LBM implementation of the LoRa API */
extern const struct lora_driver_api lbm_lora_api;
