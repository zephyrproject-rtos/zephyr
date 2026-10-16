/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Runs the Rapid Spanning Tree Protocol of stp/rstp.c on the bridge ports */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge_stp, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/ethernet_bridge_fdb.h>
#include <zephyr/net/ethernet_bridge_stp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/virtual.h>

#include "bridge_internal.h"
#include "stp/rstp_internal.h"

/* Bridge Group Address (IEEE Std 802.1Q-2014 Table 8-1) */
static const uint8_t bpdu_dst[NET_ETH_ADDR_LEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};

/* LLC header of a BPDU: DSAP and SSAP 0x42, UI (IEEE Std 802.1Q-2014 8.13.4) */
#define LLC_LEN 3U
static const uint8_t bpdu_llc[LLC_LEN] = {0x42, 0x42, 0x03};

/* The largest value of an IEEE 802.3 Length field */
#define ETH_MAX_LENGTH_FIELD 1500U

#define NUM_PORTS NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT

struct stp_port {
	struct eth_rstp_port rstp;
	struct net_if *iface;
	enum eth_bridge_stp_p2p p2p;
	/* 0 when the cost is derived from the link speed */
	uint32_t admin_path_cost;
};

struct stp_bridge {
	struct eth_rstp_bridge rstp;
	struct eth_rstp_port *ports[NUM_PORTS];
	struct stp_port port[NUM_PORTS];
	struct eth_bridge_iface_context *ctx;
	struct k_work_delayable tick;
	bool enabled;
};

static struct stp_bridge stp_bridges[CONFIG_NET_ETHERNET_BRIDGE_COUNT];

static struct eth_bridge_iface_context *bridge_ctx(struct net_if *br)
{
	if (br == NULL || net_if_l2(br) != &NET_L2_GET_NAME(VIRTUAL) ||
	    (net_virtual_get_iface_capabilities(br) & VIRTUAL_INTERFACE_BRIDGE) == 0) {
		return NULL;
	}

	return net_if_get_device(br)->data;
}

static struct stp_port *stp_port_of(struct eth_rstp_port *port)
{
	return CONTAINER_OF(port, struct stp_port, rstp);
}

static size_t port_index(struct stp_bridge *stp, struct stp_port *sp)
{
	return (size_t)(sp - stp->port);
}

static void tx_bpdu(struct eth_rstp_port *port, const struct eth_rstp_bpdu *bpdu)
{
	struct stp_port *sp = stp_port_of(port);
	uint8_t buf[ETH_RSTP_BPDU_MAX_LEN];
	struct net_pkt *pkt;
	size_t len;

	len = eth_rstp_bpdu_encode(bpdu, buf, sizeof(buf));
	if (len == 0U) {
		return;
	}

	pkt = net_pkt_alloc_with_buffer(sp->iface, LLC_LEN + len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		NET_DBG("No buffer for a BPDU on iface %d", net_if_get_by_iface(sp->iface));
		return;
	}

	/* A value up to 1500 is written as the IEEE 802.3 Length field */
	net_pkt_set_ll_proto_type(pkt, (uint16_t)(LLC_LEN + len));

	if (net_pkt_write(pkt, bpdu_llc, LLC_LEN) < 0 || net_pkt_write(pkt, buf, len) < 0) {
		net_pkt_unref(pkt);
		return;
	}

	/* BPDUs carry the address of the transmitting port (8.13.8) */
	(void)net_linkaddr_copy(net_pkt_lladdr_src(pkt), net_if_get_link_addr(sp->iface));
	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt), bpdu_dst, sizeof(bpdu_dst));

	if (net_if_try_send_data(sp->iface, pkt, K_NO_WAIT) == NET_DROP) {
		net_pkt_unref(pkt);
	}
}

static void set_port_state(struct eth_rstp_port *port, bool learning, bool forwarding)
{
	struct stp_port *sp = stp_port_of(port);
	struct stp_bridge *stp = port->user_data;
	enum eth_bridge_port_state state;

	if (forwarding) {
		state = ETH_BRIDGE_PORT_STATE_FORWARDING;
	} else if (learning) {
		state = ETH_BRIDGE_PORT_STATE_LEARNING;
	} else {
		state = ETH_BRIDGE_PORT_STATE_DISCARDING;
	}

	(void)atomic_set(&stp->ctx->ports[port_index(stp, sp)].state, state);

	NET_DBG("iface %d %s", net_if_get_by_iface(sp->iface),
		forwarding ? "forwarding" : (learning ? "learning" : "discarding"));
}

