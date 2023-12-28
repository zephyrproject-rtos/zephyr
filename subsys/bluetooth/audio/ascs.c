/* @file
 * @brief Bluetooth ASCS
 */
/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ascs, CONFIG_BT_ASCS_LOG_LEVEL);

#include "common/bt_str.h"
#include "common/assert.h"

#include "../host/att_internal.h"

#include "audio_internal.h"
#include "bap_iso.h"
#include "bap_endpoint.h"
#include "bap_unicast_server.h"
#include "pacs_internal.h"
#include "cap_internal.h"

#define ASE_BUF_SEM_TIMEOUT K_MSEC(CONFIG_BT_ASCS_ASE_BUF_TIMEOUT)

#define MAX_ASES_SESSIONS CONFIG_BT_MAX_CONN * \
				(CONFIG_BT_ASCS_ASE_SNK_COUNT + \
				 CONFIG_BT_ASCS_ASE_SRC_COUNT)

BUILD_ASSERT(CONFIG_BT_ASCS_MAX_ACTIVE_ASES <= MAX(MAX_ASES_SESSIONS,
						   CONFIG_BT_ISO_MAX_CHAN),
	     "Max active ASEs are set to more than actual number of ASEs or ISOs");

#if defined(CONFIG_BT_BAP_UNICAST_SERVER)

#define ASE_ID(_ase) ase->ep.status.id
#define ASE_DIR(_id) \
	(_id > CONFIG_BT_ASCS_ASE_SNK_COUNT ? BT_AUDIO_DIR_SOURCE : BT_AUDIO_DIR_SINK)
#define ASE_UUID(_id) \
	(_id > CONFIG_BT_ASCS_ASE_SNK_COUNT ? BT_UUID_ASCS_ASE_SRC : BT_UUID_ASCS_ASE_SNK)
#define ASE_COUNT (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT)
#define BT_BAP_ASCS_RSP_NULL ((struct bt_bap_ascs_rsp[]) { BT_BAP_ASCS_RSP(0, 0) })

static struct bt_ascs_ase {
	struct bt_conn *conn;
	struct bt_bap_ep ep;
	const struct bt_gatt_attr *attr;
	struct k_work_delayable disconnect_work;
	struct k_work_delayable state_transition_work;
	enum bt_bap_ep_state state_pending;
	bool unexpected_iso_link_loss;
} ase_pool[CONFIG_BT_ASCS_MAX_ACTIVE_ASES];

/* Minimum state size when in the codec configured state */
#define MIN_CONFIG_STATE_SIZE (1 + 1 + 1 + 1 + 1 + 2 + 3 + 3 + 3 + 3 + 5 + 1)
/* Minimum state size when in the QoS configured state */
#define MIN_QOS_STATE_SIZE    (1 + 1 + 1 + 1 + 3 + 1 + 1 + 2 + 1 + 2 + 3 + 1 + 1 + 1)

/* Calculate the size requirement of the ASE BUF, based on the maximum possible
 * size of the Codec Configured state or the QoS Configured state, as either
 * of them can be the largest state
 */
#define ASE_BUF_SIZE                                                                               \
	MIN(BT_ATT_MAX_ATTRIBUTE_LEN,                                                              \
	    MAX(MIN_CONFIG_STATE_SIZE + CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE,                   \
		MIN_QOS_STATE_SIZE + CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE))

/* Verify that the prepare count is large enough to cover the maximum value we support a client
 * writing
 */
BUILD_ASSERT(
	BT_ATT_BUF_SIZE - 3 >= ASE_BUF_SIZE ||
		DIV_ROUND_UP(ASE_BUF_SIZE, (BT_ATT_BUF_SIZE - 3)) <= CONFIG_BT_ATT_PREPARE_COUNT,
	"CONFIG_BT_ATT_PREPARE_COUNT not large enough to cover the maximum supported ASCS value");

/* It is mandatory to support long writes in ASCS unconditionally, and thus
 * CONFIG_BT_ATT_PREPARE_COUNT must be at least 1 to support the feature
 */
BUILD_ASSERT(CONFIG_BT_ATT_PREPARE_COUNT > 0, "CONFIG_BT_ATT_PREPARE_COUNT shall be at least 1");

static const struct bt_bap_unicast_server_cb *unicast_server_cb;

static K_SEM_DEFINE(ase_buf_sem, 1, 1);
NET_BUF_SIMPLE_DEFINE_STATIC(ase_buf, ASE_BUF_SIZE);

static int control_point_notify(struct bt_conn *conn, const void *data, uint16_t len);
static int ascs_ep_get_status(struct bt_bap_ep *ep, struct net_buf_simple *buf);

static void ascs_app_rsp_warn_valid(const struct bt_bap_ascs_rsp *rsp)
{
	/* Validate application error code */
	switch (rsp->code) {
	case BT_BAP_ASCS_RSP_CODE_SUCCESS:
	case BT_BAP_ASCS_RSP_CODE_CAP_UNSUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_NO_MEM:
	case BT_BAP_ASCS_RSP_CODE_UNSPECIFIED:
	case BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_CONF_REJECTED:
	case BT_BAP_ASCS_RSP_CODE_METADATA_UNSUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED:
		break;
	default:
		LOG_WRN("Invalid application error code: %u", rsp->code);
		return;
	}

	/* Validate application error code and reason combinations */
	switch (rsp->code) {
	case BT_BAP_ASCS_RSP_CODE_SUCCESS:
	case BT_BAP_ASCS_RSP_CODE_CAP_UNSUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_NO_MEM:
	case BT_BAP_ASCS_RSP_CODE_UNSPECIFIED:
		if (rsp->reason != BT_BAP_ASCS_REASON_NONE) {
			LOG_WRN("Invalid reason %u for code %u", rsp->reason, rsp->code);
		}
		break;
	case BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_CONF_REJECTED:
		if (!IN_RANGE(rsp->reason, BT_BAP_ASCS_REASON_NONE, BT_BAP_ASCS_REASON_CIS)) {
			LOG_WRN("Invalid reason %u for code %u", rsp->reason, rsp->code);
		}
		break;
	case BT_BAP_ASCS_RSP_CODE_METADATA_UNSUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED:
		if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(rsp->metadata_type)) {
			LOG_WRN("Invalid metadata type %u for code %u", rsp->metadata_type,
				rsp->code);
		}
		break;
	default:
		break;
	}
}

static bool is_valid_ase_id(uint8_t ase_id)
{
	return IN_RANGE(ase_id, 1, ASE_COUNT);
}

static enum bt_bap_ep_state ascs_ep_get_state(struct bt_bap_ep *ep)
{
	return ep->status.state;
}

static void ase_free(struct bt_ascs_ase *ase)
{
	__ASSERT(ase && ase->conn, "Non-existing ASE");

	LOG_DBG("conn %p ase %p id 0x%02x", (void *)ase->conn, ase, ase->ep.status.id);

	if (ase->ep.iso != NULL) {
		bt_bap_iso_unbind_ep(ase->ep.iso, &ase->ep);
	}

	bt_conn_unref(ase->conn);
	ase->conn = NULL;

	(void)k_work_cancel_delayable(&ase->disconnect_work);
	(void)k_work_cancel_delayable(&ase->state_transition_work);
}

static int ase_state_notify(struct bt_ascs_ase *ase)
{
	const uint8_t att_ntf_header_size = 3; /* opcode (1) + handle (2) */
	struct bt_conn *conn = ase->conn;
	struct bt_conn_info conn_info;
	uint16_t max_ntf_size;
	uint16_t ntf_size;
	int err;

	__ASSERT_NO_MSG(conn != NULL);

	err = bt_conn_get_info(conn, &conn_info);
	__ASSERT_NO_MSG(err == 0);

	if (conn_info.state != BT_CONN_STATE_CONNECTED ||
	    !bt_gatt_is_subscribed(conn, ase->attr, BT_GATT_CCC_NOTIFY)) {
		return 0;
	}

	err = k_sem_take(&ase_buf_sem, ASE_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_WRN("Failed to take ase_buf_sem: %d", err);

		return err;
	}

	ascs_ep_get_status(&ase->ep, &ase_buf);

	max_ntf_size = bt_gatt_get_mtu(conn) - att_ntf_header_size;

	ntf_size = MIN(max_ntf_size, ase_buf.len);
	if (ntf_size < ase_buf.len) {
		LOG_DBG("Sending truncated notification (%u / %u)",
			ntf_size, ase_buf.len);
	}

	err = bt_gatt_notify(conn, ase->attr, ase_buf.data, ntf_size);

	k_sem_give(&ase_buf_sem);

	return err;
}

static void ascs_disconnect_stream_work_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct bt_ascs_ase *ase = CONTAINER_OF(d_work, struct bt_ascs_ase,
					       disconnect_work);
	struct bt_bap_ep *ep = &ase->ep;
	struct bt_bap_stream *stream = ep->stream;
	struct bt_bap_stream *pair_stream;

	__ASSERT(ep != NULL && ep->iso && stream != NULL,
		 "Invalid endpoint %p, iso %p or stream %p",
		 ep, ep == NULL ? NULL : ep->iso, stream);

	if (ep->dir == BT_AUDIO_DIR_SINK) {
		pair_stream = ep->iso->tx.stream;
	} else {
		pair_stream = ep->iso->rx.stream;
	}

	LOG_DBG("ase %p ep %p stream %p pair_stream %p",
		ase, ep, stream, pair_stream);

	if (pair_stream != NULL) {
		struct bt_ascs_ase *pair_ase;

		__ASSERT(pair_stream->ep != NULL, "Invalid pair_stream %p",
			 pair_stream);

		if (pair_stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
			/* Should not disconnect ISO if the stream is paired
			 * with another one in the streaming state
			 */

			return;
		}

		pair_ase = CONTAINER_OF(pair_stream->ep, struct bt_ascs_ase,
					ep);

		/* Cancel pair ASE disconnect work if pending */
		(void)k_work_cancel_delayable(&pair_ase->disconnect_work);
	}

	if (stream != NULL &&
	    ep->iso != NULL &&
	    (ep->iso->chan.state == BT_ISO_STATE_CONNECTED ||
	     ep->iso->chan.state == BT_ISO_STATE_CONNECTING)) {
		const int err = bt_bap_stream_disconnect(stream);

		if (err != 0) {
			LOG_ERR("Failed to disconnect CIS %p: %d",
				stream, err);
		}
	}
}

