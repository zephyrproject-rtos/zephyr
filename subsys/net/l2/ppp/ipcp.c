/*
 * Copyright (c) 2019 Intel Corporation.
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/net/ppp.h>
#include <zephyr/net/dns_resolve.h>

#include "net_private.h"

#include "ppp_internal.h"

static enum net_verdict ipcp_handle(struct ppp_context *ctx,
				    struct net_if *iface,
				    struct net_pkt *pkt)
{
	return ppp_fsm_input(&ctx->ipcp.fsm, PPP_IPCP, pkt);
}

/* Length is (6): code + id + IPv4 address length. RFC 1332 and also
 * DNS in RFC 1877.
 */
#define IP_ADDRESS_OPTION_LEN (1 + 1 + 4)

static int ipcp_add_address(struct ppp_context *ctx, struct net_pkt *pkt,
			    struct in_addr *addr)
{
	net_pkt_write_u8(pkt, 1 + 1 + sizeof(addr->s_addr));
	return net_pkt_write(pkt, &addr->s_addr, sizeof(addr->s_addr));
}

static int ipcp_add_ip_address(struct ppp_context *ctx, struct net_pkt *pkt)
{
	return ipcp_add_address(ctx, pkt, &ctx->ipcp.my_options.address);
}

static int ipcp_add_dns1(struct ppp_context *ctx, struct net_pkt *pkt)
{
	return ipcp_add_address(ctx, pkt, &ctx->ipcp.my_options.dns1_address);
}

static int ipcp_add_dns2(struct ppp_context *ctx, struct net_pkt *pkt)
{
	return ipcp_add_address(ctx, pkt, &ctx->ipcp.my_options.dns2_address);
}

static int ipcp_ack_check_address(struct net_pkt *pkt, size_t oplen,
				  struct in_addr *addr)
{
	struct in_addr ack_addr;
	int ret;

	if (oplen != sizeof(ack_addr)) {
		return -EINVAL;
	}

	ret = net_pkt_read(pkt, &ack_addr, sizeof(ack_addr));
	if (ret) {
		return ret;
	}

