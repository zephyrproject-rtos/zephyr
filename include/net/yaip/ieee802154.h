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

struct ieee802154_context {
	uint16_t pan_id;
	uint16_t channel;
	uint8_t sequence;

#ifdef CONFIG_NET_L2_IEEE802154_RADIO_ALOHA
	struct nano_sem ack_lock;
	bool ack_received;
#endif /* CONFIG_NET_L2_IEEE802154_RADIO_ALOHA */
} __packed;

#endif /* __IEEE802154_H__ */