static int ascs_disconnect_stream(struct bt_bap_stream *stream)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(stream->ep, struct bt_ascs_ase,
					       ep);

	LOG_DBG("%p", stream);

	return k_work_reschedule(&ase->disconnect_work,
				 K_MSEC(CONFIG_BT_ASCS_ISO_DISCONNECT_DELAY));
}

static void ase_enter_state_idle(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ase->ep.receiver_ready = false;

	if (stream->conn != NULL) {
		bt_conn_unref(stream->conn);
		stream->conn = NULL;
	}

	ops = stream->ops;
	if (ops != NULL && ops->released != NULL) {
		ops->released(stream);
	}

	ase_free(ase);
}

static void ase_enter_state_codec_configured(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ase->ep.receiver_ready = false;

	ops = stream->ops;
	if (ops != NULL && ops->configured != NULL) {
		ops->configured(stream, &ase->ep.qos_pref);
	}
}

static void ase_enter_state_qos_configured(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ase->ep.receiver_ready = false;

	ops = stream->ops;
	if (ops != NULL && ops->qos_set != NULL) {
		ops->qos_set(stream);
	}
}

static void ase_enter_state_enabling(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ops = stream->ops;
	if (ops != NULL && ops->enabled != NULL) {
		ops->enabled(stream);
	}

	/* SINK ASEs can autonomously go into the streaming state if the CIS is connected */
	if (ase->ep.dir == BT_AUDIO_DIR_SINK && ase->ep.receiver_ready && ase->ep.iso != NULL &&
	    ase->ep.iso->chan.state == BT_ISO_STATE_CONNECTED) {
		ascs_ep_set_state(&ase->ep, BT_BAP_EP_STATE_STREAMING);
	}
}

static void ase_enter_state_streaming(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ops = stream->ops;
	if (ops != NULL && ops->started != NULL) {
		ops->started(stream);
	}
}

static void ase_metadata_updated(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ops = stream->ops;
	if (ops != NULL && ops->metadata_updated != NULL) {
		ops->metadata_updated(stream);
	}
}

static void ase_exit_state_streaming(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;
	const enum bt_bap_ep_state next_state = ascs_ep_get_state(&ase->ep);
	uint8_t reason = ase->ep.reason;

	__ASSERT_NO_MSG(stream != NULL);

	if (reason == BT_HCI_ERR_SUCCESS) {
		/* Default to BT_HCI_ERR_UNSPECIFIED if no other reason is set */
		reason = BT_HCI_ERR_UNSPECIFIED;
	}

	ops = stream->ops;

	/*
	 * On link-loss we go from streaming state to QOS configured state,
	 * and it makes sense to do the disabled callback before entering the
	 * QOS configured state
	 */
	if (next_state == BT_BAP_EP_STATE_QOS_CONFIGURED) {
		if (ops != NULL && ops->disabled != NULL) {
			ops->disabled(stream);
		} else {
			LOG_WRN("No callback for disabled set");
		}
	}

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream, reason);
	} else {
		LOG_WRN("No callback for stopped set");
	}
}

static void ase_exit_state_enabling(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;
	const enum bt_bap_ep_state next_state = ascs_ep_get_state(&ase->ep);

	ops = stream->ops;

	/*
	 * When the EP direction is BT_AUDIO_DIR_SOURCE the state machine goes from
	 * enabled to disabled where the disabled calback will be called,
	 * for BT_AUDIO_DIR_SINK we go from enabled to qos_configured,
	 * and logically we have to do the disabled callback first
	 */
	if (next_state == BT_BAP_EP_STATE_QOS_CONFIGURED && ase->ep.dir == BT_AUDIO_DIR_SINK) {
		if (ops != NULL && ops->disabled != NULL) {
			ops->disabled(stream);
		} else {
			LOG_WRN("No callback for disabled set");
		}
	}
}

static void ase_enter_state_disabling(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;
	struct bt_bap_stream_ops *ops;

	__ASSERT_NO_MSG(stream != NULL);

	ase->ep.receiver_ready = false;

	ops = stream->ops;
	if (ops != NULL && ops->disabled != NULL) {
		ops->disabled(stream);
	}
}

static void ase_enter_state_releasing(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream = ase->ep.stream;

	__ASSERT_NO_MSG(stream != NULL);

	ase->ep.receiver_ready = false;

	/* Either the client or the server may disconnect the CISes when entering the releasing
	 * state.
	 */
	if (bt_bap_stream_can_disconnect(stream)) {
		int err;

		err = ascs_disconnect_stream(stream);
		if (err < 0) {
			LOG_ERR("Failed to disconnect stream %p: %d", stream, err);
		}
	} else {
		ascs_ep_set_state(&ase->ep, BT_BAP_EP_STATE_IDLE);
	}
}

static void state_transition_work_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct bt_ascs_ase *ase = CONTAINER_OF(d_work, struct bt_ascs_ase, state_transition_work);
	const enum bt_bap_ep_state old_state = ascs_ep_get_state(&ase->ep);
	const enum bt_bap_ep_state new_state = ase->state_pending;
	int err;

	ase->ep.status.state = new_state;

	/* Notify ASE state */
	if (ase->conn != NULL) {
		err = ase_state_notify(ase);
		if (err == -ENOMEM) {
			struct bt_conn_info info;
			uint32_t retry_delay_ms;

			/* Revert back to old state */
			ase->ep.status.state = old_state;

			err = bt_conn_get_info(ase->conn, &info);
			__ASSERT_NO_MSG(err == 0);

			retry_delay_ms = BT_CONN_INTERVAL_TO_MS(info.le.interval);

			/* Reschedule the state transition */
			err = k_work_reschedule(d_work, K_MSEC(retry_delay_ms));
			if (err >= 0) {
				LOG_WRN("Out of buffers for ase state notification. "
					"Will retry in %dms", retry_delay_ms);
				return;
			}
		}

		if (err < 0) {
			LOG_ERR("Failed to notify ASE state (err %d)", err);
		}
	}

	LOG_DBG("ase %p ep %p id 0x%02x %s -> %s", ase, &ase->ep, ase->ep.status.id,
		bt_bap_ep_state_str(old_state), bt_bap_ep_state_str(new_state));

	if (old_state == new_state) {
		switch (new_state) {
		case BT_BAP_EP_STATE_ENABLING:
		case BT_BAP_EP_STATE_STREAMING:
			ase_metadata_updated(ase);
			return;
		default:
			break;
		}
	}

	/* Actions needed for exiting the old state */
	switch (old_state) {
	case BT_BAP_EP_STATE_STREAMING:
		ase_exit_state_streaming(ase);
		break;
	case BT_BAP_EP_STATE_ENABLING:
		ase_exit_state_enabling(ase);
		break;
	default:
		break;
	}

	/* Actions needed for entering the new state */
	switch (new_state) {
	case BT_BAP_EP_STATE_IDLE:
		ase_enter_state_idle(ase);
		break;
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		ase_enter_state_codec_configured(ase);
		break;
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		ase_enter_state_qos_configured(ase);
		break;
	case BT_BAP_EP_STATE_ENABLING:
		ase_enter_state_enabling(ase);
		break;
	case BT_BAP_EP_STATE_STREAMING:
		ase_enter_state_streaming(ase);
		break;
	case BT_BAP_EP_STATE_DISABLING:
		ase_enter_state_disabling(ase);
		break;
	case BT_BAP_EP_STATE_RELEASING:
		ase_enter_state_releasing(ase);
		break;
	default:
		__ASSERT_PRINT("Invalid state %d", new_state);
	}
}

int ascs_ep_set_state(struct bt_bap_ep *ep, uint8_t state)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);
	const enum bt_bap_ep_state old_state = ascs_ep_get_state(&ase->ep);
	bool valid_state_transition = false;
	int err;

	switch (state) {
	case BT_BAP_EP_STATE_IDLE:
		valid_state_transition = true;
		break;
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		switch (old_state) {
		case BT_BAP_EP_STATE_IDLE:
		case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
		case BT_BAP_EP_STATE_RELEASING:
			valid_state_transition = true;
			break;
		default:
			break;
		} break;
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		switch (old_state) {
		case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
			valid_state_transition = true;
			break;
		case BT_BAP_EP_STATE_DISABLING:
			valid_state_transition = ase->ep.dir == BT_AUDIO_DIR_SOURCE;
			break;
		case BT_BAP_EP_STATE_ENABLING:
		case BT_BAP_EP_STATE_STREAMING:
			/* Source ASE transition Streaming->QoS configured is valid on case of CIS
			 * link-loss.
			 */
			valid_state_transition = ase->ep.dir == BT_AUDIO_DIR_SINK ||
						 ase->unexpected_iso_link_loss;
			break;
		default:
			break;
		} break;
	case BT_BAP_EP_STATE_ENABLING:
		switch (old_state) {
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
		case BT_BAP_EP_STATE_ENABLING:
			valid_state_transition = true;
			break;
		default:
			break;
		} break;
	case BT_BAP_EP_STATE_STREAMING:
		switch (old_state) {
		case BT_BAP_EP_STATE_ENABLING:
		case BT_BAP_EP_STATE_STREAMING:
			valid_state_transition = true;
			break;
		default:
			break;
		} break;
	case BT_BAP_EP_STATE_DISABLING:
		switch (old_state) {
		case BT_BAP_EP_STATE_ENABLING:
		case BT_BAP_EP_STATE_STREAMING:
			valid_state_transition = ase->ep.dir == BT_AUDIO_DIR_SOURCE;
			break;
		default:
			break;
		} break;
	case BT_BAP_EP_STATE_RELEASING:
		switch (old_state) {
		case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		case BT_BAP_EP_STATE_QOS_CONFIGURED:
		case BT_BAP_EP_STATE_ENABLING:
		case BT_BAP_EP_STATE_STREAMING:
			valid_state_transition = true;
			break;
		case BT_BAP_EP_STATE_DISABLING:
			valid_state_transition = ase->ep.dir == BT_AUDIO_DIR_SOURCE;
			break;
		default:
			break;
		} break;
	}

	if (!valid_state_transition) {
		BT_ASSERT_MSG(false, "Invalid state transition: %s -> %s",
			      bt_bap_ep_state_str(old_state), bt_bap_ep_state_str(state));

		return -EBADMSG;
	}

	ase->state_pending = state;

	err = k_work_schedule(&ase->state_transition_work, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Failed to schedule state transition work err %d", err);
		return err;
	}

	return 0;
}

