/* ieee802154_mcxw.h - NXP MCXW 802.15.4 driver */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_MCXW_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_MCXW_H_

#include <zephyr/net/ieee802154_radio.h>

#include "PhyInterface.h"

#define TX_ENCRYPT_DELAY_SYM 200

#define DEFAULT_CHANNEL                  (11)
#define DEFAULT_CCA_MODE                 (gPhyCCAMode1_c)
#define IEEE802154_ACK_REQUEST           (1 << 5)
#define IEEE802154_MIN_LENGTH            (5)
#define IEEE802154_FRM_CTL_LO_OFFSET     (0)
#define IEEE802154_DSN_OFFSET            (2)
#define IEEE802154_FRM_TYPE_MASK         (0x7)
#define IEEE802154_FRM_TYPE_ACK          (0x2)
#define IEEE802154_SYMBOL_TIME_US        (16)
#define IEEE802154_TURNAROUND_LEN_SYM    (12)
#define IEEE802154_CCA_LEN_SYM           (8)
#define IEEE802154_PHY_SHR_LEN_SYM       (10)
#define IEEE802154_IMM_ACK_WAIT_SYM      (54)
#define IEEE802154_ENH_ACK_WAIT_SYM      (90)

#define NMAX_RXRING_BUFFERS              (8)
#define RX_ON_IDLE_START                 (1)
#define RX_ON_IDLE_STOP                  (0)

#define PHY_TMR_MAX_VALUE                (0x00FFFFFF)

/* The Uncertainty of the scheduling CSL of transmission by the parent, in ±10 us units. */
#define CSL_UNCERT 32

#define RADIO_SYMBOLS_PER_OCTET			(2)

typedef enum mcxw_radio_state {
	RADIO_STATE_DISABLED = 0,
	RADIO_STATE_SLEEP    = 1,
	RADIO_STATE_RECEIVE  = 2,
	RADIO_STATE_TRANSMIT = 3,
	RADIO_STATE_INVALID  = 255,
} mcxw_radio_state;

typedef struct mcxw_rx_frame {
	uint8_t *psdu;
	uint8_t length;
	int8_t rssi;
	uint8_t lqi;
	uint32_t timestamp;
	bool ack_fpb;
	bool ack_seb;
	uint64_t time;
	void *phy_buffer;
	uint8_t channel;
} mcxw_rx_frame;

typedef struct mcxw_tx_frame {
	uint8_t *psdu;
	uint8_t length;
	uint32_t tx_delay;
	uint32_t tx_delay_base;
	bool sec_processed;
	bool hdr_updated;
} mcxw_tx_frame;

struct mcxw_context {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* Pointer to the LPTMR counter device structure*/
	const struct device *counter;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

	/* RX thread stack. */
	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_MCXW_RX_STACK_SIZE);

	/* RX thread control block. */
	struct k_thread rx_thread;

	/* RX message queue */
	struct k_msgq rx_msgq;

	/* RX message queue buffer */
	char rx_msgq_buffer[NMAX_RXRING_BUFFERS * sizeof(mcxw_rx_frame)];

	/* TX synchronization semaphore */
	struct k_sem tx_wait;

	/* TX synchronization semaphore */
	struct k_sem cca_wait;

	/* Radio state */
	mcxw_radio_state state;

	/* Pan ID */
	uint16_t pan_id;

	/* Channel */
	uint8_t channel;

	/* Maximum energy detected during ED scan */
	int8_t max_ed;

	/* TX power level */
	int8_t tx_pwr_lvl;

	/* Enery detect */
	energy_scan_done_cb_t energy_scan_done;

	/* TX Status */
	int tx_status;

	/* TX frame */
	mcxw_tx_frame tx_frame;

	/* TX data */
	uint8_t tx_data[sizeof(macToPdDataMessage_t) + IEEE802154_MAX_PHY_PACKET_SIZE];

	/* RX mode */
	uint32_t rx_mode;

	/* RX ACK buffers */
	mcxw_rx_frame rx_ack_frame;

	/* RX ACK data */
	uint8_t rx_ack_data[IEEE802154_MAX_PHY_PACKET_SIZE];

	/* CSL period */
	uint32_t csl_period;

	/* CSL sample time in microseconds */
	uint32_t csl_sample_time;

	/* PHY context */
	uint8_t ot_phy_ctx;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_MCXW_H_ */
