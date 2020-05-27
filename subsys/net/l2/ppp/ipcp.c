/*
 * Copyright (c) 2019 Intel Corporation.
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_pkt.h>

#include <net/ppp.h>
#include <net/dns_resolve.h>

#include "net_private.h"

#include "ppp_internal.h"

static enum net_verdict ipcp_handle(struct ppp_context *ctx,
				    struct net_if *iface,
				    struct net_pkt *pkt)
{
	return ppp_fsm_input(&ctx->ipcp.fsm, PPP_IPCP, pkt);
}

static bool append_to_buf(struct net_buf *buf, uint8_t *data, uint8_t data_len)
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

/* Length is (6): code + id + IPv4 address length. RFC 1332 and also
 * DNS in RFC 1877.
 */
#define IP_ADDRESS_OPTION_LEN (1 + 1 + 4)

static struct net_buf *ipcp_config_info_add(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);

	/* Currently we support IP address and DNS servers */
	uint8_t options[3 * IP_ADDRESS_OPTION_LEN];
	const struct in_addr *my_addr;
	struct net_buf *buf;
	bool added;

	my_addr = &ctx->ipcp.my_options.address;

	uint8_t *option = options;
	option[0] = IPCP_OPTION_IP_ADDRESS;
	option[1] = IP_ADDRESS_OPTION_LEN;
	memcpy(&option[2], &my_addr->s_addr, sizeof(my_addr->s_addr));
	option += IP_ADDRESS_OPTION_LEN;

	my_addr = &ctx->ipcp.my_options.dns1_address;
	option[0] = IPCP_OPTION_DNS1;
	option[1] = IP_ADDRESS_OPTION_LEN;
	memcpy(&option[2], &my_addr->s_addr, sizeof(my_addr->s_addr));

	option += IP_ADDRESS_OPTION_LEN;
	my_addr = &ctx->ipcp.my_options.dns2_address;
	option[0] = IPCP_OPTION_DNS2;
	option[1] = IP_ADDRESS_OPTION_LEN;
	memcpy(&option[2], &my_addr->s_addr, sizeof(my_addr->s_addr));

	buf = ppp_get_net_buf(NULL, 0);
	if (!buf) {
		goto out_of_mem;
	}

	added = append_to_buf(buf, options, sizeof(options));
	if (!added) {
		goto out_of_mem;
	}

	NET_DBG("Added IPCP IP Address option %d.%d.%d.%d",
		option[2], option[3], option[4], option[5]);

	return buf;

out_of_mem:
	if (buf) {
		net_buf_unref(buf);
	}

	return NULL;
}

static int ipcp_config_info_req(struct ppp_fsm *fsm,
				struct net_pkt *pkt,
				uint16_t length,
				struct net_buf **ret_buf)
{
	int nack_idx = 0, address_option_idx = -1;
	struct net_buf *buf = NULL;
	struct ppp_option_pkt options[MAX_IPCP_OPTIONS];
	struct ppp_option_pkt nack_options[MAX_IPCP_OPTIONS];
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
		if (options[i].type.ipcp != IPCP_OPTION_RESERVED) {
			NET_DBG("[%s/%p] %s option %s (%d) len %d",
				fsm->name, fsm, "Check",
				ppp_option2str(PPP_IPCP, options[i].type.ipcp),
				options[i].type.ipcp, options[i].len);
		}

		switch (options[i].type.ipcp) {
		case IPCP_OPTION_RESERVED:
			continue;

		case IPCP_OPTION_IP_ADDRESS:
			/* Currently we only accept one option (IP address) */
			address_option_idx = i;
			break;

		default:
			nack_options[nack_idx].type.ipcp =
				options[i].type.ipcp;
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

		code = PPP_CONFIGURE_REJ;

		/* Create net_buf containing options that are not accepted */
		for (i = 0; i < MIN(nack_idx, ARRAY_SIZE(nack_options)); i++) {
			bool added;

			nack_buf = ppp_get_net_buf(buf, nack_options[i].len);
			if (!nack_buf) {
				goto bail_out;
			}

			if (!buf) {
				buf = nack_buf;
			}

			added = append_to_buf(nack_buf,
					      &nack_options[i].type.ipcp, 1);
			if (!added) {
				goto bail_out;
			}

			added = append_to_buf(nack_buf, &nack_options[i].len,
					      1);
			if (!added) {
				goto bail_out;
			}

			/* If there is some data, copy it to result buf */
			if (nack_options[i].value.pos) {
				added = append_to_buf(nack_buf,
						nack_options[i].value.pos,
						nack_options[i].len - 1 - 1);
				if (!added) {
					goto bail_out;
				}
			}
		}
	} else {
		struct ppp_context *ctx;
		struct in_addr addr;
		int ret;

		ctx = CONTAINER_OF(fsm, struct ppp_context, ipcp.fsm);

		if (address_option_idx < 0) {
			/* The address option was not present, but we
			 * can continue without it.
			 */
			NET_DBG("[%s/%p] No %saddress provided",
				fsm->name, fsm, "peer ");
			return PPP_CONFIGURE_ACK;
		}

		code = PPP_CONFIGURE_ACK;

		net_pkt_cursor_restore(pkt,
				       &options[address_option_idx].value);

		ret = net_pkt_read(pkt, (uint32_t *)&addr, sizeof(addr));
		if (ret < 0) {
			/* Should not happen, is the pkt corrupt? */
			return -EMSGSIZE;
		}

		memcpy(&ctx->ipcp.peer_options.address, &addr, sizeof(addr));

		if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
			char dst[INET_ADDRSTRLEN];
			char *addr_str;

			addr_str = net_addr_ntop(AF_INET, &addr, dst,
						 sizeof(dst));

			NET_DBG("[%s/%p] Received %saddress %s",
				fsm->name, fsm, "peer ", log_strdup(addr_str));
		}

