/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>

#include <zephyr/net/ppp.h>
#include <zephyr/random/rand32.h>

#include "net_private.h"

#include "ppp_internal.h"

/* This timeout is in milliseconds */
#define FSM_TIMEOUT K_MSEC(CONFIG_NET_L2_PPP_TIMEOUT)

#define MAX_NACK_LOOPS CONFIG_NET_L2_PPP_MAX_NACK_LOOPS

struct ppp_context *ppp_fsm_ctx(struct ppp_fsm *fsm)
{
	if (fsm->protocol == PPP_LCP) {
		return CONTAINER_OF(fsm, struct ppp_context, lcp.fsm);
#if defined(CONFIG_NET_IPV4)
	} else if (fsm->protocol == PPP_IPCP) {
		return CONTAINER_OF(fsm, struct ppp_context, ipcp.fsm);
#endif
#if defined(CONFIG_NET_IPV6)
	} else if (fsm->protocol == PPP_IPV6CP) {
		return CONTAINER_OF(fsm, struct ppp_context, ipv6cp.fsm);
#endif
#if defined(CONFIG_NET_L2_PPP_PAP)
	} else if (fsm->protocol == PPP_PAP) {
		return CONTAINER_OF(fsm, struct ppp_context, pap.fsm);
#endif
	}

	return NULL;
}

struct net_if *ppp_fsm_iface(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = ppp_fsm_ctx(fsm);

	NET_ASSERT(ctx->iface);

	return ctx->iface;
}

static void fsm_send_configure_req(struct ppp_fsm *fsm, bool retransmit)
{
	struct net_pkt *pkt = NULL;

	if (fsm->state != PPP_ACK_RECEIVED &&
	    fsm->state != PPP_ACK_SENT &&
	    fsm->state != PPP_REQUEST_SENT) {
		/* If we are not negotiating options, then reset them */
		if (fsm->cb.config_info_reset) {
			fsm->cb.config_info_reset(fsm);
		}

		fsm->recv_nack_loops = 0;
		fsm->nack_loops = 0;
	}

	if (!retransmit) {
		fsm->retransmits = MAX_CONFIGURE_REQ;
		fsm->req_id = ++fsm->id;
	}

	fsm->ack_received = false;

	if (fsm->cb.config_info_add) {
		pkt = fsm->cb.config_info_add(fsm);
	}

	NET_DBG("[%s/%p] Sending %s (%d) id %d to peer while in %s (%d)",
		fsm->name, fsm, ppp_pkt_type2str(PPP_CONFIGURE_REQ),
		PPP_CONFIGURE_REQ, fsm->req_id, ppp_state_str(fsm->state),
		fsm->state);

	(void)ppp_send_pkt(fsm, NULL, PPP_CONFIGURE_REQ, fsm->req_id,
			   pkt, pkt ? net_pkt_get_len(pkt) : 0);

	fsm->retransmits--;

	(void)k_work_reschedule(&fsm->timer, FSM_TIMEOUT);
}

static void ppp_fsm_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ppp_fsm *fsm = CONTAINER_OF(dwork, struct ppp_fsm, timer);

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
	case PPP_ACK_SENT:
	case PPP_REQUEST_SENT:
		if (fsm->retransmits <= 0) {
			NET_DBG("[%s/%p] %s retransmit limit %d reached",
				fsm->name, fsm,
				ppp_pkt_type2str(PPP_CONFIGURE_REQ),
				fsm->retransmits);

			ppp_change_state(fsm, PPP_STOPPED);

			if (fsm->cb.finished) {
				fsm->cb.finished(fsm);
			}
		} else {
			if (fsm->cb.retransmit) {
				fsm->cb.retransmit(fsm);
			}

			fsm_send_configure_req(fsm, true);

			if (fsm->state == PPP_ACK_RECEIVED) {
				ppp_change_state(fsm, PPP_REQUEST_SENT);
			}
		}

		break;

	case PPP_CLOSING:
	case PPP_STOPPING:
		if (fsm->retransmits <= 0) {
			ppp_change_state(fsm,
					 fsm->state == PPP_CLOSING ?
					 PPP_CLOSED : PPP_STOPPED);

			if (fsm->cb.finished) {
				fsm->cb.finished(fsm);
			}
		} else {
			fsm->req_id = ++fsm->id;

			ppp_send_pkt(fsm, NULL, PPP_TERMINATE_REQ, fsm->req_id,
				     fsm->terminate_reason,
				     strlen(fsm->terminate_reason));

			fsm->retransmits--;

			(void)k_work_reschedule(&fsm->timer, FSM_TIMEOUT);
		}

		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		break;
	}
}

