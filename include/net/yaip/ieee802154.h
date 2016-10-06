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
 * @brief IEEE 802.15.4 L2 stack public header
 */

#ifndef __IEEE802154_H__
#define __IEEE802154_H__

#include <net/net_mgmt.h>

/* This not meant to be used by any code but 802.15.4 L2 stack */
struct ieee802154_context {
	uint16_t pan_id;
	uint16_t channel;
	uint8_t sequence;
	struct nano_sem ack_lock;
	uint8_t ack_received	: 1;
	uint8_t ack_requested	: 1;
	uint8_t _unused		: 6;
} __packed;


/* Management part definitions */

#define _NET_IEEE802154_LAYER	NET_MGMT_LAYER_L2
#define _NET_IEEE802154_CODE	0x154
#define _NET_IEEE802154_BASE	(NET_MGMT_IFACE_BIT |			\
				 NET_MGMT_LAYER(_NET_IEEE802154_LAYER) |\
				 NET_MGMT_LAYER_CODE(_NET_IEEE802154_CODE))

enum net_request_ieee802154_cmd {
	NET_REQUEST_IEEE802154_CMD_SET_ACK = 1,
	NET_REQUEST_IEEE802154_CMD_UNSET_ACK,
};


#define NET_REQUEST_IEEE802154_SET_ACK					\
	(_NET_IEEE802154_BASE | NET_REQUEST_IEEE802154_CMD_SET_ACK)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_ACK);

#define NET_REQUEST_IEEE802154_UNSET_ACK				\
	(_NET_IEEE802154_BASE | NET_REQUEST_IEEE802154_CMD_UNSET_ACK)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_IEEE802154_UNSET_ACK);

#endif /* __IEEE802154_H__ */
