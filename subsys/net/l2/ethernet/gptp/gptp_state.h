/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief PTP state machines
 *
 * This is not to be included by the application.
 */

#ifndef __GPTP_STATE_H
#define __GPTP_STATE_H

#include "gptp_mi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PDelayRequest states. */
enum gptp_pdelay_req_states {
	GPTP_PDELAY_REQ_NOT_ENABLED,
	GPTP_PDELAY_REQ_INITIAL_SEND_REQ,
	GPTP_PDELAY_REQ_RESET,
	GPTP_PDELAY_REQ_SEND_REQ,
	GPTP_PDELAY_REQ_WAIT_RESP,
	GPTP_PDELAY_REQ_WAIT_FOLLOW_UP,
	GPTP_PDELAY_REQ_WAIT_ITV_TIMER,
} __packed;

/* Path Delay Response states. */
enum gptp_pdelay_resp_states {
	GPTP_PDELAY_RESP_NOT_ENABLED,
	GPTP_PDELAY_RESP_INITIAL_WAIT_REQ,
	GPTP_PDELAY_RESP_WAIT_REQ,
	GPTP_PDELAY_RESP_WAIT_TSTAMP,
} __packed;

/* SyncReceive states. */
enum gptp_sync_rcv_states {
	GPTP_SYNC_RCV_DISCARD,
	GPTP_SYNC_RCV_WAIT_SYNC,
	GPTP_SYNC_RCV_WAIT_FOLLOW_UP,
} __packed;

/* SyncSend states. */
enum gptp_sync_send_states {
	GPTP_SYNC_SEND_INITIALIZING,
	GPTP_SYNC_SEND_SEND_SYNC,
	GPTP_SYNC_SEND_SEND_FUP,
} __packed;

/* PortSyncSyncReceive states. */
enum gptp_pss_rcv_states {
	GPTP_PSS_RCV_DISCARD,
	GPTP_PSS_RCV_RECEIVED_SYNC,
} __packed;

/* PortSyncSyncSend states. */
enum gptp_pss_send_states {
	GPTP_PSS_SEND_TRANSMIT_INIT,
	GPTP_PSS_SEND_SYNC_RECEIPT_TIMEOUT,
	GPTP_PSS_SEND_SEND_MD_SYNC,
	GPTP_PSS_SEND_SET_SYNC_RECEIPT_TIMEOUT,
} __packed;

/* SiteSyncSyncReceive states. */
enum gptp_site_sync_sync_states {
	GPTP_SSS_INITIALIZING,
	GPTP_SSS_RECEIVING_SYNC,
} __packed;

/* ClockSlaveSync states. */
enum gptp_clk_slave_sync_states {
	GPTP_CLK_SLAVE_SYNC_INITIALIZING,
	GPTP_CLK_SLAVE_SYNC_SEND_SYNC_IND,
} __packed;

/* PortAnnounceReceive states. */
enum gptp_pa_rcv_states {
	GPTP_PA_RCV_DISCARD,
	GPTP_PA_RCV_RECEIVE,
} __packed;

/* PortAnnounceInformation states. */
enum gptp_pa_info_states {
	GPTP_PA_INFO_DISABLED,
	/* State to handle the transition after DISABLED state. */
	GPTP_PA_INFO_POST_DISABLED,
	GPTP_PA_INFO_AGED,
	GPTP_PA_INFO_UPDATE,
	GPTP_PA_INFO_CURRENT,
	GPTP_PA_INFO_RECEIVE,
	GPTP_PA_INFO_SUPERIOR_MASTER_PORT,
	GPTP_PA_INFO_REPEATED_MASTER_PORT,
	GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT,
} __packed;

/* PortRoleSelection states. */
enum gptp_pr_selection_states {
	GPTP_PR_SELECTION_INIT_BRIDGE,
	GPTP_PR_SELECTION_ROLE_SELECTION,
} __packed;

/* PortAnnounceTransmit states. */
enum gptp_pa_transmit_states {
	GPTP_PA_TRANSMIT_INIT,
	GPTP_PA_TRANSMIT_PERIODIC,
	GPTP_PA_TRANSMIT_IDLE,
	GPTP_PA_TRANSMIT_POST_IDLE,
} __packed;

/* ClockMasterSyncOffset states. */
enum gptp_cms_offset_states {
	GPTP_CMS_OFFSET_INITIALIZING,
	GPTP_CMS_OFFSET_INDICATION,
} __packed;

/* ClockMasterSyncSend states. */
enum gptp_cms_snd_states {
	GPTP_CMS_SND_INITIALIZING,
	GPTP_CMS_SND_INDICATION,
} __packed;

/* ClockMasterSyncReceive states. */
enum gptp_cms_rcv_states {
	GPTP_CMS_RCV_INITIALIZING,
	GPTP_CMS_RCV_WAITING,
	GPTP_CMS_RCV_SOURCE_TIME,
} __packed;