	if (memcmp(&ack_addr, addr, sizeof(ack_addr)) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int ipcp_ack_ip_address(struct ppp_context *ctx, struct net_pkt *pkt,
			       uint8_t oplen)
{
	return ipcp_ack_check_address(pkt, oplen,
				      &ctx->ipcp.my_options.address);
}

static int ipcp_ack_dns1(struct ppp_context *ctx, struct net_pkt *pkt,
			 uint8_t oplen)
{
	return ipcp_ack_check_address(pkt, oplen,
				      &ctx->ipcp.my_options.dns1_address);
}

static int ipcp_ack_dns2(struct ppp_context *ctx, struct net_pkt *pkt,
			 uint8_t oplen)
{
	return ipcp_ack_check_address(pkt, oplen,
				      &ctx->ipcp.my_options.dns2_address);
}

static int ipcp_nak_override_address(struct net_pkt *pkt, size_t oplen,
				     struct in_addr *addr)
{
	if (oplen != sizeof(*addr)) {
		return -EINVAL;
	}

	return net_pkt_read(pkt, addr, sizeof(*addr));
}

static int ipcp_nak_ip_address(struct ppp_context *ctx, struct net_pkt *pkt,
			       uint8_t oplen)
{
	return ipcp_nak_override_address(pkt, oplen,
					 &ctx->ipcp.my_options.address);
}

static int ipcp_nak_dns1(struct ppp_context *ctx, struct net_pkt *pkt,
			 uint8_t oplen)
{
	return ipcp_nak_override_address(pkt, oplen,
					 &ctx->ipcp.my_options.dns1_address);
}

static int ipcp_nak_dns2(struct ppp_context *ctx, struct net_pkt *pkt,
			 uint8_t oplen)
{
	return ipcp_nak_override_address(pkt, oplen,
					 &ctx->ipcp.my_options.dns2_address);
}

static const struct ppp_my_option_info ipcp_my_options[] = {
	PPP_MY_OPTION(IPCP_OPTION_IP_ADDRESS, ipcp_add_ip_address,
		      ipcp_ack_ip_address, ipcp_nak_ip_address),
	PPP_MY_OPTION(IPCP_OPTION_DNS1, ipcp_add_dns1,
		      ipcp_ack_dns1, ipcp_nak_dns1),
	PPP_MY_OPTION(IPCP_OPTION_DNS2, ipcp_add_dns2,
		      ipcp_ack_dns2, ipcp_nak_dns2),
};

BUILD_ASSERT(ARRAY_SIZE(ipcp_my_options) == IPCP_NUM_MY_OPTIONS);

static struct net_pkt *ipcp_config_info_add(struct ppp_fsm *fsm)
{
	return ppp_my_options_add(fsm, 3 * IP_ADDRESS_OPTION_LEN);
}

struct ipcp_peer_option_data {
	bool addr_present;
	struct in_addr addr;
};

#if defined(CONFIG_NET_L2_PPP_OPTION_SERVE_DNS)
static int ipcp_dns_address_parse(struct ppp_fsm *fsm, struct net_pkt *pkt,
				  void *user_data)
{
	struct ipcp_peer_option_data *data = user_data;
	int ret;

	ret = net_pkt_read(pkt, &data->addr, sizeof(data->addr));
	if (ret < 0) {
		/* Should not happen, is the pkt corrupt? */
		return -EMSGSIZE;
	}

	/* Request is zeros? Give our dns address in ConfNak */
	if (data->addr.s_addr == INADDR_ANY) {
		NET_DBG("[IPCP] zeroes rcvd as %s addr, sending NAK with our %s addr",
			"DNS", "DNS");
		return -EINVAL;
	}

	data->addr_present = true;

	return 0;
}
#endif

static int ipcp_ip_address_parse(struct ppp_fsm *fsm, struct net_pkt *pkt,
				 void *user_data)
{
	struct ipcp_peer_option_data *data = user_data;
	int ret;

	ret = net_pkt_read(pkt, &data->addr, sizeof(data->addr));
	if (ret < 0) {
		/* Should not happen, is the pkt corrupt? */
		return -EMSGSIZE;
	}

#if defined(CONFIG_NET_L2_PPP_OPTION_SERVE_IP)
	/* Request is zeros? Give our IP address in ConfNak */
	if (data->addr.s_addr == INADDR_ANY) {
		NET_DBG("[IPCP] zeroes rcvd as %s addr, sending NAK with our %s addr",
			"IP", "IP");
		return -EINVAL;
	}
#endif
	if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
		char dst[INET_ADDRSTRLEN];
		char *addr_str;

		addr_str = net_addr_ntop(AF_INET, &data->addr, dst,
					 sizeof(dst));

		NET_DBG("[IPCP] Received peer address %s",
			addr_str);
	}

	data->addr_present = true;

	return 0;
}

#if defined(CONFIG_NET_L2_PPP_OPTION_SERVE_IP)
static int ipcp_server_nak_ip_address(struct ppp_fsm *fsm,
				      struct net_pkt *ret_pkt, void *user_data)
{
	struct ppp_context *ctx =
		CONTAINER_OF(fsm, struct ppp_context, ipcp.fsm);

	(void)net_pkt_write_u8(ret_pkt, IPCP_OPTION_IP_ADDRESS);
	ipcp_add_ip_address(ctx, ret_pkt);

	return 0;
}
#endif

#if defined(CONFIG_NET_L2_PPP_OPTION_SERVE_DNS)
static int ipcp_server_nak_dns1_address(struct ppp_fsm *fsm,
					struct net_pkt *ret_pkt,
					void *user_data)
{
	struct ppp_context *ctx =
		CONTAINER_OF(fsm, struct ppp_context, ipcp.fsm);

	(void)net_pkt_write_u8(ret_pkt, IPCP_OPTION_DNS1);
	ipcp_add_dns1(ctx, ret_pkt);

	return 0;
}

static int ipcp_server_nak_dns2_address(struct ppp_fsm *fsm,
					struct net_pkt *ret_pkt,
					void *user_data)
{
	struct ppp_context *ctx =
		CONTAINER_OF(fsm, struct ppp_context, ipcp.fsm);

	(void)net_pkt_write_u8(ret_pkt, IPCP_OPTION_DNS2);
	ipcp_add_dns2(ctx, ret_pkt);

	return 0;
}
#endif

static const struct ppp_peer_option_info ipcp_peer_options[] = {
#if defined(CONFIG_NET_L2_PPP_OPTION_SERVE_IP)
	PPP_PEER_OPTION(IPCP_OPTION_IP_ADDRESS, ipcp_ip_address_parse,
			ipcp_server_nak_ip_address),
#else
	PPP_PEER_OPTION(IPCP_OPTION_IP_ADDRESS, ipcp_ip_address_parse, NULL),
#endif
#if defined(CONFIG_NET_L2_PPP_OPTION_SERVE_DNS)
	PPP_PEER_OPTION(IPCP_OPTION_DNS1, ipcp_dns_address_parse,
			ipcp_server_nak_dns1_address),
	PPP_PEER_OPTION(IPCP_OPTION_DNS2, ipcp_dns_address_parse,
			ipcp_server_nak_dns2_address),
#endif
};

static int ipcp_config_info_req(struct ppp_fsm *fsm,
				struct net_pkt *pkt,
				uint16_t length,
				struct net_pkt *ret_pkt)
{
	struct ppp_context *ctx =
		CONTAINER_OF(fsm, struct ppp_context, ipcp.fsm);
	struct ipcp_peer_option_data data = {
		.addr_present = false,
	};
	int ret;

