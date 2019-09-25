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

static bool append_to_buf(struct net_buf *buf, u8_t *data, u8_t data_len)
{
	if (data_len > net_buf_tailroom(buf)) {
		return false;
	}

	/* FIXME: use net_pkt api so that we can handle a case where data might
	 * split to two net_buf's
	 */
	net_buf_add_mem(buf, data, data_len);

	return true;
}

/* Length is (10): code + id + interface identifier length */
#define INTERFACE_IDENTIFIER_OPTION_LEN (1 + 1 + 8)

static struct net_buf *ipv6cp_config_info_add(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);

	/* Currently we support only one option (IP address) */
	u8_t option[INTERFACE_IDENTIFIER_OPTION_LEN];
	u8_t iid[PPP_INTERFACE_IDENTIFIER_LEN];
	struct net_linkaddr *linkaddr;
	struct net_buf *buf;
	bool added;

	linkaddr = net_if_get_link_addr(ctx->iface);
	if (linkaddr->len == 8) {
		memcpy(iid, linkaddr->addr, sizeof(iid));
	} else {
		memcpy(iid, linkaddr->addr, 3);
		iid[3] = 0xff;
		iid[4] = 0xfe;
		memcpy(iid + 5, linkaddr->addr + 3, 3);
	}

	option[0] = IPV6CP_OPTION_INTERFACE_IDENTIFIER;
	option[1] = INTERFACE_IDENTIFIER_OPTION_LEN;
	memcpy(&option[2], iid, sizeof(iid));

	buf = ppp_get_net_buf(NULL, sizeof(option));
	if (!buf) {
		goto out_of_mem;
	}

	added = append_to_buf(buf, option, sizeof(option));
	if (!added) {
		goto out_of_mem;
	}

	return buf;

out_of_mem:
	if (buf) {
		net_buf_unref(buf);
	}

	return NULL;
}

static int ipv6cp_config_info_req(struct ppp_fsm *fsm,
				struct net_pkt *pkt,
				u16_t length,
				struct net_buf **ret_buf)
{
	int nack_idx = 0, iface_id_option_idx = -1;
	struct net_buf *buf = NULL;
	struct ppp_option_pkt options[MAX_IPV6CP_OPTIONS];
	struct ppp_option_pkt nack_options[MAX_IPV6CP_OPTIONS];
	enum ppp_packet_type code;
	enum net_verdict verdict;
	int i;

	memset(options, 0, sizeof(options));
	memset(nack_options, 0, sizeof(nack_options));

	verdict = ppp_parse_options(fsm, pkt, length, options,
				    ARRAY_SIZE(options));
	if (verdict != NET_OK) {
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
		struct net_buf *nack_buf;

		/* Once rejected count logic is in, it will be possible
		 * to set this code to PPP_CONFIGURE_REJ. */
		code = PPP_CONFIGURE_NACK;

		/* Create net_buf containing options that are not accepted */
		for (i = 0; i < MIN(nack_idx, ARRAY_SIZE(nack_options)); i++) {
			bool added;

			nack_buf = ppp_get_net_buf(buf, nack_options[i].len);
			if (!nack_buf) {
				goto out_of_mem;
			}

			if (!buf) {
				buf = nack_buf;
			}

			added = append_to_buf(nack_buf,
					      &nack_options[i].type.ipv6cp, 1);
			if (!added) {
				goto out_of_mem;
			}

			added = append_to_buf(nack_buf, &nack_options[i].len,
					      1);
			if (!added) {
				goto out_of_mem;
			}

			/* If there is some data, copy it to result buf */
			if (nack_options[i].value.pos) {
				added = append_to_buf(nack_buf,
						nack_options[i].value.pos,
						nack_options[i].len - 1 - 1);
				if (!added) {
					goto out_of_mem;
				}
			}

			continue;

		out_of_mem:
			if (nack_buf) {
				net_buf_unref(nack_buf);
			}

			goto bail_out;
		}
	} else {
		u8_t iface_id[PPP_INTERFACE_IDENTIFIER_LEN];
		struct ppp_context *ctx;
		bool added;
		u8_t val;
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
			u8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

			net_sprint_ll_addr_buf(iface_id, sizeof(iface_id),
					       iid_str, sizeof(iid_str));

			NET_DBG("[%s/%p] Received %siid %s",
				fsm->name, fsm, "peer ", log_strdup(iid_str));
		}

		/* TODO: check whether iid is empty and create one if so */

		buf = ppp_get_net_buf(NULL, INTERFACE_IDENTIFIER_OPTION_LEN);
		if (!buf) {
			goto bail_out;
		}

		val = IPV6CP_OPTION_INTERFACE_IDENTIFIER;
		added = append_to_buf(buf, &val, sizeof(val));
		if (!added) {
			goto bail_out;
		}

		val = INTERFACE_IDENTIFIER_OPTION_LEN;
		added = append_to_buf(buf, &val, sizeof(val));
		if (!added) {
			goto bail_out;
		}

		added = append_to_buf(buf, iface_id, sizeof(iface_id));
		if (!added) {
			goto bail_out;
		}
	}

	if (buf) {
		*ret_buf = buf;
	}

	return code;

