/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include <esp_hosted_mcu_rpc.pb.h>

#include "esp_hosted_mcu.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_mcu_core, CONFIG_ESP_HOSTED_MCU_LOG_LEVEL);

#if defined(CONFIG_ESP_HOSTED_MCU_SDIO)
#define DT_DRV_COMPAT espressif_esp_hosted_mcu_sdio
#else
#define DT_DRV_COMPAT espressif_esp_hosted_mcu
#endif

/*
 * The active transport backend supplies its api and per-instance config;
 * both are opaque to the core, which only forwards their addresses.
 */
#if defined(CONFIG_ESP_HOSTED_MCU_SDIO)
extern const struct esp_hosted_mcu_transport_api esp_hosted_mcu_sdio_api;
struct esp_hosted_mcu_sdio_config;
extern const struct esp_hosted_mcu_sdio_config esp_hosted_mcu_sdio_config_0;
#define ESP_HOSTED_MCU_TRANSPORT_API    (&esp_hosted_mcu_sdio_api)
#define ESP_HOSTED_MCU_TRANSPORT_CONFIG (&esp_hosted_mcu_sdio_config_0)
#else
extern const struct esp_hosted_mcu_transport_api esp_hosted_mcu_spi_api;
struct esp_hosted_mcu_spi_config;
extern const struct esp_hosted_mcu_spi_config esp_hosted_mcu_spi_config_0;
#define ESP_HOSTED_MCU_TRANSPORT_API    (&esp_hosted_mcu_spi_api)
#define ESP_HOSTED_MCU_TRANSPORT_CONFIG (&esp_hosted_mcu_spi_config_0)
#endif

/* A response msg_id is its request msg_id shifted by this fixed offset. */
#define ESP_HOSTED_MCU_REQ_TO_RESP_ID 256

/* Max frames drained per receive wake before yielding back to the system. */
#define ESP_HOSTED_MCU_RX_BURST 16

/*
 * Core per-instance state. Owns the transport link, the RPC round-trip
 * machinery and the consumer dispatch table. A single instance is supported.
 */
struct esp_hosted_mcu_data {
	const struct device *dev;
	uint16_t seq_num;
	uint32_t rpc_uid;
	struct k_sem rpc_sem;
	int rpc_resp_status;
	k_tid_t tid;
	uint32_t fw_version;
	bool rx_stride_aligned;
	esp_hosted_mcu_rx_cb_t if_cb[ESP_HOSTED_MCU_MAX_IF];
	void *if_user[ESP_HOSTED_MCU_MAX_IF];
	esp_hosted_mcu_event_cb_t rpc_event_cb;
	void *rpc_event_user;
};

/* One in-flight RPC at a time; the caller thread owns the round-trip. */
static K_MUTEX_DEFINE(esp_hosted_mcu_rpc_lock);

/*
 * Serializes the shared transmit path. The frame is assembled into a single
 * static scratch buffer and the sequence counter is bumped, so the Wi-Fi and
 * Bluetooth consumers (and the control-plane RPC) must take turns building and
 * pushing a frame. Held only across assembly and the transfer, never across a
 * blocking wait, so it does not couple to the RPC round-trip.
 */
static K_MUTEX_DEFINE(esp_hosted_mcu_tx_lock);

/*
 * The event thread decodes an incoming Rpc and, when it matches the pending
 * request uid, copies the response into the requester's scratch and posts the
 * semaphore. Events (msg_type == Event) are handled inline instead.
 */
struct esp_hosted_mcu_rpc_ctx {
	uint32_t uid;
	int32_t expected_msg_id;
	Rpc *resp;
	bool pending;
};

static struct esp_hosted_mcu_rpc_ctx esp_hosted_mcu_rpc;

/*
 * Guards the shared RPC context against the event task. The rpc_lock mutex
 * above serialises callers; this spinlock makes the arm, timeout teardown and
 * the event-task dispatch match/consume mutually exclusive.
 */
static struct k_spinlock esp_hosted_mcu_rpc_ctx_lock;

static struct esp_hosted_mcu_data esp_hosted_mcu_data_0;

static const struct esp_hosted_mcu_config esp_hosted_mcu_config_0 = {
	.transport = ESP_HOSTED_MCU_TRANSPORT_API,
	.transport_config = ESP_HOSTED_MCU_TRANSPORT_CONFIG,
};

