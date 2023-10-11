/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>

#include <zephyr/net/ppp.h>

#include "net_private.h"

#include "ppp_stats.h"
#include "ppp_internal.h"

static enum net_verdict lcp_handle_ext(struct ppp_fsm *fsm,
				       enum ppp_packet_type code, uint8_t id,
				       struct net_pkt *pkt)
{
	enum net_verdict verdict = NET_DROP;

	switch (code) {
	case PPP_PROTOCOL_REJ:
		NET_DBG("PPP Protocol-Rej");
		return ppp_fsm_recv_protocol_rej(fsm, id, pkt);

	case PPP_ECHO_REQ:
		NET_DBG("PPP Echo-Req");
		return ppp_fsm_recv_echo_req(fsm, id, pkt);

	case PPP_ECHO_REPLY:
		NET_DBG("PPP Echo-Reply");
		return ppp_fsm_recv_echo_reply(fsm, id, pkt);

	case PPP_DISCARD_REQ:
		NET_DBG("PPP Discard-Req");
		return ppp_fsm_recv_discard_req(fsm, id, pkt);

	default:
		break;
	}

	return verdict;
}

static enum net_verdict lcp_handle(struct ppp_context *ctx,
				   struct net_if *iface,
				   struct net_pkt *pkt)
{
	return ppp_fsm_input(&ctx->lcp.fsm, PPP_LCP, pkt);
}

struct lcp_option_data {
	bool auth_proto_present;
	uint16_t auth_proto;
};

static const enum ppp_protocol_type lcp_supported_auth_protos[] = {
#if defined(CONFIG_NET_L2_PPP_PAP)
	PPP_PAP,
#endif
};

static int lcp_auth_proto_parse(struct ppp_fsm *fsm, struct net_pkt *pkt,
				void *user_data)
{
	struct lcp_option_data *data = user_data;
	int ret;
	int i;

	ret = net_pkt_read_be16(pkt, &data->auth_proto);
	if (ret < 0) {
		/* Should not happen, is the pkt corrupt? */
		return -EMSGSIZE;
	}

	NET_DBG("[LCP] Received auth protocol %x (%s)",
		(unsigned int) data->auth_proto,
		ppp_proto2str(data->auth_proto));

	for (i = 0; i < ARRAY_SIZE(lcp_supported_auth_protos); i++) {
		if (data->auth_proto == lcp_supported_auth_protos[i]) {
			data->auth_proto_present = true;
			return 0;
		}
	}

	return -EINVAL;
}

static int lcp_auth_proto_nack(struct ppp_fsm *fsm, struct net_pkt *ret_pkt,
			       void *user_data)
{
	(void)net_pkt_write_u8(ret_pkt, LCP_OPTION_AUTH_PROTO);
	(void)net_pkt_write_u8(ret_pkt, 4);
	return net_pkt_write_be16(ret_pkt, PPP_PAP);
}

static const struct ppp_peer_option_info lcp_peer_options[] = {
	PPP_PEER_OPTION(LCP_OPTION_AUTH_PROTO, lcp_auth_proto_parse,
			lcp_auth_proto_nack),
};

static int lcp_config_info_req(struct ppp_fsm *fsm,
			       struct net_pkt *pkt,
			       uint16_t length,
			       struct net_pkt *ret_pkt)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);
	struct lcp_option_data data = {
		.auth_proto_present = false,
	};
	int ret;

	ret = ppp_config_info_req(fsm, pkt, length, ret_pkt, PPP_LCP,
				  lcp_peer_options,
				  ARRAY_SIZE(lcp_peer_options),
				  &data);
	if (ret != PPP_CONFIGURE_ACK) {
		/* There are some issues with configuration still */
		return ret;
	}

	ctx->lcp.peer_options.auth_proto = data.auth_proto;

	if (data.auth_proto_present) {
		NET_DBG("Authentication protocol negotiated: %x (%s)",
			(unsigned int) data.auth_proto,
			ppp_proto2str(data.auth_proto));
	}

	return PPP_CONFIGURE_ACK;
}

static void lcp_lower_down(struct ppp_context *ctx)
{
	ppp_fsm_lower_down(&ctx->lcp.fsm);
}

static void lcp_lower_up(struct ppp_context *ctx)
{
#if defined(CONFIG_NET_L2_PPP_OPTION_MRU)
	/* Get currently set MTU */
	ctx->lcp.my_options.mru = net_if_get_mtu(ctx->iface);
#endif

	ppp_fsm_lower_up(&ctx->lcp.fsm);
}

static void lcp_open(struct ppp_context *ctx)
{
	ppp_fsm_open(&ctx->lcp.fsm);
}

static void lcp_close(struct ppp_context *ctx, const uint8_t *reason)
{
	if (ctx->phase != PPP_DEAD) {
		ppp_change_phase(ctx, PPP_TERMINATE);
	}

	ppp_fsm_close(&ctx->lcp.fsm, reason);
}

