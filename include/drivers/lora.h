/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LORA_H_
#define ZEPHYR_INCLUDE_DRIVERS_LORA_H_

/**
 * @file
 * @brief Public LoRa APIs
 */

#include <zephyr/types.h>
#include <device.h>

enum lora_signal_bandwidth {
	BW_125_KHZ = 0,
	BW_250_KHZ,
	BW_500_KHZ,
};

enum lora_datarate {
	SF_6 = 6,
	SF_7,
	SF_8,
	SF_9,
	SF_10,
	SF_11,
	SF_12,
};

enum lora_coding_rate {
	CR_4_5 = 1,
	CR_4_6 = 2,
	CR_4_7 = 3,
	CR_4_8 = 4,
};

struct lora_modem_config {
	u32_t frequency;
	enum lora_signal_bandwidth bandwidth;
	enum lora_datarate datarate;
	enum lora_coding_rate coding_rate;
	u16_t preamble_len;
	s8_t tx_power;
	bool tx;
};

/**
 * @typedef lora_api_config()
 * @brief Callback API for configuring the LoRa module
 *
 * @see lora_config() for argument descriptions.
 */
typedef int (*lora_api_config)(struct device *dev,
			       struct lora_modem_config *config);

/**
 * @typedef lora_api_send()
 * @brief Callback API for sending data over LoRa
 *
 * @see lora_send() for argument descriptions.
 */
typedef int (*lora_api_send)(struct device *dev,
			     u8_t *data, u32_t data_len);

/**
 * @typedef lora_api_recv()
 * @brief Callback API for receiving data over LoRa
 *
 * @see lora_recv() for argument descriptions.
 */
typedef int (*lora_api_recv)(struct device *dev, u8_t *data, u8_t size,
			     s32_t timeout, s16_t *rssi, s8_t *snr);

struct lora_driver_api {
	lora_api_config config;
	lora_api_send	send;
	lora_api_recv	recv;
};

/**
 * @brief Configure the LoRa modem
 *
 * @param dev     LoRa device
 * @param config  Data structure containing the intended configuration for the
		  modem
 * @return 0 on success, negative on error
 */
static inline int lora_config(struct device *dev,
			      struct lora_modem_config *config)
{
	const struct lora_driver_api *api = dev->driver_api;

	return api->config(dev, config);
}

/**
 * @brief Send data over LoRa
 *
 * @note This is a non-blocking call.
 *
 * @param dev       LoRa device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @return 0 on success, negative on error
 */
static inline int lora_send(struct device *dev,
			    u8_t *data, u32_t data_len)
{
	const struct lora_driver_api *api = dev->driver_api;

	return api->send(dev, data, data_len);
}

/**
 * @brief Receive data over LoRa
 *
 * @note This is a blocking call.
 *
 * @param dev       LoRa device
 * @param data      Buffer to hold received data
 * @param size      Size of the buffer to hold the received data. Max size
		    allowed is 255.
 * @param timeout   Timeout value in milliseconds. API also accepts, K_NO_WAIT
		    for no wait time and K_FOREVER for blocking until
		    data arrives.
 * @param rssi      RSSI of received data
 * @param snr       SNR of received data
 * @return Length of the data received on success, negative on error
 */
static inline int lora_recv(struct device *dev, u8_t *data, u8_t size,
			    s32_t timeout, s16_t *rssi, s8_t *snr)
{
	const struct lora_driver_api *api = dev->driver_api;

	return api->recv(dev, data, size, timeout, rssi, snr);
}

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LORA_H_ */