const struct device *esp_hosted_mcu_get_dev(void)
{
	return esp_hosted_mcu_data_0.dev;
}

int esp_hosted_mcu_register_if(esp_hosted_mcu_if_type_t if_type, esp_hosted_mcu_rx_cb_t cb,
			       void *user_data)
{
	if (if_type <= ESP_HOSTED_MCU_INVALID_IF || if_type >= ESP_HOSTED_MCU_MAX_IF) {
		return -EINVAL;
	}

	/*
	 * Publish the user pointer before the callback. The receive thread reads
	 * the callback first and only then the user pointer, so this ordering
	 * guarantees it never observes a live callback with a stale user pointer.
	 */
	esp_hosted_mcu_data_0.if_user[if_type] = user_data;
	compiler_barrier();
	esp_hosted_mcu_data_0.if_cb[if_type] = cb;
	return 0;
}

int esp_hosted_mcu_unregister_if(esp_hosted_mcu_if_type_t if_type)
{
	if (if_type <= ESP_HOSTED_MCU_INVALID_IF || if_type >= ESP_HOSTED_MCU_MAX_IF) {
		return -EINVAL;
	}

	/* Clear the callback first so the receive thread stops dispatching. */
	esp_hosted_mcu_data_0.if_cb[if_type] = NULL;
	compiler_barrier();
	esp_hosted_mcu_data_0.if_user[if_type] = NULL;
	return 0;
}

int esp_hosted_mcu_register_rpc_event(esp_hosted_mcu_event_cb_t cb, void *user_data)
{
	/* Publish the user pointer before the callback, as in register_if. */
	esp_hosted_mcu_data_0.rpc_event_user = user_data;
	compiler_barrier();
	esp_hosted_mcu_data_0.rpc_event_cb = cb;
	return 0;
}

/*
 * Assemble a framed buffer: payload header, endpoint TLV wrapper, then the
 * encoded Rpc protobuf. Returns the total framed length, or negative on error.
 */
static int esp_hosted_mcu_frame_rpc(struct esp_hosted_mcu_data *data, const char *ep, Rpc *rpc,
				    uint8_t *out, size_t out_size)
{
	struct esp_hosted_mcu_header *hdr = (struct esp_hosted_mcu_header *)out;
	struct esp_hosted_mcu_tlv *tlv =
		(struct esp_hosted_mcu_tlv *)(out + ESP_HOSTED_MCU_HEADER_SIZE);
	size_t proto_size = 0;

	if (!pb_get_encoded_size(&proto_size, Rpc_fields, rpc)) {
		return -EINVAL;
	}

	size_t tlv_size = sizeof(struct esp_hosted_mcu_tlv) + proto_size;
	size_t total = ESP_HOSTED_MCU_HEADER_SIZE + tlv_size;

	if (total > out_size || total > ESP_HOSTED_MCU_FRAME_SIZE) {
		return -ENOMEM;
	}

	memset(out, 0, total);

	tlv->ep_type = ESP_HOSTED_MCU_TLV_EP;
	tlv->ep_length = ESP_HOSTED_MCU_EP_NAME_LEN;
	memcpy(tlv->ep_value, ep, ESP_HOSTED_MCU_EP_NAME_LEN);
	tlv->data_type = ESP_HOSTED_MCU_TLV_DATA;
	tlv->data_length = proto_size;

	pb_ostream_t stream = pb_ostream_from_buffer(tlv->data_value, proto_size);

	if (!pb_encode(&stream, Rpc_fields, rpc)) {
		return -EINVAL;
	}

	hdr->if_type = ESP_HOSTED_MCU_SERIAL_IF;
	hdr->if_num = 0;
	hdr->offset = ESP_HOSTED_MCU_HEADER_SIZE;
	hdr->len = tlv_size;
	hdr->seq_num = ++data->seq_num;
	hdr->checksum = 0;
	hdr->checksum = esp_hosted_mcu_checksum(out, total);

	return (int)total;
}