static void ascs_ep_get_status_config(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_audio_codec_qos_pref *pref = &ep->qos_pref;

	cfg = net_buf_simple_add(buf, sizeof(*cfg));
	cfg->framing = pref->unframed_supported ? BT_ASCS_QOS_FRAMING_UNFRAMED
						: BT_ASCS_QOS_FRAMING_FRAMED;
	cfg->phy = pref->phy;
	cfg->rtn = pref->rtn;
	cfg->latency = sys_cpu_to_le16(pref->latency);
	sys_put_le24(pref->pd_min, cfg->pd_min);
	sys_put_le24(pref->pd_max, cfg->pd_max);
	sys_put_le24(pref->pref_pd_min, cfg->prefer_pd_min);
	sys_put_le24(pref->pref_pd_max, cfg->prefer_pd_max);
	cfg->codec.id = ep->codec_cfg.id;
	cfg->codec.cid = sys_cpu_to_le16(ep->codec_cfg.cid);
	cfg->codec.vid = sys_cpu_to_le16(ep->codec_cfg.vid);

	LOG_DBG("dir %s unframed_supported 0x%02x phy 0x%02x rtn %u "
		"latency %u pd_min %u pd_max %u pref_pd_min %u pref_pd_max %u codec id 0x%02x",
		bt_audio_dir_str(ep->dir), pref->unframed_supported, pref->phy, pref->rtn,
		pref->latency, pref->pd_min, pref->pd_max, pref->pref_pd_min, pref->pref_pd_max,
		ep->stream->codec_cfg->id);

	cfg->cc_len = ep->codec_cfg.data_len;
	net_buf_simple_add_mem(buf, ep->codec_cfg.data, ep->codec_cfg.data_len);
}

static void ascs_ep_get_status_qos(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_qos *qos;

	qos = net_buf_simple_add(buf, sizeof(*qos));
	qos->cig_id = ep->cig_id;
	qos->cis_id = ep->cis_id;
	sys_put_le24(ep->stream->qos->interval, qos->interval);
	qos->framing = ep->stream->qos->framing;
	qos->phy = ep->stream->qos->phy;
	qos->sdu = sys_cpu_to_le16(ep->stream->qos->sdu);
	qos->rtn = ep->stream->qos->rtn;
	qos->latency = sys_cpu_to_le16(ep->stream->qos->latency);
	sys_put_le24(ep->stream->qos->pd, qos->pd);

	LOG_DBG("dir %s codec id 0x%02x interval %u framing 0x%02x phy 0x%02x "
		"rtn %u latency %u pd %u",
		bt_audio_dir_str(ep->dir), ep->stream->codec_cfg->id, ep->stream->qos->interval,
		ep->stream->qos->framing, ep->stream->qos->phy, ep->stream->qos->rtn,
		ep->stream->qos->latency, ep->stream->qos->pd);
}

static void ascs_ep_get_status_enable(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;

	enable = net_buf_simple_add(buf, sizeof(*enable));
	enable->cig_id = ep->cig_id;
	enable->cis_id = ep->cis_id;

	enable->metadata_len = ep->codec_cfg.meta_len;
	net_buf_simple_add_mem(buf, ep->codec_cfg.meta, ep->codec_cfg.meta_len);

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x",
		bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);
}

static ssize_t ascs_ase_read_status_idle(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_ascs_ase_status status = {
		.id = POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr)),
		.state = BT_BAP_EP_STATE_IDLE,
	};

	LOG_DBG("conn %p id 0x%02x", (void *)conn, status.id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &status, sizeof(status));
}

static int ascs_ep_get_status(struct bt_bap_ep *ep, struct net_buf_simple *buf)
{
	if (!ep || !buf) {
		return -EINVAL;
	}

	LOG_DBG("ep %p id 0x%02x state %s", ep, ep->status.id,
		bt_bap_ep_state_str(ep->status.state));

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	(void)net_buf_simple_add_mem(buf, &ep->status, sizeof(ep->status));

	switch (ep->status.state) {
	case BT_BAP_EP_STATE_IDLE:
	/* Fallthrough */
	case BT_BAP_EP_STATE_RELEASING:
		break;
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		ascs_ep_get_status_config(ep, buf);
		break;
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		ascs_ep_get_status_qos(ep, buf);
		break;
	case BT_BAP_EP_STATE_ENABLING:
	/* Fallthrough */
	case BT_BAP_EP_STATE_STREAMING:
	/* Fallthrough */
	case BT_BAP_EP_STATE_DISABLING:
		ascs_ep_get_status_enable(ep, buf);
		break;
	default:
		LOG_ERR("Invalid Endpoint state");
		break;
	}

	return 0;
}

static int ascs_iso_accept(const struct bt_iso_accept_info *info, struct bt_iso_chan **iso_chan)
{
	LOG_DBG("conn %p", (void *)info->acl);

	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		struct bt_ascs_ase *ase = &ase_pool[i];
		enum bt_bap_ep_state state;
		struct bt_iso_chan *chan;

		if (ase->conn != info->acl ||
		    ase->ep.cig_id != info->cig_id ||
		    ase->ep.cis_id != info->cis_id) {
			continue;
		}

		state = ascs_ep_get_state(&ase->ep);
		if (state != BT_BAP_EP_STATE_ENABLING && state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
			LOG_WRN("ase %p cannot accept ISO connection", ase);
			break;
		}

		__ASSERT(ase->ep.iso != NULL, "ep %p not bound with ISO", &ase->ep);

		chan = &ase->ep.iso->chan;
		if (chan->iso != NULL) {
			LOG_WRN("ase %p chan %p already connected", ase, chan);
			return -EALREADY;
		}

		*iso_chan = chan;

		LOG_DBG("iso_chan %p", *iso_chan);

		return 0;
	}

	return -EACCES;
}

#if defined(CONFIG_BT_AUDIO_RX)
static void ascs_iso_recv(struct bt_iso_chan *chan,
			  const struct bt_iso_recv_info *info,
			  struct net_buf *buf)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;

	ep = iso->rx.ep;
	if (ep == NULL) {
		/* In the case that the CIS has been setup as bidirectional, and
		 * only one of the directions have an ASE configured yet,
		 * we should only care about valid ISO packets when doing this
		 * check. The reason is that some controllers send HCI ISO data
		 * packets to the host, even if no SDU was sent on the remote
		 * side. This basically means that empty PDUs are sent to the
		 * host as HCI ISO data packets, which we should just ignore
		 */
		if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
			LOG_ERR("iso %p not bound with ep", chan);
		}

		return;
	}

	if (ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA)) {
			LOG_DBG("ep %p is not in the streaming state: %s", ep,
				bt_bap_ep_state_str(ep->status.state));
		}

		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	ops = stream->ops;

	if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p len %zu", stream, stream->ep, net_buf_frags_len(buf));
	}

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(stream, info, buf);
	} else {
		LOG_WRN("No callback for recv set");
	}
}
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
static void ascs_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;

	ep = iso->tx.ep;
	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	ops = stream->ops;

	if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p", stream, stream->ep);
	}

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(stream);
	}
}
#endif /* CONFIG_BT_AUDIO_TX */

static void ascs_update_sdu_size(struct bt_bap_ep *ep)
{
	struct bt_iso_chan_io_qos *io_qos;
	struct bt_audio_codec_qos *codec_qos = &ep->qos;

	if (ep->dir == BT_AUDIO_DIR_SINK) {
		io_qos = ep->iso->chan.qos->rx;
	} else if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		io_qos = ep->iso->chan.qos->tx;
	} else {
		return;
	}

	io_qos->sdu = codec_qos->sdu;
	io_qos->rtn = codec_qos->rtn;
}

static void ascs_ep_iso_connected(struct bt_bap_ep *ep)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);
	const struct bt_bap_stream_ops *stream_ops;
	struct bt_bap_stream *stream;

	if (ep->status.state != BT_BAP_EP_STATE_ENABLING) {
		LOG_DBG("ep %p not in enabling state: %s", ep,
			bt_bap_ep_state_str(ep->status.state));
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	/* Reset reason */
	ep->reason = BT_HCI_ERR_SUCCESS;
	ase->unexpected_iso_link_loss = false;

	/* Some values are not provided by the HCI events when the CIS is established for the
	 * peripheral, so we update them here based on the parameters provided by the BAP Unicast
	 * Client
	 */
	ascs_update_sdu_size(ep);

	LOG_DBG("stream %p ep %p dir %s", stream, ep, bt_audio_dir_str(ep->dir));

	stream_ops = stream->ops;
	if (stream_ops != NULL && stream_ops->connected != NULL) {
		stream_ops->connected(stream);
	}

	if (ep->dir == BT_AUDIO_DIR_SINK && ep->receiver_ready) {
		/* Source ASEs shall be ISO connected first, and then receive
		 * the receiver start ready command to enter the streaming
		 * state
		 */
		ascs_ep_set_state(ep, BT_BAP_EP_STATE_STREAMING);
	}
}

static void ascs_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);

	if (iso->rx.ep == NULL && iso->tx.ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	if (iso->rx.ep != NULL) {
		ascs_ep_iso_connected(iso->rx.ep);
	}

	if (iso->tx.ep != NULL) {
		ascs_ep_iso_connected(iso->tx.ep);
	}
}