/* Info_is enumeration2. */
enum gptp_info_is {
	GPTP_INFO_IS_RECEIVED,
	GPTP_INFO_IS_MINE,
	GPTP_INFO_IS_AGED,
	GPTP_INFO_IS_DISABLED,
} __packed;

enum gptp_time_source {
	GPTP_TS_ATOMIC_CLOCK        = 0x10,
	GPTP_TS_GPS                 = 0x20,
	GPTP_TS_TERRESTRIAL_AUDIO   = 0x30,
	GPTS_TS_PTP                 = 0x40,
	GPTP_TS_NTP                 = 0x50,
	GPTP_TS_HAND_SET            = 0x60,
	GPTP_TS_OTHER               = 0x90,
	GPTP_TS_INTERNAL_OSCILLATOR = 0xA0,
} __packed;

/**
 * @brief gPTP time-synchronization spanning tree priority vector
 *
 * Defines the best master selection information.
 */
struct gptp_priority_vector {
	/** Identity of the source clock. */
	struct gptp_root_system_identity root_system_id;

	/** Steps removed from the announce message transmitter and the
	 * master clock. Note that this field must be right after
	 * root_system_id as we are comparing root system id and steps
	 * removed in one memcmp()
	 */
	uint16_t steps_removed;

	/** Port identity of the transmitting time-aware system. */
	struct gptp_port_identity src_port_id;

	/** Port number of the receiving port. */
	uint16_t port_number;
} __packed;

/* Pdelay Request state machine variables. */
struct gptp_pdelay_req_state {
	/** Initial Path Delay Response Peer Timestamp. */
	uint64_t ini_resp_evt_tstamp;

	/** Initial Path Delay Response Ingress Timestamp. */
	uint64_t ini_resp_ingress_tstamp;

	/** Timer for the Path Delay Request. */
	struct k_timer pdelay_timer;

	/** Pointer to the received Path Delay Response. */
	struct net_pkt *rcvd_pdelay_resp_ptr;

	/** Pointer to the received Path Delay Follow Up. */
	struct net_pkt *rcvd_pdelay_follow_up_ptr;

	/** Pointer to the Path Delay Request to be transmitted. */
	struct net_pkt *tx_pdelay_req_ptr;

	/** Path Delay Response messages received. */
	uint32_t rcvd_pdelay_resp;

	/** Path Delay Follow Up messages received. */
	uint32_t rcvd_pdelay_follow_up;

	/** Number of lost Path Delay Responses. */
	uint16_t lost_responses;

	/** Current state of the state machine. */
	enum gptp_pdelay_req_states state;

	/** Timer expired, a new Path Delay Request needs to be sent. */
	bool pdelay_timer_expired;

	/** NeighborRateRatio has been computed successfully. */
	bool neighbor_rate_ratio_valid;

	/** Path Delay has already been computed after initialization. */
	bool init_pdelay_compute;

	/** Count consecutive Pdelay_req with multiple responses. */
	uint8_t multiple_resp_count;
};

/**
 * @brief Pdelay Response state machine variables.
 */
struct gptp_pdelay_resp_state {
	/** Current state of the state machine. */
	enum gptp_pdelay_resp_states state;
};

/* SyncReceive state machine variables. */
struct gptp_sync_rcv_state {
	/** Time at which a Sync Message without Follow Up will be discarded. */
	uint64_t follow_up_receipt_timeout;

	/** Timer for the Follow Up discard. */
	struct k_timer follow_up_discard_timer;

	/** Pointer to the received Sync message. */
	struct net_pkt *rcvd_sync_ptr;

	/** Pointer to the received Follow Up message. */
	struct net_pkt *rcvd_follow_up_ptr;

	/** Current state of the state machine. */
	enum gptp_sync_rcv_states state;

	/** A Sync Message has been received. */
	bool rcvd_sync;

	/** A Follow Up Message has been received. */
	bool rcvd_follow_up;

	/** A Follow Up Message has been received. */
	bool follow_up_timeout_expired;
};

/* SyncSend state machine variables. */
struct gptp_sync_send_state {
	/** Pointer to the received MDSyncSend structure. */
	struct gptp_md_sync_info *sync_send_ptr;

	/** Pointer to the sync message to be sent. */
	struct net_pkt *sync_ptr;

	/** Current state of the state machine. */
	enum gptp_sync_send_states state;

	/** A MDSyncSend structure has been received. */
	bool rcvd_md_sync;

	/** The timestamp for the sync message has been received. */
	bool md_sync_timestamp_avail;
};

/* PortSyncSyncReceive state machine variables. */
struct gptp_pss_rcv_state {
	/** Sync receive provided by the MD Sync Receive State Machine. */
	struct gptp_md_sync_info sync_rcv;

