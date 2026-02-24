/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/atomic.h>

#include "sx126x_hal.h"
#include "sx126x_regs.h"

#define SX126X_STATE_IDLE 0
#define SX126X_STATE_TX   1
#define SX126X_STATE_RX   2

#define SX126X_MODULATION_LORA  0
#define SX126X_MODULATION_FSK   1

struct sx126x_tx_result {
	int status;
};

struct sx126x_rx_result {
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	int status;
};

struct sx126x_data {
	struct sx126x_hal_data hal;

	/* Current state (atomic for lock-free state transitions) */
	atomic_t state;
	struct k_mutex lock;

	/* Current configuration */
	struct lora_modem_config config;
	bool config_valid;

	/* TX completion via message queue */
	struct k_msgq tx_msgq;
	struct sx126x_tx_result tx_result;

	/* RX completion via message queue */
	struct k_msgq rx_msgq;
	struct sx126x_rx_result rx_result;

	/* FSK/GFSK Configuration storage */
	struct {
		uint32_t bitrate;
		uint32_t fdev;
		uint32_t frequency;
		uint8_t bandwidth;
		uint8_t shaping;
		uint16_t preamble_len;
		uint8_t preamble_detect;
		uint8_t sync_word_len;
		uint8_t addr_comp;
		uint8_t packet_type;
		uint8_t payload_len;
		uint8_t crc_type;
		uint8_t whitening;
		int8_t tx_power;
	} fsk_config;

	/* RX data buffer (shared between IRQ handler and recv) */
	uint8_t rx_buf[SX126X_MAX_PAYLOAD_LEN];

	/* Async RX callback */
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;

	/* Async TX signal */
	struct k_poll_signal *tx_async_signal;

	/* Deferred work for interrupt handling */
	struct k_work irq_work;
	const struct device *dev;

	/* Current modulation type */
	uint8_t current_modulation;

	/* LoRa sync word (1-byte spec value; 0 = use public_network from config) */
	uint8_t lora_sync_word;
};

/* ============================================ */
/* FSK/GFSK Public API                          */
/* ============================================ */

/**
 * @brief Configure SX126x for FSK/GFSK modulation
 *
 * @param dev Device instance
 * @param frequency RF frequency in Hz
 * @param bitrate Bit rate in bps (e.g., 50000 for 50 kbps)
 * @param fdev Frequency deviation in Hz (e.g., 25000 for 25 kHz)
 * @param shaping Gaussian shaping (SX126X_FSK_MOD_SHAPING_*)
 * @param bandwidth RX bandwidth (use SX126X_FSK_BW_* constants)
 * @param tx_power TX power in dBm
 * @return 0 on success, negative error code otherwise
 */
int sx126x_fsk_config(const struct device *dev,
		      uint32_t frequency, uint32_t bitrate,
		      uint32_t fdev, uint8_t shaping,
		      uint8_t bandwidth, int8_t tx_power);

/**
 * @brief Set FSK packet parameters
 *
 * @param dev Device instance
 * @param preamble_len Preamble length in bytes
 * @param sync_word Sync word bytes (up to 8 bytes)
 * @param sync_word_len Sync word length in bytes
 * @param fixed_len True for fixed length packets, false for variable
 * @param payload_len Payload length (for fixed) or max length (for variable)
 * @param crc_on Enable CRC
 * @param whitening Enable whitening (DC-free encoding)
 * @return 0 on success, negative error code otherwise
 */
int sx126x_fsk_set_packet_params(const struct device *dev,
				 uint16_t preamble_len,
				 const uint8_t *sync_word, uint8_t sync_word_len,
				 bool fixed_len, uint8_t payload_len,
				 bool crc_on, bool whitening);

/**
 * @brief Send data using FSK modulation
 *
 * @param dev Device instance
 * @param data_buf Data buffer to send
 * @param data_len Data length in bytes
 * @return 0 on success, negative error code otherwise
 */
int sx126x_fsk_send(const struct device *dev, uint8_t *data_buf, uint32_t data_len);

/**
 * @brief Receive data using FSK modulation
 *
 * @param dev Device instance
 * @param data_buf Buffer for received data
 * @param size Buffer size
 * @param timeout Reception timeout
 * @param rssi Pointer to store RSSI (can be NULL)
 * @return Number of bytes received on success, negative error code otherwise
 */
int sx126x_fsk_recv(const struct device *dev, uint8_t *data_buf,
		    uint8_t size, k_timeout_t timeout, int16_t *rssi);

/**
 * @brief Receive data asynchronously using FSK modulation
 *
 * Reception is cancelled by calling this function again with @p cb = NULL.
 *
 * @param dev Device instance
 * @param cb Callback to run on receiving data. If NULL, any pending
 *	     asynchronous receptions will be cancelled.
 * @param user_data User data passed to callback
 * @return 0 when reception successfully setup, negative on error
 */
int sx126x_fsk_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data);

/**
 * @brief Switch back to LoRa modulation
 *
 * @param dev Device instance
 * @return 0 on success, negative error code otherwise
 */
int sx126x_set_lora_mode(const struct device *dev);

/**
 * @brief Set LoRa Sync Word
 *
 * @param dev Device instance
 * @param sync_word 16-bit Sync Word (e.g. 0x1424 for private, 0x3444 for public)
 * @return 0 on success, negative error code otherwise
 */
int sx126x_set_lora_sync_word(const struct device *dev, uint8_t sync_word);

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_ */