static void ascs_ep_iso_disconnected(struct bt_bap_ep *ep, uint8_t reason)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);
	const struct bt_bap_stream_ops *stream_ops;
	struct bt_bap_stream *stream;

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p ep %p state %s reason 0x%02x", stream, stream->ep,
		bt_bap_ep_state_str(ep->status.state), reason);

	stream_ops = stream->ops;
	if (stream_ops != NULL && stream_ops->disconnected != NULL) {
		stream_ops->disconnected(stream, reason);
	}

	/* Cancel ASE disconnect work if pending */
	(void)k_work_cancel_delayable(&ase->disconnect_work);
	ep->reason = reason;

	if (ep->status.state == BT_BAP_EP_STATE_RELEASING) {
		ascs_ep_set_state(ep, BT_BAP_EP_STATE_IDLE);
	} else if (ep->status.state == BT_BAP_EP_STATE_STREAMING ||
		   ep->status.state == BT_BAP_EP_STATE_DISABLING) {
		/* ASCS_v1.0 3.2 ASE state machine transitions
		 *
		 * If the server detects link loss of a CIS for an ASE in the Streaming
		 * state or the Disabling state, the server shall immediately transition
		 * that ASE to the QoS Configured state.
		 */
		ase->unexpected_iso_link_loss = true;

		ascs_ep_set_state(ep, BT_BAP_EP_STATE_QOS_CONFIGURED);
	}
}

static void ascs_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);

	if (iso->rx.ep == NULL && iso->tx.ep == NULL) {
		return;
	}

	if (iso->rx.ep != NULL) {
		ascs_ep_iso_disconnected(iso->rx.ep, reason);
	}

	if (iso->tx.ep != NULL) {
		ascs_ep_iso_disconnected(iso->tx.ep, reason);
	}
}

static struct bt_iso_chan_ops ascs_iso_ops = {
#if defined(CONFIG_BT_AUDIO_RX)
	.recv = ascs_iso_recv,
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_AUDIO_TX)
	.sent = ascs_iso_sent,
#endif /* CONFIG_BT_AUDIO_TX */
	.connected = ascs_iso_connected,
	.disconnected = ascs_iso_disconnected,
};

static void ascs_ase_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, CONFIG_BT_L2CAP_TX_MTU);

static void ascs_cp_rsp_init(uint8_t op)
{
	struct bt_ascs_cp_rsp *rsp;

	net_buf_simple_reset(&rsp_buf);

	rsp = net_buf_simple_add(&rsp_buf, sizeof(*rsp));
	rsp->op = op;
	rsp->num_ase = 0;
}

/* Add response to an opcode/ASE ID */
static void ascs_cp_rsp_add(uint8_t id, uint8_t code, uint8_t reason)
{
	struct bt_ascs_cp_rsp *rsp = (void *)rsp_buf.__buf;
	struct bt_ascs_cp_ase_rsp *ase_rsp;

	LOG_DBG("id 0x%02x code %s (0x%02x) reason %s (0x%02x)", id,
		bt_ascs_rsp_str(code), code, bt_ascs_reason_str(reason), reason);

	if (rsp->num_ase == 0xff) {
		return;
	}

	switch (code) {
	/* If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be
	 * set to 0xFF.
	 */
	case BT_BAP_ASCS_RSP_CODE_NOT_SUPPORTED:
	case BT_BAP_ASCS_RSP_CODE_INVALID_LENGTH:
		rsp->num_ase = BT_ASCS_UNSUPP_OR_LENGTH_ERR_NUM_ASE;
		break;
	default:
		rsp->num_ase++;
		break;
	}

	ase_rsp = net_buf_simple_add(&rsp_buf, sizeof(*ase_rsp));
	ase_rsp->id = id;
	ase_rsp->code = code;
	ase_rsp->reason = reason;
}

static void ascs_cp_rsp_success(uint8_t id)
{
	ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);
}

static int ase_release(struct bt_ascs_ase *ase, uint8_t reason, struct bt_bap_ascs_rsp *rsp)
{
	enum bt_bap_ep_state state = ascs_ep_get_state(&ase->ep);
	int err;

	if (state == BT_BAP_EP_STATE_IDLE || state == BT_BAP_EP_STATE_RELEASING) {
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(state));
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				       BT_BAP_ASCS_REASON_NONE);
		return -EBADMSG;
	}

	if (unicast_server_cb == NULL || unicast_server_cb->release == NULL) {
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
				       BT_BAP_ASCS_REASON_NONE);
		return -ENOTSUP;
	}

	err = unicast_server_cb->release(ase->ep.stream, rsp);
	if (err) {
		if (rsp->code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					       BT_BAP_ASCS_REASON_NONE);
		}

		LOG_ERR("Release failed: err %d, code %u, reason %u", err, rsp->code, rsp->reason);
		return err;
	}

	/* Set reason in case this exits the streaming state */
	ase->ep.reason = reason;

	ascs_ep_set_state(&ase->ep, BT_BAP_EP_STATE_RELEASING);

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);
	return 0;
}

int bt_ascs_release_ase(struct bt_bap_ep *ep)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);
	const enum bt_bap_ep_state state = ascs_ep_get_state(&ase->ep);

	if (state == BT_BAP_EP_STATE_IDLE) {
		ase_free(ase);
		return 0;
	}

	return ase_release(ase, BT_HCI_ERR_LOCALHOST_TERM_CONN, BT_BAP_ASCS_RSP_NULL);
}

static int ase_disable(struct bt_ascs_ase *ase, uint8_t reason, struct bt_bap_ascs_rsp *rsp)
{
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;
	int err;

	LOG_DBG("ase %p", ase);

	ep = &ase->ep;

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
		/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(ep->status.state));
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				       BT_BAP_ASCS_REASON_NONE);
		return -EBADMSG;
	}

	stream = ep->stream;

	if (unicast_server_cb == NULL || unicast_server_cb->disable == NULL) {
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED, BT_BAP_ASCS_REASON_NONE);
		return -ENOTSUP;
	}

	err = unicast_server_cb->disable(stream, rsp);
	if (err) {
		if (rsp->code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					       BT_BAP_ASCS_REASON_NONE);
		}

		LOG_ERR("Disable failed: err %d, code %u, reason %u", err, rsp->code, rsp->reason);
		return err;
	}

	/* Set reason in case this exits the streaming state */
	ep->reason = reason;

	/* The ASE state machine goes into different states from this operation
	 * based on whether it is a source or a sink ASE.
	 */
	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		ascs_ep_set_state(ep, BT_BAP_EP_STATE_DISABLING);
	} else {
		ascs_ep_set_state(ep, BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);
	return 0;
}

int bt_ascs_disable_ase(struct bt_bap_ep *ep)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);

	return ase_disable(ase, BT_HCI_ERR_LOCALHOST_TERM_CONN, BT_BAP_ASCS_RSP_NULL);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		struct bt_ascs_ase *ase = &ase_pool[i];

		if (ase->conn != conn) {
			continue;
		}

		if (ase->ep.status.state != BT_BAP_EP_STATE_IDLE) {
			/* We must set the state to idle when the ACL is disconnected immediately,
			 * as when the ACL disconnect callbacks have been called, the application
			 * should expect there to be only a single reference to the bt_conn pointer
			 * from the stack.
			 * We trigger the work handler directly rather than e.g. calling
			 * ase_enter_state_idle to trigger "regular" state change behavior (such) as
			 * calling stream->stopped when leaving the streaming state.
			 */
			ase->ep.reason = reason;
			ase->state_pending = BT_BAP_EP_STATE_IDLE;
			state_transition_work_handler(&ase->state_transition_work.work);
			/* At this point, `ase` object have been free'd */
		}
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = disconnected,
};

struct bap_iso_find_params {
	struct bt_conn *acl;
	uint8_t cig_id;
	uint8_t cis_id;
};

static bool bap_iso_find_func(struct bt_bap_iso *iso, void *user_data)
{
	struct bap_iso_find_params *params = user_data;
	const struct bt_bap_ep *ep;

	if (iso->rx.ep != NULL) {
		ep = iso->rx.ep;
	} else if (iso->tx.ep != NULL) {
		ep = iso->tx.ep;
	} else {
		return false;
	}

	return ep->stream->conn == params->acl &&
	       ep->cig_id == params->cig_id &&
	       ep->cis_id == params->cis_id;
}

static struct bt_bap_iso *bap_iso_get_or_new(struct bt_conn *conn, uint8_t cig_id, uint8_t cis_id)
{
	struct bt_bap_iso *iso;
	struct bap_iso_find_params params = {
		.acl = bt_conn_ref(conn),
		.cig_id = cig_id,
		.cis_id = cis_id,
	};

	iso = bt_bap_iso_find(bap_iso_find_func, &params);
	bt_conn_unref(conn);

	if (iso) {
		return iso;
	}

	iso = bt_bap_iso_new();
	if (!iso) {
		return NULL;
	}

	bt_bap_iso_init(iso, &ascs_iso_ops);

	return iso;
}

static uint8_t ase_attr_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			   void *user_data)
{
	struct bt_ascs_ase *ase = user_data;

	if (ase->ep.status.id == POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr))) {
		ase->attr = attr;

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

void ascs_ep_init(struct bt_bap_ep *ep, uint8_t id)
{
	LOG_DBG("ep %p id 0x%02x", ep, id);

	(void)memset(ep, 0, sizeof(*ep));
	ep->status.id = id;
	ep->dir = ASE_DIR(id);
	ep->reason = BT_HCI_ERR_SUCCESS;
}

static void ase_init(struct bt_ascs_ase *ase, struct bt_conn *conn, uint8_t id)
{
	memset(ase, 0, sizeof(*ase));

	ascs_ep_init(&ase->ep, id);

	ase->conn = bt_conn_ref(conn);

	/* Lookup ASE characteristic */
	bt_gatt_foreach_attr_type(0x0001, 0xffff, ASE_UUID(id), NULL, 0, ase_attr_cb, ase);

	__ASSERT(ase->attr, "ASE characteristic not found\n");

	k_work_init_delayable(&ase->disconnect_work, ascs_disconnect_stream_work_handler);
	k_work_init_delayable(&ase->state_transition_work, state_transition_work_handler);
}

static struct bt_ascs_ase *ase_new(struct bt_conn *conn, uint8_t id)
{
	struct bt_ascs_ase *ase = NULL;

	__ASSERT(id > 0 && id <= ASE_COUNT, "invalid ASE_ID 0x%02x", id);

	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		if (ase_pool[i].conn == NULL) {
			ase = &ase_pool[i];
			break;
		}
	}

	if (ase == NULL) {
		return NULL;
	}

	ase_init(ase, conn, id);

	LOG_DBG("conn %p new ase %p id 0x%02x", (void *)conn, ase, id);

	return ase;
}

