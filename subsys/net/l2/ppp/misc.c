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

#include "ppp_internal.h"

const char *ppp_phase_str(enum ppp_phase phase)
{
#if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) || defined(CONFIG_NET_SHELL)
	switch (phase) {
	case PPP_DEAD:
		return "DEAD";
	case PPP_ESTABLISH:
		return "ESTABLISH";
	case PPP_AUTH:
		return "AUTH";
	case PPP_NETWORK:
		return "NETWORK";
	case PPP_RUNNING:
		return "RUNNING";
	case PPP_TERMINATE:
		return "TERMINATE";
	}
#else
	ARG_UNUSED(phase);
#endif

	return "";
}

#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
static void validate_phase_transition(enum ppp_phase current,
				      enum ppp_phase new)
{
	static const uint8_t valid_transitions[] = {
		[PPP_DEAD] = 1 << PPP_ESTABLISH,
		[PPP_ESTABLISH] = 1 << PPP_DEAD |
				1 << PPP_AUTH |
				1 << PPP_TERMINATE,
		[PPP_AUTH] = 1 << PPP_TERMINATE |
				1 << PPP_NETWORK,
		[PPP_NETWORK] = 1 << PPP_TERMINATE |
				1 << PPP_RUNNING,
		[PPP_RUNNING] = 1 << PPP_TERMINATE |
				1 << PPP_NETWORK,
		[PPP_TERMINATE] = 1 << PPP_DEAD,
	};

	if (!(valid_transitions[current] & 1 << new)) {
		NET_DBG("Invalid phase transition: %s (%d) => %s (%d)",
			ppp_phase_str(current), current,
			ppp_phase_str(new), new);
	}
}
#else
static inline void validate_phase_transition(enum ppp_phase current,
					     enum ppp_phase new)
{
	ARG_UNUSED(current);
	ARG_UNUSED(new);
}
#endif

#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
void ppp_change_phase_debug(struct ppp_context *ctx, enum ppp_phase new_phase,
			    const char *caller, int line)
#else
void ppp_change_phase(struct ppp_context *ctx, enum ppp_phase new_phase)
#endif
{
	NET_ASSERT(ctx);

	if (ctx->phase == new_phase) {
		return;
	}

	NET_ASSERT(new_phase >= PPP_DEAD &&
		   new_phase <= PPP_TERMINATE);

#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("[%p] phase %s (%d) => %s (%d) (%s():%d)",
		ctx, ppp_phase_str(ctx->phase), ctx->phase,
		ppp_phase_str(new_phase), new_phase, caller, line);
#endif

	validate_phase_transition(ctx->phase, new_phase);

	ctx->phase = new_phase;

	if (ctx->phase == PPP_DEAD) {
		ppp_mgmt_raise_phase_dead_event(ctx->iface);
	} else if (ctx->phase == PPP_RUNNING) {
		ppp_mgmt_raise_phase_running_event(ctx->iface);
	}
}

const char *ppp_state_str(enum ppp_state state)
{
#if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) || defined(CONFIG_NET_SHELL)
	switch (state) {
	case PPP_INITIAL:
		return "INITIAL";
	case PPP_STARTING:
		return "STARTING";
	case PPP_CLOSED:
		return "CLOSED";
	case PPP_STOPPED:
		return "STOPPED";
	case PPP_CLOSING:
		return "CLOSING";
	case PPP_STOPPING:
		return "STOPPING";
	case PPP_REQUEST_SENT:
		return "REQUEST_SENT";
	case PPP_ACK_RECEIVED:
		return "ACK_RECEIVED";
	case PPP_ACK_SENT:
		return "ACK_SENT";
	case PPP_OPENED:
		return "OPENED";
	}
#else
	ARG_UNUSED(state);
#endif

	return "";
}

const char *ppp_pkt_type2str(enum ppp_packet_type type)
{
#if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) || defined(CONFIG_NET_SHELL)
	switch (type) {
	case PPP_CONFIGURE_REQ:
		return "Configure-Req";
	case PPP_CONFIGURE_ACK:
		return "Configure-Ack";
	case PPP_CONFIGURE_NACK:
		return "Configure-Nack";
	case PPP_CONFIGURE_REJ:
		return "Configure-Rej";
	case PPP_TERMINATE_REQ:
		return "Terminate-Req";
	case PPP_TERMINATE_ACK:
		return "Terminate-Ack";
	case PPP_CODE_REJ:
		return "Code-Rej";
	case PPP_PROTOCOL_REJ:
		return "Protocol-Rej";
	case PPP_ECHO_REQ:
		return "Echo-Req";
	case PPP_ECHO_REPLY:
		return "Echo-Reply";
	case PPP_DISCARD_REQ:
		return "Discard-Req";
	}
#else
	ARG_UNUSED(type);
#endif

	return "";
}

const char *ppp_proto2str(uint16_t proto)
{
#if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG)
	switch (proto) {
	case PPP_IP:
		return "IPv4";
	case PPP_IPV6:
		return "IPv6";
	case PPP_ECP:
		return "ECP";
	case PPP_CCP:
		return "CCP";
	case PPP_LCP:
		return "LCP";
	case PPP_IPCP:
		return "IPCP";
	case PPP_IPV6CP:
		return "IPV6CP";
	case PPP_PAP:
		return "PAP";
	case PPP_CHAP:
		return "CHAP";
	case PPP_EAP:
		return "EAP";
	}
#else
	ARG_UNUSED(proto);
#endif

	return "";
}

