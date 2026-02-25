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
 * @brief Radio modulation type
 *
 * Selects the modulation used for the next @ref lora_config call.
 * LoRa-capable modems (e.g. SX126x family) support multiple modulations on
 * the same hardware. FSK is required for LoRaWAN regional plans that mandate
 * an FSK data rate (e.g. EU868 DR7).
 *
 * The value @c LORA_MOD_LORA is zero so that zero-initialized
 * @ref lora_modem_config structs default to LoRa, preserving backward
 * compatibility with existing code that does not set this field.
 */
enum lora_modulation {
	LORA_MOD_LORA = 0, /**< LoRa chirp spread spectrum (default) */
	LORA_MOD_FSK  = 1, /**< (G)FSK - also used for LoRaWAN FSK data rates */
};

/**
 * @brief FSK Gaussian pulse shaping
 *
 * Controls the Gaussian low-pass filter applied to the frequency modulation
 * signal before transmission. Stronger shaping (lower BT product) reduces
 * spectral occupancy at the cost of increased inter-symbol interference.
 */
enum lora_fsk_shaping {
	LORA_FSK_SHAPING_NONE        = 0, /**< No shaping filter */
	LORA_FSK_SHAPING_GAUSS_BT_0_3 = 1, /**< Gaussian filter BT=0.3 */
	LORA_FSK_SHAPING_GAUSS_BT_0_5 = 2, /**< Gaussian filter BT=0.5 */
	LORA_FSK_SHAPING_GAUSS_BT_0_7 = 3, /**< Gaussian filter BT=0.7 */
	LORA_FSK_SHAPING_GAUSS_BT_1_0 = 4, /**< Gaussian filter BT=1.0 */
};

/**
 * @brief FSK RX channel bandwidth
 *
 * Nominal single-sideband RX filter bandwidth in kHz. Select a bandwidth
 * slightly wider than @c 2 * fdev + bitrate to accommodate both the signal
 * and frequency offset. Wider bandwidths reduce sensitivity; narrower
 * bandwidths improve it at the cost of tolerance to frequency error.
 */
enum lora_fsk_bandwidth {
	FSK_BW_4_KHZ   = 4,    /**< ~4.8 kHz */
	FSK_BW_5_KHZ   = 5,    /**< ~5.8 kHz */
	FSK_BW_7_KHZ   = 7,    /**< ~7.3 kHz */
	FSK_BW_9_KHZ   = 9,    /**< ~9.7 kHz */
	FSK_BW_11_KHZ  = 11,   /**< ~11.7 kHz */
	FSK_BW_14_KHZ  = 14,   /**< ~14.6 kHz */
	FSK_BW_19_KHZ  = 19,   /**< ~19.5 kHz */
	FSK_BW_23_KHZ  = 23,   /**< ~23.4 kHz */
	FSK_BW_29_KHZ  = 29,   /**< ~29.3 kHz */
	FSK_BW_39_KHZ  = 39,   /**< ~39.0 kHz */
	FSK_BW_46_KHZ  = 46,   /**< ~46.9 kHz */
	FSK_BW_58_KHZ  = 58,   /**< ~58.6 kHz */
	FSK_BW_78_KHZ  = 78,   /**< ~78.2 kHz */
	FSK_BW_93_KHZ  = 93,   /**< ~93.8 kHz */
	FSK_BW_117_KHZ = 117,  /**< ~117.3 kHz */
	FSK_BW_156_KHZ = 156,  /**< ~156.2 kHz */
	FSK_BW_187_KHZ = 187,  /**< ~187.2 kHz */
	FSK_BW_234_KHZ = 234,  /**< ~234.3 kHz */
	FSK_BW_312_KHZ = 312,  /**< ~312.0 kHz */
	FSK_BW_373_KHZ = 373,  /**< ~373.6 kHz */
	FSK_BW_467_KHZ = 467,  /**< ~467.0 kHz */
};

/**
 * @brief FSK/GFSK modulation configuration
 *
 * Passed inside @ref lora_modem_config when @c modulation is set to
 * @ref LORA_MOD_FSK. All fields are ignored for other modulations.
 */
struct lora_fsk_config {
	/** Bit rate in bits per second (e.g. 50000 for 50 kbps) */
	uint32_t bitrate;

