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

#include <zephyr.h>
#include <tc_util.h>

#include <net/nbuf.h>

/** FAKE ieee802.15.4 driver **/
#include <net/ieee802154_radio.h>

extern struct net_buf *current_buf;
extern struct nano_sem driver_lock;

static int fake_cca(struct device *dev)
{
	return 0;
}

static int fake_set_channel(struct device *dev, uint16_t channel)
{
	TC_PRINT("Channel %u\n", channel);

	return 0;
}

static int fake_set_pan_id(struct device *dev, uint16_t pan_id)
{
	TC_PRINT("PAN id 0x%x\n", pan_id);

	return 0;
}

static int fake_set_short_addr(struct device *dev, uint16_t short_addr)
{
	TC_PRINT("Short address: 0x%x\n", short_addr);

	return 0;
}

static int fake_set_ieee_addr(struct device *dev, const uint8_t *ieee_addr)
{
	TC_PRINT("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
		 ieee_addr[0], ieee_addr[1], ieee_addr[2], ieee_addr[3],
		 ieee_addr[4], ieee_addr[5], ieee_addr[6], ieee_addr[7]);

	return 0;
}

static int fake_set_txpower(struct device *dev, int16_t dbm)
{
	TC_PRINT("TX power %d dbm\n", dbm);

	return 0;
}

static int fake_tx(struct device *dev, struct net_buf *buf)
{
	TC_PRINT("Sending buffer %p - length %u\n",
		 buf, net_buf_frags_len(buf));

	current_buf = net_buf_ref(buf);

	nano_sem_give(&driver_lock);

	return 0;
}

static int fake_start(struct device *dev)
{
	TC_PRINT("FAKE ieee802154 driver started\n");

	return 0;
}

static int fake_stop(struct device *dev)
{
	TC_PRINT("FAKE ieee802154 driver stopped\n");

	return 0;
}

static void fake_iface_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	static uint8_t mac[8] = { 0x00, 0x12, 0x4b, 0x00,
				  0x00, 0x9e, 0xa3, 0xc2 };

	net_if_set_link_addr(iface, mac, 8);

	ctx->pan_id = 0xabcd;
	ctx->channel = 26;
	ctx->sequence = 62;

	TC_PRINT("FAKE ieee802154 iface initialized\n");
}

static int fake_init(struct device *dev)
{
	fake_stop(dev);

	return 0;
}

static struct ieee802154_radio_api fake_radio_api = {
	.iface_api.init	= fake_iface_init,
	.iface_api.send	= ieee802154_radio_send,

	.cca		= fake_cca,
	.set_channel	= fake_set_channel,
	.set_pan_id	= fake_set_pan_id,
	.set_short_addr	= fake_set_short_addr,
	.set_ieee_addr	= fake_set_ieee_addr,
	.set_txpower	= fake_set_txpower,
	.start		= fake_start,
	.stop		= fake_stop,
	.tx		= fake_tx,
};

NET_DEVICE_INIT(fake, "fake_ieee802154",
		fake_init, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 127);
