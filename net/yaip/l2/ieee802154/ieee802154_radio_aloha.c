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

#include <net/net_core.h>
#include <net/net_if.h>

#include <errno.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"
#include "ieee802154_radio_utils.h"

static int aloha_radio_send(struct net_if *iface, struct net_buf *buf)
{
	uint8_t retries = CONFIG_NET_L2_IEEE802154_RADIO_ALOHA_TX_RETRIES;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_radio_api *radio =
		(struct ieee802154_radio_api *)iface->dev->driver_api;
	bool ack_required = prepare_for_ack(ctx, buf);
	int ret;

	while (retries) {
		retries--;

		ret = radio->tx(iface->dev, buf);
		if (ret) {
			continue;
		}

		ctx->sequence++;

		ret = wait_for_ack(ctx, ack_required);
		if (!ret) {
			break;
		}
	}

	net_nbuf_unref(buf);

	return ret;
}

static enum net_verdict aloha_radio_handle_ack(struct net_if *iface,
					       struct net_buf *buf)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return handle_ack(ctx, buf);
}

/* Declare the public Radio driver function used by the HW drivers */
FUNC_ALIAS(aloha_radio_send,
	   ieee802154_radio_send, int);

FUNC_ALIAS(aloha_radio_handle_ack,
	   ieee802154_radio_handle_ack, enum net_verdict);
