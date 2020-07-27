/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_pkt.h>

#include <net/ppp.h>

#include "net_private.h"
#include "ipv6.h"

#include "ppp_internal.h"

static enum net_verdict ipv6cp_handle(struct ppp_context *ctx,
				    struct net_if *iface,
				    struct net_pkt *pkt)
{
	return ppp_fsm_input(&ctx->ipv6cp.fsm, PPP_IPV6CP, pkt);
}

/* Length is (10): code + id + interface identifier length */
#define INTERFACE_IDENTIFIER_OPTION_LEN (1 + 1 + 8)

static int ipv6cp_add_iid(struct ppp_context *ctx, struct net_pkt *pkt)
{
	uint8_t *iid = ctx->ipv6cp.my_options.iid;
	size_t iid_len = sizeof(ctx->ipv6cp.my_options.iid);
	struct net_linkaddr *linkaddr;

	linkaddr = net_if_get_link_addr(ctx->iface);
	if (linkaddr->len == 8) {
		memcpy(iid, linkaddr->addr, iid_len);
	} else {
		memcpy(iid, linkaddr->addr, 3);
		iid[3] = 0xff;
		iid[4] = 0xfe;
		memcpy(iid + 5, linkaddr->addr + 3, 3);
	}

	net_pkt_write_u8(pkt, INTERFACE_IDENTIFIER_OPTION_LEN);
	return net_pkt_write(pkt, iid, iid_len);
}

static int ipv6cp_ack_iid(struct ppp_context *ctx, struct net_pkt *pkt,
			  uint8_t oplen)
{
	uint8_t *req_iid = ctx->ipv6cp.my_options.iid;
	uint8_t ack_iid[PPP_INTERFACE_IDENTIFIER_LEN];
	int ret;

	if (oplen != sizeof(ack_iid)) {
		return -EINVAL;
	}

	ret = net_pkt_read(pkt, ack_iid, sizeof(ack_iid));
	if (ret) {
		return ret;
	}

	if (memcmp(ack_iid, req_iid, sizeof(ack_iid)) != 0) {
		return -EINVAL;
	}

	return 0;
}

static const struct ppp_my_option_info ipv6cp_my_options[] = {
	PPP_MY_OPTION(IPV6CP_OPTION_INTERFACE_IDENTIFIER,
		      ipv6cp_add_iid, ipv6cp_ack_iid, NULL),
};

BUILD_ASSERT(ARRAY_SIZE(ipv6cp_my_options) == IPV6CP_NUM_MY_OPTIONS);

static struct net_pkt *ipv6cp_config_info_add(struct ppp_fsm *fsm)
{
	return ppp_my_options_add(fsm, INTERFACE_IDENTIFIER_OPTION_LEN);
}

static int ipv6cp_config_info_req(struct ppp_fsm *fsm,
				struct net_pkt *pkt,
				uint16_t length,
				struct net_pkt *ret_pkt)
{
	int nack_idx = 0, iface_id_option_idx = -1;
	struct ppp_option_pkt options[MAX_IPV6CP_OPTIONS];
	struct ppp_option_pkt nack_options[MAX_IPV6CP_OPTIONS];
	enum ppp_packet_type code;
	int ret;
	int i;

	memset(options, 0, sizeof(options));
	memset(nack_options, 0, sizeof(nack_options));

	ret = ppp_parse_options_array(fsm, pkt, length, options,
				      ARRAY_SIZE(options));
	if (ret < 0) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(options); i++) {
		if (options[i].type.ipv6cp != IPV6CP_OPTION_RESERVED) {
			NET_DBG("[%s/%p] %s option %s (%d) len %d",
				fsm->name, fsm, "Check",
				ppp_option2str(PPP_IPV6CP,
					       options[i].type.ipv6cp),
				options[i].type.ipv6cp, options[i].len);
		}

		switch (options[i].type.ipv6cp) {
		case IPV6CP_OPTION_RESERVED:
			continue;

		case IPV6CP_OPTION_INTERFACE_IDENTIFIER:
			/* Currently we only accept one option (iface id) */
			iface_id_option_idx = i;
			break;

		default:
			nack_options[nack_idx].type.ipv6cp =
				options[i].type.ipv6cp;
			nack_options[nack_idx].len = options[i].len;

			if (options[i].len > 2) {
				memcpy(&nack_options[nack_idx].value,
				       &options[i].value,
				       sizeof(nack_options[nack_idx].value));
			}

			nack_idx++;
			break;
		}
	}

	if (nack_idx > 0) {
		code = PPP_CONFIGURE_REJ;

		/* Fill ret_pkt with options that are not accepted */
		for (i = 0; i < MIN(nack_idx, ARRAY_SIZE(nack_options)); i++) {
			net_pkt_write_u8(ret_pkt, nack_options[i].type.ipv6cp);
			net_pkt_write_u8(ret_pkt, nack_options[i].len);

			/* If there is some data, copy it to result buf */
			if (nack_options[i].value.pos) {
				net_pkt_cursor_restore(pkt,
						       &nack_options[i].value);
				net_pkt_copy(ret_pkt, pkt,
					     nack_options[i].len - 1 - 1);
			}
		}
	} else {
		uint8_t iface_id[PPP_INTERFACE_IDENTIFIER_LEN];
		struct ppp_context *ctx;
		int ret;

		ctx = CONTAINER_OF(fsm, struct ppp_context, ipv6cp.fsm);

		if (iface_id_option_idx < 0) {
			/* Interface id option was not present */
			return -EINVAL;
		}

		code = PPP_CONFIGURE_ACK;

		net_pkt_cursor_restore(pkt,
				       &options[iface_id_option_idx].value);

		ret = net_pkt_read(pkt, iface_id, sizeof(iface_id));
		if (ret < 0) {
			/* Should not happen, is the pkt corrupt? */
			return -EMSGSIZE;
		}

		memcpy(ctx->ipv6cp.peer_options.iid, iface_id,
		       sizeof(iface_id));

		if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
			uint8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

			net_sprint_ll_addr_buf(iface_id, sizeof(iface_id),
					       iid_str, sizeof(iid_str));

			NET_DBG("[%s/%p] Received %siid %s",
				fsm->name, fsm, "peer ", log_strdup(iid_str));
		}

		/* TODO: check whether iid is empty and create one if so */

		net_pkt_write_u8(ret_pkt, IPV6CP_OPTION_INTERFACE_IDENTIFIER);
		net_pkt_write_u8(ret_pkt, INTERFACE_IDENTIFIER_OPTION_LEN);

		net_pkt_write(ret_pkt, iface_id, sizeof(iface_id));
	}

	return code;
}

