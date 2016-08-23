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

#include <misc/util.h>

#include <stdlib.h>
#include <errno.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"
#include "ieee802154_radio_utils.h"

static int csma_ca_radio_send(struct net_if *iface, struct net_buf *buf)
{
	const uint8_t max_bo = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BO;
	const uint8_t max_be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BE;
	uint8_t retries = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_radio_api *radio =
		(struct ieee802154_radio_api *)iface->dev->driver_api;
	bool ack_required = prepare_for_ack(ctx, buf);
	uint8_t be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MIN_BE;
	uint8_t nb = 0;
	int ret = -EIO;

loop:
	while (retries) {
		retries--;

		if (be) {
			uint8_t bo_n = sys_rand32_get() & (2 << (be + 1));

			sys_thread_busy_wait(bo_n * 20);
		}

		while (1) {
			if (!radio->cca(iface->dev)) {
				break;
			}

			be = min(be + 1, max_be);
			nb++;

			if (nb > max_bo) {
				goto loop;
			}
		}

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

static enum net_verdict csma_ca_radio_handle_ack(struct net_if *iface,
						 struct net_buf *buf)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return handle_ack(ctx, buf);
}

/* Declare the public Radio driver function used by the HW drivers */
FUNC_ALIAS(csma_ca_radio_send,
	   ieee802154_radio_send, int);

FUNC_ALIAS(csma_ca_radio_handle_ack,
	   ieee802154_radio_handle_ack, enum net_verdict);