		if (addr.s_addr) {
			bool added;
			uint8_t val;

			/* The address is the remote address, we then need
			 * to figure out what our address should be.
			 *
			 * TODO:
			 *   - check that the IP address can be accepted
			 */

			buf = ppp_get_net_buf(NULL, IP_ADDRESS_OPTION_LEN);
			if (!buf) {
				goto bail_out;
			}

			val = IPCP_OPTION_IP_ADDRESS;
			added = append_to_buf(buf, &val, sizeof(val));
			if (!added) {
				goto bail_out;
			}

			val = IP_ADDRESS_OPTION_LEN;
			added = append_to_buf(buf, &val, sizeof(val));
			if (!added) {
				goto bail_out;
			}

			added = append_to_buf(buf, (uint8_t *)&addr.s_addr,
					      sizeof(addr.s_addr));
			if (!added) {
				goto bail_out;
			}
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

static void ipcp_set_dns_servers(struct ppp_fsm *fsm)
{
#if defined(CONFIG_NET_L2_PPP_OPTION_DNS_USE)
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);

	struct dns_resolve_context *dnsctx;
	struct sockaddr_in dns1 = {
		.sin_family = AF_INET,
		.sin_port = htons(53),
		.sin_addr = ctx->ipcp.my_options.dns1_address
	};
	struct sockaddr_in dns2 = {
		.sin_family = AF_INET,
		.sin_port = htons(53),
		.sin_addr = ctx->ipcp.my_options.dns2_address
	};
	const struct sockaddr *dns_servers[] = {
		(struct sockaddr *) &dns1,
		(struct sockaddr *) &dns2,
		NULL
	};
	int i, ret;

	if (!dns1.sin_addr.s_addr) {
		return;
	}

	if (!dns2.sin_addr.s_addr) {
		dns_servers[1] = NULL;
	}

	dnsctx = dns_resolve_get_default();
	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (!dnsctx->queries[i].cb) {
			continue;
		}

		dns_resolve_cancel(dnsctx, dnsctx->queries[i].id);
	}
	dns_resolve_close(dnsctx);

	ret = dns_resolve_init(dnsctx, NULL, dns_servers);
	if (ret < 0) {
		NET_ERR("Could not set DNS servers");
		return;
	}
#endif
}

