/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_tag, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag.h>

struct net_if *dsa_tag_recv(struct net_if *iface, struct net_pkt *pkt)
{
	const struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	const struct dsa_switch_context *dsa_switch_ctx = eth_ctx->dsa_switch_ctx;

	/* For no tag type, use NULL recv to let host ethernet driver handle. */
	if (dsa_switch_ctx->dapi->recv == NULL) {
		return iface;
	}

	return dsa_switch_ctx->dapi->recv(iface, pkt);
}

struct net_pkt *dsa_tag_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dsa_switch_context *dsa_switch_ctx = dev->data;

	/* For no tag type, use NULL xmit to let host ethernet driver handle. */
	if (dsa_switch_ctx->dapi->xmit == NULL) {
		/* Here utilize iface field to store origin user port iface.
		 * Host ethernet driver needs to handle then.
		 */
		pkt->iface = iface;
		return pkt;
	}

	return dsa_switch_ctx->dapi->xmit(iface, pkt);
}

void dsa_tag_setup(const struct device *dev_cpu)
{
	const struct dsa_port_config *cfg = dev_cpu->config;
	struct dsa_switch_context *dsa_switch_ctx = dev_cpu->data;
	bool match = false;

	STRUCT_SECTION_FOREACH(dsa_tag_register, tag) {
		if (tag->proto == cfg->tag_proto) {
			dsa_switch_ctx->dapi->recv = tag->recv;
			dsa_switch_ctx->dapi->xmit = tag->xmit;
			match = true;
			break;
		}
	}

	if ((!match) && (cfg->tag_proto != DSA_TAG_PROTO_NOTAG)) {
		LOG_ERR("DSA tag protocol %d not supported", cfg->tag_proto);
	}

	if (dsa_switch_ctx->dapi->connect_tag_protocol != NULL &&
	    dsa_switch_ctx->dapi->connect_tag_protocol(dsa_switch_ctx, cfg->tag_proto) != 0) {
		LOG_ERR("Failed to connect tag protocol");
	}
}