	/** PortSyncSync structure to be transmitted to the Site Sync Sync. */
	struct gptp_mi_port_sync_sync pss;

	/** SyncReceiptTimeoutTimer for PortAnnounce state machines. */
	struct k_timer rcv_sync_receipt_timeout_timer;

	/** Ratio of the Grand Master frequency with the Local Clock. */
	double rate_ratio;

	/** Current state of the state machine. */
	enum gptp_pss_rcv_states state;

	/** A MDSyncReceive structure is ready to be processed. */
	bool rcvd_md_sync;

	/** Expiry of SyncReceiptTimeoutTimer. */
	bool rcv_sync_receipt_timeout_timer_expired;
};

/* PortSyncSyncSend state machine variables. */
struct gptp_pss_send_state {
	/** Precise Origin Timestamp of the last received PortSyncSync. */
	struct net_ptp_time last_precise_orig_ts;

	/** Half Sync Interval Timer. */
	struct k_timer half_sync_itv_timer;

	/** syncReceiptTimeout Timer. */
	struct k_timer send_sync_receipt_timeout_timer;

	/** GM Phase Change of the last received PortSyncSync. */
	struct gptp_scaled_ns last_gm_phase_change;

	/** Follow Up Correction Field of the last received PortSyncSync. */
	int64_t last_follow_up_correction_field;

	/** Upstream Tx Time of the last received PortSyncSync. */
	uint64_t last_upstream_tx_time;

	/** PortSyncSync structure received from the SiteSyncSync. */
	struct gptp_mi_port_sync_sync *pss_sync_ptr;

	/** Rate Ratio of the last received PortSyncSync. */
	double last_rate_ratio;

	/** GM Freq Change of the last received PortSyncSync. */
	double last_gm_freq_change;

	/** Current state of the state machine. */
	enum gptp_pss_send_states state;

	/** GM Time Base Indicator of the last received PortSyncSync. */
	uint16_t last_gm_time_base_indicator;

	/** Received Port Number of the last received PortSyncSync. */
	uint16_t last_rcvd_port_num;

	/** Sync send to be transmitted to the MD Sync Send State Machine. */
	struct gptp_md_sync_info sync_send;

	/** A PortSyncSync structure is ready to be processed. */
	bool rcvd_pss_sync;

	/** Flag when the half_sync_itv_timer has expired. */
	bool half_sync_itv_timer_expired;

	/** Flag when the half_sync_itv_timer has expired twice. */
	bool sync_itv_timer_expired;

	/** Source Port Identity of the last received PortSyncSync. */
	struct gptp_port_identity last_src_port_id;

	/** Flag when the syncReceiptTimeoutTime has expired. */
	bool send_sync_receipt_timeout_timer_expired;
};

/* SiteSyncSync state machine variables. */
struct gptp_site_sync_sync_state {
	/** PortSyncSync structure to be sent to other ports and to the Slave.
	 */
	struct gptp_mi_port_sync_sync pss_send;

	/** Pointer to the PortSyncSync structure received. */
	struct gptp_mi_port_sync_sync *pss_rcv_ptr;

	/** Current state of the state machine. */
	enum gptp_site_sync_sync_states state;

	/** A PortSyncSync structure is ready to be processed. */
	bool rcvd_pss;
};

/* ClockSlaveSync state machine variables. */
struct gptp_clk_slave_sync_state {
	/** Pointer to the PortSyncSync structure received. */
	struct gptp_mi_port_sync_sync *pss_rcv_ptr;

	/** Current state of the state machine. */
	enum gptp_clk_slave_sync_states state;

	/** A PortSyncSync structure is ready to be processed. */
	bool rcvd_pss;

	/** The local clock has expired. */
	bool rcvd_local_clk_tick;
};

/* ClockMasterSyncOffset state machine variables. */
struct gptp_clk_master_sync_offset_state {
	/** Current state of the state machine. */
	enum gptp_cms_offset_states state;

	/** Notifies the state machine when Sync Receipt Time is received. */
	bool rcvd_sync_receipt_time;
};

/* ClockMasterSyncSend state machine variables. */
struct gptp_clk_master_sync_snd_state {
	/** Time when synchronization info will be sent. */
	struct gptp_uscaled_ns sync_send_time;

	/** PortSyncSync structure transmitted by the state machine. */
	struct gptp_mi_port_sync_sync pss_snd;

	/** Current state of the state machine. */
	enum gptp_cms_snd_states state;
};

/* ClockMasterSyncReceive state machine variables. */
struct gptp_clk_master_sync_rcv_state {
	/** The received ClockSourceTime.invoke parameters. Note that the
	 * standard defines this as a pointer, but storing the struct here is
	 * more convenient
	 */
	struct gptp_clk_src_time_invoke_params rcvd_clk_src_req;