static void lcp_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	memset(&ctx->lcp.peer_options.auth_proto, 0,
	       sizeof(ctx->lcp.peer_options.auth_proto));

	ppp_link_down(ctx);

	if (!net_if_is_carrier_ok(ctx->iface)) {
		return;
	}

	ppp_change_phase(ctx, PPP_ESTABLISH);
}

static void lcp_up(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	/* TODO: Set MRU/MTU of the network interface here */

	ppp_link_established(ctx, fsm);
}

static void lcp_starting(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	ppp_link_needed(ctx);
}

static void lcp_finished(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	ppp_link_terminated(ctx);
}

#if defined(CONFIG_NET_L2_PPP_OPTION_MRU)

#define MRU_OPTION_LEN 4

static int lcp_add_mru(struct ppp_context *ctx, struct net_pkt *pkt)
{
	net_pkt_write_u8(pkt, MRU_OPTION_LEN);
	return net_pkt_write_be16(pkt, ctx->lcp.my_options.mru);
}

static int lcp_ack_mru(struct ppp_context *ctx, struct net_pkt *pkt,
		       uint8_t oplen)
{
	int ret;
	uint16_t mru;

	/* Handle ACK : */
	if (oplen != sizeof(mru)) {
		return -EINVAL;
	}

	ret = net_pkt_read(pkt, &mru, sizeof(mru));
	if (ret) {
		return ret;
	}
	if (mru != ctx->lcp.my_options.mru) {
		/* Didn't acked our MRU: */
		return -EINVAL;
	}

	return 0;
}

static int lcp_nak_mru(struct ppp_context *ctx, struct net_pkt *pkt,
		       uint8_t oplen)
{
	int ret;
	uint16_t mru;

	/* Handle NAK: accept only smaller/equal than ours */
	if (oplen != sizeof(mru)) {
		return -EINVAL;
	}

	ret = net_pkt_read(pkt, &mru, sizeof(mru));
	if (ret) {
		return ret;
	}

	if (mru <= ctx->lcp.my_options.mru) {
		/* OK, reset the MRU also in our side: */
		ctx->lcp.my_options.mru = mru;
	} else {
		return -EINVAL;
	}

	return 0;
}

static const struct ppp_my_option_info lcp_my_options[] = {
	PPP_MY_OPTION(LCP_OPTION_MRU, lcp_add_mru, lcp_ack_mru, lcp_nak_mru),
};
BUILD_ASSERT(ARRAY_SIZE(lcp_my_options) == LCP_NUM_MY_OPTIONS);

static struct net_pkt *lcp_config_info_add(struct ppp_fsm *fsm)
{
	return ppp_my_options_add(fsm, MRU_OPTION_LEN);
}

static int lcp_config_info_nack(struct ppp_fsm *fsm, struct net_pkt *pkt,
				uint16_t length, bool rejected)
{
	struct ppp_context *ctx =
		CONTAINER_OF(fsm, struct ppp_context, lcp.fsm);
	int ret;

	ret = ppp_my_options_parse_conf_nak(fsm, pkt, length);
	if (ret) {
		return ret;
	}

	if (!ctx->lcp.my_options.mru) {
		return -EINVAL;
	}

	return 0;
}
#endif

static void lcp_init(struct ppp_context *ctx)
{
	NET_DBG("proto %s (0x%04x) fsm %p", ppp_proto2str(PPP_LCP), PPP_LCP,
		&ctx->lcp.fsm);

	memset(&ctx->lcp.fsm, 0, sizeof(ctx->lcp.fsm));

	ppp_fsm_init(&ctx->lcp.fsm, PPP_LCP);

	ppp_fsm_name_set(&ctx->lcp.fsm, ppp_proto2str(PPP_LCP));

	ctx->lcp.my_options.mru = net_if_get_mtu(ctx->iface);

#if defined(CONFIG_NET_L2_PPP_OPTION_MRU)
	ctx->lcp.fsm.my_options.info = lcp_my_options;
	ctx->lcp.fsm.my_options.data = ctx->lcp.my_options_data;
	ctx->lcp.fsm.my_options.count = ARRAY_SIZE(lcp_my_options);

	ctx->lcp.fsm.cb.config_info_add = lcp_config_info_add;
	ctx->lcp.fsm.cb.config_info_req = lcp_config_info_req;
	ctx->lcp.fsm.cb.config_info_nack = lcp_config_info_nack;
	ctx->lcp.fsm.cb.config_info_rej = ppp_my_options_parse_conf_rej;
#endif

	ctx->lcp.fsm.cb.up = lcp_up;
	ctx->lcp.fsm.cb.down = lcp_down;
	ctx->lcp.fsm.cb.starting = lcp_starting;
	ctx->lcp.fsm.cb.finished = lcp_finished;
	if (IS_ENABLED(CONFIG_NET_L2_PPP_AUTH_SUPPORT)) {
		ctx->lcp.fsm.cb.config_info_req = lcp_config_info_req;
	}
	ctx->lcp.fsm.cb.proto_extension = lcp_handle_ext;
}

PPP_PROTOCOL_REGISTER(LCP, PPP_LCP,
		      lcp_init, lcp_handle,
		      lcp_lower_up, lcp_lower_down,
		      lcp_open, lcp_close);
