/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup lora_interface
 * @brief Main header file for LoRa driver API.
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_LORA_H_
#define ZEPHYR_INCLUDE_DRIVERS_LORA_H_

/**
 * @brief Interfaces for LoRa transceivers.
 * @defgroup lora_interface LoRa
 * @since 2.2
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LoRa signal bandwidth
 *
 * This enumeration defines the bandwidth of a LoRa signal.
 *
 * The bandwidth determines how much spectrum is used to transmit data. Wider bandwidths enable
 * higher data rates but typically reduce sensitivity and range.
 */
enum lora_signal_bandwidth {
	BW_125_KHZ = 0,	/**< 125 kHz */
	BW_250_KHZ,	/**< 250 kHz */
	BW_500_KHZ,	/**< 500 kHz */
};

/**
 * @brief LoRa data-rate
 *
 * This enumeration represents the data rate of a LoRa signal, expressed as a Spreading Factor (SF).
 *
 * The Spreading Factor determines how many chirps are used to encode each symbol (2^SF chips per
 * symbol). Higher values result in lower data rates but increased range and robustness.
 */
enum lora_datarate {
	SF_6 = 6, /**< Spreading factor 6 (fastest, shortest range) */
	SF_7,     /**< Spreading factor 7 */
	SF_8,     /**< Spreading factor 8 */
	SF_9,     /**< Spreading factor 9 */
	SF_10,    /**< Spreading factor 10 */
	SF_11,    /**< Spreading factor 11 */
	SF_12,    /**< Spreading factor 12 (slowest, longest range) */
};

/**
 * @brief LoRa coding rate
 *
 * This enumeration defines the LoRa coding rate, used for forward error correction (FEC).
 *
 * The coding rate is expressed as 4/x, where a lower denominator (e.g., 4/5) means less redundancy,
 * resulting in a higher data rate but reduced robustness. Higher redundancy (e.g., 4/8) improves
 * error tolerance at the cost of data rate.
 */
enum lora_coding_rate {
	CR_4_5 = 1,  /**< Coding rate 4/5 (4 information bits, 1 error correction bit) */
	CR_4_6 = 2,  /**< Coding rate 4/6 (4 information bits, 2 error correction bits) */
	CR_4_7 = 3,  /**< Coding rate 4/7 (4 information bits, 3 error correction bits) */
	CR_4_8 = 4,  /**< Coding rate 4/8 (4 information bits, 4 error correction bits) */
};

/**
 * @struct lora_modem_config
 * Structure containing the configuration of a LoRa modem
 */
struct lora_modem_config {
	/** Frequency in Hz to use for transceiving */
	uint32_t frequency;

	/** The bandwidth to use for transceiving */
	enum lora_signal_bandwidth bandwidth;

	/** The data-rate to use for transceiving */
	enum lora_datarate datarate;

	/** The coding rate to use for transceiving */
	enum lora_coding_rate coding_rate;

	/** Length of the preamble */
	uint16_t preamble_len;

	/** TX-power in dBm to use for transmission */
	int8_t tx_power;

	/** Set to true for transmission, false for receiving */
	bool tx;

	/**
	 * Invert the In-Phase and Quadrature (IQ) signals. Normally this
	 * should be set to false. In advanced use-cases where a
	 * differentation is needed between "uplink" and "downlink" traffic,
	 * the IQ can be inverted to create two different channels on the
	 * same frequency
	 */
	bool iq_inverted;