int esp_hosted_mcu_rpc_call(Rpc *req, Rpc *resp, int32_t timeout)
{
	const struct device *dev = esp_hosted_mcu_data_0.dev;
	const struct esp_hosted_mcu_config *cfg = dev->config;
	struct esp_hosted_mcu_data *data = dev->data;
	static uint8_t tx[ESP_HOSTED_MCU_FRAME_SIZE];
	int ret;

	k_mutex_lock(&esp_hosted_mcu_rpc_lock, K_FOREVER);

	req->msg_type = RpcType_Req;
	req->uid = ++data->rpc_uid;

	memset(resp, 0, sizeof(*resp));
	k_sem_reset(&data->rpc_sem);

	K_SPINLOCK(&esp_hosted_mcu_rpc_ctx_lock) {
		esp_hosted_mcu_rpc.uid = req->uid;
		esp_hosted_mcu_rpc.expected_msg_id = req->msg_id + ESP_HOSTED_MCU_REQ_TO_RESP_ID;
		esp_hosted_mcu_rpc.resp = resp;
		esp_hosted_mcu_rpc.pending = true;
	}

	/*
	 * Assemble and push the request under the shared transmit lock so the
	 * sequence counter and the bus are not touched by a concurrent data
	 * frame. Release it before waiting for the response; the wait must not
	 * block another consumer's transmit.
	 */
	k_mutex_lock(&esp_hosted_mcu_tx_lock, K_FOREVER);
	int len = esp_hosted_mcu_frame_rpc(data, ESP_HOSTED_MCU_EP_RSP, req, tx, sizeof(tx));

	if (len < 0) {
		ret = len;
	} else {
		ret = cfg->transport->transfer(dev, tx, len, NULL);
	}
	k_mutex_unlock(&esp_hosted_mcu_tx_lock);

	if (ret < 0) {
		K_SPINLOCK(&esp_hosted_mcu_rpc_ctx_lock) {
			esp_hosted_mcu_rpc.pending = false;
		}
		goto out;
	}

	if (k_sem_take(&data->rpc_sem, K_MSEC(timeout)) != 0) {
		K_SPINLOCK(&esp_hosted_mcu_rpc_ctx_lock) {
			esp_hosted_mcu_rpc.pending = false;
		}
		LOG_ERR("RPC msg_id %d timed out", req->msg_id);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = data->rpc_resp_status;

out:
	k_mutex_unlock(&esp_hosted_mcu_rpc_lock);
	return ret;
}

/* Pull the int32 "resp" (esp_err_t) field common to every Rpc_Resp_* body. */
static int esp_hosted_mcu_resp_status(const Rpc *resp)
{
	switch (resp->msg_id) {
	case RpcId_Resp_WifiInit:
		return resp->payload.resp_wifi_init.resp;
	case RpcId_Resp_WifiStart:
		return resp->payload.resp_wifi_start.resp;
	case RpcId_Resp_WifiStop:
		return resp->payload.resp_wifi_stop.resp;
	case RpcId_Resp_WifiSetConfig:
		return resp->payload.resp_wifi_set_config.resp;
	case RpcId_Resp_WifiConnect:
		return resp->payload.resp_wifi_connect.resp;
	case RpcId_Resp_WifiDisconnect:
		return resp->payload.resp_wifi_disconnect.resp;
	case RpcId_Resp_WifiScanStart:
		return resp->payload.resp_wifi_scan_start.resp;
	case RpcId_Resp_WifiScanGetApNum:
		return resp->payload.resp_wifi_scan_get_ap_num.resp;
	case RpcId_Resp_WifiScanGetApRecords:
		return resp->payload.resp_wifi_scan_get_ap_records.resp;
	case RpcId_Resp_IfaceMacAddrSetGet:
		return resp->payload.resp_iface_mac_addr_set_get.resp;
	case RpcId_Resp_SetWifiMode:
		return resp->payload.resp_set_wifi_mode.resp;
	case RpcId_Resp_GetWifiMode:
		return resp->payload.resp_get_wifi_mode.resp;
	case RpcId_Resp_WifiStaGetApInfo:
		return resp->payload.resp_wifi_sta_get_ap_info.resp;
	case RpcId_Resp_WifiSetChannel:
		return resp->payload.resp_wifi_set_channel.resp;
	case RpcId_Resp_WifiGetChannel:
		return resp->payload.resp_wifi_get_channel.resp;
	case RpcId_Resp_WifiSetPs:
		return resp->payload.resp_wifi_set_ps.resp;
	case RpcId_Resp_WifiGetPs:
		return resp->payload.resp_wifi_get_ps.resp;
	case RpcId_Resp_WifiDeauthSta:
		return resp->payload.resp_wifi_deauth_sta.resp;
	case RpcId_Resp_WifiApGetStaAid:
		return resp->payload.resp_wifi_ap_get_sta_aid.resp;
	case RpcId_Resp_FeatureControl:
		return resp->payload.resp_feature_control.resp;
	default:
		return 0;
	}
}

/* Decode one RPC blob out of a received SERIAL frame and dispatch it. */
static void esp_hosted_mcu_dispatch_serial(struct esp_hosted_mcu_data *data,
					   const struct esp_hosted_mcu_tlv *tlv,
					   uint16_t payload_len)
{
	const uint16_t tlv_hdr = offsetof(struct esp_hosted_mcu_tlv, data_value);
	Rpc rpc = Rpc_init_zero;
	pb_istream_t stream;

	/*
	 * The coprocessor controls the frame contents, so bound the inner TLV against
	 * the validated payload length before handing data_value/data_length to
	 * the decoder. Otherwise a short frame with a large data_length would
	 * read past the receive buffer.
	 */
	if (payload_len < tlv_hdr || tlv->data_length > payload_len - tlv_hdr) {
		LOG_ERR("Malformed SERIAL frame (len %u, tlv %u)", payload_len, tlv->data_length);
		return;
	}

	stream = pb_istream_from_buffer(tlv->data_value, tlv->data_length);

	if (!pb_decode(&stream, Rpc_fields, &rpc)) {
		LOG_ERR("RPC decode failed");
		return;
	}

	if (rpc.msg_type == RpcType_Event) {
		if (data->rpc_event_cb != NULL) {
			data->rpc_event_cb(&rpc, data->rpc_event_user);
		}
		/* The event was consumed inline; free any decoded pointer fields. */
		pb_release(Rpc_fields, &rpc);
		return;
	}

	if (rpc.msg_type != RpcType_Resp) {
		pb_release(Rpc_fields, &rpc);
		return;
	}

	bool claimed = false;

	K_SPINLOCK(&esp_hosted_mcu_rpc_ctx_lock) {
		if (esp_hosted_mcu_rpc.pending && rpc.uid == esp_hosted_mcu_rpc.uid &&
		    rpc.msg_id == esp_hosted_mcu_rpc.expected_msg_id) {
			/* Ownership of the decoded message moves to the waiter. */
			*esp_hosted_mcu_rpc.resp = rpc;
			data->rpc_resp_status = esp_hosted_mcu_resp_status(&rpc);
			esp_hosted_mcu_rpc.pending = false;
			claimed = true;
			k_sem_give(&data->rpc_sem);
		}
	}

	/* An unmatched response owns its decoded fields and must free them. */
	if (!claimed) {
		pb_release(Rpc_fields, &rpc);
	}
}

/*
 * Parse the boot-time priv event. Its body is a stream of {tag, length,
 * value} triplets; only the firmware version is consumed, every other tag
 * is skipped by its own length. This is the fallback source for firmware
 * too old to answer the version RPC, so it only fills in a version that
 * has not been learned already.
 */
static void esp_hosted_mcu_priv_event(struct esp_hosted_mcu_data *data, const uint8_t *payload,
				      uint16_t len)
{
	const struct esp_hosted_mcu_priv_event *ev =
		(const struct esp_hosted_mcu_priv_event *)payload;
	const uint16_t hdr_len = sizeof(struct esp_hosted_mcu_priv_event);
	uint16_t off = 0;
	uint16_t body;

	if (len < hdr_len) {
		return;
	}

	LOG_INF("coprocessor priv event %u", ev->event_type);

	/* The coprocessor owns event_len, so bound it by what was received. */
	body = MIN(ev->event_len, len - hdr_len);

	while (body - off >= 2) {
		uint8_t tag = ev->event_data[off];
		uint8_t tag_len = ev->event_data[off + 1];

		off += 2;
		if (tag_len > body - off) {
			return;
		}

		if (tag == ESP_HOSTED_MCU_PRIV_TAG_FIRMWARE_VERSION &&
		    tag_len == sizeof(uint32_t) && data->fw_version == 0) {
			data->fw_version = sys_get_le32(&ev->event_data[off]);
		} else if (tag == ESP_HOSTED_MCU_PRIV_TAG_SDIO_BUF_CONFIG &&
			   tag_len == sizeof(struct esp_hosted_mcu_sdio_buf_config)) {
			const struct esp_hosted_mcu_sdio_buf_config *cfg =
				(const struct esp_hosted_mcu_sdio_buf_config *)&ev->event_data[off];

			/*
			 * The coprocessor-to-host mode decides how received frames are
			 * packed. In SW_AGGR each frame is padded to four bytes, so the
			 * receive walk must advance by the aligned stride to find the
			 * next header. STREAM and PACKET stay byte-tight.
			 */
			data->rx_stride_aligned =
				(cfg->e2h_mode == ESP_HOSTED_MCU_SDIO_TX_MODE_SW_AGGR);
			LOG_INF("coprocessor SDIO e2h mode %u", cfg->e2h_mode);
		}

		off += tag_len;
	}
}

static void esp_hosted_mcu_process_one(struct esp_hosted_mcu_data *data,
				       const struct esp_hosted_mcu_header *hdr)
{
	const uint8_t *payload = (const uint8_t *)hdr + hdr->offset;

	/*
	 * The coprocessor sends frames with the header checksum field left zero, so
	 * the receive path does not validate it. The transmit path still fills
	 * it in because the coprocessor verifies host-to-coprocessor frames.
	 */

	switch (hdr->if_type) {
	case ESP_HOSTED_MCU_SERIAL_IF:
		esp_hosted_mcu_dispatch_serial(data, (const struct esp_hosted_mcu_tlv *)payload,
					       hdr->len);
		break;
	case ESP_HOSTED_MCU_STA_IF:
	case ESP_HOSTED_MCU_AP_IF:
	case ESP_HOSTED_MCU_HCI_IF:
		if (data->if_cb[hdr->if_type] != NULL) {
			data->if_cb[hdr->if_type](payload, hdr->len, hdr->hci_pkt_type,
						  data->if_user[hdr->if_type]);
		}
		break;
	case ESP_HOSTED_MCU_PRIV_IF:
		if (hdr->priv_pkt_type == ESP_HOSTED_MCU_PACKET_TYPE_EVENT) {
			esp_hosted_mcu_priv_event(data, payload, hdr->len);
		}
		break;
	default:
		LOG_DBG("unhandled if_type %u", hdr->if_type);
		break;
	}
}

/*
 * A single transport read may carry several concatenated frames. Walk the
 * buffer frame by frame, each starting with its own payload header, until
 * the bytes are consumed or a header no longer fits.
 *
 * Returns the number of bytes consumed. A trailing partial frame is left
 * unconsumed so the caller can carry it over and complete it with the next
 * read: a transport that caps a read at the frame size can split an
 * aggregated batch mid-frame, and dropping the remainder would restart the
 * next walk in the middle of a payload.
 */
static uint16_t esp_hosted_mcu_process_frame(struct esp_hosted_mcu_data *data, const uint8_t *buf,
					     uint16_t len)
{
	uint16_t off = 0;

	while (len - off >= ESP_HOSTED_MCU_HEADER_SIZE) {
		const struct esp_hosted_mcu_header *hdr =
			(const struct esp_hosted_mcu_header *)(buf + off);
		uint32_t frame_len = (uint32_t)hdr->offset + hdr->len;

		if (hdr->len == 0 || hdr->len > ESP_HOSTED_MCU_MAX_PAYLOAD ||
		    hdr->offset != ESP_HOSTED_MCU_HEADER_SIZE) {
			/*
			 * The header itself is not usable, so no resync point
			 * can be derived from it. Drop what is left.
			 */
			return len;
		}
		if (off + frame_len > len) {
			/* Partial frame: hand the tail back to the caller. */
			return off;
		}

		esp_hosted_mcu_process_one(data, hdr);

		/*
		 * When the coprocessor aggregates frames it pads each one to a
		 * four-byte boundary, so step to the next aligned header. A
		 * byte-tight coprocessor sets no padding and the stride equals
		 * the frame length.
		 */
		if (data->rx_stride_aligned) {
			frame_len = ROUND_UP(frame_len, 4);
		}
		off += frame_len;
	}

	return off;
}

static void esp_hosted_mcu_event_task(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct esp_hosted_mcu_config *cfg = dev->config;
	struct esp_hosted_mcu_data *data = dev->data;
	/*
	 * Sized for one full read plus a carried-over partial frame, so a
	 * batch split by the transport can be rejoined in place.
	 */
	static uint8_t rx[2 * ESP_HOSTED_MCU_FRAME_SIZE];
	uint16_t carry = 0;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		if (!cfg->transport->data_ready(dev)) {
			/*
			 * Block on the transport's data-ready interrupt so a
			 * queued frame wakes the task immediately. The poll
			 * interval bounds the wait so a missed interrupt still
			 * falls back to polling and cannot wedge the receiver.
			 */
			if (cfg->transport->wait_for_rx != NULL) {
				cfg->transport->wait_for_rx(
					dev, CONFIG_ESP_HOSTED_MCU_EVENT_TASK_POLL_MS);
			} else {
				k_msleep(CONFIG_ESP_HOSTED_MCU_EVENT_TASK_POLL_MS);
			}
			continue;
		}

		/*
		 * Drain the frames the coprocessor has queued before sleeping again.
		 * Polling one frame per tick cannot keep up with a burst and lets
		 * the coprocessor's receive buffers overrun; keep reading while data is
		 * ready so TCP and bulk transfers stay responsive. Cap the burst
		 * and yield between frames so a sustained flood cannot monopolise
		 * the CPU and starve the network and transmit threads.
		 */
		for (int i = 0; i < ESP_HOSTED_MCU_RX_BURST; i++) {
			int len = cfg->transport->transfer(dev, NULL, 0, rx + carry);
			uint16_t total;
			uint16_t done;

			if (len <= 0) {
				break;
			}

			total = carry + (uint16_t)len;
			done = esp_hosted_mcu_process_frame(data, rx, total);

			/*
			 * Keep an unfinished trailing frame and prepend it to
			 * the next read. A carry that cannot grow any further
			 * is unrecoverable, so drop it rather than wedge the
			 * walk on a frame that will never complete.
			 */
			carry = total - done;
			if (carry > ESP_HOSTED_MCU_FRAME_SIZE) {
				LOG_WRN("dropping %u byte partial frame", carry);
				carry = 0;
			} else if (carry != 0) {
				memmove(rx, rx + done, carry);
			}

			k_yield();

			if (!cfg->transport->data_ready(dev)) {
				break;
			}
		}
	}
}

