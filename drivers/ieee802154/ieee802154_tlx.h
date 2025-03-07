/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_TLX_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_TLX_H_

/* Timeouts */
#define TLX_TX_WAIT_TIME_MS                 (10)
#define TLX_ACK_WAIT_TIME_MS                (5)

/* Received data parsing */
#ifndef IEEE802154_FRAME_LENGTH_PANID
#define IEEE802154_FRAME_LENGTH_PANID       (2)
#endif
#ifndef IEEE802154_FRAME_LENGTH_ADDR_SHORT
#define IEEE802154_FRAME_LENGTH_ADDR_SHORT  (2)
#endif
#ifndef IEEE802154_FRAME_LENGTH_ADDR_EXT
#define IEEE802154_FRAME_LENGTH_ADDR_EXT    (8)
#endif
#define TLX_PAYLOAD_OFFSET                  (5)
#define TLX_PAYLOAD_MIN                     (5)
#define TLX_PAYLOAD_MAX                     (127)
#define TLX_LENGTH_OFFSET                   (4)
#define TLX_RSSI_OFFSET                     (11)
#define TLX_BROADCAST_ADDRESS               ((uint8_t [2]) { 0xff, 0xff })
#define TLX_FCS_LENGTH                      (2)
#define TLX_CMD_ID_DATA_REQ                 (0x04)

/* Generic */
#define TLX_TRX_LENGTH                      (256)
#define TLX_RSSI_TO_LQI_SCALE               (3)
#define TLX_RSSI_TO_LQI_MIN                 (-87)
#define TLX_CCA_TIME_MAX_US                 (200)
#define TLX_LOGIC_CHANNEL_TO_PHYSICAL(p)    (((p) - 10) * 5)
#define TLX_MAC_KEYS_ITEMS                  (3)
#ifndef IEEE802154_CRYPTO_LENGTH_AES_BLOCK
#define IEEE802154_CRYPTO_LENGTH_AES_BLOCK  (16)
#endif

#define ZB_RADIO_TIMESTAMP_GET(p)           (uint32_t)(    \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p)])           | \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p) + 1] << 8)  | \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p) + 2] << 16) | \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p) + 3] << 24))

#define TLX_TX_PWR_NOT_SET                 INT16_MAX
#define TLX_TX_CH_NOT_SET                  UINT16_MAX

#define IEEE802154_TLX_FLASH_MAC_OFFSET 0x1000

#ifdef CONFIG_OPENTHREAD_FTD
/* radio source match table type */
struct tlx_src_match_table {
	bool enabled;
	struct {
		bool valid;
		bool ext;
		uint8_t addr[MAX(IEEE802154_FRAME_LENGTH_ADDR_EXT,
			IEEE802154_FRAME_LENGTH_ADDR_SHORT)];
	} item[2 * CONFIG_OPENTHREAD_MAX_CHILDREN];
};
#endif /* CONFIG_OPENTHREAD_FTD */

#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
/* radio ACK table type */
struct tlx_enh_ack_table {
	struct {
		bool valid;
		uint8_t addr_short[IEEE802154_FRAME_LENGTH_ADDR_SHORT];
		uint8_t addr_ext[IEEE802154_FRAME_LENGTH_ADDR_EXT];
		struct ieee802154_header_ie ie_header;
	} item[CONFIG_OPENTHREAD_MAX_CHILDREN];
};
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
/* radio MAC keys type */
struct tlx_mac_keys {
	struct {
		uint8_t key_id;
		uint8_t key[IEEE802154_CRYPTO_LENGTH_AES_BLOCK];
		uint32_t frame_cnt;
		bool frame_cnt_local;
	} item[TLX_MAC_KEYS_ITEMS];
	uint32_t frame_cnt;
};
#endif

/* data structure */
struct tlx_data {
	uint8_t mac_addr[IEEE802154_FRAME_LENGTH_ADDR_EXT];
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	uint8_t *rx_buffer;
	uint8_t *tx_buffer;
#else
	uint8_t rx_buffer[TLX_TRX_LENGTH] __aligned(4);
	uint8_t tx_buffer[TLX_TRX_LENGTH] __aligned(4);
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
	struct net_if *iface;
	struct k_sem tx_wait;
	struct k_sem ack_wait;
	uint8_t filter_pan_id[IEEE802154_FRAME_LENGTH_PANID];
	uint8_t filter_short_addr[IEEE802154_FRAME_LENGTH_ADDR_SHORT];
	uint8_t filter_ieee_addr[IEEE802154_FRAME_LENGTH_ADDR_EXT];
	volatile bool is_started;
	volatile bool ack_handler_en;
	volatile bool rf_mode_154;
	volatile uint8_t ack_sn;
	uint16_t current_channel;
	int16_t current_dbm;
	volatile bool ack_sending;
#ifdef CONFIG_OPENTHREAD_FTD
	struct tlx_src_match_table *src_match_table;
#endif /* CONFIG_OPENTHREAD_FTD */
#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
	struct tlx_enh_ack_table *enh_ack_table;
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */
#ifdef CONFIG_PM_DEVICE
	atomic_t current_pm_lock;
#endif /* CONFIG_PM_DEVICE */
	ieee802154_event_cb_t event_handler;
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	struct tlx_mac_keys *mac_keys;
#endif
};

#endif
