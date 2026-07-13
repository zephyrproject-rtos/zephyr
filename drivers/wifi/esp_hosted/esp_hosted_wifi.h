/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_WIFI_H_
#define ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_WIFI_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include <esp_hosted_proto.pb.h>

#define DT_DRV_COMPAT espressif_esp_hosted

#define ESP_HOSTED_MAC_ADDR_LEN  (6)
#define ESP_HOSTED_MAC_STR_LEN   (17)
#define ESP_HOSTED_MAX_SSID_LEN  (32)
#define ESP_HOSTED_MAX_PASS_LEN  (64)
#define ESP_HOSTED_SYNC_TIMEOUT  (5000)
#define ESP_HOSTED_SCAN_TIMEOUT  (10000)
#define ESP_HOSTED_QUEUE_TIMEOUT (1000)
#define ESP_HOSTED_SPI_CONFIG                                                                      \
	(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_MODE_CPOL)

#define TLV_HEADER_SIZE      (14)
#define TLV_HEADER_TYPE_EP   (1)
#define TLV_HEADER_TYPE_DATA (2)
#define TLV_HEADER_EP_RESP   "ctrlResp"
#define TLV_HEADER_EP_EVENT  "ctrlEvnt"

#define ESP_FRAME_SIZE           (1600)
#define ESP_FRAME_HEADER_SIZE    (12)
#define ESP_FRAME_MAX_PAYLOAD    (ESP_FRAME_SIZE - ESP_FRAME_HEADER_SIZE)
#define ESP_FRAME_FLAGS_FRAGMENT (1 << 0)

typedef enum {
	ESP_HOSTED_STA_IF = 0,
	ESP_HOSTED_SAP_IF,
	ESP_HOSTED_SERIAL_IF,
	ESP_HOSTED_HCI_IF,
	ESP_HOSTED_PRIV_IF,
	ESP_HOSTED_TEST_IF,
	ESP_HOSTED_MAX_IF,
} esp_hosted_interface_t;

typedef enum {
	ESP_PACKET_TYPE_EVENT,
} esp_hosted_priv_packet_t;

typedef enum {
	ESP_PRIV_EVENT_INIT,
} esp_hosted_priv_event_t;

/* TLV payload. */
typedef struct __packed {
	uint8_t ep_type;
	uint16_t ep_length;
	uint8_t ep_value[8];
	uint8_t data_type;
	uint16_t data_length;
	uint8_t data_value[];
} esp_hosted_tlv_t;

/* Event payload. */
typedef struct __packed {
	uint8_t event_type;
	uint8_t event_len;
	uint8_t event_data[];
} esp_hosted_event_t;

/* ESP frame struct (with data, event or TLV payload) */
typedef struct __packed {
	uint8_t if_type: 4;
	uint8_t if_num: 4;
	uint8_t flags;
	uint16_t len;
	uint16_t offset;
	uint16_t checksum;
	uint16_t seq_num;
	uint8_t reserved2;
	union {
		uint8_t hci_pkt_type;
		uint8_t priv_pkt_type;
	};
	uint8_t payload[];
} esp_hosted_frame_t;

typedef struct {
	struct gpio_dt_spec cs_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec dataready_gpio;
	struct gpio_dt_spec handshake_gpio;
	struct spi_dt_spec spi_bus;
} esp_hosted_config_t;

typedef struct {
	uint16_t seq_num;
	uint64_t last_hb_ms;
	struct net_if *iface[2];
	uint8_t mac_addr[2][6];
	k_tid_t tid;
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats;
#endif
	enum wifi_iface_state state[2];
	struct {
		uint32_t major1;
		uint32_t major2;
		uint32_t minor;
		uint32_t rev_patch1;
		uint32_t rev_patch2;
	} fw_version;

	/* RX reassembly accumulator. */
	uint8_t acc_buf[ESP_FRAME_SIZE * 2] __aligned(4);

	/* queue of outgoing frames to transmit */
	struct k_msgq tx_msgq;
	uint8_t tx_qbuf[ESP_FRAME_SIZE * CONFIG_WIFI_ESP_HOSTED_TX_QUEUE_SIZE];

	/* queue of received control responses/events */
	struct k_msgq rx_msgq;
	uint8_t rx_qbuf[sizeof(CtrlMsg) * CONFIG_WIFI_ESP_HOSTED_RX_QUEUE_SIZE];
} esp_hosted_data_t;
#endif /* ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_WIFI_H_ */
