/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/net_mgmt.h>

#include <net/ppp.h>

#include "net_private.h"

#include "ppp_stats.h"
#include "ppp_internal.h"

static enum net_verdict lcp_handle_ext(struct ppp_fsm *fsm,
				       enum ppp_packet_type code, u8_t id,
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

static bool append_to_buf(struct net_buf *buf, u8_t *data, u8_t data_len)
{
	if (data_len > net_buf_tailroom(buf)) {
		return false;
	}

	/* FIXME: use net_pkt api so that we can handle a case where data
	 * might split to two net_buf's
	 */
	net_buf_add_mem(buf, data, data_len);

	return true;
}

static int lcp_config_info_req(struct ppp_fsm *fsm,
			       struct net_pkt *pkt,
			       u16_t length,
			       struct net_buf **buf)
{
	struct ppp_option_pkt options[MAX_LCP_OPTIONS];
	struct ppp_option_pkt nack_options[MAX_LCP_OPTIONS];
	struct net_buf *nack = NULL;
	enum ppp_packet_type code;
	enum net_verdict verdict;
	int i, nack_idx = 0;
	int count_rej = 0, count_nack = 0;

	memset(options, 0, sizeof(options));
	memset(nack_options, 0, sizeof(nack_options));

	verdict = ppp_parse_options(fsm, pkt, length, options,
				    ARRAY_SIZE(options));
	if (verdict != NET_OK) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(options); i++) {
		if (options[i].type.lcp != LCP_OPTION_RESERVED) {
			NET_DBG("[%s/%p] %s option %s (%d) len %d",
				fsm->name, fsm, "Check",
				ppp_option2str(PPP_LCP, options[i].type.lcp),
				options[i].type.lcp, options[i].len);
		}

		switch (options[i].type.lcp) {
		case LCP_OPTION_RESERVED:
			continue;

		case LCP_OPTION_MRU:
			break;

		/* TODO: Check from ctx->lcp.my_options what options to accept
		 */
		case LCP_OPTION_ASYNC_CTRL_CHAR_MAP:
			count_nack++;
			goto ignore_option;

		case LCP_OPTION_AUTH_PROTO:
			count_nack++;
			goto ignore_option;

		case LCP_OPTION_QUALITY_PROTO:
			count_rej++;
			goto ignore_option;

		case LCP_OPTION_MAGIC_NUMBER:
			count_nack++;
			goto ignore_option;

		case LCP_OPTION_PROTO_COMPRESS:
			count_rej++;
			goto ignore_option;

		case LCP_OPTION_ADDR_CTRL_COMPRESS:
			count_rej++;
			goto ignore_option;

		default:
		ignore_option:
			nack_options[nack_idx].type.lcp = options[i].type.lcp;
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

		if (count_rej > 0) {
			code = PPP_CONFIGURE_REJ;
		} else {
			code = PPP_CONFIGURE_NACK;
		}

		/* Create net_buf containing options that are not accepted */
		for (i = 0; i < MIN(nack_idx, ARRAY_SIZE(nack_options)); i++) {
			bool added;

			nack_buf = ppp_get_net_buf(nack, nack_options[i].len);
			if (!nack_buf) {
				goto out_of_mem;
			}

			if (!nack) {
				nack = nack_buf;
			}

			added = append_to_buf(nack_buf,
					      &nack_options[i].type.lcp, 1);
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
			if (nack) {
				net_buf_unref(nack);
			} else {
				if (nack_buf) {
					net_buf_unref(nack_buf);
				}
			}

			return -ENOMEM;
		}
	} else {
		code = PPP_CONFIGURE_ACK;
	}

	if (nack) {
		*buf = nack;
	}

	return code;
}

static void lcp_lower_down(struct ppp_context *ctx)
{
	ppp_fsm_lower_down(&ctx->lcp.fsm);
}

static void lcp_lower_up(struct ppp_context *ctx)
{
	ppp_fsm_lower_up(&ctx->lcp.fsm);
}

static void lcp_open(struct ppp_context *ctx)
{
	ppp_fsm_open(&ctx->lcp.fsm);
}

static void lcp_close(struct ppp_context *ctx, const u8_t *reason)
{
	if (ctx->phase != PPP_DEAD) {
		ppp_change_phase(ctx, PPP_TERMINATE);
	}

	ppp_change_state(&ctx->lcp.fsm, PPP_STOPPED);

	ppp_fsm_close(&ctx->lcp.fsm, reason);
}

static void lcp_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	ppp_link_down(ctx);

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

static void lcp_init(struct ppp_context *ctx)
{
	NET_DBG("proto %s (0x%04x) fsm %p", ppp_proto2str(PPP_LCP), PPP_LCP,
		&ctx->lcp.fsm);

	memset(&ctx->lcp.fsm, 0, sizeof(ctx->lcp.fsm));

	ppp_fsm_init(&ctx->lcp.fsm, PPP_LCP);

	ppp_fsm_name_set(&ctx->lcp.fsm, ppp_proto2str(PPP_LCP));

	ctx->lcp.fsm.cb.up = lcp_up;
	ctx->lcp.fsm.cb.down = lcp_down;
	ctx->lcp.fsm.cb.starting = lcp_starting;
	ctx->lcp.fsm.cb.finished = lcp_finished;
	ctx->lcp.fsm.cb.proto_extension = lcp_handle_ext;
	ctx->lcp.fsm.cb.config_info_req = lcp_config_info_req;

	ctx->lcp.my_options.negotiate_proto_compression = false;
	ctx->lcp.my_options.negotiate_addr_compression = false;
	ctx->lcp.my_options.negotiate_async_map = false;
	ctx->lcp.my_options.negotiate_magic = false;
	ctx->lcp.my_options.negotiate_mru =
		IS_ENABLED(CONFIG_NET_L2_PPP_OPTION_MRU_NEG) ? true : false;
}

PPP_PROTOCOL_REGISTER(LCP, PPP_LCP,
		      lcp_init, lcp_handle,
		      lcp_lower_up, lcp_lower_down,
		      lcp_open, lcp_close);