static int ipv6cp_config_info_ack(struct ppp_fsm *fsm,
				  struct net_pkt *pkt,
				  uint16_t length)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);
	uint8_t *iid = ctx->ipv6cp.my_options.iid;
	size_t iid_len = sizeof(ctx->ipv6cp.my_options.iid);
	int ret;

	ret = ppp_my_options_parse_conf_ack(fsm, pkt, length);
	if (ret) {
		return -EINVAL;
	}

	if (!ppp_my_option_is_acked(fsm, IPV6CP_OPTION_INTERFACE_IDENTIFIER)) {
		NET_ERR("IID was not acked");
		return -EINVAL;
	}

	if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
		uint8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

		net_sprint_ll_addr_buf(iid, iid_len,
				       iid_str, sizeof(iid_str));

		NET_DBG("[%s/%p] Received %siid %s",
			fsm->name, fsm, "", log_strdup(iid_str));
	}

	return 0;
}

static void ipv6cp_lower_down(struct ppp_context *ctx)
{
	ppp_fsm_lower_down(&ctx->ipv6cp.fsm);
}

static void ipv6cp_lower_up(struct ppp_context *ctx)
{
	ppp_fsm_lower_up(&ctx->ipv6cp.fsm);
}

static void ipv6cp_open(struct ppp_context *ctx)
{
	ppp_fsm_open(&ctx->ipv6cp.fsm);
}

static void ipv6cp_close(struct ppp_context *ctx, const uint8_t *reason)
{
	ppp_fsm_close(&ctx->ipv6cp.fsm, reason);
}

static void setup_iid_address(uint8_t *iid, struct in6_addr *addr)
{
	addr->s6_addr[0] = 0xfe;
	addr->s6_addr[1] = 0x80;
	UNALIGNED_PUT(0, &addr->s6_addr16[1]);
	UNALIGNED_PUT(0, &addr->s6_addr32[1]);
	memcpy(&addr->s6_addr[8], iid, PPP_INTERFACE_IDENTIFIER_LEN);

	/* TODO: should we toggle local/global bit */
	/* addr->s6_addr[8] ^= 0x02; */
}

static void add_iid_address(struct net_if *iface, uint8_t *iid)
{
	struct net_if_addr *ifaddr;
	struct in6_addr addr;

	setup_iid_address(iid, &addr);

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s address to interface %p",
			log_strdup(net_sprint_ipv6_addr(&addr)), iface);
	} else {
		/* As DAD is disabled, we need to mark the address
		 * as a preferred one.
		 */
		ifaddr->addr_state = NET_ADDR_PREFERRED;
	}
}

