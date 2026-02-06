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
 * @version 0.8.0
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
	BW_7_KHZ = 7,		/**< 7.81 kHz */
	BW_10_KHZ = 10,		/**< 10.42 kHz */
	BW_15_KHZ = 15,		/**< 15.63 kHz */
	BW_20_KHZ = 20,		/**< 20.83 kHz */
	BW_31_KHZ = 31,		/**< 31.25 kHz */
	BW_41_KHZ = 41,		/**< 41.67 kHz */
	BW_62_KHZ = 62,		/**< 62.5 kHz */
	BW_125_KHZ = 125,	/**< 125 kHz */
	BW_200_KHZ = 200,	/**< 203 kHz */
	BW_250_KHZ = 250,	/**< 250 kHz */
	BW_400_KHZ = 400,	/**< 406 kHz */
	BW_500_KHZ = 500,	/**< 500 kHz */
	BW_800_KHZ = 800,	/**< 812 kHz */
	BW_1000_KHZ = 1000,	/**< 1000 kHz */
	BW_1600_KHZ = 1600,	/**< 1625 kHz */
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
	SF_5 = 5,	/**< Spreading factor 5 (fastest, shortest range) */
	SF_6 = 6,	/**< Spreading factor 6 */
	SF_7 = 7,	/**< Spreading factor 7 */
	SF_8 = 8,	/**< Spreading factor 8 */
	SF_9 = 9,	/**< Spreading factor 9 */
	SF_10 = 10,	/**< Spreading factor 10 */
	SF_11 = 11,	/**< Spreading factor 11 */
	SF_12 = 12,	/**< Spreading factor 12 (slowest, longest range) */
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
 * @brief Number of symbols used for CAD detection
 *
 * More symbols improve detection reliability at the cost of increased time-on-air and power
 * consumption.
 */
enum lora_cad_symbol_num {
	LORA_CAD_SYMB_1 = 1,	/**< 1 symbol */
	LORA_CAD_SYMB_2 = 2,	/**< 2 symbols */
	LORA_CAD_SYMB_4 = 4,	/**< 4 symbols */
	LORA_CAD_SYMB_8 = 8,	/**< 8 symbols */
	LORA_CAD_SYMB_16 = 16,	/**< 16 symbols */
};

/**
 * @struct lora_cad_config
 * @brief Configuration for Channel Activity Detection
 *
 * Set fields to 0 to let the driver auto-derive sensible defaults from the current modem
 * configuration (SF, BW).
 */
struct lora_cad_config {
	/** Number of symbols for CAD detection. 0 = driver default. */
	enum lora_cad_symbol_num symbol_num;

	/** Detection peak threshold. 0 = auto-derive from SF/BW. */
	uint8_t det_peak;

	/** Minimum detection threshold. 0 = auto-derive from SF/BW. */
	uint8_t det_min;
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