void ppp_fsm_init(struct ppp_fsm *fsm, uint16_t protocol)
{
	fsm->protocol = protocol;
	fsm->state = PPP_INITIAL;
	fsm->flags = 0U;

	k_work_init_delayable(&fsm->timer, ppp_fsm_timeout);
}

static void fsm_down(struct ppp_fsm *fsm)
{
	size_t i;

	for (i = 0; i < fsm->my_options.count; i++) {
		fsm->my_options.data[i].flags = 0;
	}

	if (fsm->cb.down) {
		fsm->cb.down(fsm);
	}
}

static void terminate(struct ppp_fsm *fsm, enum ppp_state next_state)
{
	if (fsm->state != PPP_OPENED) {
		k_work_cancel_delayable(&fsm->timer);
	} else {
		fsm_down(fsm);
	}

	fsm->retransmits = MAX_TERMINATE_REQ;
	fsm->req_id = ++fsm->id;

	(void)ppp_send_pkt(fsm, NULL, PPP_TERMINATE_REQ, fsm->req_id,
			   fsm->terminate_reason,
			   strlen(fsm->terminate_reason));

	if (fsm->retransmits == 0) {
		ppp_change_state(fsm, next_state);

		if (fsm->cb.finished) {
			fsm->cb.finished(fsm);
		}

		return;
	}

	(void)k_work_reschedule(&fsm->timer, FSM_TIMEOUT);

	fsm->retransmits--;

	ppp_change_state(fsm, next_state);
}

void ppp_fsm_close(struct ppp_fsm *fsm, const uint8_t *reason)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
	case PPP_ACK_SENT:
	case PPP_OPENED:
	case PPP_REQUEST_SENT:
		if (reason) {
			int len = strlen(reason);

			len = MIN(sizeof(fsm->terminate_reason) - 1, len);
			strncpy(fsm->terminate_reason, reason, len);
		}

		terminate(fsm, PPP_CLOSING);
		break;

	case PPP_INITIAL:
	case PPP_STARTING:
		ppp_change_state(fsm, PPP_INITIAL);
		break;

	case PPP_STOPPED:
		ppp_change_state(fsm, PPP_CLOSED);
		break;

	case PPP_STOPPING:
		ppp_change_state(fsm, PPP_CLOSING);
		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		break;
	}
}

void ppp_fsm_lower_down(struct ppp_fsm *fsm)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
	case PPP_ACK_SENT:
	case PPP_REQUEST_SENT:
	case PPP_STOPPING:
		ppp_change_state(fsm, PPP_STARTING);
		k_work_cancel_delayable(&fsm->timer);
		break;

	case PPP_CLOSED:
		ppp_change_state(fsm, PPP_INITIAL);
		break;

	case PPP_CLOSING:
		ppp_change_state(fsm, PPP_INITIAL);
		k_work_cancel_delayable(&fsm->timer);
		break;

	case PPP_OPENED:
		ppp_change_state(fsm, PPP_STARTING);
		fsm_down(fsm);

		break;

	case PPP_STOPPED:
		ppp_change_state(fsm, PPP_STARTING);
		if (fsm->cb.starting) {
			fsm->cb.starting(fsm);
		}

		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		break;
	}
}

void ppp_fsm_lower_up(struct ppp_fsm *fsm)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_CLOSED:
		break;

	case PPP_INITIAL:
		ppp_change_state(fsm, PPP_CLOSED);
		break;

	case PPP_STARTING:
		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);

		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		break;
	}
}

void ppp_fsm_open(struct ppp_fsm *fsm)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_CLOSED:
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		fsm_send_configure_req(fsm, false);
		break;

	case PPP_CLOSING:
		ppp_change_state(fsm, PPP_STOPPING);
		if (fsm->flags & FSM_RESTART) {
			ppp_fsm_lower_down(fsm);
			ppp_fsm_lower_up(fsm);
		}

		break;

	case PPP_INITIAL:
		ppp_change_state(fsm, PPP_STARTING);
		if (fsm->cb.starting) {
			fsm->cb.starting(fsm);
		}

		break;

	case PPP_OPENED:
	case PPP_STOPPED:
		if (fsm->flags & FSM_RESTART) {
			ppp_fsm_lower_down(fsm);
			ppp_fsm_lower_up(fsm);
		}

		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		break;
	}
}