static void flush_fdb(struct eth_rstp_port *port)
{
	struct stp_bridge *stp = port->user_data;

	eth_bridge_fdb_flush(stp->ctx->iface, stp_port_of(port)->iface, true);
}

static const struct eth_rstp_ops stp_ops = {
	.tx_bpdu = tx_bpdu,
	.set_port_state = set_port_state,
	.flush_fdb = flush_fdb,
};

static uint32_t speed_to_cost(uint32_t mbps)
{
	/* Table 13-4: 20 000 000 000 / (Link Speed in Kb/s) */
	return (uint32_t)(20000000ULL / mbps);
}

/* Port Path Cost from the link speed, and point-to-point from the duplex.
 * Without a PHY, the fastest speed the MAC supports is used, and the link
 * is taken to be full duplex as switched Ethernet is.
 */
static void link_params(struct net_if *iface, uint32_t *cost, bool *full_duplex)
{
	const struct device *phy = net_eth_get_phy(iface);
	enum ethernet_hw_caps caps;
	struct phy_link_state state;

	*cost = ETH_RSTP_DEFAULT_PATH_COST;
	*full_duplex = true;

	if (phy != NULL && phy_get_link_state(phy, &state) == 0 && state.is_up) {
		*full_duplex = PHY_LINK_IS_FULL_DUPLEX(state.speed) != 0;

		if (PHY_LINK_IS_SPEED_1000M(state.speed) != 0) {
			*cost = speed_to_cost(1000U);
		} else if (PHY_LINK_IS_SPEED_100M(state.speed) != 0) {
			*cost = speed_to_cost(100U);
		} else if (PHY_LINK_IS_SPEED_10M(state.speed) != 0) {
			*cost = speed_to_cost(10U);
		} else if ((state.speed & LINK_FULL_2500BASE) != 0) {
			*cost = speed_to_cost(2500U);
		} else if ((state.speed & LINK_FULL_5000BASE) != 0) {
			*cost = speed_to_cost(5000U);
		}

		return;
	}

	caps = net_eth_get_hw_capabilities(iface);

	if ((caps & ETHERNET_LINK_5000BASE) != 0) {
		*cost = speed_to_cost(5000U);
	} else if ((caps & ETHERNET_LINK_2500BASE) != 0) {
		*cost = speed_to_cost(2500U);
	} else if ((caps & ETHERNET_LINK_1000BASE) != 0) {
		*cost = speed_to_cost(1000U);
	} else if ((caps & ETHERNET_LINK_100BASE) != 0) {
		*cost = speed_to_cost(100U);
	} else if ((caps & ETHERNET_LINK_10BASE) != 0) {
		*cost = speed_to_cost(10U);
	}
}

/* Bring the protocol's view of a port up to date with the interface */
static void refresh_port(struct stp_port *sp)
{
	struct eth_rstp_port *port = &sp->rstp;
	bool full_duplex;
	uint32_t cost;
	bool p2p;
	bool up;

	if (sp->iface == NULL) {
		return;
	}

	up = net_if_is_up(sp->iface);
	link_params(sp->iface, &cost, &full_duplex);

	if (sp->admin_path_cost != 0U) {
		cost = sp->admin_path_cost;
	}

	if (port->port_path_cost != cost) {
		(void)eth_rstp_port_set_path_cost(port, cost);
	}

	p2p = (sp->p2p == ETH_BRIDGE_STP_P2P_AUTO) ? full_duplex
						   : (sp->p2p == ETH_BRIDGE_STP_P2P_ON);
	if (port->oper_p2p != p2p) {
		eth_rstp_port_set_p2p(port, p2p);
	}

	if (port->port_enabled != up) {
		eth_rstp_port_set_enabled(port, up);
	}
}

static void tick_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct stp_bridge *stp = CONTAINER_OF(dwork, struct stp_bridge, tick);

	k_mutex_lock(&stp->ctx->lock, K_FOREVER);

	if (stp->enabled) {
		ARRAY_FOR_EACH(stp->port, i) {
			refresh_port(&stp->port[i]);
		}

		eth_rstp_tick(&stp->rstp);
		(void)k_work_reschedule(&stp->tick, K_SECONDS(1));
	}

	k_mutex_unlock(&stp->ctx->lock);
}

static struct stp_bridge *stp_of(struct eth_bridge_iface_context *ctx)
{
	__ASSERT_NO_MSG(ctx->id >= 0 && ctx->id < CONFIG_NET_ETHERNET_BRIDGE_COUNT);

	return &stp_bridges[ctx->id];
}