	/** Set to true to disable the 16-bit payload CRC */
	bool packet_crc_disable;
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
 * @typedef lora_cad_cb()
 * @brief Callback API for channel activity detection asynchronously
 *
 * @param dev               LoRa device
 * @param activity_detected true if LoRa activity was detected on the channel
 * @param user_data         User data passed to @ref lora_cad_async
 */
typedef void (*lora_cad_cb)(const struct device *dev, bool activity_detected,
			    void *user_data);

/**
 * @typedef lora_cad_recv_cb()
 * @brief Callback API for CAD followed by receive asynchronously
 *
 * @param dev               LoRa device
 * @param activity_detected true if LoRa activity was detected on the channel.
 *                          When false, the remaining parameters are unused.
 * @param data              Pointer to received data (valid when activity_detected
 *                          is true and size > 0)
 * @param size              Length of received data. 0 if activity was detected
 *                          but no packet was received (RX timeout).
 * @param rssi              RSSI of received data
 * @param snr               SNR of received data
 * @param user_data         User data passed to @ref lora_cad_recv_async
 */
typedef void (*lora_cad_recv_cb)(const struct device *dev, bool activity_detected,
				 uint8_t *data, uint16_t size,
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
 * @typedef lora_api_airtime()
 * @brief Callback API for querying packet airtime
 *
 * @see lora_airtime() for argument descriptions.
 */
typedef uint32_t (*lora_api_airtime)(const struct device *dev, uint32_t data_len);

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
 * @typedef lora_api_cad_config()
 * @brief Callback API for configuring CAD parameters
 *
 * @see lora_cad_config() for argument descriptions.
 */
typedef int (*lora_api_cad_config)(const struct device *dev,
				   struct lora_cad_config *config);

/**
 * @typedef lora_api_cad()
 * @brief Callback API for channel activity detection
 *
 * @see lora_cad() for argument descriptions.
 */
typedef int (*lora_api_cad)(const struct device *dev, k_timeout_t timeout);

/**
 * @typedef lora_api_cad_async()
 * @brief Callback API for channel activity detection asynchronously
 *
 * @see lora_cad_async() for argument descriptions.
 */
typedef int (*lora_api_cad_async)(const struct device *dev, lora_cad_cb cb,
				  void *user_data);

/**
 * @typedef lora_api_cad_recv()
 * @brief Callback API for CAD followed by receive
 *
 * @see lora_cad_recv() for argument descriptions.
 */
typedef int (*lora_api_cad_recv)(const struct device *dev, uint8_t *data,
				 uint8_t size, k_timeout_t timeout,
				 int16_t *rssi, int8_t *snr);

/**
 * @typedef lora_api_cad_recv_async()
 * @brief Callback API for CAD followed by receive asynchronously
 *
 * @see lora_cad_recv_async() for argument descriptions.
 */
typedef int (*lora_api_cad_recv_async)(const struct device *dev,
				       lora_cad_recv_cb cb, void *user_data);

/**
 * @typedef lora_api_cad_send()
 * @brief Callback API for CAD followed by send (Listen Before Talk)
 *
 * @see lora_cad_send() for argument descriptions.
 */
typedef int (*lora_api_cad_send)(const struct device *dev,
				 uint8_t *data, uint32_t data_len);

/**
 * @typedef lora_api_cad_send_async()
 * @brief Callback API for CAD followed by send asynchronously
 *
 * @see lora_cad_send_async() for argument descriptions.
 */
typedef int (*lora_api_cad_send_async)(const struct device *dev,
				       uint8_t *data, uint32_t data_len,
				       struct k_poll_signal *async);

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
	lora_api_airtime airtime;
	lora_api_send send;
	lora_api_send_async send_async;
	lora_api_recv recv;
	lora_api_recv_async recv_async;
	lora_api_cad_config cad_config;
	lora_api_cad cad;
	lora_api_cad_async cad_async;
	lora_api_cad_recv cad_recv;
	lora_api_cad_recv_async cad_recv_async;
	lora_api_cad_send cad_send;
	lora_api_cad_send_async cad_send_async;
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
 * @brief Query the airtime of a packet with a given length
 *
 * @note Uses the current radio configuration from @ref lora_config
 *
 * @param dev       LoRa device
 * @param data_len  Length of the data
 * @return Airtime of packet in milliseconds
 */
static inline uint32_t lora_airtime(const struct device *dev, uint32_t data_len)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	return api->airtime(dev, data_len);
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
 * @brief Configure Channel Activity Detection parameters
 *
 * Sets the CAD detection parameters. Set struct fields to 0 to use
 * driver-recommended defaults derived from the current modem configuration
 * (SF, BW).
 *
 * If @p config is NULL, the driver uses its internal defaults. This is
 * useful for radios that do not expose tunable CAD parameters or when
 * the application does not need fine-grained control.
 *
 * Calling this function is optional. If not called, CAD operations use
 * driver defaults.
 *
 * @param dev     LoRa device
 * @param config  CAD configuration parameters, or NULL for driver defaults
 * @return 0 on success, negative on error
 */
static inline int lora_cad_config(const struct device *dev,
				  struct lora_cad_config *config)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad_config == NULL) {
		return -ENOSYS;
	}

	return api->cad_config(dev, config);
}

/**
 * @brief Perform Channel Activity Detection
 *
 * Checks whether a LoRa signal is present on the channel using the modem
 * configuration previously set by @ref lora_config and the CAD parameters
 * optionally set by @ref lora_cad_config.
 *
 * @note This is a blocking call.
 *
 * @param dev      LoRa device
 * @param timeout  Maximum time to wait for CAD to complete
 * @return 0 if no activity detected (channel free)
 * @return 1 if activity detected (channel busy)
 * @return -EBUSY if the modem is in use
 * @return -ETIMEDOUT if the operation timed out
 * @return negative on other errors
 */
static inline int lora_cad(const struct device *dev, k_timeout_t timeout)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad == NULL) {
		return -ENOSYS;
	}

	return api->cad(dev, timeout);
}