bail_out:
	if (buf) {
		net_buf_unref(buf);
	}

	return -ENOMEM;
}

static int config_info_ack_rej(struct ppp_context *ctx,
			       struct ppp_fsm *fsm,
			       struct net_pkt *pkt,
			       u16_t length,
			       u8_t code)
{
	struct ppp_option_pkt nack_options[MAX_IPV6CP_OPTIONS];
	u8_t iface_id[PPP_INTERFACE_IDENTIFIER_LEN];
	int i, ret, iface_id_option_idx = -1;
	enum net_verdict verdict;

	memset(nack_options, 0, sizeof(nack_options));

	verdict = ppp_parse_options(fsm, pkt, length, nack_options,
				    ARRAY_SIZE(nack_options));
	if (verdict != NET_OK) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(nack_options); i++) {
		if (nack_options[i].type.ipv6cp != IPV6CP_OPTION_RESERVED) {
			NET_DBG("[%s/%p] %s option %s (%d) len %d",
				fsm->name, fsm, "Check",
				ppp_option2str(PPP_IPV6CP,
					       nack_options[i].type.ipv6cp),
				nack_options[i].type.ipv6cp,
				nack_options[i].len);
		}

		switch (nack_options[i].type.ipv6cp) {
		case IPV6CP_OPTION_RESERVED:
			continue;

		case IPV6CP_OPTION_INTERFACE_IDENTIFIER:
			iface_id_option_idx = i;
			break;

		default:
			continue;
		}
	}

	if (iface_id_option_idx < 0) {
		return -EINVAL;
	}

	net_pkt_cursor_restore(pkt, &nack_options[iface_id_option_idx].value);

	ret = net_pkt_read(pkt, iface_id, sizeof(iface_id));
	if (ret < 0) {
		/* Should not happen, is the pkt corrupt? */
		return -EMSGSIZE;
	}

	memcpy(ctx->ipv6cp.peer_accepted.iid, iface_id, sizeof(iface_id));

	if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
		u8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

		net_sprint_ll_addr_buf(iface_id, sizeof(iface_id),
				       iid_str, sizeof(iid_str));

		NET_DBG("[%s/%p] Received %siid %s",
			fsm->name, fsm, "", log_strdup(iid_str));
	}

	return 0;
}

static int ipv6cp_config_info_rej(struct ppp_fsm *fsm,
				struct net_pkt *pkt,
				u16_t length)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);

	return config_info_ack_rej(ctx, fsm, pkt, length, PPP_CONFIGURE_REJ);
}

static int ipv6cp_config_info_ack(struct ppp_fsm *fsm,
				  struct net_pkt *pkt,
				  u16_t length)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipv6cp.fsm);

	return config_info_ack_rej(ctx, fsm, pkt, length, PPP_CONFIGURE_ACK);
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

static void ipv6cp_close(struct ppp_context *ctx, const u8_t *reason)
{
	ppp_fsm_close(&ctx->ipv6cp.fsm, reason);
}

static void setup_iid_address(u8_t *iid, struct in6_addr *addr)
{
	addr->s6_addr[0] = 0xfe;
	addr->s6_addr[1] = 0x80;
	UNALIGNED_PUT(0, &addr->s6_addr16[1]);
	UNALIGNED_PUT(0, &addr->s6_addr32[1]);
	memcpy(&addr->s6_addr[8], iid, PPP_INTERFACE_IDENTIFIER_LEN);

	/* TODO: should we toggle local/global bit */
	/* addr->s6_addr[8] ^= 0x02; */
}

static void add_iid_address(struct net_if *iface, u8_t *iid)
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

	ctx->is_network_up = true;
	ctx->is_ipv6cp_up = true;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	add_iid_address(ctx->iface, ctx->ipv6cp.peer_accepted.iid);

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
			u8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];
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
	struct in6_addr peer_addr;
	int ret;

	if (!ctx->is_network_up) {
		return;
	}

	ctx->is_network_up = false;

	ppp_network_down(ctx, PPP_IPV6);

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
			u8_t iid_str[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];
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

	ctx->ipv6cp.fsm.cb.up = ipv6cp_up;
	ctx->ipv6cp.fsm.cb.down = ipv6cp_down;
	ctx->ipv6cp.fsm.cb.finished = ipv6cp_finished;
	ctx->ipv6cp.fsm.cb.proto_reject = ipv6cp_proto_reject;
	ctx->ipv6cp.fsm.cb.config_info_ack = ipv6cp_config_info_ack;
	ctx->ipv6cp.fsm.cb.config_info_add = ipv6cp_config_info_add;
	ctx->ipv6cp.fsm.cb.config_info_req = ipv6cp_config_info_req;
	ctx->ipv6cp.fsm.cb.config_info_rej = ipv6cp_config_info_rej;
}

PPP_PROTOCOL_REGISTER(IPV6CP, PPP_IPV6CP,
		      ipv6cp_init, ipv6cp_handle,
		      ipv6cp_lower_up, ipv6cp_lower_down,
		      ipv6cp_open, ipv6cp_close);