int esp_hosted_mcu_send_frame(esp_hosted_mcu_if_type_t if_type, uint8_t if_num, uint8_t pkt_type,
			      const uint8_t *payload, uint16_t len)
{
	const struct device *dev = esp_hosted_mcu_data_0.dev;
	const struct esp_hosted_mcu_config *cfg = dev->config;
	struct esp_hosted_mcu_data *data = dev->data;
	static uint8_t tx[ESP_HOSTED_MCU_FRAME_SIZE];
	struct esp_hosted_mcu_header *hdr = (struct esp_hosted_mcu_header *)tx;
	uint16_t total = ESP_HOSTED_MCU_HEADER_SIZE + len;
	int ret;

	if (len > ESP_HOSTED_MCU_MAX_PAYLOAD) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&esp_hosted_mcu_tx_lock, K_FOREVER);

	memset(hdr, 0, ESP_HOSTED_MCU_HEADER_SIZE);
	hdr->if_type = if_type;
	hdr->if_num = if_num;
	hdr->offset = ESP_HOSTED_MCU_HEADER_SIZE;
	hdr->len = len;
	hdr->hci_pkt_type = pkt_type;
	hdr->seq_num = ++data->seq_num;
	memcpy(tx + ESP_HOSTED_MCU_HEADER_SIZE, payload, len);
	hdr->checksum = 0;
	hdr->checksum = esp_hosted_mcu_checksum(tx, total);

	ret = cfg->transport->transfer(dev, tx, total, NULL);

	k_mutex_unlock(&esp_hosted_mcu_tx_lock);

	return ret;
}

