/*
 * Copyright (c) 2025, Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TISCI_PROTOCOL_ORIGINAL__
#define __TISCI_PROTOCOL_ORIGINAL__
#include <stdint.h>
#include <zephyr/types.h> /* For u8, u16, etc. */
#include <zephyr/types.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

/**
 * @struct ti_sci_msg_fwl_region_cfg
 * @brief Request and Response for firewalls settings
 *
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to set config info
 *			This field is unused in case of a simple firewall  and must be initialized
 *			to zero.  In case of a region based firewall, this field indicates the
 *			region in question. (index starting from 0) In case of a channel based
 *			firewall, this field indicates the channel in question (index starting
 *			from 0)
 * @param n_permission_regs:	Number of permission registers to set
 * @param control:		Contents of the firewall CONTROL register to set
 * @param permissions:	Contents of the firewall PERMISSION register to set
 * @param start_address:	Contents of the firewall START_ADDRESS register to set
 * @param end_address:	Contents of the firewall END_ADDRESS register to set
 */
struct ti_sci_msg_fwl_region {
	uint16_t fwl_id;
	uint16_t region;
	uint32_t n_permission_regs;
	uint32_t control;
	uint32_t permissions[3];
	uint64_t start_address;
	uint64_t end_address;
} __packed;

/**
 * @brief Request and Response for firewall owner change
 * @struct ti_sci_msg_fwl_owner
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to set config info
 *			This field is unused in case of a simple firewall  and must be initialized
 *			to zero.  In case of a region based firewall, this field indicates the
 *			region in question. (index starting from 0) In case of a channel based
 *			firewall, this field indicates the channel in question (index starting
 *			from 0)
 * @param n_permission_regs:	Number of permission registers <= 3
 * @param control:		Control register value for this region
 * @param owner_index:	New owner index to change to. Owner indexes are setup in DMSC firmware boot
 *configuration data
 * @param owner_privid:	New owner priv-id, used to lookup owner_index is not known, must be set to
 *zero otherwise
 * @param owner_permission_bits: New owner permission bits
 */
struct ti_sci_msg_fwl_owner {
	uint16_t fwl_id;
	uint16_t region;
	uint8_t owner_index;
	uint8_t owner_privid;
	uint16_t owner_permission_bits;
} __packed;

/**
 * Configures a Navigator Subsystem UDMAP transmit channel
 *
 * Configures a Navigator Subsystem UDMAP transmit channel registers.
 * See ti_sci_msg_rm_udmap_tx_ch_cfg_req
 */
struct ti_sci_msg_rm_udmap_tx_ch_cfg {
	uint32_t valid_params;
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_TX_FILT_EINFO_VALID    BIT(9)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_TX_FILT_PSWORDS_VALID  BIT(10)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_TX_SUPR_TDPKT_VALID    BIT(11)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_TX_CREDIT_COUNT_VALID  BIT(12)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_TX_FDEPTH_VALID        BIT(13)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_TX_TDTYPE_VALID        BIT(15)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_EXTENDED_CH_TYPE_VALID BIT(16)
	uint16_t nav_id;
	uint16_t index;
	uint8_t tx_pause_on_err;
	uint8_t tx_filt_einfo;
	uint8_t tx_filt_pswords;
	uint8_t tx_atype;
	uint8_t tx_chan_type;
	uint8_t tx_supr_tdpkt;
	uint16_t tx_fetch_size;
	uint8_t tx_credit_count;
	uint16_t txcq_qnum;
	uint8_t tx_priority;
	uint8_t tx_qos;
	uint8_t tx_orderid;
	uint16_t fdepth;
	uint8_t tx_sched_priority;
	uint8_t tx_burst_size;
	uint8_t tx_tdtype;
	uint8_t extended_ch_type;
} __packed;

/**
 * Configures a Navigator Subsystem UDMAP receive channel
 *
 * Configures a Navigator Subsystem UDMAP receive channel registers.
 * See ti_sci_msg_rm_udmap_rx_ch_cfg_req
 */
struct ti_sci_msg_rm_udmap_rx_ch_cfg {
	uint32_t valid_params;
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_RX_FLOWID_START_VALID BIT(9)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_RX_FLOWID_CNT_VALID   BIT(10)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_RX_IGNORE_SHORT_VALID BIT(11)
#define TI_SCI_MSG_VALUE_RM_UDMAP_CH_RX_IGNORE_LONG_VALID  BIT(12)
	uint16_t nav_id;
	uint16_t index;
	uint16_t rx_fetch_size;
	uint16_t rxcq_qnum;
	uint8_t rx_priority;
	uint8_t rx_qos;
	uint8_t rx_orderid;
	uint8_t rx_sched_priority;
	uint16_t flowid_start;
	uint16_t flowid_cnt;
	uint8_t rx_pause_on_err;
	uint8_t rx_atype;
	uint8_t rx_chan_type;
	uint8_t rx_ignore_short;
	uint8_t rx_ignore_long;
	uint8_t rx_burst_size;
};

#endif /* __TISCI_PROTOCOL_ORIGINAL__*/