int ppp_send_pkt(struct ppp_fsm *fsm, struct net_if *iface,
		 enum ppp_packet_type type, uint8_t id,
		 void *data, uint32_t data_len)
{
	/* Note that the data parameter is the received PPP packet if
	 * we want to send PROTOCOL or CODE reject packet.
	 */
	struct net_pkt *req_pkt = data;
	uint16_t protocol = 0;
	size_t len = 0;
	struct ppp_packet ppp;
	struct net_pkt *pkt = NULL;
	int ret;
	struct ppp_context *ctx = ppp_fsm_ctx(fsm);

	if (!iface) {
		if (!fsm) {
			return -ENOENT;
		}

		iface = ppp_fsm_iface(fsm);
	}

	if (fsm) {
		protocol = fsm->protocol;
	}

	switch (type) {
	case PPP_CODE_REJ:
		len = net_pkt_get_len(req_pkt);
		len = MIN(len, ctx->lcp.my_options.mru);
		break;

	case PPP_CONFIGURE_ACK:
	case PPP_CONFIGURE_NACK:
	case PPP_CONFIGURE_REJ:
	case PPP_CONFIGURE_REQ:
		pkt = data;
		/* 2 + 1 + 1 (configure-[req|ack|nack|rej]) +
		 * data_len (options)
		 */
		len = sizeof(ppp) + data_len;
		break;

	case PPP_DISCARD_REQ:
		break;

	case PPP_ECHO_REQ:
		len = sizeof(ppp) + sizeof(uint32_t) + data_len;
		break;

	case PPP_ECHO_REPLY:
		len = sizeof(ppp) + net_pkt_remaining_data(req_pkt);
		break;

	case PPP_PROTOCOL_REJ:
		len = sizeof(ppp) + sizeof(uint16_t) +
			net_pkt_remaining_data(req_pkt);
		protocol = PPP_LCP;
		break;

	case PPP_TERMINATE_REQ:
	case PPP_TERMINATE_ACK:
		len = sizeof(ppp);
		break;

	default:
		break;
	}

	if (len < sizeof(ppp)) {
		return -EINVAL;
	}

	ppp.code = type;
	ppp.id = id;
	ppp.length = htons(len);

	if (!pkt) {
		pkt = net_pkt_alloc_with_buffer(iface,
						sizeof(uint16_t) + len,
						AF_UNSPEC, 0,
						PPP_BUF_ALLOC_TIMEOUT);
		if (!pkt) {
			goto out_of_mem;
		}
	} else {
		struct net_buf *buf;

		buf = net_pkt_get_reserve_tx_data(PPP_BUF_ALLOC_TIMEOUT);
		if (!buf) {
			LOG_ERR("failed to allocate buffer");
			goto out_of_mem;
		}

		net_pkt_frag_insert(pkt, buf);
		net_pkt_cursor_init(pkt);
	}

	ret = net_pkt_write_be16(pkt, protocol);
	if (ret < 0) {
		goto out_of_mem;
	}

	ret = net_pkt_write(pkt, &ppp, sizeof(ppp));
	if (ret < 0) {
		goto out_of_mem;
	}

	if (type == PPP_CODE_REJ) {
		if (!req_pkt) {
			goto out_of_mem;
		}

		net_pkt_cursor_init(req_pkt);
		net_pkt_copy(pkt, req_pkt, len);

	} else if (type == PPP_ECHO_REQ) {
		struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
						       lcp.fsm);
		if (ctx->lcp.magic) {
			ctx->lcp.magic = sys_rand32_get();
		}

		ret = net_pkt_write_be32(pkt, ctx->lcp.magic);
		if (ret < 0) {
			goto out_of_mem;
		}

		data_len = MIN(data_len, ctx->lcp.my_options.mru);
		if (data_len > 0) {
			if (data_len == sizeof(uint32_t)) {
				ret = net_pkt_write_be32(pkt,
						       POINTER_TO_UINT(data));
			} else {
				ret = net_pkt_write(pkt, data, data_len);
			}

			if (ret < 0) {
				goto out_of_mem;
			}
		}
	} else if (type == PPP_ECHO_REPLY) {
		net_pkt_copy(pkt, req_pkt, len);
	} else if (type == PPP_PROTOCOL_REJ) {
		net_pkt_cursor_init(req_pkt);
		net_pkt_copy(pkt, req_pkt, len);
	}

	NET_DBG("[%s/%p] Sending %zd bytes pkt %p (options len %d)",
		fsm ? fsm->name : "?", fsm, net_pkt_get_len(pkt), pkt,
		data_len);

	net_pkt_set_ppp(pkt, true);

	if (fsm) {
		/* Do not call net_send_data() directly in order to make this
		 * thread run before the sending happens. If we call the
		 * net_send_data() from this thread, then in fast link (like
		 * when running inside QEMU) the reply might arrive before we
		 * have returned from this function. That is bad because the
		 * fsm would be in wrong state and the received pkt is dropped.
		 */
		ppp_queue_pkt(pkt);
	} else {
		ret = net_send_data(pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
		}
	}

	return 0;

