/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_B9X_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_B9X_H_

/* Timeouts */
#define B9X_TX_WAIT_TIME_MS                 (10)
#define B9X_ACK_WAIT_TIME_MS                (10)

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
#define B9X_PAYLOAD_OFFSET                  (5)
#define B9X_PAYLOAD_MIN                     (5)
#define B9X_PAYLOAD_MAX                     (127)
#define B9X_LENGTH_OFFSET                   (4)
#define B9X_RSSI_OFFSET                     (11)
#define B9X_BROADCAST_ADDRESS               ((uint8_t [2]) { 0xff, 0xff })
#define B9X_FCS_LENGTH                      (2)
#define B9X_CMD_ID_DATA_REQ                 (0x04)

/* Generic */
#define B9X_TRX_LENGTH                      (256)
#define B9X_RSSI_TO_LQI_SCALE               (3)
#define B9X_RSSI_TO_LQI_MIN                 (-87)
#define B9X_CCA_TIME_MAX_US                 (200)
#define B9X_LOGIC_CHANNEL_TO_PHYSICAL(p)    (((p) - 10) * 5)
#define B9X_ACK_IE_MAX_SIZE                 (16)
#define B9X_MAC_KEYS_ITEMS                  (3)
#ifndef IEEE802154_CRYPTO_LENGTH_AES_BLOCK
#define IEEE802154_CRYPTO_LENGTH_AES_BLOCK  (16)
#endif

#define ZB_RADIO_TIMESTAMP_GET(p)           (uint32_t)(    \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p)])           | \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p) + 1] << 8)  | \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p) + 2] << 16) | \
	(p[rf_zigbee_dma_rx_offset_time_stamp(p) + 3] << 24))

#define B9X_TX_PWR_NOT_SET                 INT16_MAX
#define B9X_TX_CH_NOT_SET                  UINT16_MAX


#define B9X_TX_POWER_MIN                    (-30)
#define B9X_TX_POWER_MAX                    (9)

/* TX power lookup table */
#if CONFIG_SOC_RISCV_TELINK_B91
static const uint8_t b9x_tx_pwr_lt[] = {
	RF_POWER_N30dBm,        /* -30.0 dBm: -30 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -29 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -28 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -27 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -26 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -25 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -24 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -23 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -22 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -21 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -20 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -19 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -18 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -17 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -16 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -15 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -14 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -13 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -12 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -11 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm: -10 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm:  -9 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm:  -8 */
	RF_POWER_N6p54dBm,      /*  -6.5 dBm:  -7 */
	RF_POWER_N6p54dBm,      /*  -6.5 dBm:  -6 */
	RF_POWER_N4p77dBm,      /*  -4.7 dBm:  -5 */
	RF_POWER_N4p77dBm,      /*  -4.7 dBm:  -4 */
	RF_POWER_N3p37dBm,      /*  -3.3 dBm:  -3 */
	RF_POWER_N2p01dBm,      /*  -2.0 dBm:  -2 */
	RF_POWER_N1p37dBm,      /*  -1.3 dBm:  -1 */
	RF_POWER_P0p01dBm,      /*   0.0 dBm:   0 */
	RF_POWER_P0p80dBm,      /*   0.8 dBm:   1 */
	RF_POWER_P2p32dBm,      /*   2.3 dBm:   2 */
	RF_POWER_P3p25dBm,      /*   3.2 dBm:   3 */
	RF_POWER_P4p35dBm,      /*   4.3 dBm:   4 */
	RF_POWER_P5p68dBm,      /*   5.6 dBm:   5 */
	RF_POWER_P5p68dBm,      /*   5.6 dBm:   6 */
	RF_POWER_P6p98dBm,      /*   6.9 dBm:   7 */
	RF_POWER_P8p05dBm,      /*   8.0 dBm:   8 */
	RF_POWER_P9p11dBm,      /*   9.1 dBm:   9 */
};
#elif CONFIG_SOC_RISCV_TELINK_B92
static const uint8_t b9x_tx_pwr_lt[] = {
	RF_POWER_N30dBm,        /* -30.0 dBm: -30 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -29 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -28 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -27 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -26 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -25 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -24 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -23 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -22 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -21 */
	RF_POWER_N13p42dBm,     /* -13.4 dBm: -20 */
	RF_POWER_N13p42dBm,     /* -13.4 dBm: -19 */
	RF_POWER_N13p42dBm,     /* -13.4 dBm: -18 */
	RF_POWER_N13p42dBm,     /* -13.4 dBm: -17 */
	RF_POWER_N13p42dBm,     /* -13.4 dBm: -16 */
	RF_POWER_N9p03dBm,      /*  -9.0 dBm: -15 */
	RF_POWER_N9p03dBm,      /*  -9.0 dBm: -14 */
	RF_POWER_N9p03dBm,      /*  -9.0 dBm: -13 */
	RF_POWER_N9p03dBm,      /*  -9.0 dBm: -12 */
	RF_POWER_N9p03dBm,      /*  -9.0 dBm: -11 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm: -10 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -9 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -8 */
	RF_POWER_N2p51dBm,      /*  -2.5 dBm:  -7 */
	RF_POWER_N2p51dBm,      /*  -2.5 dBm:  -6 */
	RF_POWER_N1p52dBm,      /*  -1.5 dBm:  -5 */
	RF_POWER_N1p52dBm,      /*  -1.5 dBm:  -4 */
	RF_POWER_N1p15dBm,      /*  -1.0 dBm:  -3 */
	RF_POWER_N1p15dBm,      /*  -1.0 dBm:  -2 */
	RF_POWER_N0p43dBm,      /*  -0.4 dBm:  -1 */
	RF_POWER_P0p01dBm,      /*   0.0 dBm:   0 */
	RF_POWER_P0p31dBm,      /*   0.3 dBm:   1 */
	RF_POWER_P1p03dBm,      /*   1.0 dBm:   2 */
	RF_POWER_P1p62dBm,      /*   1.6 dBm:   3 */
	RF_POWER_P2p01dBm,      /*   2.0 dBm:   4 */
	RF_POWER_P2p51dBm,      /*   2.5 dBm:   5 */
	RF_POWER_P3p00dBm,      /*   3.0 dBm:   6 */
	RF_POWER_P3p51dBm,      /*   3.5 dBm:   7 */
	RF_POWER_P4p02dBm,      /*   4.0 dBm:   8 */
	RF_POWER_P4p61dBm,      /*   4.6 dBm:   9 */
};

