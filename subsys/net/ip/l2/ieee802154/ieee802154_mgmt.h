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
 * @brief IEEE 802.15.4 Management
 */

#ifndef __IEEE802154_MGMT_H__
#define __IEEE802154_MGMT_H__

#ifdef CONFIG_NET_MGMT

static inline bool ieee802154_is_scanning(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return !(!ctx->scan_ctx);
}

static inline void ieee802154_mgmt_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	k_sem_init(&ctx->res_lock, 1, 1);
}

enum net_verdict ieee802154_handle_beacon(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu);

#else /* CONFIG_NET_MGMT */

#define ieee802154_is_scanning(...) false
#define ieee802154_mgmt_init(...)
#define ieee802154_handle_beacon(...) NET_DROP

#endif /* CONFIG_NET_MGMT */

#endif /* __IEEE802154_MGMT_H__ */
