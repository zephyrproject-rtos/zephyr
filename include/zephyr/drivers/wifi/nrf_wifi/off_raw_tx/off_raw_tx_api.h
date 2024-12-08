/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file
 *
 * @addtogroup nrf70_off_raw_tx_api nRF70 Offloaded raw TX API
 * @{
 *
 * @brief File containing API's for the Offloaded raw TX feature.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_OFF_RAW_TX_API_H_
#define INCLUDE_ZEPHYR_DRIVERS_OFF_RAW_TX_API_H_

#include <stdbool.h>
#include <stdint.h>
#include "osal_api.h"

/*  Minimum frame size for raw packet transmission */
#define NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MIN 26
/*  Maximum frame size for raw packet transmission */
#define NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MAX 600
/* Maximum length of country code*/
#define NRF_WIFI_COUNTRY_CODE_LEN 2
/**
 * @brief- Transmission rates
 * Rate to be used for transmitting a packet.
 */
enum nrf_wifi_off_raw_tx_rate {
	/** 1 Mbps */
	RATE_1M,
	/** 2 Mbps */
	RATE_2M,
	/** 5.5 Mbps */
	RATE_5_5M,
	/** 11 Mbps */
	RATE_11M,
	/** 6 Mbps */
	RATE_6M,
	/** 9 Mbps */
	RATE_9M,
	/** 12 Mbps */
	RATE_12M,
	/** 18 Mbps */
	RATE_18M,
	/** 24 Mbps */
	RATE_24M,
	/** 36 Mbps */
	RATE_36M,
	/** 48 Mbps */
	RATE_48M,
	/** 54 Mbps */
	RATE_54M,
	/** MCS 0 */
	RATE_MCS0,
	/** MCS 1 */
	RATE_MCS1,
	/** MCS 2 */
	RATE_MCS2,
	/** MCS 3 */
	RATE_MCS3,
	/** MCS 4 */
	RATE_MCS4,
	/** MCS 5 */
	RATE_MCS5,
	/** MCS 6 */
	RATE_MCS6,
	/** MCS 7 */
	RATE_MCS7,
	/** Invalid rate */
	RATE_MAX
};


/**
 * @brief- HE guard interval value
 * Value of the guard interval to be used between symbols when transmitting using HE.
 */
enum nrf_wifi_off_raw_tx_he_gi {
	/** 800 ns */
	HE_GI_800NS,
	/** 1600 ns */
	HE_GI_1600NS,
	/** 3200 ns */
	HE_GI_3200NS,
	/** Invalid value */
	HE_GI_MAX
};


/**
 * @brief- HE long training field duration
 * Value of the long training field duration to be used when transmitting using HE.
 */
enum nrf_wifi_off_raw_tx_he_ltf {
	/** 3.2us */
	HE_LTF_3200NS,
	/** 6.4us */
	HE_LTF_6400NS,
	/** 12.8us */
	HE_LTF_12800NS,
	/** Invalid value */
	HE_LTF_MAX
};

/**
 * @brief- Throughput mode
 * Throughput mode to be used for transmitting the packet.
 */
enum nrf_wifi_off_raw_tx_tput_mode {
	/** Legacy mode */
	TPUT_MODE_LEGACY,
	/** High Throughput mode (11n) */
	TPUT_MODE_HT,
	/** Very high throughput mode (11ac) */
	TPUT_MODE_VHT,
	/** HE SU mode */
	TPUT_MODE_HE_SU,
	/** HE ER SU mode */
	TPUT_MODE_HE_ER_SU,
	/** HE TB mode */
	TPUT_MODE_HE_TB,
	/** Highest throughput mode currently defined */
	TPUT_MODE_MAX
};

/**
 * @brief This structure defines the Offloaded raw tx debug statistics.
 *
 */
struct nrf_wifi_off_raw_tx_stats {
	/** Number of packets sent */
	unsigned int off_raw_tx_pkt_sent;
};

/**
 * @brief- Configuration parameters for offloaded raw TX
 * Parameters which can be used to configure the offloaded raw TX operation.
 */
struct nrf_wifi_off_raw_tx_conf {
	/** Time interval (in microseconds) between transmissions */
	unsigned int period_us;
	/** Transmit power in dBm (0 to 20) */
	unsigned int tx_pwr;
	/** Channel number on which to transmit */
	unsigned int chan;
	/** Set to TRUE to use short preamble for FALSE to disable short preamble */
	bool short_preamble;
	/* Number of times a packet should be retried at each possible rate */
	unsigned int num_retries;
	/** Throughput mode for packet transmittion. Refer &enum nrf_wifi_off_raw_tx_tput_mode */
	enum nrf_wifi_off_raw_tx_tput_mode tput_mode;
	/* Rate at which packet needs to be transmitted. Refer &enum nrf_wifi_off_raw_tx_rate */
	enum nrf_wifi_off_raw_tx_rate rate;
	/** HE GI. Refer &enum nrf_wifi_off_raw_tx_he_gi */
	enum nrf_wifi_off_raw_tx_he_gi he_gi;
	/** HE GI. Refer &enum nrf_wifi_off_raw_tx_he_ltf */
	enum nrf_wifi_off_raw_tx_he_ltf he_ltf;
	/* Pointer to packet to be transmitted */
	void *pkt;
	/** Packet length of the frame to be transmitted, (min 26 bytes and max 600 bytes) */
	unsigned int pkt_len;
};


/**
 * @brief Initialize the nRF70 for operating in the offloaded raw TX mode.
 * @param mac_addr MAC address to be used for the nRF70 device.
 * @param country_code Country code to be set for regularity domain.
 *
 * This function is initializes the nRF70 device for offloaded raw TX mode by:
 *  - Powering it up,
 *  - Downloading a firmware patch (if any).
 *  - Initializing the firmware to accept further commands
 *
 * The mac_addr parameter is used to set the MAC address of the nRF70 device.
 * This address can be used to override the MAC addresses programmed in the OTP and
 * the value configured (if any) in CONFIG_WIFI_FIXED_MAC_ADDRESS.
 * The priority order in which the MAC address values for the nRF70 device are used is:
 * - If mac_addr is provided, the MAC address is set to the value provided.
 * - If CONFIG_WIFI_FIXED_MAC_ADDRESS is enabled, the MAC address uses the Kconfig value.
 * - If none of the above are provided, the MAC address is set to the value programmed in the OTP.
 *
 * @retval 0 If the operation was successful.
 * @retval -1 If the operation failed.
 */
int nrf70_off_raw_tx_init(uint8_t *mac_addr, unsigned char *country_code);

/**
 * @brief Initialize the nRF70 for operating in the offloaded raw TX mode.
 *
 * This function is deinitializes the nRF70 device.
 *
 */
void nrf70_off_raw_tx_deinit(void);

/**
 * @brief Update the configured offloaded raw TX parameters.
 * @param conf Configuration parameters to be updated for the offloaded raw TX operation.
 *
 * This function is used to update configured parameters for offloaded raw TX operation.
 * This function should be used to when the parameters need to be updated during an ongoing
 * raw TX operation without having to stop it.
 *
 * @retval 0 If the operation was successful.
 * @retval -1 If the operation failed.
 */
int nrf70_off_raw_tx_conf_update(struct nrf_wifi_off_raw_tx_conf *conf);

/**
 * @brief Start the offloaded raw TX.
 * @param conf Configuration parameters necessary for the offloaded raw TX operation.
 *
 * This function is used to start offloaded raw TX operation. When this function is invoked
 * the nRF70 device will start transmitting frames as per the configuration specified by @p conf.
 *
 * @retval 0 If the operation was successful.
 * @retval -1 If the operation failed.
 */
int nrf70_off_raw_tx_start(struct nrf_wifi_off_raw_tx_conf *conf);

/**
 * @brief Stop the offloaded raw TX.
 *
 * This function is used to stop offloaded raw TX operation. When this function is invoked
 * the nRF70 device will stop transmitting frames.
 *
 * @retval 0 If the operation was successful.
 * @retval -1 If the operation failed.
 */
int nrf70_off_raw_tx_stop(void);

/**
 * @brief Get the MAC address of the nRF70 device.
 * @param mac_addr Buffer to store the MAC address.
 *
 * This function is used to get the MAC address of the nRF70 device.
 * The MAC address is stored in the buffer pointed by mac_addr.
 * The MAC address is expected to be a 6 byte value.
 *
 * @retval 0 If the operation was successful.
 * @retval -1 If the operation failed.
 */
int nrf70_off_raw_tx_mac_addr_get(uint8_t *mac_addr);

/**
 * @brief Get statistics of the offloaded raw TX.
 * @param off_raw_tx_stats Statistics of the offloaded raw TX operation.
 *
 * This function is used to get statistics of offloaded raw TX operation. When this function
 * is invoked the nRF70 device will show statistics.
 *
 * @retval 0 If the operation was successful.
 * @retval -1 If the operation failed.
 */
int nrf70_off_raw_tx_stats(struct nrf_wifi_off_raw_tx_stats *off_raw_tx_stats);
/**
 * @}
 */
#endif /* INCLUDE_ZEPHYR_DRIVERS_OFF_RAW_TX_API_H_ */