void eth_bridge_stp_init(struct eth_bridge_iface_context *ctx)
{
	struct stp_bridge *stp = stp_of(ctx);

	stp->ctx = ctx;
	eth_rstp_bridge_init(&stp->rstp, &stp_ops, stp->ports, ARRAY_SIZE(stp->ports),
			     net_if_get_link_addr(ctx->iface)->addr);
	k_work_init_delayable(&stp->tick, tick_handler);
}

bool eth_bridge_stp_is_running(struct eth_bridge_iface_context *ctx)
{
	return stp_of(ctx)->enabled;
}

void eth_bridge_stp_port_added(struct eth_bridge_iface_context *ctx, size_t index)
{
	struct stp_bridge *stp = stp_of(ctx);
	struct stp_port *sp = &stp->port[index];

	eth_rstp_port_init(&sp->rstp, &stp->rstp, (uint16_t)(index + 1U));
	sp->rstp.user_data = stp;
	sp->iface = ctx->ports[index].iface;
	sp->p2p = ETH_BRIDGE_STP_P2P_AUTO;
	sp->admin_path_cost = 0U;
	stp->ports[index] = &sp->rstp;

	if (stp->enabled) {
		eth_rstp_port_begin(&sp->rstp);
		refresh_port(sp);
	}
}

void eth_bridge_stp_port_removed(struct eth_bridge_iface_context *ctx, size_t index)
{
	struct stp_bridge *stp = stp_of(ctx);
	struct stp_port *sp = &stp->port[index];

	if (stp->enabled) {
		eth_rstp_port_set_enabled(&sp->rstp, false);
	}

	stp->ports[index] = NULL;
	sp->iface = NULL;
}

bool eth_bridge_stp_input(struct net_if *bridge, struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	uint8_t buf[LLC_LEN + ETH_RSTP_BPDU_MAX_LEN];
	struct eth_bridge_iface_context *ctx;
	struct eth_rstp_bpdu bpdu;
	struct stp_bridge *stp;
	size_t len;

	if (memcmp(hdr->dst.addr, bpdu_dst, sizeof(bpdu_dst)) != 0 ||
	    net_ntohs(hdr->type) > ETH_MAX_LENGTH_FIELD) {
		return false;
	}

	ctx = net_if_get_device(bridge)->data;
	stp = stp_of(ctx);
	if (!stp->enabled) {
		return false;
	}

	/* The frame is a BPDU or another frame for the Bridge Group Address,
	 * which is never forwarded (8.6.3): either way it ends here.
	 */
	len = MIN(net_ntohs(hdr->type), net_pkt_get_len(pkt) - sizeof(*hdr));
	len = MIN(len, sizeof(buf));

	net_pkt_cursor_init(pkt);
	if (net_pkt_skip(pkt, sizeof(*hdr)) < 0 || net_pkt_read(pkt, buf, len) < 0) {
		return true;
	}

	if (len < LLC_LEN || memcmp(buf, bpdu_llc, LLC_LEN) != 0 ||
	    eth_rstp_bpdu_decode(&buf[LLC_LEN], len - LLC_LEN, &bpdu) < 0) {
		NET_DBG("DROP: not a valid BPDU on iface %d", net_if_get_by_iface(iface));
		return true;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (stp->enabled && stp->ports[eth_ctx->bridge_port] != NULL) {
		eth_rstp_rx(stp->ports[eth_ctx->bridge_port], &bpdu);
	}

	k_mutex_unlock(&ctx->lock);

	return true;
}

int eth_bridge_stp_enable(struct net_if *br, bool enable)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);
	struct stp_bridge *stp;

	if (ctx == NULL) {
		return -EINVAL;
	}

	stp = stp_of(ctx);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (stp->enabled == enable) {
		k_mutex_unlock(&ctx->lock);
		return -EALREADY;
	}

	stp->enabled = enable;

	if (enable) {
		eth_rstp_set_bridge_address(&stp->rstp, net_if_get_link_addr(br)->addr);
		eth_rstp_start(&stp->rstp);

		ARRAY_FOR_EACH(stp->port, i) {
			refresh_port(&stp->port[i]);
		}

		(void)k_work_reschedule(&stp->tick, K_SECONDS(1));
	} else {
		eth_rstp_stop(&stp->rstp);

		ARRAY_FOR_EACH(ctx->ports, i) {
			if (ctx->ports[i].iface != NULL) {
				(void)atomic_set(&ctx->ports[i].state,
						 ETH_BRIDGE_PORT_STATE_FORWARDING);
			}
		}

		eth_bridge_fdb_flush(br, NULL, true);
	}

	k_mutex_unlock(&ctx->lock);

	/* The tick checks enabled under the lock, so it cannot run again */
	if (!enable) {
		(void)k_work_cancel_delayable(&stp->tick);
	}

	NET_DBG("Spanning tree %s on bridge %d", enable ? "started" : "stopped",
		net_if_get_by_iface(br));

	return 0;
}