out_of_mem:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	return -ENOMEM;
}

static enum net_verdict fsm_recv_configure_req(struct ppp_fsm *fsm,
					       uint8_t id,
					       struct net_pkt *pkt,
					       uint16_t remaining_len)
{
	struct net_pkt *out = NULL;
	int len = 0;
	enum ppp_packet_type code;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_ACK_SENT:
	case PPP_ACK_RECEIVED:
		break;

	case PPP_CLOSED:
		(void)ppp_send_pkt(fsm, net_pkt_iface(pkt), PPP_TERMINATE_ACK,
				   id, NULL, 0);
		return NET_OK;

	case PPP_CLOSING:
	case PPP_STOPPING:
		return NET_OK;

	case PPP_OPENED:
		fsm_down(fsm);

		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	case PPP_REQUEST_SENT:
		/* Received request while waiting ACK */
		break;

	case PPP_STOPPED:
		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		return NET_DROP;
	}

	out = net_pkt_alloc_with_buffer(net_pkt_iface(pkt),
					sizeof(uint16_t) + sizeof(uint16_t) +
						sizeof(uint8_t) + sizeof(uint8_t) +
						remaining_len,
					AF_UNSPEC, 0, PPP_BUF_ALLOC_TIMEOUT);
	if (!out) {
		return NET_DROP;
	}

	net_pkt_cursor_init(out);

	if (fsm->cb.config_info_req) {
		int ret;

		ret = fsm->cb.config_info_req(fsm, pkt, remaining_len, out);
		if (ret < 0) {
			goto unref_out_pkt;
		}

		if (fsm->nack_loops >= MAX_NACK_LOOPS &&
		    ret == PPP_CONFIGURE_NACK) {
			ret = PPP_CONFIGURE_REJ;
		}

		code = ret;
		len = net_pkt_get_len(out);
	} else if (remaining_len) {
		code = PPP_CONFIGURE_REJ;

		net_pkt_copy(out, pkt, remaining_len);
		len = remaining_len;
	} else {
		code = PPP_CONFIGURE_ACK;
	}

	NET_DBG("[%s/%p] Sending %s (%d) id %d to peer while in %s (%d)",
		fsm->name, fsm, ppp_pkt_type2str(code), code, id,
		ppp_state_str(fsm->state), fsm->state);

	(void)ppp_send_pkt(fsm, NULL, code, id, out, len);

	if (code == PPP_CONFIGURE_ACK) {
		if (fsm->state == PPP_ACK_RECEIVED) {
			k_work_cancel_delayable(&fsm->timer);

			ppp_change_state(fsm, PPP_OPENED);

			if (fsm->cb.up) {
				fsm->cb.up(fsm);
			}
		} else {
			ppp_change_state(fsm, PPP_ACK_SENT);
		}

		fsm->nack_loops = 0;
	} else {
		if (fsm->state != PPP_ACK_RECEIVED) {
			ppp_change_state(fsm, PPP_REQUEST_SENT);
		}

		if (code == PPP_CONFIGURE_NACK) {
			fsm->nack_loops++;
		}
	}

	return NET_OK;

unref_out_pkt:
	net_pkt_unref(out);

	return NET_DROP;
}

