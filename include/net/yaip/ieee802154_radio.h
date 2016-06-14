/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Public IEEE 802.15.4 Radio API
 */

#ifndef __IEEE802154_RADIO_H__
#define __IEEE802154_RADIO_H__

#include <device.h>
#include <net/net_if.h>

struct ieee802154_radio_api {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;

	/** Clear Channel Assesment - Check channel's activity */
	int (*cca)(struct device *dev);

	/** Set current channel */
	int (*set_channel)(struct device *dev, uint16_t channel);

	/** Set current PAN id */
	int (*set_pan_id)(struct device *dev, uint16_t pan_id);

	/** Set current device's short address */
	int (*set_short_addr)(struct device *dev, uint16_t short_addr);

	/** Set current devices's full length address */
	int (*set_ieee_addr)(struct device *dev, const uint8_t *ieee_addr);

	/** Set TX power level in dbm */
	int (*set_txpower)(struct device *dev, int16_t dbm);

	/** Transmit a buffer */
	int (*tx)(struct device *dev, struct net_buf *buf);

	/** Start the device */
	int (*start)(struct device *dev);

	/** Stop the device */
	int (*stop)(struct device *dev);
} __packed;

/**
 * @brief Radio driver sending function that hw drivers should use
 *
 * @details This function should be used to fill in struct net_if's send pointer.
 *
 * @param iface A valid pointer on a network interface to send from
 * @param buf A valid pointer on a buffer to send
 *
 * @return 0 on success, negative value otherwise
 */
extern int ieee802154_radio_send(struct net_if *iface,
				 struct net_buf *buf);

/**
 * @brief Radio driver ACK handling function that hw drivers should use
 *
 * @details ACK handling requires fast handling and thus such function
 *          helps to hook direcly the hw drivers to the radio driver.
 *
 * @param iface A valid pointer on a network interface that received the packet
 * @param buf A valid pointer on a buffer to check
 *
 * @return NET_OK if it was handled, NET_CONTINUE otherwise
 */
extern enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
						    struct net_buf *buf);

#endif /* __IEEE802154_RADIO_H__ */
