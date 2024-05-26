/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file extends interface of ieee802154_radio.h for OpenThread.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_OPENTHREAD_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_OPENTHREAD_H_

#include <zephyr/net/ieee802154_radio.h>

/**
 *  OpenThread specific capabilities of ieee802154 driver.
 *  This type extends @ref ieee802154_hw_caps.
 */
enum ieee802154_openthread_hw_caps {
	/** Capability to transmit with @ref IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA
	 *  mode.
	 */
	IEEE802154_OPENTHREAD_HW_MULTIPLE_CCA = BIT(IEEE802154_HW_CAPS_BITS_PRIV_START),
};

/** @brief TX mode */
enum ieee802154_openthread_tx_mode {
	/**
	 * The @ref IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA mode allows to send
	 * a scheduled packet if the channel is reported idle after at most
	 * 1 + max_extra_cca_attempts CCAs performed back-to-back.
	 *
	 * This mode is a non-standard experimental OpenThread feature. It allows transmission
	 * of a packet within a certain time window.
	 * The earliest transmission time is specified as in the other TXTIME modes:
	 * When the first CCA reports an idle channel then the first symbol of the packet's PHR
	 * SHALL be present at the local antenna at the time represented by the scheduled
	 * TX timestamp (referred to as T_tx below).
	 *
	 * If the first CCA reports a busy channel, then additional CCAs up to
	 * max_extra_cca_attempts will be done until one of them reports an idle channel and
	 * the packet is sent out or the max number of attempts is reached in which case
	 * the transmission fails.
	 *
	 * The timing of these additional CCAs depends on the capabilities of the driver
	 * which reports them in the T_recca and T_ccatx driver attributes
	 * (see @ref IEEE802154_OPENTHREAD_ATTR_T_RECCA and
	 * @ref IEEE802154_OPENTHREAD_ATTR_T_CCATX). Based on these attributes the upper layer
	 * can calculate the latest point in time (T_txmax) that the first symbol of the scheduled
	 * packet's PHR SHALL be present at the local antenna:
	 *
	 * T_maxtxdelay = max_extra_cca_attempts * (aCcaTime + T_recca) - T_recca + T_ccatx
	 * T_txmax = T_tx + T_maxtxdelay
	 *
	 * See IEEE 802.15.4-2020, section 11.3, table 11-1 for the definition of aCcaTime.
	 *
	 * Drivers implementing this TX mode SHOULD keep T_recca and T_ccatx as short as possible.
	 * T_ccatx SHALL be less than or equal aTurnaroundTime as defined in ibid.,
	 * section 11.3, table 11-1.
	 *
	 * CCA SHALL be executed as defined by the phyCcaMode PHY PIB attribute (see ibid.,
	 * section 11.3, table 11-2).
	 *
	 * Requires IEEE802154_OPENTHREAD_HW_MULTIPLE_CCA capability.
	 */
	IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA = IEEE802154_TX_MODE_PRIV_START
};

/**
 *  OpenThread specific configuration types of ieee802154 driver.
 *  This type extends @ref ieee802154_config_type.
 */
enum ieee802154_openthread_config_type {
	/** Allows to configure extra CCA for transmission requested with mode
	 *  @ref IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA.
	 *  Requires IEEE802154_OPENTHREAD_HW_MULTIPLE_CCA capability.
	 */
	IEEE802154_OPENTHREAD_CONFIG_MAX_EXTRA_CCA_ATTEMPTS  = IEEE802154_CONFIG_PRIV_START
};

/**
 * Thread vendor OUI for vendor specific header or nested information elements,
 * see IEEE 802.15.4-2020, sections 7.4.2.2 and 7.4.4.30.
 *
 * in little endian
 */
#define IEEE802154_OPENTHREAD_THREAD_IE_VENDOR_OUI { 0x9b, 0xb8, 0xea }

/** length of IEEE 802.15.4-2020 vendor OUIs */
#define IEEE802154_OPENTHREAD_VENDOR_OUI_LEN 3

/** OpenThread specific configuration data of ieee802154 driver. */
struct ieee802154_openthread_config {
	union {
		/** Common configuration */
		struct ieee802154_config common;

		/** ``IEEE802154_OPENTHREAD_CONFIG_MAX_EXTRA_CCA_ATTEMPTS``
		 *
		 *  The maximum number of extra CCAs to be performed when transmission is
		 *  requested with mode @ref IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA.
		 */
		uint8_t max_extra_cca_attempts;
	};
};

/**
 *  OpenThread specific attributes of ieee802154 driver.
 *  This type extends @ref ieee802154_attr
 */
enum ieee802154_openthread_attr {

	/** Attribute: Maximum time between consecutive CCAs performed back-to-back.
	 *
	 *  This is attribute for T_recca parameter mentioned for
	 *  @ref IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA.
	 *  Time is expressed in microseconds.
	 */
	IEEE802154_OPENTHREAD_ATTR_T_RECCA = IEEE802154_ATTR_PRIV_START,

	/** Attribute: Maximum time between detection of CCA idle channel and the moment of
	 *  start of SHR at the local antenna.
	 *
	 *  This is attribute for T_ccatx parameter mentioned for
	 *  @ref IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA.
	 *  Time is expressed in microseconds.
	 */
	IEEE802154_OPENTHREAD_ATTR_T_CCATX
};

/**
 *  OpenThread specific attribute value data of ieee802154 driver.
 *  This type extends @ref ieee802154_attr_value
 */
struct ieee802154_openthread_attr_value {
	union {
		/** Common attribute value */
		struct ieee802154_attr_value common;

		/** @brief Attribute value for @ref IEEE802154_OPENTHREAD_ATTR_T_RECCA */
		uint16_t t_recca;

		/** @brief Attribute value for @ref IEEE802154_OPENTHREAD_ATTR_T_CCATX */
		uint16_t t_ccatx;

	};
};

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_OPENTHREAD_H_ */