static enum net_verdict fsm_recv_configure_ack(struct ppp_fsm *fsm, uint8_t id,
					       struct net_pkt *pkt,
					       uint16_t remaining_len)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	if (id != fsm->req_id || fsm->ack_received) {
		return NET_DROP;
	}

	if (fsm->cb.config_info_ack) {
		if (fsm->cb.config_info_ack(fsm, pkt, remaining_len) < 0) {
			NET_DBG("[%s/%p] %s %s received", fsm->name, fsm,
				"Invalid",
				ppp_pkt_type2str(PPP_CONFIGURE_ACK));
			return NET_DROP;
		}
	}

	fsm->ack_received = true;
	fsm->recv_nack_loops = 0;

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
		k_work_cancel_delayable(&fsm->timer);
		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	case PPP_ACK_SENT:
		k_work_cancel_delayable(&fsm->timer);
		ppp_change_state(fsm, PPP_OPENED);
		fsm->retransmits = MAX_CONFIGURE_REQ;
		if (fsm->cb.up) {
			fsm->cb.up(fsm);
		}

		break;

	case PPP_CLOSED:
	case PPP_STOPPED:
		(void)ppp_send_pkt(fsm, net_pkt_iface(pkt), PPP_TERMINATE_ACK,
				   id, NULL, 0);
		break;

	case PPP_OPENED:
		fsm_down(fsm);

		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	case PPP_REQUEST_SENT:
		ppp_change_state(fsm, PPP_ACK_RECEIVED);
		fsm->retransmits = MAX_CONFIGURE_REQ;
		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		return NET_DROP;
	}

	return NET_OK;
}

static enum net_verdict fsm_recv_configure_nack_rej(struct ppp_fsm *fsm,
						    enum ppp_packet_type code,
						    uint8_t id,
						    struct net_pkt *pkt,
						    uint16_t length)
{
	bool ret = false;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	if (id != fsm->req_id || fsm->ack_received) {
		return NET_DROP;
	}

	if (code == PPP_CONFIGURE_NACK) {
		bool rejected = false;

		fsm->recv_nack_loops++;

		if (fsm->recv_nack_loops >= MAX_NACK_LOOPS) {
			rejected = true;
		}

		if (fsm->cb.config_info_nack) {
			int err;

			err = fsm->cb.config_info_nack(fsm, pkt, length,
						       rejected);
			if (err < 0) {
				NET_DBG("[%s/%p] %s failed (%d)",
					fsm->name, fsm, "Nack", err);
			} else {
				ret = true;
			}
		}

		if (!ret) {
			NET_DBG("[%s/%p] %s %s (id %d)", fsm->name, fsm,
				"Invalid", ppp_pkt_type2str(code), id);
			return NET_DROP;
		}
	} else {
		fsm->recv_nack_loops = 0;

		if (fsm->cb.config_info_rej) {
			int err;

			err = fsm->cb.config_info_rej(fsm, pkt, length);
			if (err < 0) {
				NET_DBG("[%s/%p] %s failed (%d)",
					fsm->name, fsm, "Reject", err);
			} else {
				ret = true;
			}
		}

		if (!ret) {
			NET_DBG("[%s/%p] %s %s (id %d)", fsm->name, fsm,
				"Invalid", ppp_pkt_type2str(code), id);
			return NET_DROP;
		}
	}

	fsm->ack_received = true;

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
		k_work_cancel_delayable(&fsm->timer);
		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	case PPP_ACK_SENT:
	case PPP_REQUEST_SENT:
		k_work_cancel_delayable(&fsm->timer);
		fsm_send_configure_req(fsm, false);
		break;

	case PPP_CLOSED:
	case PPP_STOPPED:
		(void)ppp_send_pkt(fsm, net_pkt_iface(pkt), PPP_TERMINATE_ACK,
				   id, NULL, 0);
		break;

	case PPP_OPENED:
		fsm_down(fsm);

		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		return NET_DROP;
	}

	return NET_OK;
}