static struct bt_ascs_ase *ase_find(struct bt_conn *conn, uint8_t id)
{
	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		struct bt_ascs_ase *ase = &ase_pool[i];

		if (ase->conn == conn && ase->ep.status.id == id) {
			return ase;
		}
	}

	return NULL;
}

static ssize_t ascs_ase_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint8_t ase_id = POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr));
	struct bt_ascs_ase *ase = NULL;
	ssize_t ret_val;
	int err;

	LOG_DBG("conn %p attr %p buf %p len %u offset %u", (void *)conn, attr, buf, len, offset);

	/* The callback can be used locally to read the ASE_ID in which case conn won't be set. */
	if (conn != NULL) {
		ase = ase_find(conn, ase_id);
	}

	/* If NULL, we haven't assigned an ASE, this also means that we are currently in IDLE */
	if (ase == NULL) {
		return ascs_ase_read_status_idle(conn, attr, buf, len, offset);
	}

	err = k_sem_take(&ase_buf_sem, ASE_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to take ase_buf_sem: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	ascs_ep_get_status(&ase->ep, &ase_buf);

	ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset, ase_buf.data, ase_buf.len);

	k_sem_give(&ase_buf_sem);

	return ret_val;
}

static void ascs_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("attr %p value 0x%04x", attr, value);
}

struct codec_cap_lookup_id_data {
	uint8_t id;
	uint16_t cid;
	uint16_t vid;
	const struct bt_audio_codec_cap *codec_cap;
};

static bool codec_lookup_id(const struct bt_pacs_cap *cap, void *user_data)
{
	struct codec_cap_lookup_id_data *data = user_data;

	if (cap->codec_cap->id == data->id && cap->codec_cap->cid == data->cid &&
	    cap->codec_cap->vid == data->vid) {
		data->codec_cap = cap->codec_cap;

		return false;
	}

	return true;
}

static int ascs_ep_set_codec(struct bt_bap_ep *ep, uint8_t id, uint16_t cid, uint16_t vid,
			     uint8_t *cc, uint8_t len, struct bt_bap_ascs_rsp *rsp)
{
	struct bt_audio_codec_cfg *codec_cfg;
	struct codec_cap_lookup_id_data lookup_data = {
		.id = id,
		.cid = cid,
		.vid = vid,
	};

	if (ep == NULL) {
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_INVALID,
				       BT_BAP_ASCS_REASON_CODEC_DATA);
		return -EINVAL;
	}

	codec_cfg = &ep->codec_cfg;

	LOG_DBG("ep %p dir %s codec id 0x%02x cid 0x%04x vid 0x%04x len %u",
		ep, bt_audio_dir_str(ep->dir), id, cid, vid, len);

	bt_pacs_cap_foreach(ep->dir, codec_lookup_id, &lookup_data);

	if (lookup_data.codec_cap == NULL) {
		LOG_DBG("Codec with id %u for dir %s is not supported by our capabilities",
			id, bt_audio_dir_str(ep->dir));

		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_INVALID,
				       BT_BAP_ASCS_REASON_CODEC);
		return -ENOENT;
	}

	codec_cfg->id = id;
	codec_cfg->cid = cid;
	codec_cfg->vid = vid;
	codec_cfg->data_len = len;
	memcpy(codec_cfg->data, cc, len);
	codec_cfg->path_id = lookup_data.codec_cap->path_id;

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);

	return 0;
}

static int ase_config(struct bt_ascs_ase *ase, const struct bt_ascs_config *cfg)
{
	struct bt_bap_stream *stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	int err;

	LOG_DBG("ase %p latency 0x%02x phy 0x%02x codec 0x%02x "
		"cid 0x%04x vid 0x%04x codec config len 0x%02x",
		ase, cfg->latency, cfg->phy, cfg->codec.id, cfg->codec.cid, cfg->codec.vid,
		cfg->cc_len);

	if (cfg->latency < BT_ASCS_CONFIG_LATENCY_LOW ||
	    cfg->latency > BT_ASCS_CONFIG_LATENCY_HIGH) {
		LOG_WRN("Invalid latency: 0x%02x", cfg->latency);
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_CONF_INVALID,
				BT_BAP_ASCS_REASON_LATENCY);
		return -EINVAL;
	}

	if (cfg->phy < BT_ASCS_CONFIG_PHY_LE_1M ||
	    cfg->phy > BT_ASCS_CONFIG_PHY_LE_CODED) {
		LOG_WRN("Invalid PHY: 0x%02x", cfg->phy);
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_CONF_INVALID,
				BT_BAP_ASCS_REASON_PHY);
		return -EINVAL;
	}

	if (cfg->cc_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE) {
		LOG_DBG("Can not store %u codec configuration data", cfg->cc_len);

		return -ENOMEM;
	}

	switch (ase->ep.status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_BAP_EP_STATE_IDLE:
		/* or 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		LOG_WRN("Invalid operation in state: %s",
			bt_bap_ep_state_str(ase->ep.status.state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				BT_BAP_ASCS_REASON_NONE);
		return -EINVAL;
	}

	/* Store current codec configuration to be able to restore it
	 * in case of error.
	 */
	(void)memcpy(&codec_cfg, &ase->ep.codec_cfg, sizeof(codec_cfg));

	err = ascs_ep_set_codec(&ase->ep, cfg->codec.id, sys_le16_to_cpu(cfg->codec.cid),
				sys_le16_to_cpu(cfg->codec.vid), (uint8_t *)cfg->cc, cfg->cc_len,
				&rsp);
	if (err) {
		ascs_app_rsp_warn_valid(&rsp);
		(void)memcpy(&ase->ep.codec_cfg, &codec_cfg, sizeof(codec_cfg));
		ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);
		return err;
	}

	if (ase->ep.stream != NULL) {
		if (unicast_server_cb != NULL &&
		    unicast_server_cb->reconfig != NULL) {
			err = unicast_server_cb->reconfig(ase->ep.stream, ase->ep.dir,
							  &ase->ep.codec_cfg, &ase->ep.qos_pref,
							  &rsp);
		} else {
			err = -ENOTSUP;
			rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					      BT_BAP_ASCS_REASON_NONE);
		}

		if (err) {
			ascs_app_rsp_warn_valid(&rsp);

			if (rsp.code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
				rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
						      BT_BAP_ASCS_REASON_NONE);
			}

			LOG_ERR("Reconfig failed: err %d, code %u, reason %u",
				err, rsp.code, rsp.reason);

			(void)memcpy(&ase->ep.codec_cfg, &codec_cfg, sizeof(codec_cfg));
			ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);

			return err;
		}

		stream = ase->ep.stream;
	} else {
		stream = NULL;
		if (unicast_server_cb != NULL &&
		    unicast_server_cb->config != NULL) {
			err = unicast_server_cb->config(ase->conn, &ase->ep, ase->ep.dir,
							&ase->ep.codec_cfg, &stream,
							&ase->ep.qos_pref, &rsp);
		} else {
			err = -ENOTSUP;
			rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					      BT_BAP_ASCS_REASON_NONE);
		}

		if (err || stream == NULL) {
			ascs_app_rsp_warn_valid(&rsp);

			if (rsp.code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
				rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
						      BT_BAP_ASCS_REASON_NONE);
			}

			LOG_ERR("Config failed: err %d, stream %p, code %u, reason %u",
				err, stream, rsp.code, rsp.reason);

			(void)memcpy(&ase->ep.codec_cfg, &codec_cfg, sizeof(codec_cfg));
			ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);

			return err ? err : -ENOMEM;
		}

		bt_bap_stream_init(stream);
	}

	ascs_cp_rsp_success(ASE_ID(ase));

	bt_bap_stream_attach(ase->conn, stream, &ase->ep, &ase->ep.codec_cfg);

	ascs_ep_set_state(&ase->ep, BT_BAP_EP_STATE_CODEC_CONFIGURED);

	return 0;
}

static struct bt_bap_ep *ep_lookup_stream(struct bt_conn *conn, struct bt_bap_stream *stream)
{
	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		struct bt_ascs_ase *ase = &ase_pool[i];

		if (ase->conn == conn && ase->ep.stream == stream) {
			return &ase->ep;
		}
	}

	return NULL;
}

int bt_ascs_config_ase(struct bt_conn *conn, struct bt_bap_stream *stream,
		       struct bt_audio_codec_cfg *codec_cfg,
		       const struct bt_audio_codec_qos_pref *qos_pref)
{
	int err;
	struct bt_ascs_ase *ase = NULL;
	struct bt_bap_ep *ep;
	struct codec_cap_lookup_id_data lookup_data;

	CHECKIF(conn == NULL || stream == NULL || codec_cfg == NULL || qos_pref == NULL) {
		LOG_DBG("NULL value(s) supplied)");
		return -EINVAL;
	}

	ep = ep_lookup_stream(conn, stream);
	if (ep != NULL) {
		LOG_DBG("Stream already configured for conn %p", (void *)stream->conn);
		return -EALREADY;
	}

	/* Get a free ASE or NULL if all ASE instances are aready in use */
	for (int i = 1; i <= ASE_COUNT; i++) {
		if (ase_find(conn, i) == NULL) {
			ase = ase_new(conn, i);
			break;
		}
	}

	if (ase == NULL) {
		LOG_WRN("No free ASE found.");
		return -ENOTSUP;
	}

	ep = &ase->ep;

	if (ep == NULL) {
		return -EINVAL;
	}

	lookup_data.id = codec_cfg->id;
	lookup_data.cid = codec_cfg->cid;
	lookup_data.vid = codec_cfg->vid;

	bt_pacs_cap_foreach(ep->dir, codec_lookup_id, &lookup_data);

	if (lookup_data.codec_cap == NULL) {
		LOG_DBG("Codec with id %u for dir %s is not supported by our capabilities",
			codec_cfg->id, bt_audio_dir_str(ep->dir));
		return -ENOENT;
	}

	(void)memcpy(&ep->codec_cfg, codec_cfg, sizeof(ep->codec_cfg));

	ep->qos_pref = *qos_pref;

	bt_bap_stream_attach(conn, stream, ep, &ep->codec_cfg);

	err = ascs_ep_set_state(ep, BT_BAP_EP_STATE_CODEC_CONFIGURED);
	if (err != 0) {
		bt_bap_stream_detach(stream);
		ase_free(ase);
		return err;
	}

	return 0;
}