static int ipcp_config_info_nack(struct ppp_fsm *fsm,
				 struct net_pkt *pkt,
				 uint16_t length,
				 bool rejected)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);
	struct ppp_option_pkt nack_options[MAX_IPCP_OPTIONS];
	enum net_verdict verdict;
	int i, ret, address_option_idx = -1;
	struct in_addr addr, *dst_addr;

	memset(nack_options, 0, sizeof(nack_options));

	verdict = ppp_parse_options(fsm, pkt, length, nack_options,
				    ARRAY_SIZE(nack_options));
	if (verdict != NET_OK) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(nack_options); i++) {
		if (nack_options[i].type.ipcp != IPCP_OPTION_RESERVED) {
			NET_DBG("[%s/%p] %s option %s (%d) len %d",
				fsm->name, fsm, "Check",
				ppp_option2str(PPP_IPCP,
					       nack_options[i].type.ipcp),
				nack_options[i].type.ipcp,
				nack_options[i].len);
		}

		switch (nack_options[i].type.ipcp) {
		case IPCP_OPTION_RESERVED:
			continue;

		case IPCP_OPTION_IP_ADDRESSES:
			continue;

		case IPCP_OPTION_IP_COMP_PROTO:
			continue;

		case IPCP_OPTION_DNS1:
			dst_addr = &ctx->ipcp.my_options.dns1_address;
			break;

		case IPCP_OPTION_DNS2:
			dst_addr = &ctx->ipcp.my_options.dns2_address;
			break;

		case IPCP_OPTION_IP_ADDRESS:
			dst_addr = &ctx->ipcp.my_options.address;
			address_option_idx = i;
			break;

		default:
			continue;
		}

		net_pkt_cursor_restore(pkt, &nack_options[i].value);

		ret = net_pkt_read(pkt, (uint32_t *)&addr, sizeof(addr));
		if (ret < 0) {
			/* Should not happen, is the pkt corrupt? */
			return -EMSGSIZE;
		}

		memcpy(dst_addr, &addr, sizeof(addr));

		if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
			char dst[INET_ADDRSTRLEN];
			char *addr_str;

			addr_str = net_addr_ntop(AF_INET, &addr, dst,
						 sizeof(dst));

			NET_DBG("[%s/%p] Received %s address %s",
				fsm->name, fsm,
				ppp_option2str(PPP_IPCP,
					       nack_options[i].type.ipcp),
				log_strdup(addr_str));
		}
	}

	if (address_option_idx < 0) {
		return -EINVAL;
	}

	ipcp_set_dns_servers(fsm);

	return 0;
}

static void ipcp_lower_down(struct ppp_context *ctx)
{
	ppp_fsm_lower_down(&ctx->ipcp.fsm);
}

static void ipcp_lower_up(struct ppp_context *ctx)
{
	ppp_fsm_lower_up(&ctx->ipcp.fsm);
}

static void ipcp_open(struct ppp_context *ctx)
{
	ppp_fsm_open(&ctx->ipcp.fsm);
}

static void ipcp_close(struct ppp_context *ctx, const uint8_t *reason)
{
	ppp_fsm_close(&ctx->ipcp.fsm, reason);
}

static void ipcp_up(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);
	struct net_if_addr *addr;
	char dst[INET_ADDRSTRLEN];
	char *addr_str;

	if (ctx->is_ipcp_up) {
		return;
	}

	addr_str = net_addr_ntop(AF_INET, &ctx->ipcp.my_options.address,
				 dst, sizeof(dst));

	addr = net_if_ipv4_addr_add(ctx->iface,
				    &ctx->ipcp.my_options.address,
				    NET_ADDR_MANUAL,
				    0);
	if (addr == NULL) {
		NET_ERR("Could not set IP address %s", log_strdup(addr_str));
		return;
	}

	NET_DBG("PPP up with address %s", log_strdup(addr_str));
	ppp_network_up(ctx, PPP_IP);

	ctx->is_network_up = true;
	ctx->is_ipcp_up = true;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);
}

static void ipcp_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);

	if (!ctx->is_network_up) {
		return;
	}

	ctx->is_network_up = false;
	ctx->is_ipcp_up = false;

	ppp_network_down(ctx, PPP_IP);
}

static void ipcp_finished(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);

	if (!ctx->is_ipcp_open) {
		return;
	}

	ctx->is_ipcp_open = false;

	ppp_network_done(ctx, PPP_IP);
}

static void ipcp_proto_reject(struct ppp_fsm *fsm)
{
	ppp_fsm_lower_down(fsm);
}

static void ipcp_init(struct ppp_context *ctx)
{
	NET_DBG("proto %s (0x%04x) fsm %p", ppp_proto2str(PPP_IPCP), PPP_IPCP,
		&ctx->ipcp.fsm);

	memset(&ctx->ipcp.fsm, 0, sizeof(ctx->ipcp.fsm));

	ppp_fsm_init(&ctx->ipcp.fsm, PPP_IPCP);

	ppp_fsm_name_set(&ctx->ipcp.fsm, ppp_proto2str(PPP_IPCP));

	ctx->ipcp.fsm.cb.up = ipcp_up;
	ctx->ipcp.fsm.cb.down = ipcp_down;
	ctx->ipcp.fsm.cb.finished = ipcp_finished;
	ctx->ipcp.fsm.cb.proto_reject = ipcp_proto_reject;
	ctx->ipcp.fsm.cb.config_info_add = ipcp_config_info_add;
	ctx->ipcp.fsm.cb.config_info_req = ipcp_config_info_req;
	ctx->ipcp.fsm.cb.config_info_nack = ipcp_config_info_nack;
}

PPP_PROTOCOL_REGISTER(IPCP, PPP_IPCP,
		      ipcp_init, ipcp_handle,
		      ipcp_lower_up, ipcp_lower_down,
		      ipcp_open, ipcp_close);