	/**
	 * Frequency deviation in Hz (e.g. 25000 for ±25 kHz).
	 * Carson's rule: occupied bandwidth ≈ 2*(fdev + bitrate/2).
	 */
	uint32_t fdev;

	/** Gaussian pulse shaping applied to the modulated signal */
	enum lora_fsk_shaping shaping;

	/** RX channel filter bandwidth */
	enum lora_fsk_bandwidth bandwidth;

	/**
	 * Preamble length in bytes (minimum 2; typical 4-8).
	 * Zero selects a driver-defined default.
	 */
	uint16_t preamble_len;

	/**
	 * Sync word bytes (up to 8 bytes).
	 *
	 * The sync word marks the start of each packet and must match
	 * between transmitter and receiver. Common values:
	 *  - LoRaWAN GFSK (EU868 DR7): { 0xC1, 0x94, 0xC1 } (3 bytes)
	 *  - Custom peer-to-peer:       any agreed-upon sequence
	 *
	 * Set @c sync_word_len to 0 to use the driver's built-in default.
	 */
	uint8_t sync_word[8];

	/**
	 * Number of valid bytes in @c sync_word (0-8).
	 *
	 * Set to 0 to use the driver's built-in default sync word.
	 * Longer sync words improve false-sync rejection at the cost of
	 * a small overhead per packet.
	 */
	uint8_t sync_word_len;

	/**
	 * Maximum payload length in bytes.
	 * For variable-length packets this is the receive buffer limit.
	 * For fixed-length packets this is the exact TX/RX payload size.
	 * Zero selects the driver-defined maximum (up to 255 bytes).
	 */
	uint8_t payload_len;

	/**
	 * Use variable-length packet format.
	 * When true, a length byte is prepended to each packet.
	 * When false, @c payload_len must be set to the exact packet size.
	 */
	bool variable_len;

	/** Enable 16-bit CRC on payload */
	bool crc_on;

	/** Enable data whitening (DC-free encoding) */
	bool whitening;
};

/**
 * @struct lora_modem_config
 * Structure containing the configuration of a LoRa modem
 *
 * The @c modulation field selects the active modulation. Fields that belong to
 * a specific modulation (e.g. @c bandwidth for LoRa, @c fsk for FSK) are
 * ignored when the corresponding modulation is not selected. Because
 * @c LORA_MOD_LORA has the value 0, zero-initialized structs default to LoRa
 * mode and all existing code that does not set @c modulation continues to work
 * unchanged.
 */
struct lora_modem_config {
	/** Frequency in Hz to use for transceiving */
	uint32_t frequency;

	/** The bandwidth to use for transceiving (LoRa mode only) */
	enum lora_signal_bandwidth bandwidth;

	/** The data-rate to use for transceiving (LoRa mode only) */
	enum lora_datarate datarate;

	/** The coding rate to use for transceiving (LoRa mode only) */
	enum lora_coding_rate coding_rate;

	/** Length of the preamble (LoRa mode only) */
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
	 * same frequency. LoRa mode only.
	 */
	bool iq_inverted;

	/**
	 * Sets the sync-byte to use (LoRa mode only):
	 *  - false: for using the private network sync-byte
	 *  - true:  for using the public network sync-byte
	 * The public network sync-byte is only intended for advanced usage.
	 * Normally the private network sync-byte should be used for peer
	 * to peer communications and the LoRaWAN APIs should be used for
	 * interacting with a public network.
	 */
	bool public_network;

	/**
	 * Custom LoRa sync word (1-byte network ID; LoRa mode only).
	 *
	 * When non-zero this overrides @c public_network and programs the
	 * supplied byte directly as the network discriminator (e.g. 0x2D
	 * for Meshtastic). The driver maps it to the 2-byte register value
	 * internally. When zero the @c public_network field is used instead.
	 */
	uint8_t lora_sync_word;

	/** Set to true to disable the 16-bit payload CRC (LoRa mode only) */
	bool packet_crc_disable;

	/**
	 * Modulation type to use.
	 *
	 * Defaults to @ref LORA_MOD_LORA (value 0) when zero-initialized,
	 * so existing code that does not set this field continues to work
	 * unchanged. Set to @ref LORA_MOD_FSK to use FSK/GFSK and fill in
	 * the @c fsk sub-struct below.
	 */
	enum lora_modulation modulation;

	/** FSK configuration (valid only when @c modulation == LORA_MOD_FSK) */
	struct lora_fsk_config fsk;
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