#endif

#ifdef CONFIG_OPENTHREAD_FTD
/* radio source match table type */
struct b9x_src_match_table {
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
struct b9x_enh_ack_table {
	struct {
		bool valid;
		uint8_t addr_short[IEEE802154_FRAME_LENGTH_ADDR_SHORT];
		uint8_t addr_ext[IEEE802154_FRAME_LENGTH_ADDR_EXT];
		uint16_t ie_header_len;
		uint8_t ie_header[B9X_ACK_IE_MAX_SIZE];
	} item[CONFIG_OPENTHREAD_MAX_CHILDREN];
};
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */

#ifdef CONFIG_IEEE802154_2015
/* radio MAC keys type */
struct b9x_mac_keys {
	struct {
		uint8_t key_id;
		uint8_t key[IEEE802154_CRYPTO_LENGTH_AES_BLOCK];
		uint32_t frame_cnt;
		bool frame_cnt_local;
	} item[B9X_MAC_KEYS_ITEMS];
	uint32_t frame_cnt;
};
#endif /* CONFIG_IEEE802154_2015 */

/* data structure */
struct b9x_data {
	uint8_t mac_addr[IEEE802154_FRAME_LENGTH_ADDR_EXT];
	uint8_t rx_buffer[B9X_TRX_LENGTH] __aligned(4);
	uint8_t tx_buffer[B9X_TRX_LENGTH] __aligned(4);
	struct net_if *iface;
	struct k_sem tx_wait;
	struct k_sem ack_wait;
	uint8_t filter_pan_id[IEEE802154_FRAME_LENGTH_PANID];
	uint8_t filter_short_addr[IEEE802154_FRAME_LENGTH_ADDR_SHORT];
	uint8_t filter_ieee_addr[IEEE802154_FRAME_LENGTH_ADDR_EXT];
	volatile bool is_started;
	volatile bool ack_handler_en;
	volatile uint8_t ack_sn;
	uint16_t current_channel;
	int16_t current_dbm;
	volatile bool ack_sending;
#ifdef CONFIG_OPENTHREAD_FTD
	struct b9x_src_match_table *src_match_table;
#endif /* CONFIG_OPENTHREAD_FTD */
#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
	struct b9x_enh_ack_table *enh_ack_table;
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */
#ifdef CONFIG_PM_DEVICE
	atomic_t current_pm_lock;
#endif /* CONFIG_PM_DEVICE */
	ieee802154_event_cb_t event_handler;
#ifdef CONFIG_IEEE802154_2015
	struct b9x_mac_keys *mac_keys;
#endif /* CONFIG_IEEE802154_2015 */
};

#endif