static void ipv6cp_up(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);
	struct net_nbr *nbr;
	struct in6_addr peer_addr;
	struct net_linkaddr peer_lladdr;

	if (ctx->is_ipv6cp_up) {
		return;
	}

	ppp_network_up(ctx, PPP_IPV6);

	ctx->is_ipv6cp_up = true;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	add_iid_address(ctx->iface, ctx->ipv6cp.my_options.iid);

	/* Add peer to neighbor table */
	setup_iid_address(ctx->ipv6cp.peer_options.iid, &peer_addr);

	peer_lladdr.addr = ctx->ipv6cp.peer_options.iid;
	peer_lladdr.len = sizeof(ctx->ipv6cp.peer_options.iid);

	/* TODO: What should be the type? */
	peer_lladdr.type = NET_LINK_DUMMY;

	nbr = net_ipv6_nbr_add(ctx->iface, &peer_addr, &peer_lladdr,
			       false, NET_IPV6_NBR_STATE_STATIC);
	if (!nbr) {
		NET_ERR("[%s/%p] Cannot add peer %s to nbr table",
			fsm->name, fsm,
			log_strdup(net_sprint_addr(AF_INET6,
						   (const void *)&peer_addr)));
	} else {
		if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
			uint8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];
			char dst[INET6_ADDRSTRLEN];
			char *addr_str;

			net_sprint_ll_addr_buf(peer_lladdr.addr,
					       peer_lladdr.len,
					       iid_str, sizeof(iid_str));

			addr_str = net_addr_ntop(AF_INET6, &peer_addr, dst,
						 sizeof(dst));

			NET_DBG("[%s/%p] Peer %s [%s] %s nbr cache",
				fsm->name, fsm, log_strdup(addr_str),
				log_strdup(iid_str), "added to");
		}
	}
}

static void ipv6cp_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);
	struct net_linkaddr peer_lladdr;
	struct in6_addr my_addr;
	struct in6_addr peer_addr;
	int ret;

	if (!ctx->is_ipv6cp_up) {
		return;
	}

	ctx->is_ipv6cp_up = false;

	ppp_network_down(ctx, PPP_IPV6);

	/* Remove my address */
	setup_iid_address(ctx->ipv6cp.my_options.iid, &my_addr);
	net_if_ipv6_addr_rm(ctx->iface, &my_addr);

	/* Remove peer from neighbor table */
	setup_iid_address(ctx->ipv6cp.peer_options.iid, &peer_addr);

	peer_lladdr.addr = ctx->ipv6cp.peer_options.iid;
	peer_lladdr.len = sizeof(ctx->ipv6cp.peer_options.iid);

	/* TODO: What should be the type? */
	peer_lladdr.type = NET_LINK_DUMMY;

	ret = net_ipv6_nbr_rm(ctx->iface, &peer_addr);
	if (!ret) {
		NET_ERR("[%s/%p] Cannot rm peer %s from nbr table",
			fsm->name, fsm,
			log_strdup(net_sprint_addr(AF_INET6,
						   (const void *)&peer_addr)));
	} else {
		if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
			uint8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];
			char dst[INET6_ADDRSTRLEN];
			char *addr_str;

			net_sprint_ll_addr_buf(ctx->ipv6cp.peer_options.iid,
					sizeof(ctx->ipv6cp.peer_options.iid),
					iid_str, sizeof(iid_str));

			addr_str = net_addr_ntop(AF_INET6, &peer_addr, dst,
						 sizeof(dst));

			NET_DBG("[%s/%p] Peer %s [%s] %s nbr cache",
				fsm->name, fsm, log_strdup(addr_str),
				log_strdup(iid_str), "removed from");
		}
	}
}

static void ipv6cp_finished(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);

	if (!ctx->is_ipv6cp_open) {
		return;
	}

	ctx->is_ipv6cp_open = false;

	ppp_network_done(ctx, PPP_IPV6);
}

static void ipv6cp_proto_reject(struct ppp_fsm *fsm)
{
	ppp_fsm_lower_down(fsm);
}

static void ipv6cp_init(struct ppp_context *ctx)
{
	NET_DBG("proto %s (0x%04x) fsm %p", ppp_proto2str(PPP_IPV6CP),
		PPP_IPV6CP, &ctx->ipv6cp.fsm);

	memset(&ctx->ipv6cp.fsm, 0, sizeof(ctx->ipv6cp.fsm));

	ppp_fsm_init(&ctx->ipv6cp.fsm, PPP_IPV6CP);

	ppp_fsm_name_set(&ctx->ipv6cp.fsm, ppp_proto2str(PPP_IPV6CP));

	ctx->ipv6cp.fsm.my_options.info = ipv6cp_my_options;
	ctx->ipv6cp.fsm.my_options.data = ctx->ipv6cp.my_options_data;
	ctx->ipv6cp.fsm.my_options.count = ARRAY_SIZE(ipv6cp_my_options);

	ctx->ipv6cp.fsm.cb.up = ipv6cp_up;
	ctx->ipv6cp.fsm.cb.down = ipv6cp_down;
	ctx->ipv6cp.fsm.cb.finished = ipv6cp_finished;
	ctx->ipv6cp.fsm.cb.proto_reject = ipv6cp_proto_reject;
	ctx->ipv6cp.fsm.cb.config_info_ack = ipv6cp_config_info_ack;
	ctx->ipv6cp.fsm.cb.config_info_rej = ppp_my_options_parse_conf_rej;
	ctx->ipv6cp.fsm.cb.config_info_add = ipv6cp_config_info_add;
	ctx->ipv6cp.fsm.cb.config_info_req = ipv6cp_config_info_req;
}

PPP_PROTOCOL_REGISTER(IPV6CP, PPP_IPV6CP,
		      ipv6cp_init, ipv6cp_handle,
		      ipv6cp_lower_up, ipv6cp_lower_down,
		      ipv6cp_open, ipv6cp_close);
