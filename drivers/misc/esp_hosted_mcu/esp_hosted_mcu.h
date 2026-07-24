/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESP_HOSTED_MCU_H_
#define ZEPHYR_DRIVERS_MISC_ESP_HOSTED_MCU_H_

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <esp_hosted_mcu_rpc.pb.h>

/*
 * Interface types multiplexed over a single transport, demuxed by the
 * if_type nibble of the payload header. Mirrors esp_hosted_mcu_if_type_t in
 * the coprocessor firmware; keep the numbering in sync with it.
 */
typedef enum {
	ESP_HOSTED_MCU_INVALID_IF = 0,
	ESP_HOSTED_MCU_STA_IF,
	ESP_HOSTED_MCU_AP_IF,
	ESP_HOSTED_MCU_SERIAL_IF,
	ESP_HOSTED_MCU_HCI_IF,
	ESP_HOSTED_MCU_PRIV_IF,
	ESP_HOSTED_MCU_TEST_IF,
	ESP_HOSTED_MCU_ETH_IF,
	ESP_HOSTED_MCU_MAX_IF,
} esp_hosted_mcu_if_type_t;

/* Payload header flags. */
#define ESP_HOSTED_MCU_FLAG_FRAGMENT (1 << 0)
#define ESP_HOSTED_MCU_FLAG_WAKEUP   (1 << 1)
#define ESP_HOSTED_MCU_FLAG_PS_START (1 << 2)
#define ESP_HOSTED_MCU_FLAG_PS_STOP  (1 << 3)

/* Priv-interface packet and event types announced by the coprocessor at boot. */
#define ESP_HOSTED_MCU_PACKET_TYPE_EVENT (0x33)
#define ESP_HOSTED_MCU_PRIV_EVENT_INIT   (0x22)

/*
 * Transport buffer sizing. The coprocessor caps a single SPI transaction at
 * 1600 bytes; keep the host frame the same so a full coprocessor buffer fits.
 */
#define ESP_HOSTED_MCU_FRAME_SIZE  (1600)
#define ESP_HOSTED_MCU_HEADER_SIZE (sizeof(struct esp_hosted_mcu_header))
#define ESP_HOSTED_MCU_MAX_PAYLOAD (ESP_HOSTED_MCU_FRAME_SIZE - ESP_HOSTED_MCU_HEADER_SIZE)

/*
 * On-wire payload header. Same layout the coprocessor firmware uses
 * (struct esp_payload_header). Every frame, regardless of interface,
 * starts with this 12-byte header.
 */
struct esp_hosted_mcu_header {
	uint8_t if_type: 4;
	uint8_t if_num: 4;
	uint8_t flags;
	uint16_t len;
	uint16_t offset;
	uint16_t checksum;
	uint16_t seq_num;
	uint8_t throttle_cmd: 2;
	uint8_t reserved: 6;
	union {
		uint8_t reserved3;
		uint8_t hci_pkt_type;
		uint8_t priv_pkt_type;
	};
} __packed;

/*
 * Priv-interface event body (coprocessor capabilities / chip id at boot).
 * Follows the header when if_type == PRIV and priv_pkt_type == EVENT.
 * event_data holds a stream of {tag, length, value} triplets.
 */
struct esp_hosted_mcu_priv_event {
	uint8_t event_type;
	uint8_t event_len;
	uint8_t event_data[];
} __packed;

/*
 * Tags carried in the init event. Only the firmware version and the SDIO
 * buffer config are consumed; the rest are skipped by their length. Keep in
 * sync with the coprocessor firmware (ESP_PRIV_TAG_TYPE).
 */
#define ESP_HOSTED_MCU_PRIV_TAG_FIRMWARE_VERSION (0x17)
#define ESP_HOSTED_MCU_PRIV_TAG_SDIO_BUF_CONFIG  (0x1b)

/* Firmware version, packed by the coprocessor as (major << 16) | (minor << 8) | patch. */
#define ESP_HOSTED_MCU_FW_VERSION_VAL(major, minor, patch)                                         \
	(((major) << 16) | ((minor) << 8) | (patch))
#define ESP_HOSTED_MCU_FW_VERSION_MAJOR(v) (((v) >> 16) & 0xff)
#define ESP_HOSTED_MCU_FW_VERSION_MINOR(v) (((v) >> 8) & 0xff)
#define ESP_HOSTED_MCU_FW_VERSION_PATCH(v) ((v) & 0xff)

/*
 * SDIO datapath transmit modes advertised in the buffer-config tag. In
 * SW_AGGR the coprocessor packs several frames into one transfer, each padded
 * to a four-byte boundary, so the receive walk must advance by the aligned
 * stride. STREAM and PACKET keep one frame per transfer, byte-tight. A
 * coprocessor that omits the tag (older firmware, or SPI) is treated as
 * byte-tight.
 */
#define ESP_HOSTED_MCU_SDIO_TX_MODE_SW_AGGR (0)
#define ESP_HOSTED_MCU_SDIO_TX_MODE_STREAM  (1)
#define ESP_HOSTED_MCU_SDIO_TX_MODE_PACKET  (2)

/* Wire layout of the SDIO buffer-config tag value (five packed bytes). */
struct esp_hosted_mcu_sdio_buf_config {
	uint8_t transport;
	uint8_t e2h_mode;
	uint8_t e2h_bufsz_512b;
	uint8_t h2e_mode;
	uint8_t h2e_bufsz_512b;
} __packed;