static enum net_verdict fsm_recv_terminate_req(struct ppp_fsm *fsm, uint8_t id,
					       struct net_pkt *pkt,
					       uint16_t length)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
	case PPP_ACK_SENT:
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	case PPP_OPENED:
		if (length > 0) {
			net_pkt_read(pkt, fsm->terminate_reason,
				     MIN(length,
					 sizeof(fsm->terminate_reason) - 1));

			NET_DBG("[%s/%p] %s (%s)",
				fsm->name, fsm, "Terminated by peer",
				fsm->terminate_reason);
		} else {
			NET_DBG("[%s/%p] Terminated by peer",
				fsm->name, fsm);
		}

		fsm->retransmits = 0;
		ppp_change_state(fsm, PPP_STOPPING);

		fsm_down(fsm);

		(void)k_work_reschedule(&fsm->timer, FSM_TIMEOUT);
		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		return NET_DROP;
	}

	(void)ppp_send_pkt(fsm, net_pkt_iface(pkt), PPP_TERMINATE_ACK, id,
			   NULL, 0);

	return NET_OK;
}

static enum net_verdict fsm_recv_terminate_ack(struct ppp_fsm *fsm, uint8_t id,
					       struct net_pkt *pkt,
					       uint16_t length)
{
	enum ppp_state new_state;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_CLOSING:
		new_state = PPP_CLOSED;
		goto stopped;

	case PPP_OPENED:
		fsm_down(fsm);

		fsm_send_configure_req(fsm, false);
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	case PPP_STOPPING:
		new_state = PPP_STOPPED;
		goto stopped;

	case PPP_ACK_RECEIVED:
		ppp_change_state(fsm, PPP_REQUEST_SENT);
		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		return NET_DROP;
	}

	return NET_OK;

stopped:
	k_work_cancel_delayable(&fsm->timer);
	ppp_change_state(fsm, new_state);

	if (fsm->cb.finished) {
		fsm->cb.finished(fsm);
	}

	return NET_OK;
}

static enum net_verdict fsm_recv_code_rej(struct ppp_fsm *fsm,
					  struct net_pkt *pkt)
{
	uint8_t code, id;
	int ret;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	ret = net_pkt_read_u8(pkt, &code);
	if (ret < 0) {
		return NET_DROP;
	}

	ret = net_pkt_read_u8(pkt, &id);
	if (ret < 0) {
		return NET_DROP;
	}

	NET_DBG("[%s/%p] Received Code-Rej code %d id %d", fsm->name, fsm,
		code, id);

	if (fsm->state == PPP_ACK_RECEIVED) {
		ppp_change_state(fsm, PPP_REQUEST_SENT);
	}

	return NET_OK;
}

void ppp_fsm_proto_reject(struct ppp_fsm *fsm)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	switch (fsm->state) {
	case PPP_ACK_RECEIVED:
	case PPP_ACK_SENT:
	case PPP_STOPPING:
	case PPP_REQUEST_SENT:
		k_work_cancel_delayable(&fsm->timer);
		ppp_change_state(fsm, PPP_STOPPED);
		if (fsm->cb.finished) {
			fsm->cb.finished(fsm);
		}

		break;

	case PPP_CLOSED:
		ppp_change_state(fsm, PPP_CLOSED);
		if (fsm->cb.finished) {
			fsm->cb.finished(fsm);
		}

		break;

	case PPP_CLOSING:
		k_work_cancel_delayable(&fsm->timer);
		ppp_change_state(fsm, PPP_CLOSED);
		if (fsm->cb.finished) {
			fsm->cb.finished(fsm);
		}

		break;

	case PPP_OPENED:
		terminate(fsm, PPP_STOPPING);
		break;

	case PPP_STOPPED:
		ppp_change_state(fsm, PPP_STOPPED);
		if (fsm->cb.finished) {
			fsm->cb.finished(fsm);
		}

		break;

	default:
		NET_DBG("[%s/%p] %s state %s (%d)", fsm->name, fsm, "Invalid",
			ppp_state_str(fsm->state), fsm->state);
		break;
	}
}

enum net_verdict ppp_fsm_input(struct ppp_fsm *fsm, uint16_t proto,
			       struct net_pkt *pkt)
{
	uint8_t code, id;
	uint16_t length;
	int ret;
	struct ppp_context *ctx = ppp_fsm_ctx(fsm);

	ret = net_pkt_read_u8(pkt, &code);
	if (ret < 0) {
		NET_DBG("[%s/%p] Cannot read %s (pkt len %zd)",
			fsm->name, fsm, "code", net_pkt_get_len(pkt));
		return NET_DROP;
	}