static bool is_valid_config_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_config_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	for (uint8_t i = 0U; i < op->num_ases; i++) {
		const struct bt_ascs_config *config;

		if (buf->len < sizeof(*config)) {
			LOG_WRN("Malformed params array");
			return false;
		}

		config = net_buf_simple_pull_mem(buf, sizeof(*config));
		if (buf->len < config->cc_len) {
			LOG_WRN("Malformed codec specific config");
			return false;
		}

		(void)net_buf_simple_pull_mem(buf, config->cc_len);
	}

	if (buf->len > 0) {
		LOG_WRN("Unexpected data");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_config(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_config_op *req;
	const struct bt_ascs_config *cfg;

	if (!is_valid_config_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (uint8_t i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		int err;

		cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));
		(void)net_buf_simple_pull(buf, cfg->cc_len);

		LOG_DBG("ase 0x%02x cc_len %u", cfg->ase, cfg->cc_len);

		if (!cfg->ase || cfg->ase > ASE_COUNT) {
			LOG_WRN("Invalid ASE ID: %u", cfg->ase);
			ascs_cp_rsp_add(cfg->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase = ase_find(conn, cfg->ase);
		if (ase != NULL) {
			ase_config(ase, cfg);
			continue;
		}

		ase = ase_new(conn, cfg->ase);
		if (!ase) {
			ascs_cp_rsp_add(cfg->ase, BT_BAP_ASCS_RSP_CODE_NO_MEM,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("No free ASE found for config ASE ID 0x%02x", cfg->ase);
			continue;
		}

		err = ase_config(ase, cfg);
		if (err != 0) {
			ase_free(ase);
		}
	}

	return buf->size;
}

void bt_ascs_foreach_ep(struct bt_conn *conn, bt_bap_ep_func_t func, void *user_data)
{
	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		struct bt_ascs_ase *ase = &ase_pool[i];

		if (ase->conn == conn) {
			func(&ase->ep, user_data);
		}
	}
}

static void ase_qos(struct bt_ascs_ase *ase, uint8_t cig_id, uint8_t cis_id,
		    struct bt_audio_codec_qos *qos, struct bt_bap_ascs_rsp *rsp)
{
	struct bt_bap_ep *ep = &ase->ep;
	struct bt_bap_stream *stream;

	LOG_DBG("ase %p cig 0x%02x cis 0x%02x interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u "
		"latency %u pd %u", ase, cig_id, cis_id, qos->interval, qos->framing, qos->phy,
		qos->sdu, qos->rtn, qos->latency, qos->pd);

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
	/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(ep->status.state));
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				       BT_BAP_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("NULL stream");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED, BT_BAP_ASCS_REASON_NONE);
		return;
	}

	if (stream->ep == NULL) {
		LOG_ERR("NULL stream->ep");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED, BT_BAP_ASCS_REASON_NONE);
		return;
	}

	rsp->reason = bt_audio_verify_qos(qos);
	if (rsp->reason != BT_BAP_ASCS_REASON_NONE) {
		rsp->code = BT_BAP_ASCS_RSP_CODE_CONF_INVALID;
		return;
	}

	rsp->reason = bt_bap_stream_verify_qos(stream, qos);
	if (rsp->reason != BT_BAP_ASCS_REASON_NONE) {
		rsp->code = BT_BAP_ASCS_RSP_CODE_CONF_INVALID;
		return;
	}

	if (unicast_server_cb != NULL && unicast_server_cb->qos != NULL) {
		int err = unicast_server_cb->qos(stream, qos, rsp);

		if (err) {
			if (rsp->code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
				*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
						       BT_BAP_ASCS_REASON_NONE);
			}

			LOG_DBG("Application returned error: err %d status %u reason %u",
				err, rsp->code, rsp->reason);
			return;
		}
	}

	/* QoS->QoS transition. Unbind ISO if CIG/CIS changed. */
	if (ep->iso != NULL && (ep->cig_id != cig_id || ep->cis_id != cis_id)) {
		bt_bap_iso_unbind_ep(ep->iso, ep);
	}

	if (ep->iso == NULL) {
		struct bt_bap_iso *iso;

		iso = bap_iso_get_or_new(ase->conn, cig_id, cis_id);
		if (iso == NULL) {
			LOG_ERR("Could not allocate bap_iso");
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM,
					       BT_BAP_ASCS_REASON_NONE);
			return;
		}

		if (bt_bap_iso_get_ep(false, iso, ep->dir) != NULL) {
			LOG_ERR("iso %p already in use in dir %s",
			       &iso->chan, bt_audio_dir_str(ep->dir));
			bt_bap_iso_unref(iso);
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_INVALID,
					       BT_BAP_ASCS_REASON_CIS);
			return;
		}

		bt_bap_iso_bind_ep(iso, ep);
		bt_bap_iso_unref(iso);
	}

	/* Store the QoS once accepted */
	ep->qos = *qos;
	stream->qos = &ep->qos;

	/* We setup the data path here, as this is the earliest where
	 * we have the ISO <-> EP coupling completed (due to setting
	 * the CIS ID in the QoS procedure).
	 */
	if (ep->dir == BT_AUDIO_DIR_SINK) {
		bt_audio_codec_cfg_to_iso_path(&ep->iso->rx.path, stream->codec_cfg);
	} else {
		bt_audio_codec_cfg_to_iso_path(&ep->iso->tx.path, stream->codec_cfg);
	}

	ep->cig_id = cig_id;
	ep->cis_id = cis_id;

	ascs_ep_set_state(ep, BT_BAP_EP_STATE_QOS_CONFIGURED);

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);
}

static bool is_valid_qos_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_qos_op *op;
	struct net_buf_simple_state state;
	size_t params_size;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	params_size = sizeof(struct bt_ascs_qos) * op->num_ases;
	if (buf->len < params_size) {
		LOG_WRN("Malformed params array");
		return false;
	}

	(void)net_buf_simple_pull_mem(buf, params_size);

	if (buf->len > 0) {
		LOG_WRN("Unexpected data");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_qos(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_qos_op *req;

	if (!is_valid_qos_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (uint8_t i = 0; i < req->num_ases; i++) {
		struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
							     BT_BAP_ASCS_REASON_NONE);
		struct bt_audio_codec_qos cqos;
		const struct bt_ascs_qos *qos;
		struct bt_ascs_ase *ase;

		qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

		LOG_DBG("ase 0x%02x", qos->ase);

		if (!is_valid_ase_id(qos->ase)) {
			ascs_cp_rsp_add(qos->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", qos->ase);
			continue;
		}

		ase = ase_find(conn, qos->ase);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ASE");
			ascs_cp_rsp_add(qos->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		cqos.interval = sys_get_le24(qos->interval);
		cqos.framing = qos->framing;
		cqos.phy = qos->phy;
		cqos.sdu = sys_le16_to_cpu(qos->sdu);
		cqos.rtn = qos->rtn;
		cqos.latency = sys_le16_to_cpu(qos->latency);
		cqos.pd = sys_get_le24(qos->pd);

		ase_qos(ase, qos->cig, qos->cis, &cqos, &rsp);
		ascs_cp_rsp_add(qos->ase, rsp.code, rsp.reason);
	}

	return buf->size;
}

struct ascs_parse_result {
	int err;
	struct bt_conn *conn;
	struct bt_bap_ascs_rsp *rsp;
	const struct bt_bap_ep *ep;
};

static bool is_context_available(struct bt_conn *conn, enum bt_audio_dir dir, uint16_t context)
{
	return (context & bt_pacs_get_available_contexts_for_conn(conn, dir)) == context;
}

static bool ascs_parse_metadata(struct bt_data *data, void *user_data)
{
	struct ascs_parse_result *result = user_data;
	const struct bt_bap_ep *ep = result->ep;
	const uint8_t data_len = data->data_len;
	const uint8_t data_type = data->type;
	const uint8_t *data_value = data->data;

	LOG_DBG("type 0x%02x len %u", data_type, data_len);

	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(data_type)) {
		LOG_WRN("Unknown metadata type 0x%02x", data_type);
		return true;
	}

	switch (data_type) {
	/* TODO: Consider rejecting BT_AUDIO_METADATA_TYPE_PREF_CONTEXT type */
	case BT_AUDIO_METADATA_TYPE_PREF_CONTEXT:
	case BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT: {
		uint16_t context;

		if (data_len != sizeof(context)) {
			*result->rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
						       data_type);
			result->err = -EBADMSG;
			return false;
		}

		context = sys_get_le16(data_value);
		if (context == BT_AUDIO_CONTEXT_TYPE_PROHIBITED) {
			*result->rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
						       data_type);
			result->err = -EINVAL;
			return false;
		}

		/* The CAP acceptor shall not accept metadata with unsupported stream context. */
		if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR) &&
		    data_type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
			if (!is_context_available(result->conn, ep->dir, context)) {
				LOG_WRN("Context 0x%04x is unavailable", context);
				*result->rsp = BT_BAP_ASCS_RSP(
					BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data_type);
				result->err = -EACCES;
				return false;
			}
		}

		break;
	}
	case BT_AUDIO_METADATA_TYPE_STREAM_LANG:
		if (data_len != 3) {
			*result->rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
						       data_type);
			result->err = -EBADMSG;
			return false;
		}

		break;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: {
		/* Verify that the CCID is a known CCID on the writing device */
		if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR)) {
			for (uint8_t i = 0; i < data_len; i++) {
				const uint8_t ccid = data_value[i];

				if (!bt_cap_acceptor_ccid_exist(ep->stream->conn, ccid)) {
					LOG_WRN("CCID %u is unknown", ccid);

					/* TBD:
					 * Should we reject the Metadata?
					 *
					 * Should unknown CCIDs trigger a
					 * discovery procedure for TBS or MCS?
					 *
					 * Or should we just accept as is, and
					 * then let the application decide?
					 */
				}
			}
		}

		break;
	}
	case BT_AUDIO_METADATA_TYPE_PARENTAL_RATING:
		if (data_len != 1) {
			*result->rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
						       data_type);
			result->err = -EBADMSG;
			return false;
		}

		break;
	case BT_AUDIO_METADATA_TYPE_AUDIO_STATE: {
		uint8_t state;

		if (data_len != sizeof(state)) {
			*result->rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
						       data_type);
			result->err = -EBADMSG;
			return false;
		}

		break;
	}
	/* TODO: Consider rejecting BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE type */
	case BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE:
		if (data_len != 0) {
			*result->rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
						       data_type);
			result->err = -EBADMSG;
			return false;
		}

		break;
	default:
		break;
	}

	return true;
}