/*
 * Endpoint TLV that wraps an RPC protobuf blob inside a SERIAL frame.
 * Two TLVs are concatenated: an endpoint-name TLV then a data TLV.
 * Response frames carry RPC_EP_RSP, asynchronous events RPC_EP_EVT.
 * Both endpoint strings are 6 bytes so the layout is fixed.
 */
#define ESP_HOSTED_MCU_EP_NAME_LEN (6)
#define ESP_HOSTED_MCU_EP_RSP      "RPCRsp"
#define ESP_HOSTED_MCU_EP_EVT      "RPCEvt"
#define ESP_HOSTED_MCU_TLV_EP      (1)
#define ESP_HOSTED_MCU_TLV_DATA    (2)

struct esp_hosted_mcu_tlv {
	uint8_t ep_type;
	uint16_t ep_length;
	uint8_t ep_value[ESP_HOSTED_MCU_EP_NAME_LEN];
	uint8_t data_type;
	uint16_t data_length;
	uint8_t data_value[];
} __packed;

/* RPC request/response round-trip and scan completion timeouts (ms). */
#define ESP_HOSTED_MCU_RPC_TIMEOUT  (5000)
#define ESP_HOSTED_MCU_SCAN_TIMEOUT (10000)

/* 16-bit sum-of-bytes checksum over the payload, matching the coprocessor. */
static inline uint16_t esp_hosted_mcu_checksum(const uint8_t *buf, uint16_t len)
{
	uint16_t sum = 0;

	for (uint16_t i = 0; i < len; i++) {
		sum += buf[i];
	}

	return sum;
}

/*
 * Transport backend contract. A backend (SPI or SDIO) moves whole framed
 * buffers to and from the coprocessor and reports when the coprocessor has data pending.
 * The core owns framing, RPC and demux and is transport agnostic above this
 * line. The device passed to these ops is the core esp-hosted-mcu device.
 */
struct esp_hosted_mcu_transport_api {
	/* Bring the link up: configure bus/GPIOs, reset the coprocessor. */
	int (*init)(const struct device *dev);
	/*
	 * Exchange one frame. tx may be NULL for a receive-only poll; rx
	 * receives up to ESP_HOSTED_MCU_FRAME_SIZE bytes. Returns the number of
	 * valid rx bytes, 0 if the coprocessor had nothing, negative on error.
	 */
	int (*transfer)(const struct device *dev, const uint8_t *tx, uint16_t tx_len, uint8_t *rx);
	/* True when the coprocessor has asserted data-ready. */
	bool (*data_ready)(const struct device *dev);
	/*
	 * Optional. Block up to timeout_ms for the coprocessor to signal new receive
	 * data, returning early on the transport's data-ready interrupt. May
	 * wake spuriously; the caller must re-check data_ready(). NULL when the
	 * transport has no event source and the caller should poll instead.
	 */
	void (*wait_for_rx)(const struct device *dev, int timeout_ms);
};

/* Core per-instance immutable config, filled from devicetree. */
struct esp_hosted_mcu_config {
	const struct esp_hosted_mcu_transport_api *transport;
	const void *transport_config;
};

/*
 * Consumer API. A protocol consumer (Wi-Fi netif, Bluetooth HCI) registers a
 * receive callback per interface type and drives the coprocessor with RPC
 * calls and framed sends. All calls target the single core instance.
 */

/*
 * Receive callback for a demuxed data frame. payload/len is the frame body,
 * pkt_type is the header's per-interface type byte (the HCI H4 indicator for
 * HCI_IF, zero for Wi-Fi data). user_data is the pointer passed at register.
 */
typedef void (*esp_hosted_mcu_rx_cb_t)(const uint8_t *payload, uint16_t len, uint8_t pkt_type,
				       void *user_data);

/* Callback for an asynchronous RPC event frame (SERIAL_IF, msg_type Event). */
typedef void (*esp_hosted_mcu_event_cb_t)(const Rpc *rpc, void *user_data);

/* Register a receive handler for one interface type. */
int esp_hosted_mcu_register_if(esp_hosted_mcu_if_type_t if_type, esp_hosted_mcu_rx_cb_t cb,
			       void *user_data);

/* Remove the receive handler for one interface type. */
int esp_hosted_mcu_unregister_if(esp_hosted_mcu_if_type_t if_type);

/* Register the handler for asynchronous RPC events. */
int esp_hosted_mcu_register_rpc_event(esp_hosted_mcu_event_cb_t cb, void *user_data);

/* The core esp-hosted-mcu device handle (single instance). */
const struct device *esp_hosted_mcu_get_dev(void);

/*
 * Firmware version reported by the coprocessor, packed with
 * ESP_HOSTED_MCU_FW_VERSION_VAL(). Zero when the coprocessor reported no
 * version, which is the case for firmware predating the version query.
 * Consumers may branch on it to adapt to older firmware.
 */
uint32_t esp_hosted_mcu_fw_version(void);

/*
 * Send an RPC request and block for its matching response. On success resp
 * holds the decoded response. Returns the coprocessor esp_err_t status, or a
 * negative errno on transport/timeout failure.
 */
int esp_hosted_mcu_rpc_call(Rpc *req, Rpc *resp, int32_t timeout);

/*
 * Build a payload header around payload/len and send it over the transport.
 * pkt_type is placed in the header's per-interface type byte (0 for Wi-Fi
 * data). Returns 0 on success or a negative errno.
 */
int esp_hosted_mcu_send_frame(esp_hosted_mcu_if_type_t if_type, uint8_t if_num, uint8_t pkt_type,
			      const uint8_t *payload, uint16_t len);

#endif /* ZEPHYR_DRIVERS_MISC_ESP_HOSTED_MCU_H_ */