	/** Current state of the state machine */
	enum gptp_cms_rcv_states state;

	/** A ClockSourceTime.invoke function is received from the
	 * Clock source entity
	 */
	bool rcvd_clock_source_req;

	/** The local clock has expired */
	bool rcvd_local_clock_tick;
};

/* PortAnnounceReceive state machine variables. */
struct gptp_port_announce_receive_state {
	/** Current state of the state machine. */
	enum gptp_pa_rcv_states state;

	/** An announce message is ready to be processed. */
	bool rcvd_announce;
};

struct gptp_port_announce_information_state {
	/** Timer for the announce expiry. */
	struct k_timer ann_rcpt_expiry_timer;

	/** PortRoleInformation state machine variables. */
	enum gptp_pa_info_states state;

	/* Expired announce information. */
	bool ann_expired;
};

/* PortRoleSelection state machine variables. */
struct gptp_port_role_selection_state {
	enum gptp_pr_selection_states state;
};

/**
 * @brief PortAnnounceTransmit state machine variables.
 */
struct gptp_port_announce_transmit_state {
	/** Timer for the announce expiry. */
	struct k_timer ann_send_periodic_timer;

	/** PortRoleTransmit state machine variables. */
	enum gptp_pa_transmit_states state;

	/** Trigger announce information. */
	bool ann_trigger;
};

/**
 * @brief Structure maintaining per Time-Aware States.
 */
struct gptp_states {
	/** SiteSyncSync state machine variables. */
	struct gptp_site_sync_sync_state site_ss;

	/** ClockSlaveSync state machine variables. */
	struct gptp_clk_slave_sync_state clk_slave_sync;

	/** PortRoleSelection state machine variables. */
	struct gptp_port_role_selection_state pr_sel;

	/** ClockMasterSyncOffset state machine variables. */
	struct gptp_clk_master_sync_offset_state clk_master_sync_offset;

	/** ClockMasterSyncSend state machine variables. */
	struct gptp_clk_master_sync_snd_state clk_master_sync_send;

	/** ClockMasterSyncReceive state machine variables. */
	struct gptp_clk_master_sync_rcv_state clk_master_sync_receive;
};

/**
 * @brief Structure maintaining per Port States.
 */
struct gptp_port_states {
	/** PathDelayRequest state machine variables. */
	struct gptp_pdelay_req_state pdelay_req;

	/** SyncReceive state machine variables. */
	struct gptp_sync_rcv_state sync_rcv;

	/** SyncSend state machine variables. */
	struct gptp_sync_send_state sync_send;

	/** PortSyncSyncReceive state machine variables. */
	struct gptp_pss_rcv_state pss_rcv;

	/** PortSyncSync Send state machine variables. */
	struct gptp_pss_send_state pss_send;

	/** PortAnnounceInformation state machine variables. */
	struct gptp_port_announce_information_state pa_info;

	/** PortAnnounceTransmit state machine variables. */
	struct gptp_port_announce_transmit_state pa_transmit;

	/** PathDelayResponse state machine variables. */
	struct gptp_pdelay_resp_state pdelay_resp;

	/** PortAnnounceReceive state machine variables. */
	struct gptp_port_announce_receive_state pa_rcv;
};

/**
 * @brief Structure maintaining per port BMCA state machines variables.
 */
struct gptp_port_bmca_data {
	/** Pointer to announce message. */
	struct net_pkt *rcvd_announce_ptr;

	/** The masterPriorityVector for the port. */
	struct gptp_priority_vector master_priority;

	/** The portPriorityVector for the port. */
	struct gptp_priority_vector port_priority;

	/** Announce interval. */
	struct gptp_uscaled_ns announce_interval;

	/** Announce receipt timeout time interval. */
	struct gptp_uscaled_ns ann_rcpt_timeout_time_interval;

	/** Origin and state of the port's spanning tree information. */
	enum gptp_info_is info_is;

	/** Last announce message time source. */
	enum gptp_time_source ann_time_source;

	/** Last announce message flags. */
	struct gptp_flags ann_flags;

	/** The value of steps removed for the port. */
	uint16_t port_steps_removed;

	/** The value of steps removed for the port. */
	uint16_t message_steps_removed;

	/** Last announce message current UTC offset value. */
	int16_t ann_current_utc_offset;

	/** A qualified announce message has been received. */
	bool rcvd_msg;

	/** Indicate if PortAnnounceInformation should copy the newly determined
	 * master_prioriry and master_steps_removed.
	 */
	bool updt_info;

	/** Cause a port to transmit Announce Information. */
	bool new_info;
};

#ifdef __cplusplus
}
#endif

#endif /* __GPTP_STATE_H */
