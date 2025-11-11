/* ieee802154_stm32wba.h - STM32WBAxx 802.15.4 driver */

/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_STM32WBA_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_STM32WBA_H_
#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154_radio.h>

#define STM32WBA_PHR_LENGTH             1

#define STM32WBA_PWR_MIN                (-20)
#define STM32WBA_PWR_MAX                10


#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
/**
 *  STM32WBA specific configuration types of ieee802154 driver.
 *  This type extends @ref ieee802154_config_type.
 */
enum ieee802154_stm32wba_config_type {
	/**
	 * Allows to configure the CCA energy detection threshold value
	 */
	IEEE802154_STM32WBA_CONFIG_CCA_THRESHOLD = IEEE802154_CONFIG_PRIV_START,

	/**
	 * Configure (enable/disable) the continuous reception mode
	 */
	IEEE802154_STM32WBA_CONFIG_CONTINUOUS_RECEPTION,

	/**
	 * Set the maximum frame retries on a transmission failure
	 */
	IEEE802154_STM32WBA_CONFIG_MAX_FRAME_RETRIES,

	/**
	 * Set the maximum CSMA retries on a transmission failure
	 */
	IEEE802154_STM32WBA_CONFIG_MAX_CSMA_FRAME_RETRIES,

	/**
	 * Set the minimum CSMA backoff exponent value
	 */
	IEEE802154_STM32WBA_CONFIG_MIN_CSMA_BE,

	/**
	 * Set the maximum CSMA backoff exponent value
	 */
	IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BE,

	/**
	 * Set the maximum CSMA backoff attempts counter
	 */
	IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BACKOFF,

	/**
	 * Configure (enable/disable) the MAC implicit broadcast PIB
	 */
	IEEE802154_STM32WBA_CONFIG_IMPLICIT_BROADCAST,

	/**
	 * Configure (enable/disable) the antenna diversity
	 */
	IEEE802154_STM32WBA_CONFIG_ANTENNA_DIV,

	/**
	 * Reset the radio
	 */
	IEEE802154_STM32WBA_CONFIG_RADIO_RESET,
};

/** STM32WBA specific configuration data of ieee802154 driver. */
struct ieee802154_stm32wba_config {
	union {
		/** Common configuration */
		struct ieee802154_config common;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_CCA_THRESHOLD */
		int8_t cca_thr;

		/** @brief Attribute value for
		 * @ref IEEE802154_STM32WBA_CONFIG_CONTINUOUS_RECEPTION
		 */
		bool en_cont_rec;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_MAX_FRAME_RETRIES */
		uint8_t max_frm_retries;

		/** @brief Attribute value for
		 * @ref IEEE802154_STM32WBA_CONFIG_MAX_CSMA_FRAME_RETRIES
		 */
		uint8_t max_csma_frm_retries;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_MIN_CSMA_BE */
		uint8_t min_csma_be;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BE */
		uint8_t max_csma_be;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BACKOFF */
		uint8_t max_csma_backoff;

		/** @brief Attribute value for
		 * @ref IEEE802154_STM32WBA_CONFIG_IMPLICIT_BROADCAST
		 */
		bool impl_brdcast;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_ANTENNA_DIV */
		uint8_t ant_div;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_CONFIG_RADIO_RESET */
		bool radio_reset;
	};
};

/**
 *  STM32WBA specific attribute types of ieee802154 driver.
 *  This type extends @ref ieee802154_attr.
 */
enum ieee802154_stm32wba_attr {
	/**
	 * Get the CCA energy detection threshold value
	 */
	IEEE802154_STM32WBA_ATTR_CCA_THRESHOLD = IEEE802154_CONFIG_PRIV_START,

	/**
	 * Get the IEEE EUI64 of the device
	 */
	IEEE802154_STM32WBA_ATTR_IEEE_EUI64,

	/**
	 * Get the transmit power value
	 */
	IEEE802154_STM32WBA_ATTR_TX_POWER,

	/**
	 * Get a random number
	 */
	IEEE802154_STM32WBA_ATTR_RAND_NUM,
};

/**
 *  STM32WBA specific attribute value data of ieee802154 driver.
 *  This type extends @ref ieee802154_attr_value
 */
struct ieee802154_stm32wba_attr_value {
	union {
		/** Common attribute value */
		struct ieee802154_attr_value common;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_ATTR_CCA_THRESHOLD */
		int8_t *cca_thr;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_ATTR_IEEE_EUI64 */
		uint8_t eui64[8];

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_ATTR_TX_POWER */
		int8_t *tx_power;

		/** @brief Attribute value for @ref IEEE802154_STM32WBA_ATTR_RAND_NUM */
		uint8_t *rand_num;
	};
};
#endif /* CONFIG_NET_L2_CUSTOM_IEEE802154 */

struct stm32wba_802154_rx_frame {
	uint8_t *psdu; /* Pointer to a received frame. */
	uint8_t length; /* Received frame's length */
	uint64_t time; /* RX timestamp. */
	uint8_t lqi; /* Last received frame LQI value. */
	int8_t rssi; /* Last received frame RSSI value. */
	bool ack_fpb; /* FPB value in ACK sent for the received frame. */
	bool ack_seb; /* SEB value in ACK sent for the received frame. */
};

struct stm32wba_802154_data_t {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

	/* RX thread stack. */
	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_STM32WBA_RX_STACK_SIZE);

	/* RX thread control block. */
	struct k_thread rx_thread;

	/* RX fifo queue. */
	struct k_fifo rx_fifo;

	/* Buffers for passing received frame pointers and data to the
	 * RX thread via rx_fifo object.
	 */
	struct stm32wba_802154_rx_frame rx_frames[CONFIG_IEEE802154_STM32WBA_RX_BUFFERS];

	/* Frame pending bit value in ACK sent for the last received frame. */
	bool last_frame_ack_fpb;

	/* Security Enabled bit value in ACK sent for the last received frame. */
	bool last_frame_ack_seb;

	/* CCA complete semaphore. Unlocked when CCA is complete. */
	struct k_sem cca_wait;

	/* CCA result. Holds information whether channel is free or not. */
	bool channel_free;

	/* TX synchronization semaphore. Unlocked when frame has been
	 * sent or send procedure failed.
	 */
	struct k_sem tx_wait;

	/* TX buffer. First byte is PHR (length), remaining bytes are
	 * MPDU data.
	 */
	uint8_t tx_psdu[STM32WBA_PHR_LENGTH + IEEE802154_MAX_PHY_PACKET_SIZE];

	/* TX result, updated in radio transmit callbacks. */
	uint8_t tx_result;

	/* A buffer for the received ACK frame. psdu pointer be NULL if no
	 * ACK was requested/received.
	 */
	struct stm32wba_802154_rx_frame ack_frame;

	/* Callback handler of the currently ongoing energy scan.
	 * It shall be NULL if energy scan is not in progress.
	 */
	energy_scan_done_cb_t energy_scan_done_cb;

	/* Callback handler to notify of any important radio events.
	 * Can be NULL if event notification is not needed.
	 */
	ieee802154_event_cb_t event_handler;

	/* Indicates if currently processed TX frame is secured. */
	bool tx_frame_is_secured;

	/* Indicates if currently processed TX frame has dynamic data updated. */
	bool tx_frame_mac_hdr_rdy;

	/* The TX power in dBm. */
	int8_t txpwr;

	/* Indicates if RxOnWhenIdle mode is enabled. */
	bool rx_on_when_idle;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_STM32WBA_H_ */