uint32_t esp_hosted_mcu_fw_version(void)
{
	return esp_hosted_mcu_data_0.fw_version;
}

/*
 * Ask the coprocessor for its firmware version and log what it runs against
 * the version this host was built for. The version is informational: the
 * firmware lives on a separate chip and is flashed independently, so a
 * mismatch is reported and never fails init. Firmware predating this request
 * does not answer it, in which case the boot event may have supplied the
 * version already.
 */
static void esp_hosted_mcu_check_fw_version(struct esp_hosted_mcu_data *data)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	uint32_t expected = ESP_HOSTED_MCU_FW_VERSION_VAL(CONFIG_ESP_HOSTED_MCU_FW_VERSION_MAJOR,
							  CONFIG_ESP_HOSTED_MCU_FW_VERSION_MINOR,
							  CONFIG_ESP_HOSTED_MCU_FW_VERSION_PATCH);

	req.msg_id = RpcId_Req_GetCoprocessorFwVersion;

	if (esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT) == 0) {
		const Rpc_Resp_GetCoprocessorFwVersion *fw =
			&resp.payload.resp_get_coprocessor_fwversion;

		data->fw_version =
			ESP_HOSTED_MCU_FW_VERSION_VAL(fw->major1, fw->minor1, fw->patch1);
	}

	/* The response carries a callback-typed field, so release it either way. */
	pb_release(Rpc_fields, &resp);

	if (data->fw_version == 0) {
		LOG_WRN("coprocessor firmware version unknown, expected v%u.%u.%u",
			CONFIG_ESP_HOSTED_MCU_FW_VERSION_MAJOR,
			CONFIG_ESP_HOSTED_MCU_FW_VERSION_MINOR,
			CONFIG_ESP_HOSTED_MCU_FW_VERSION_PATCH);
		return;
	}

	LOG_INF("coprocessor firmware v%u.%u.%u", ESP_HOSTED_MCU_FW_VERSION_MAJOR(data->fw_version),
		ESP_HOSTED_MCU_FW_VERSION_MINOR(data->fw_version),
		ESP_HOSTED_MCU_FW_VERSION_PATCH(data->fw_version));

	if (ESP_HOSTED_MCU_FW_VERSION_MAJOR(data->fw_version) !=
	    ESP_HOSTED_MCU_FW_VERSION_MAJOR(expected)) {
		LOG_WRN("coprocessor firmware major version differs from the expected v%u.%u.%u",
			CONFIG_ESP_HOSTED_MCU_FW_VERSION_MAJOR,
			CONFIG_ESP_HOSTED_MCU_FW_VERSION_MINOR,
			CONFIG_ESP_HOSTED_MCU_FW_VERSION_PATCH);
	}
}