	ret = ppp_config_info_req(fsm, pkt, length, ret_pkt, PPP_IPCP,
				  ipcp_peer_options,
				  ARRAY_SIZE(ipcp_peer_options),
				  &data);
	if (ret != PPP_CONFIGURE_ACK) {
		/* There are some issues with configuration still */
		return ret;
	}

	if (!data.addr_present) {
		NET_DBG("[%s/%p] No %saddress provided",
			fsm->name, fsm, "peer ");
		return PPP_CONFIGURE_ACK;
	}

	/* The address is the remote address, we then need
	 * to figure out what our address should be.
	 *
	 * TODO:
	 *   - check that the IP address can be accepted
	 */

	memcpy(&ctx->ipcp.peer_options.address, &data.addr, sizeof(data.addr));

	return PPP_CONFIGURE_ACK;
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
	int ifindex = net_if_get_by_iface(ctx->iface);
	int interfaces[2] = { ifindex, ifindex };
	int ret;

	if (!dns1.sin_addr.s_addr) {
		return;
	}

	if (!dns2.sin_addr.s_addr) {
		dns_servers[1] = NULL;
	}

	dnsctx = dns_resolve_get_default();
	ret = dns_resolve_reconfigure_with_interfaces(dnsctx, NULL, dns_servers,
						      interfaces);
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
	int ret;

	ret = ppp_my_options_parse_conf_nak(fsm, pkt, length);
	if (ret) {
		return ret;
	}

	if (!ctx->ipcp.my_options.address.s_addr) {
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
		NET_ERR("Could not set IP address %s", addr_str);
		return;
	}

	NET_DBG("PPP up with address %s", addr_str);
	ppp_network_up(ctx, PPP_IP);

	ctx->is_ipcp_up = true;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);
}

static void ipcp_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       ipcp.fsm);

	/* Ensure address is always removed if it exists */
	if (ctx->ipcp.my_options.address.s_addr) {
		(void)net_if_ipv4_addr_rm(
			ctx->iface, &ctx->ipcp.my_options.address);
	}
	memset(&ctx->ipcp.my_options.address, 0,
	       sizeof(ctx->ipcp.my_options.address));
	memset(&ctx->ipcp.my_options.dns1_address, 0,
	       sizeof(ctx->ipcp.my_options.dns1_address));
	memset(&ctx->ipcp.my_options.dns2_address, 0,
	       sizeof(ctx->ipcp.my_options.dns2_address));

	if (!ctx->is_ipcp_up) {
		return;
	}

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

	ctx->ipcp.fsm.my_options.info = ipcp_my_options;
	ctx->ipcp.fsm.my_options.data = ctx->ipcp.my_options_data;
	ctx->ipcp.fsm.my_options.count = ARRAY_SIZE(ipcp_my_options);

	ctx->ipcp.fsm.cb.up = ipcp_up;
	ctx->ipcp.fsm.cb.down = ipcp_down;
	ctx->ipcp.fsm.cb.finished = ipcp_finished;
	ctx->ipcp.fsm.cb.proto_reject = ipcp_proto_reject;
	ctx->ipcp.fsm.cb.config_info_add = ipcp_config_info_add;
	ctx->ipcp.fsm.cb.config_info_req = ipcp_config_info_req;
	ctx->ipcp.fsm.cb.config_info_nack = ipcp_config_info_nack;
	ctx->ipcp.fsm.cb.config_info_rej = ppp_my_options_parse_conf_rej;
}

PPP_PROTOCOL_REGISTER(IPCP, PPP_IPCP,
		      ipcp_init, ipcp_handle,
		      ipcp_lower_up, ipcp_lower_down,
		      ipcp_open, ipcp_close);