/**
 * @brief Perform Channel Activity Detection asynchronously
 *
 * Starts a single CAD operation using the parameters optionally set by
 * @ref lora_cad_config. When complete, invokes @p cb with the result.
 *
 * Cancel a pending operation by calling this function again with
 * @p cb = NULL.
 *
 * @param dev        LoRa device
 * @param cb         Callback invoked on completion. NULL to cancel.
 * @param user_data  User data passed to callback
 * @return 0 on success, negative on error
 */
static inline int lora_cad_async(const struct device *dev, lora_cad_cb cb,
				 void *user_data)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad_async == NULL) {
		return -ENOSYS;
	}

	return api->cad_async(dev, cb, user_data);
}

/**
 * @brief Perform CAD followed by receive if activity is detected
 *
 * The radio performs CAD first using the parameters optionally set by
 * @ref lora_cad_config. If activity is detected, it automatically enters
 * RX mode to receive the incoming packet. If no activity is detected,
 * returns immediately.
 *
 * @note This is a blocking call.
 *
 * @param dev      LoRa device
 * @param data     Buffer to hold received data
 * @param size     Size of the receive buffer (max 255)
 * @param timeout  Maximum time to wait for the complete operation
 * @param rssi     RSSI of received data (output)
 * @param snr      SNR of received data (output)
 * @return Length of received data if activity detected and packet received
 * @return 0 if no activity detected (channel free)
 * @return -ETIMEDOUT if the operation timed out
 * @return negative on other errors
 */
static inline int lora_cad_recv(const struct device *dev, uint8_t *data,
				uint8_t size, k_timeout_t timeout,
				int16_t *rssi, int8_t *snr)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad_recv == NULL) {
		return -ENOSYS;
	}

	return api->cad_recv(dev, data, size, timeout, rssi, snr);
}

/**
 * @brief Perform CAD followed by receive asynchronously
 *
 * Starts a CAD operation using the parameters optionally set by
 * @ref lora_cad_config. If activity is detected, the radio enters RX mode.
 * The callback reports the outcome: no activity, or the received packet data.
 *
 * Cancel a pending operation by calling this function again with
 * @p cb = NULL.
 *
 * @param dev        LoRa device
 * @param cb         Callback invoked on completion. NULL to cancel.
 * @param user_data  User data passed to callback
 * @return 0 on success, negative on error
 */
static inline int lora_cad_recv_async(const struct device *dev,
				      lora_cad_recv_cb cb, void *user_data)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad_recv_async == NULL) {
		return -ENOSYS;
	}

	return api->cad_recv_async(dev, cb, user_data);
}

/**
 * @brief Perform CAD followed by send if channel is free (Listen Before Talk)
 *
 * The radio performs CAD first using the parameters optionally set by
 * @ref lora_cad_config. If no activity is detected (channel is free), it
 * automatically transmits the provided data. If activity is detected, the
 * data is not sent and the function returns immediately.
 *
 * @note This is a blocking call.
 *
 * @param dev       LoRa device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @return 0 if channel free and data transmitted successfully
 * @return 1 if activity detected (channel busy), data NOT transmitted
 * @return -EBUSY if the modem is in use
 * @return negative on other errors
 */
static inline int lora_cad_send(const struct device *dev,
				uint8_t *data, uint32_t data_len)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad_send == NULL) {
		return -ENOSYS;
	}

	return api->cad_send(dev, data, data_len);
}

/**
 * @brief Perform CAD followed by send asynchronously (Listen Before Talk)
 *
 * Starts a CAD operation using the parameters optionally set by
 * @ref lora_cad_config. If no activity is detected, the radio transmits
 * the provided data. If activity is detected, the data is not sent.
 *
 * The signal result indicates the outcome:
 *  - 0: channel free, data transmitted
 *  - 1: channel busy, data NOT transmitted
 *  - negative: error
 *
 * @param dev       LoRa device
 * @param data      Data to be sent
 * @param data_len  Length of the data to be sent
 * @param async     A pointer to a valid and ready to be signaled
 *                  struct k_poll_signal. (Note: if NULL this function will not
 *                  notify the end of the operation).
 * @return 0 on success, negative on error
 */
static inline int lora_cad_send_async(const struct device *dev,
				      uint8_t *data, uint32_t data_len,
				      struct k_poll_signal *async)
{
	const struct lora_driver_api *api =
		(const struct lora_driver_api *)dev->api;

	if (api->cad_send_async == NULL) {
		return -ENOSYS;
	}

	return api->cad_send_async(dev, data, data_len, async);
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