/* Bring the link up: init transport, spawn the receive/event thread. */
static int esp_hosted_mcu_core_init(const struct device *dev)
{
	const struct esp_hosted_mcu_config *cfg = dev->config;
	struct esp_hosted_mcu_data *data = dev->data;
	static struct k_thread event_thread;
	int ret;

	static K_KERNEL_STACK_DEFINE(event_stack, CONFIG_ESP_HOSTED_MCU_EVENT_TASK_STACK_SIZE);

	data->dev = dev;
	k_sem_init(&data->rpc_sem, 0, 1);

	ret = cfg->transport->init(dev);
	if (ret) {
		return ret;
	}

	data->tid = k_thread_create(&event_thread, event_stack, K_KERNEL_STACK_SIZEOF(event_stack),
				    esp_hosted_mcu_event_task, (void *)dev, NULL, NULL,
				    CONFIG_ESP_HOSTED_MCU_EVENT_TASK_PRIORITY, 0, K_NO_WAIT);
	if (!data->tid) {
		return -EAGAIN;
	}

	/* The receive thread is up, so the version round-trip can complete. */
	esp_hosted_mcu_check_fw_version(data);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, esp_hosted_mcu_core_init, NULL, &esp_hosted_mcu_data_0,
		      &esp_hosted_mcu_config_0, POST_KERNEL, CONFIG_ESP_HOSTED_MCU_INIT_PRIORITY,
		      NULL);