static int ascs_verify_metadata(struct bt_bap_ep *ep, const struct bt_ascs_metadata *meta,
				struct bt_bap_ascs_rsp *rsp)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);
	struct ascs_parse_result result = {
		.conn = ase->conn,
		.rsp = rsp,
		.err = 0,
		.ep = ep,
	};
	int err;

	if (meta->len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
		LOG_WRN("Not enough space for Codec Config Metadata: %u > %d", meta->len,
			CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		return -ENOMEM;
	}

	/* Parse LTV entries */
	err = bt_audio_data_parse(meta->data, meta->len, ascs_parse_metadata, &result);
	if (err != 0 && err != -ECANCELED) {
		/* ECANCELED is called if the callback stops the parsing prematurely, in which case
		 * result.err will be set
		 */
		LOG_DBG("Failed to parse metadata: %d", err);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_INVALID,
				       BT_BAP_ASCS_REASON_NONE);

		return err;
	}

	return result.err;
}

static void ase_metadata(struct bt_ascs_ase *ase, struct bt_ascs_metadata *meta)
{
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	uint8_t state;
	int err;

	LOG_DBG("ase %p meta->len %u", ase, meta->len);

	ep = &ase->ep;
	state = ep->status.state;

	switch (state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				BT_BAP_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;

	err = ascs_verify_metadata(ep, meta, &rsp);
	if (err != 0) {
		LOG_DBG("Invalid metadata from client: %d", err);

		/* rsp will be set by ascs_verify_metadata*/
	} else if (unicast_server_cb != NULL && unicast_server_cb->metadata != NULL) {
		err = unicast_server_cb->metadata(stream, meta->data, meta->len, &rsp);
	} else {
		err = -ENOTSUP;
		rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
				      BT_BAP_ASCS_REASON_NONE);
	}

	if (err) {
		ascs_app_rsp_warn_valid(&rsp);

		if (rsp.code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
			rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					      BT_BAP_ASCS_REASON_NONE);
		}

		LOG_ERR("Metadata failed: err %d, code %u, reason %u", err, rsp.code, rsp.reason);
		ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);
		return;
	}

	ep->codec_cfg.meta_len = meta->len;
	(void)memcpy(ep->codec_cfg.meta, meta->data, meta->len);

	/* Set the state to the same state to trigger the notifications */
	ascs_ep_set_state(ep, ep->status.state);
	ascs_cp_rsp_success(ASE_ID(ase));
}

static int ase_enable(struct bt_ascs_ase *ase, struct bt_ascs_metadata *meta)
{
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	int err;

	LOG_DBG("ase %p meta->len %u", ase, meta->len);

	ep = &ase->ep;

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (ep->status.state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		err = -EBADMSG;
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(ep->status.state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				BT_BAP_ASCS_REASON_NONE);
		return err;
	}

	stream = ep->stream;

	err = ascs_verify_metadata(ep, meta, &rsp);
	if (err != 0) {
		LOG_DBG("Invalid metadata from client: %d", err);

		/* rsp will be set by ascs_verify_metadata*/
	} else if (unicast_server_cb != NULL && unicast_server_cb->enable != NULL) {
		err = unicast_server_cb->enable(stream, meta->data, meta->len, &rsp);
	} else {
		err = -ENOTSUP;
		rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
				      BT_BAP_ASCS_REASON_NONE);
	}

	if (err) {
		ascs_app_rsp_warn_valid(&rsp);

		if (rsp.code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
			rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					      BT_BAP_ASCS_REASON_NONE);
		}

		LOG_ERR("Enable rejected: err %d, code %u, reason %u", err, rsp.code, rsp.reason);
		ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);

		return -EFAULT;
	}

	ep->codec_cfg.meta_len = meta->len;
	(void)memcpy(ep->codec_cfg.meta, meta->data, meta->len);

	ascs_ep_set_state(ep, BT_BAP_EP_STATE_ENABLING);

	ascs_cp_rsp_success(ASE_ID(ase));

	return 0;
}

static bool is_valid_enable_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_enable_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	for (uint8_t i = 0U; i < op->num_ases; i++) {
		const struct bt_ascs_metadata *metadata;

		if (buf->len < sizeof(*metadata)) {
			LOG_WRN("Malformed params array");
			return false;
		}

		metadata = net_buf_simple_pull_mem(buf, sizeof(*metadata));
		if (buf->len < metadata->len) {
			LOG_WRN("Malformed metadata");
			return false;
		}

		(void)net_buf_simple_pull_mem(buf, metadata->len);
	}

	if (buf->len > 0) {
		LOG_WRN("Unexpected data");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_enable(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_enable_op *req;
	struct bt_ascs_metadata *meta;
	int i;

	if (!is_valid_enable_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		meta = net_buf_simple_pull_mem(buf, sizeof(*meta));
		(void)net_buf_simple_pull(buf, meta->len);

		if (!is_valid_ase_id(meta->ase)) {
			ascs_cp_rsp_add(meta->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", meta->ase);
			continue;
		}

		ase = ase_find(conn, meta->ase);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ase 0x%02x", meta->ase);
			ascs_cp_rsp_add(meta->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase_enable(ase, meta);
	}

	return buf->size;
}

static void ase_start(struct bt_ascs_ase *ase)
{
	struct bt_bap_ep *ep;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	int err;

	LOG_DBG("ase %p", ase);

	ep = &ase->ep;

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (ep->status.state != BT_BAP_EP_STATE_ENABLING) {
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(ep->status.state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				BT_BAP_ASCS_REASON_NONE);
		return;
	}

	if (ep->iso->chan.state != BT_ISO_STATE_CONNECTED) {
		/* An ASE may not go into the streaming state unless the CIS
		 * is connected
		 */
		LOG_WRN("Start failed: CIS not connected: %u",
			ep->iso->chan.state);
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				BT_BAP_ASCS_REASON_NONE);
		return;
	}

	if (unicast_server_cb != NULL && unicast_server_cb->start != NULL) {
		err = unicast_server_cb->start(ep->stream, &rsp);
	} else {
		err = -ENOTSUP;
		rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
				      BT_BAP_ASCS_REASON_NONE);
	}

	if (err) {
		ascs_app_rsp_warn_valid(&rsp);

		if (rsp.code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
			rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					      BT_BAP_ASCS_REASON_NONE);
		}

		LOG_ERR("Start failed: err %d, code %u, reason %u", err, rsp.code, rsp.reason);
		ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);

		return;
	}

	ep->receiver_ready = true;

	ascs_ep_set_state(ep, BT_BAP_EP_STATE_STREAMING);

	ascs_cp_rsp_success(ASE_ID(ase));
}