	/**
	 * Sets the sync-byte to use:
	 *  - false: for using the private network sync-byte
	 *  - true:  for using the public network sync-byte
	 * The public network sync-byte is only intended for advanced usage.
	 * Normally the private network sync-byte should be used for peer
	 * to peer communications and the LoRaWAN APIs should be used for
	 * interacting with a public network.
	 */
	bool public_network;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @typedef lora_recv_cb()
 * @brief Callback API for receiving data asynchronously
 *
 * @see lora_recv() for argument descriptions.
 */
typedef void (*lora_recv_cb)(const struct device *dev, uint8_t *data, uint16_t size,
			     int16_t rssi, int8_t snr, void *user_data);

/**
 * @typedef lora_api_config()
 * @brief Callback API for configuring the LoRa module
 *
 * @see lora_config() for argument descriptions.
 */
typedef int (*lora_api_config)(const struct device *dev,
			       struct lora_modem_config *config);

/**
 * @typedef lora_api_send()
 * @brief Callback API for sending data over LoRa
 *
 * @see lora_send() for argument descriptions.
 */
typedef int (*lora_api_send)(const struct device *dev,
			     uint8_t *data, uint32_t data_len);

/**
 * @typedef lora_api_send_async()
 * @brief Callback API for sending data asynchronously over LoRa
 *
 * @see lora_send_async() for argument descriptions.
 */
typedef int (*lora_api_send_async)(const struct device *dev,
				   uint8_t *data, uint32_t data_len,
				   struct k_poll_signal *async);

/**
 * @typedef lora_api_recv()
 * @brief Callback API for receiving data over LoRa
 *
 * @see lora_recv() for argument descriptions.
 */
typedef int (*lora_api_recv)(const struct device *dev, uint8_t *data,
			     uint8_t size,
			     k_timeout_t timeout, int16_t *rssi, int8_t *snr);

/**
 * @typedef lora_api_recv_async()
 * @brief Callback API for receiving data asynchronously over LoRa
 *
 * @param dev Modem to receive data on.
 * @param cb Callback to run on receiving data.
 */
typedef int (*lora_api_recv_async)(const struct device *dev, lora_recv_cb cb,
			     void *user_data);

/**
 * @typedef lora_api_test_cw()
 * @brief Callback API for transmitting a continuous wave
 *
 * @see lora_test_cw() for argument descriptions.
 */
typedef int (*lora_api_test_cw)(const struct device *dev, uint32_t frequency,
				int8_t tx_power, uint16_t duration);

__subsystem struct lora_driver_api {
	lora_api_config config;
	lora_api_send send;
	lora_api_send_async send_async;
	lora_api_recv recv;
	lora_api_recv_async recv_async;
	lora_api_test_cw test_cw;
};

/** @endcond */

/**
 * @brief Configure the LoRa modem
 *
 * @param dev     LoRa device
 * @param config  Data structure containing the intended configuration for the
		  modem
 * @return 0 on success, negative on error
 */
static inline int lora_config(const struct device *dev,
			      struct lora_modem_config *config)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	return api->config(dev, config);
}

/**
 * @brief Send data over LoRa
 *
 * @note This blocks until transmission is complete.
 *
 * @param dev       LoRa device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @return 0 on success, negative on error
 */
static inline int lora_send(const struct device *dev,
			    uint8_t *data, uint32_t data_len)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	return api->send(dev, data, data_len);
}

/**
 * @brief Asynchronously send data over LoRa
 *
 * @note This returns immediately after starting transmission, and locks
 *       the LoRa modem until the transmission completes.
 *
 * @param dev       LoRa device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transmission).
 * @return 0 on success, negative on error
 */
static inline int lora_send_async(const struct device *dev,
				  uint8_t *data, uint32_t data_len,
				  struct k_poll_signal *async)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	return api->send_async(dev, data, data_len, async);
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
 * @param timeout   Duration to wait for a packet.
 * @param rssi      RSSI of received data
 * @param snr       SNR of received data
 * @return Length of the data received on success, negative on error
 */
static inline int lora_recv(const struct device *dev, uint8_t *data,
			    uint8_t size,
			    k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	return api->recv(dev, data, size, timeout, rssi, snr);
}

/**
 * @brief Receive data asynchronously over LoRa
 *
 * Receive packets continuously under the configuration previously setup
 * by @ref lora_config.
 *
 * Reception is cancelled by calling this function again with @p cb = NULL.
 * This can be done within the callback handler.
 *
 * @param dev Modem to receive data on.
 * @param cb Callback to run on receiving data. If NULL, any pending
 *	     asynchronous receptions will be cancelled.
 * @param user_data User data passed to callback
 * @return 0 when reception successfully setup, negative on error
 */
static inline int lora_recv_async(const struct device *dev, lora_recv_cb cb,
			       void *user_data)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	return api->recv_async(dev, cb, user_data);
}

/**
 * @brief Transmit an unmodulated continuous wave at a given frequency
 *
 * @note Only use this functionality in a test setup where the
 * transmission does not interfere with other devices.
 *
 * @param dev       LoRa device
 * @param frequency Output frequency (Hertz)
 * @param tx_power  TX power (dBm)
 * @param duration  Transmission duration in seconds.
 * @return 0 on success, negative on error
 */
static inline int lora_test_cw(const struct device *dev, uint32_t frequency,
			       int8_t tx_power, uint16_t duration)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->test_cw == NULL) {
		return -ENOSYS;
	}

	return api->test_cw(dev, frequency, tx_power, duration);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LORA_H_ */