static enum eth_bridge_stp_role role_of(enum eth_rstp_role role)
{
	switch (role) {
	case ETH_RSTP_ROLE_ROOT:
		return ETH_BRIDGE_STP_ROLE_ROOT;
	case ETH_RSTP_ROLE_DESIGNATED:
		return ETH_BRIDGE_STP_ROLE_DESIGNATED;
	case ETH_RSTP_ROLE_ALTERNATE:
		return ETH_BRIDGE_STP_ROLE_ALTERNATE;
	case ETH_RSTP_ROLE_BACKUP:
		return ETH_BRIDGE_STP_ROLE_BACKUP;
	default:
		return ETH_BRIDGE_STP_ROLE_DISABLED;
	}
}

int eth_bridge_stp_get_status(struct net_if *br, struct eth_bridge_stp_status *status)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);
	struct stp_bridge *stp;

	if (ctx == NULL || status == NULL) {
		return -EINVAL;
	}

	stp = stp_of(ctx);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	*status = (struct eth_bridge_stp_status){
		.enabled = stp->enabled,
		.version = (enum eth_bridge_stp_version)stp->rstp.force_version,
		.bridge_id = stp->rstp.bridge_id,
		.root_id = stp->rstp.root_prio.root_id,
		.root_path_cost = stp->rstp.root_prio.root_path_cost,
		.max_age = (uint8_t)stp->rstp.root_times.max_age,
		.forward_delay = (uint8_t)stp->rstp.root_times.forward_delay,
		.bridge_max_age = (uint8_t)stp->rstp.bridge_times.max_age,
		.bridge_forward_delay = (uint8_t)stp->rstp.bridge_times.forward_delay,
		.tx_hold_count = stp->rstp.tx_hold_count,
	};

	if (!stp->enabled) {
		status->root_id = stp->rstp.bridge_id;
		status->root_path_cost = 0U;
	}

	ARRAY_FOR_EACH(stp->port, i) {
		if (stp->enabled && stp->ports[i] != NULL &&
		    stp->port[i].rstp.port_id == stp->rstp.root_port_id &&
		    stp->rstp.root_port_id != 0U) {
			status->root_port = stp->port[i].iface;
		}
	}

	k_mutex_unlock(&ctx->lock);

	return 0;
}

/* Find the port of iface on br and lock the bridge */
static int lock_port(struct net_if *br, struct net_if *iface, struct stp_bridge **stp,
		     struct stp_port **sp)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);

	if (ctx == NULL) {
		return -EINVAL;
	}

	*stp = stp_of(ctx);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ARRAY_FOR_EACH((*stp)->port, i) {
		if (iface != NULL && (*stp)->port[i].iface == iface) {
			*sp = &(*stp)->port[i];
			return 0;
		}
	}

	k_mutex_unlock(&ctx->lock);

	return -ENOENT;
}

static void unlock_port(struct stp_bridge *stp)
{
	k_mutex_unlock(&stp->ctx->lock);
}

int eth_bridge_stp_port_get_status(struct net_if *br, struct net_if *iface,
				   struct eth_bridge_stp_port_status *status)
{
	struct eth_rstp_port *port;
	struct stp_bridge *stp;
	struct stp_port *sp;
	int ret;

	if (status == NULL) {
		return -EINVAL;
	}

	ret = lock_port(br, iface, &stp, &sp);
	if (ret < 0) {
		return ret;
	}

	port = &sp->rstp;

	*status = (struct eth_bridge_stp_port_status){
		.role = stp->enabled ? role_of(port->role) : ETH_BRIDGE_STP_ROLE_DISABLED,
		.learning = !stp->enabled || port->learning,
		.forwarding = !stp->enabled || port->forwarding,
		.oper_edge = port->oper_edge,
		.admin_edge = port->admin_edge,
		.auto_edge = port->auto_edge,
		.oper_p2p = port->oper_p2p,
		.p2p = sp->p2p,
		.path_cost_auto = sp->admin_path_cost == 0U,
		.send_rstp = port->send_rstp,
		.port_id = port->port_id,
		.path_cost = port->port_path_cost,
		.designated_root = port->port_prio.root_id,
		.designated_bridge = port->port_prio.designated_bridge_id,
		.designated_port = port->port_prio.designated_port_id,
		.designated_cost = port->port_prio.root_path_cost,
	};