	ret = net_pkt_read_u8(pkt, &id);
	if (ret < 0) {
		NET_DBG("[%s/%p] Cannot read %s (pkt len %zd)",
			fsm->name, fsm, "id", net_pkt_get_len(pkt));
		return NET_DROP;
	}

	ret = net_pkt_read_be16(pkt, &length);
	if (ret < 0) {
		NET_DBG("[%s/%p] Cannot read %s (pkt len %zd)",
			fsm->name, fsm, "length", net_pkt_get_len(pkt));
		return NET_DROP;
	}

	if (length > ctx->lcp.my_options.mru) {
		NET_DBG("[%s/%p] Too long msg %d", fsm->name, fsm, length);
		return NET_DROP;
	}

	if (fsm->state == PPP_INITIAL || fsm->state == PPP_STARTING) {
		NET_DBG("[%s/%p] Received %s packet in wrong state %s (%d)",
			fsm->name, fsm, ppp_proto2str(proto),
			ppp_state_str(fsm->state), fsm->state);
		return NET_DROP;
	}

	/* Length will only contain payload/data length */
	length -= sizeof(code) + sizeof(id) + sizeof(length);

	NET_DBG("[%s/%p] %s %s (%d) id %d payload len %d", fsm->name, fsm,
		ppp_proto2str(proto), ppp_pkt_type2str(code), code, id,
		length);

	switch (code) {
	case PPP_CODE_REJ:
		return fsm_recv_code_rej(fsm, pkt);

	case PPP_CONFIGURE_ACK:
		return fsm_recv_configure_ack(fsm, id, pkt, length);

	case PPP_CONFIGURE_NACK:
		return fsm_recv_configure_nack_rej(fsm, code, id, pkt, length);

	case PPP_CONFIGURE_REQ:
		return fsm_recv_configure_req(fsm, id, pkt, length);

	case PPP_CONFIGURE_REJ:
		return fsm_recv_configure_nack_rej(fsm, code, id, pkt, length);

	case PPP_TERMINATE_ACK:
		return fsm_recv_terminate_ack(fsm, id, pkt, length);

	case PPP_TERMINATE_REQ:
		return fsm_recv_terminate_req(fsm, id, pkt, length);

	default:
		if (fsm->cb.proto_extension) {
			enum net_verdict verdict;

			verdict = fsm->cb.proto_extension(fsm, code, id, pkt);
			if (verdict != NET_DROP) {
				return verdict;
			}
		}

		(void)ppp_send_pkt(fsm, net_pkt_iface(pkt), PPP_CODE_REJ,
				   id, pkt, 0);
	}

	return NET_DROP;
}

enum net_verdict ppp_fsm_recv_protocol_rej(struct ppp_fsm *fsm,
					   uint8_t id,
					   struct net_pkt *pkt)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	return NET_DROP;
}

enum net_verdict ppp_fsm_recv_echo_req(struct ppp_fsm *fsm,
				       uint8_t id,
				       struct net_pkt *pkt)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	(void)ppp_send_pkt(fsm, net_pkt_iface(pkt), PPP_ECHO_REPLY,
		id, pkt, 0);

	return NET_OK;
}

enum net_verdict ppp_fsm_recv_echo_reply(struct ppp_fsm *fsm,
					 uint8_t id,
					 struct net_pkt *pkt)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

#if defined(CONFIG_NET_SHELL)
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);
	if (ctx->shell.echo_reply.cb) {
		ctx->shell.echo_reply.cb(ctx->shell.echo_reply.user_data,
					 ctx->shell.echo_reply.user_data_len);
	}
#endif /* CONFIG_NET_SHELL */

	return NET_OK;
}

enum net_verdict ppp_fsm_recv_discard_req(struct ppp_fsm *fsm,
					  uint8_t id,
					  struct net_pkt *pkt)
{
	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	return NET_OK;
}

void ppp_send_proto_rej(struct net_if *iface, struct net_pkt *pkt,
			uint16_t protocol)
{
	uint8_t code, id;
	int ret;

	ret = net_pkt_read_u8(pkt, &code);
	if (ret < 0) {
		goto quit;
	}

	ret = net_pkt_read_u8(pkt, &id);
	if (ret < 0) {
		goto quit;
	}

	(void)ppp_send_pkt(NULL, iface, PPP_PROTOCOL_REJ, id, pkt, 0);

quit:
	return;
}
