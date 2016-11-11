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

#include <errno.h>

#include <net/net_if.h>
#include <net/ieee802154.h>

static int ieee802154_set_ack(uint32_t mgmt_request, struct net_if *iface,
			      void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	ARG_UNUSED(data);
	ARG_UNUSED(len);

	if (mgmt_request == NET_REQUEST_IEEE802154_SET_ACK) {
		ctx->ack_requested = true;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_UNSET_ACK) {
		ctx->ack_requested = false;
	}

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_ACK,
				  ieee802154_set_ack);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_UNSET_ACK,
				  ieee802154_set_ack);
