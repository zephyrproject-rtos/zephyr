/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp.h"
#include "ocpp_i.h"

LOG_MODULE_REGISTER(ocpp_rpc, LOG_LEVEL_INF);

#define	OCPP_WAMP_RPC_TYPE_IDX 1

int ocpp_send_to_server(ocpp_wamp_rpc_msg_t *snd, k_timeout_t timeout)
{
	int ret;
	int sock;
	ocpp_info_t *ctx  = snd->ctx;

	switch (snd->msg[OCPP_WAMP_RPC_TYPE_IDX ]) {
	case OCPP_WAMP_RPC_REQ:
		/* ocpp spec - allow only one active call at a time
		   release lock on response received from CS or timeout */
		ret = k_mutex_lock(snd->sndlock, timeout);
		if (ret) {
			return ret;
		}
		k_poll_signal_reset(snd->rspsig);
		break;

	case OCPP_WAMP_RPC_RESP:
	case OCPP_WAMP_RPC_ERR:
		break;

	default:
		return -EINVAL;
	}

	k_mutex_lock(&ctx->ilock, K_FOREVER);
	sock = ((ocpp_upstream_info_t *)ctx->ui)->wssock;
	k_mutex_unlock(&ctx->ilock);
	if (sock < 0) {
		ret = -EAGAIN;
		goto unlock;
	}

	ret = websocket_send_msg(sock, snd->msg, snd->msg_len,
				 WEBSOCKET_OPCODE_DATA_TEXT, true,
				 true, 5000); //fixme
	if (ret < 0) {
		goto unlock;
	}

	if (snd->rspsig) {
		struct k_poll_event events[1] = {
			K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					K_POLL_MODE_NOTIFY_ONLY,
					snd->rspsig),
		};

		ret = k_poll(events, 1, timeout);
		goto unlock;
	}

	return 0;

unlock:
	if (snd->sndlock) {
		k_mutex_unlock(snd->sndlock);
	}

	return ret;
}

int ocpp_receive_from_server(ocpp_wamp_rpc_msg_t *rcv,
			     uint32_t timeout)
{
	int ret;
	int sock;
	int msg_type;
	uint64_t remaining = 0;
	ocpp_info_t *ctx  = rcv->ctx;

	k_mutex_lock(&ctx->ilock, K_MSEC(timeout));
	sock = ((ocpp_upstream_info_t *)ctx->ui)->wssock;
	k_mutex_unlock(&ctx->ilock);
	if (sock < 0) {
		return -EAGAIN;
	}

	ret = websocket_recv_msg(sock, rcv->msg,
				 rcv->msg_len,
				 &msg_type,
				 &remaining,
				 timeout);

	return ret;
}