	unlock_port(stp);

	return 0;
}

int eth_bridge_stp_set_bridge_priority(struct net_if *br, uint16_t priority)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ret = eth_rstp_set_bridge_priority(&stp_of(ctx)->rstp, priority);
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int eth_bridge_stp_set_times(struct net_if *br, uint8_t max_age, uint8_t forward_delay)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ret = eth_rstp_set_times(&stp_of(ctx)->rstp, max_age, forward_delay);
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int eth_bridge_stp_set_tx_hold_count(struct net_if *br, uint8_t count)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ret = eth_rstp_set_tx_hold_count(&stp_of(ctx)->rstp, count);
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int eth_bridge_stp_set_version(struct net_if *br, enum eth_bridge_stp_version version)
{
	struct eth_bridge_iface_context *ctx = bridge_ctx(br);
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ret = eth_rstp_set_force_version(&stp_of(ctx)->rstp, (uint8_t)version);
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int eth_bridge_stp_port_set_priority(struct net_if *br, struct net_if *iface, uint8_t priority)
{
	struct stp_bridge *stp;
	struct stp_port *sp;
	int ret;

	ret = lock_port(br, iface, &stp, &sp);
	if (ret < 0) {
		return ret;
	}

	ret = eth_rstp_port_set_priority(&sp->rstp, priority);
	unlock_port(stp);

	return ret;
}

int eth_bridge_stp_port_set_path_cost(struct net_if *br, struct net_if *iface, uint32_t cost)
{
	struct stp_bridge *stp;
	struct stp_port *sp;
	int ret;

	if (cost > ETH_RSTP_MAX_PATH_COST) {
		return -EINVAL;
	}

	ret = lock_port(br, iface, &stp, &sp);
	if (ret < 0) {
		return ret;
	}

	sp->admin_path_cost = cost;
	refresh_port(sp);
	unlock_port(stp);

	return 0;
}

int eth_bridge_stp_port_set_edge(struct net_if *br, struct net_if *iface, bool admin_edge,
				 bool auto_edge)
{
	struct stp_bridge *stp;
	struct stp_port *sp;
	int ret;

	ret = lock_port(br, iface, &stp, &sp);
	if (ret < 0) {
		return ret;
	}

	eth_rstp_port_set_edge(&sp->rstp, admin_edge, auto_edge);
	unlock_port(stp);

	return 0;
}

int eth_bridge_stp_port_set_p2p(struct net_if *br, struct net_if *iface,
				enum eth_bridge_stp_p2p p2p)
{
	struct stp_bridge *stp;
	struct stp_port *sp;
	int ret;

	if (p2p != ETH_BRIDGE_STP_P2P_AUTO && p2p != ETH_BRIDGE_STP_P2P_ON &&
	    p2p != ETH_BRIDGE_STP_P2P_OFF) {
		return -EINVAL;
	}

	ret = lock_port(br, iface, &stp, &sp);
	if (ret < 0) {
		return ret;
	}

	sp->p2p = p2p;
	refresh_port(sp);
	unlock_port(stp);

	return 0;
}

int eth_bridge_stp_port_mcheck(struct net_if *br, struct net_if *iface)
{
	struct stp_bridge *stp;
	struct stp_port *sp;
	int ret;

	ret = lock_port(br, iface, &stp, &sp);
	if (ret < 0) {
		return ret;
	}

	eth_rstp_port_mcheck(&sp->rstp);
	unlock_port(stp);

	return 0;
}

static void iface_event_handler(uint64_t mgmt_event, struct net_if *iface, void *info,
				size_t info_length, void *user_data)
{
	struct ethernet_context *eth_ctx;
	struct eth_bridge_iface_context *ctx;
	struct stp_bridge *stp;

	ARG_UNUSED(mgmt_event);
	ARG_UNUSED(info);
	ARG_UNUSED(info_length);
	ARG_UNUSED(user_data);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	eth_ctx = net_if_l2_data(iface);
	ctx = bridge_ctx(eth_ctx->bridge);
	if (ctx == NULL) {
		return;
	}

	stp = stp_of(ctx);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (stp->enabled && stp->port[eth_ctx->bridge_port].iface == iface) {
		refresh_port(&stp->port[eth_ctx->bridge_port]);
	}

	k_mutex_unlock(&ctx->lock);
}

NET_MGMT_REGISTER_EVENT_HANDLER(eth_bridge_stp_events, NET_EVENT_IF_UP | NET_EVENT_IF_DOWN,
				iface_event_handler, NULL);
