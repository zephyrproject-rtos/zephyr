/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_B91_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_B91_H_

/* Timeouts */
#define B91_TX_WAIT_TIME_MS                 (10)
#define B91_ACK_WAIT_TIME_MS                (10)

/* Received data parsing */
#define B91_PAYLOAD_OFFSET                  (5)
#define B91_PAYLOAD_MIN                     (5)
#define B91_PAYLOAD_MAX                     (127)
#define B91_FRAME_TYPE_OFFSET               (0)
#define B91_FRAME_TYPE_MASK                 (0x07)
#define B91_DEST_ADDR_TYPE_OFFSET           (1)
#define B91_DEST_ADDR_TYPE_MASK             (0x0c)
#define B91_DEST_ADDR_TYPE_SHORT            (8)
#define B91_DEST_ADDR_TYPE_IEEE             (0x0c)
#define B91_PAN_ID_OFFSET                   (3)
#define B91_PAN_ID_SIZE                     (2)
#define B91_DEST_ADDR_OFFSET                (5)
#define B91_SHORT_ADDRESS_SIZE              (2)
#define B91_IEEE_ADDRESS_SIZE               (8)
#define B91_LENGTH_OFFSET                   (4)
#define B91_RSSI_OFFSET                     (11)
#define B91_BROADCAST_ADDRESS               ((uint8_t [2]) { 0xff, 0xff })
#define B91_ACK_FRAME_LEN                   (3)
#define B91_ACK_TYPE                        (2)
#define B91_ACK_REQUEST                     (1 << 5)
#define B91_DSN_OFFSET                      (2)
#define B91_FCS_LENGTH                      (2)

/* Generic */
#define B91_TRX_LENGTH                      (256)
#define B91_RSSI_TO_LQI_SCALE               (3)
#define B91_RSSI_TO_LQI_MIN                 (-87)
#define B91_CCA_TIME_MAX_US                 (200)
#define B91_LOGIC_CHANNEL_TO_PHYSICAL(p)    (((p) - 10) * 5)

/* TX power lookup table */
#define B91_TX_POWER_MIN                    (-30)
#define B91_TX_POWER_MAX                    (9)
static const uint8_t b91_tx_pwr_lt[] = {
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

/* data structure */
struct b91_data {
	uint8_t mac_addr[B91_IEEE_ADDRESS_SIZE];
	uint8_t rx_buffer[B91_TRX_LENGTH];
	uint8_t tx_buffer[B91_TRX_LENGTH];
	struct net_if *iface;
	struct k_sem tx_wait;
	struct k_sem ack_wait;
	uint8_t filter_pan_id[B91_PAN_ID_SIZE];
	uint8_t filter_short_addr[B91_SHORT_ADDRESS_SIZE];
	uint8_t filter_ieee_addr[B91_IEEE_ADDRESS_SIZE];
	bool is_started;
	bool ack_handler_en;
	uint16_t current_channel;
};

#endif