#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
static void validate_state_transition(enum ppp_state current,
				      enum ppp_state new)
{
	/* See RFC 1661 ch. 4.1 */
	static const uint16_t valid_transitions[] = {
		[PPP_INITIAL] = 1 << PPP_CLOSED |
				1 << PPP_STARTING,
		[PPP_STARTING] = 1 << PPP_INITIAL |
				1 << PPP_REQUEST_SENT,
		[PPP_CLOSED] = 1 << PPP_INITIAL |
				1 << PPP_REQUEST_SENT,
		[PPP_STOPPED] = 1 << PPP_STARTING |
				1 << PPP_CLOSED |
				1 << PPP_ACK_RECEIVED |
				1 << PPP_REQUEST_SENT,
		[PPP_CLOSING] = 1 << PPP_INITIAL |
				1 << PPP_STOPPING |
				1 << PPP_CLOSED,
		[PPP_STOPPING] = 1 << PPP_STARTING |
				1 << PPP_CLOSING |
				1 << PPP_STOPPED,
		[PPP_REQUEST_SENT] = 1 << PPP_STARTING |
				1 << PPP_CLOSING |
				1 << PPP_STOPPED |
				1 << PPP_ACK_SENT |
				1 << PPP_ACK_RECEIVED,
		[PPP_ACK_RECEIVED] = 1 << PPP_STARTING |
				1 << PPP_CLOSING |
				1 << PPP_OPENED |
				1 << PPP_REQUEST_SENT |
				1 << PPP_STOPPED,
		[PPP_ACK_SENT] = 1 << PPP_STARTING |
				1 << PPP_CLOSING |
				1 << PPP_STOPPED |
				1 << PPP_REQUEST_SENT |
				1 << PPP_OPENED,
		[PPP_OPENED] = 1 << PPP_STARTING |
				1 << PPP_CLOSING |
				1 << PPP_ACK_SENT |
				1 << PPP_REQUEST_SENT |
				1 << PPP_CLOSING |
				1 << PPP_STOPPING,
	};

	if (!(valid_transitions[current] & 1 << new)) {
		NET_DBG("Invalid state transition: %s (%d) => %s (%d)",
			ppp_state_str(current), current,
			ppp_state_str(new), new);
	}
}
#else
static inline void validate_state_transition(enum ppp_state current,
					     enum ppp_state new)
{
	ARG_UNUSED(current);
	ARG_UNUSED(new);
}
#endif

#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
void ppp_change_state_debug(struct ppp_fsm *fsm, enum ppp_state new_state,
			    const char *caller, int line)
#else
void ppp_change_state(struct ppp_fsm *fsm, enum ppp_state new_state)
#endif
{
	NET_ASSERT(fsm);

	if (fsm->state == new_state) {
		return;
	}

	NET_ASSERT(new_state >= PPP_INITIAL &&
		   new_state <= PPP_OPENED);

#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("[%s/%p] state %s (%d) => %s (%d) (%s():%d)",
		fsm->name, fsm, ppp_state_str(fsm->state), fsm->state,
		ppp_state_str(new_state), new_state, caller, line);
#endif

	validate_state_transition(fsm->state, new_state);

	fsm->state = new_state;
}

const char *ppp_option2str(enum ppp_protocol_type protocol,
			   int type)
{
#if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) || defined(CONFIG_NET_SHELL)
	switch (protocol) {
	case PPP_LCP:
		switch (type) {
		case LCP_OPTION_RESERVED:
			return "RESERVED";
		case LCP_OPTION_MRU:
			return "MRU";
		case LCP_OPTION_ASYNC_CTRL_CHAR_MAP:
			return "ASYNC_CTRL_CHAR_MAP";
		case LCP_OPTION_AUTH_PROTO:
			return "AUTH_PROTO";
		case LCP_OPTION_QUALITY_PROTO:
			return "QUALITY_PROTO";
		case LCP_OPTION_MAGIC_NUMBER:
			return "MAGIC_NUMBER";
		case LCP_OPTION_PROTO_COMPRESS:
			return "PROTO_COMPRESS";
		case LCP_OPTION_ADDR_CTRL_COMPRESS:
			return "ADDR_CTRL_COMPRESS";
		}

		break;

#if defined(CONFIG_NET_IPV4)
	case PPP_IPCP:
		switch (type) {
		case IPCP_OPTION_RESERVED:
			return "RESERVED";
		case IPCP_OPTION_IP_ADDRESSES:
			return "IP_ADDRESSES";
		case IPCP_OPTION_IP_COMP_PROTO:
			return "IP_COMPRESSION_PROTOCOL";
		case IPCP_OPTION_IP_ADDRESS:
			return "IP_ADDRESS";
		case IPCP_OPTION_DNS1:
			return "DNS1";
		case IPCP_OPTION_NBNS1:
			return "NBNS1";
		case IPCP_OPTION_DNS2:
			return "DNS2";
		case IPCP_OPTION_NBNS2:
			return "NBNS2";
		}

		break;
#endif

#if defined(CONFIG_NET_IPV6)
	case PPP_IPV6CP:
		switch (type) {
		case IPV6CP_OPTION_RESERVED:
			return "RESERVED";
		case IPV6CP_OPTION_INTERFACE_IDENTIFIER:
			return "INTERFACE_IDENTIFIER";
		}

		break;
#endif

	default:
		break;
	}
#else
	ARG_UNUSED(type);
#endif

	return "";
}

void ppp_fsm_name_set(struct ppp_fsm *fsm, const char *name)
{
#if CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG
	fsm->name = name;
#else
	ARG_UNUSED(fsm);
	ARG_UNUSED(name);
#endif
}