static bool is_valid_start_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	if (buf->len != op->num_ases) {
		LOG_WRN("Number_of_ASEs mismatch");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_start(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *req;
	int i;

	if (!is_valid_start_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		LOG_DBG("ase 0x%02x", id);

		if (!is_valid_ase_id(id)) {
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", id);
			continue;
		}

		/* If the ASE_ID  written by the client represents a Sink ASE, the
		 * server shall not accept the Receiver Start Ready operation for that
		 * ASE. The server shall send a notification of the ASE Control Point
		 * characteristic to the client, and the server shall set the
		 * Response_Code value for that ASE to 0x05 (Invalid ASE direction).
		 */
		if (ASE_DIR(id) == BT_AUDIO_DIR_SINK) {
			LOG_WRN("Start failed: invalid operation for Sink");
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_DIR,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase = ase_find(conn, id);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ASE");
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase_start(ase);
	}

	return buf->size;
}

static bool is_valid_disable_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_disable_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	if (buf->len != op->num_ases) {
		LOG_WRN("Number_of_ASEs mismatch");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_disable(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_disable_op *req;

	if (!is_valid_disable_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (uint8_t i = 0; i < req->num_ases; i++) {
		struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
							     BT_BAP_ASCS_REASON_NONE);
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		LOG_DBG("ase 0x%02x", id);

		if (!is_valid_ase_id(id)) {
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", id);
			continue;
		}

		ase = ase_find(conn, id);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ASE");
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase_disable(ase, BT_HCI_ERR_REMOTE_USER_TERM_CONN, &rsp);
		ascs_cp_rsp_add(id, rsp.code, rsp.reason);
	}

	return buf->size;
}

static void ase_stop(struct bt_ascs_ase *ase)
{
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	int err;

	LOG_DBG("ase %p", ase);

	ep = &ase->ep;

	if (ep->status.state != BT_BAP_EP_STATE_DISABLING) {
		LOG_WRN("Invalid operation in state: %s", bt_bap_ep_state_str(ep->status.state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
				BT_BAP_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;
	if (unicast_server_cb != NULL && unicast_server_cb->stop != NULL) {
		err = unicast_server_cb->stop(stream, &rsp);
	} else {
		err = -ENOTSUP;
		rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
				      BT_BAP_ASCS_REASON_NONE);
	}

	if (err) {
		ascs_app_rsp_warn_valid(&rsp);

		if (rsp.code == BT_BAP_ASCS_RSP_CODE_SUCCESS) {
			rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
					      BT_BAP_ASCS_REASON_NONE);
		}

		LOG_ERR("Stop failed: err %d, code %u, reason %u", err, rsp.code, rsp.reason);
		ascs_cp_rsp_add(ASE_ID(ase), rsp.code, rsp.reason);
		return;
	}

	/* If the Receiver Stop Ready operation has completed successfully the
	 * Unicast Client or the Unicast Server may terminate a CIS established
	 * for that ASE by following the Connected Isochronous Stream Terminate
	 * procedure defined in Volume 3, Part C, Section 9.3.15.
	 */
	if (bt_bap_stream_can_disconnect(stream)) {
		err = ascs_disconnect_stream(stream);
		if (err < 0) {
			LOG_ERR("Failed to disconnect stream %p: %d", stream, err);
		}
	}

	ascs_ep_set_state(ep, BT_BAP_EP_STATE_QOS_CONFIGURED);

	ascs_cp_rsp_success(ASE_ID(ase));
}

static bool is_valid_stop_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_stop_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	if (buf->len != op->num_ases) {
		LOG_WRN("Number_of_ASEs mismatch");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_stop(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *req;
	int i;

	if (!is_valid_stop_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		LOG_DBG("ase 0x%02x", id);

		if (!is_valid_ase_id(id)) {
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", id);
			continue;
		}

		/* If the ASE_ID  written by the client represents a Sink ASE, the
		 * server shall not accept the Receiver Stop Ready operation for that
		 * ASE. The server shall send a notification of the ASE Control Point
		 * characteristic to the client, and the server shall set the
		 * Response_Code value for that ASE to 0x05 (Invalid ASE direction).
		 */
		if (ASE_DIR(id) == BT_AUDIO_DIR_SINK) {
			LOG_WRN("Stop failed: invalid operation for Sink");
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_DIR,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase = ase_find(conn, id);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ASE");
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase_stop(ase);
	}

	return buf->size;
}

static bool is_valid_metadata_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_metadata_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	for (uint8_t i = 0U; i < op->num_ases; i++) {
		const struct bt_ascs_metadata *metadata;

		if (buf->len < sizeof(*metadata)) {
			LOG_WRN("Malformed params array");
			return false;
		}

		metadata = net_buf_simple_pull_mem(buf, sizeof(*metadata));
		if (buf->len < metadata->len) {
			LOG_WRN("Malformed metadata");
			return false;
		}

		(void)net_buf_simple_pull_mem(buf, metadata->len);
	}

	if (buf->len > 0) {
		LOG_WRN("Unexpected data");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_metadata(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_metadata_op *req;
	struct bt_ascs_metadata *meta;
	int i;

	if (!is_valid_metadata_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		meta = net_buf_simple_pull_mem(buf, sizeof(*meta));
		(void)net_buf_simple_pull(buf, meta->len);

		if (meta->len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_DBG("Cannot store %u octets of metadata", meta->len);

			ascs_cp_rsp_add(meta->ase, BT_BAP_ASCS_RSP_CODE_NO_MEM,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		if (!is_valid_ase_id(meta->ase)) {
			ascs_cp_rsp_add(meta->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", meta->ase);
			continue;
		}

		ase = ase_find(conn, meta->ase);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ase 0x%02x", meta->ase);
			ascs_cp_rsp_add(meta->ase, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase_metadata(ase, meta);
	}

	return buf->size;
}

static bool is_valid_release_len(struct net_buf_simple *buf)
{
	const struct bt_ascs_release_op *op;
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (buf->len < sizeof(*op)) {
		LOG_WRN("Invalid length %u < %zu", buf->len, sizeof(*op));
		return false;
	}

	op = net_buf_simple_pull_mem(buf, sizeof(*op));
	if (op->num_ases < 1) {
		LOG_WRN("Number_of_ASEs parameter value is less than 1");
		return false;
	}

	if (buf->len != op->num_ases) {
		LOG_WRN("Number_of_ASEs mismatch");
		return false;
	}

	net_buf_simple_restore(buf, &state);

	return true;
}

static ssize_t ascs_release(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_ascs_release_op *req;
	int i;

	if (!is_valid_release_len(buf)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	LOG_DBG("num_ases %u", req->num_ases);

	for (i = 0; i < req->num_ases; i++) {
		struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_UNSPECIFIED,
							     BT_BAP_ASCS_REASON_NONE);
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		LOG_DBG("ase 0x%02x", id);

		if (!is_valid_ase_id(id)) {
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE,
					BT_BAP_ASCS_REASON_NONE);
			LOG_WRN("Unknown ase 0x%02x", id);
			continue;
		}

		ase = ase_find(conn, id);
		if (!ase) {
			LOG_DBG("Invalid operation for idle ASE");
			ascs_cp_rsp_add(id, BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE,
					BT_BAP_ASCS_REASON_NONE);
			continue;
		}

		ase_release(ase, BT_HCI_ERR_REMOTE_USER_TERM_CONN, &rsp);
		ascs_cp_rsp_add(id, rsp.code, rsp.reason);
	}

	return buf->size;
}

static ssize_t ascs_cp_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	const struct bt_ascs_ase_cp *req;
	struct net_buf_simple buf;
	ssize_t ret;

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		/* Return 0 to allow long writes */
		return 0;
	}

	if (offset != 0 && (flags & BT_GATT_WRITE_FLAG_EXECUTE) == 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&buf, (void *) data, len);

	req = net_buf_simple_pull_mem(&buf, sizeof(*req));

	LOG_DBG("conn %p attr %p buf %p len %u op %s (0x%02x)",
		(void *)conn, attr, data, len, bt_ascs_op_str(req->op), req->op);

	ascs_cp_rsp_init(req->op);

	switch (req->op) {
	case BT_ASCS_CONFIG_OP:
		ret = ascs_config(conn, &buf);
		break;
	case BT_ASCS_QOS_OP:
		ret = ascs_qos(conn, &buf);
		break;
	case BT_ASCS_ENABLE_OP:
		ret = ascs_enable(conn, &buf);
		break;
	case BT_ASCS_START_OP:
		ret = ascs_start(conn, &buf);
		break;
	case BT_ASCS_DISABLE_OP:
		ret = ascs_disable(conn, &buf);
		break;
	case BT_ASCS_STOP_OP:
		ret = ascs_stop(conn, &buf);
		break;
	case BT_ASCS_METADATA_OP:
		ret = ascs_metadata(conn, &buf);
		break;
	case BT_ASCS_RELEASE_OP:
		ret = ascs_release(conn, &buf);
		break;
	default:
		ascs_cp_rsp_add(BT_ASCS_ASE_ID_NONE, BT_BAP_ASCS_RSP_CODE_NOT_SUPPORTED,
				BT_BAP_ASCS_REASON_NONE);
		LOG_DBG("Unknown opcode");
		goto respond;
	}

	if (ret == BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN)) {
		ascs_cp_rsp_add(BT_ASCS_ASE_ID_NONE, BT_BAP_ASCS_RSP_CODE_INVALID_LENGTH,
				BT_BAP_ASCS_REASON_NONE);
	}

respond:
	control_point_notify(conn, rsp_buf.data, rsp_buf.len);

	return len;
}

#define BT_ASCS_ASE_DEFINE(_uuid, _id) \
	BT_AUDIO_CHRC(_uuid, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      ascs_ase_read, NULL, UINT_TO_POINTER(_id)), \
	BT_AUDIO_CCC(ascs_ase_cfg_changed)
#define BT_ASCS_ASE_SNK_DEFINE(_n, ...) BT_ASCS_ASE_DEFINE(BT_UUID_ASCS_ASE_SNK, (_n) + 1)
#define BT_ASCS_ASE_SRC_DEFINE(_n, ...) BT_ASCS_ASE_DEFINE(BT_UUID_ASCS_ASE_SRC, (_n) + 1 + \
							   CONFIG_BT_ASCS_ASE_SNK_COUNT)

BT_GATT_SERVICE_DEFINE(ascs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ASCS),
	BT_AUDIO_CHRC(BT_UUID_ASCS_ASE_CP,
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_PREPARE_WRITE,
		      NULL, ascs_cp_write, NULL),
	BT_AUDIO_CCC(ascs_cp_cfg_changed),
#if CONFIG_BT_ASCS_ASE_SNK_COUNT > 0
	LISTIFY(CONFIG_BT_ASCS_ASE_SNK_COUNT, BT_ASCS_ASE_SNK_DEFINE, (,)),
#endif /* CONFIG_BT_ASCS_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_ASCS_ASE_SRC_COUNT > 0
	LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT, BT_ASCS_ASE_SRC_DEFINE, (,)),
#endif /* CONFIG_BT_ASCS_ASE_SRC_COUNT > 0 */
);

static int control_point_notify(struct bt_conn *conn, const void *data, uint16_t len)
{
	return bt_gatt_notify_uuid(conn, BT_UUID_ASCS_ASE_CP, ascs_svc.attrs, data, len);
}

static struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L2,
	.accept = ascs_iso_accept,
};

int bt_ascs_init(const struct bt_bap_unicast_server_cb *cb)
{
	int err;

	if (unicast_server_cb != NULL) {
		return -EALREADY;
	}

	err = bt_iso_server_register(&iso_server);
	if (err) {
		LOG_ERR("Failed to register ISO server %d", err);
		return err;
	}

	unicast_server_cb = cb;

	return 0;
}

void bt_ascs_cleanup(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ase_pool); i++) {
		struct bt_ascs_ase *ase = &ase_pool[i];

		if (ase->conn != NULL) {
			bt_ascs_release_ase(&ase->ep);
		}
	}

	if (unicast_server_cb != NULL) {
		bt_iso_server_unregister(&iso_server);
		unicast_server_cb = NULL;
	}
}
#endif /* BT_BAP_UNICAST_SERVER */
