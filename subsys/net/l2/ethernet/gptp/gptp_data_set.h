/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PTP data sets
 *
 * This is not to be included by the application.
 */

#ifndef __GPTP_DS_H
#define __GPTP_DS_H

#if defined(CONFIG_NET_GPTP)

#include <net/gptp.h>
#include "gptp_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parameters for PTP data sets. */
#define GPTP_ALLOWED_LOST_RESP 3

#if defined(CONFIG_NET_GPTP_NEIGHBOR_PROP_DELAY_THR)
#define GPTP_NEIGHBOR_PROP_DELAY_THR CONFIG_NET_GPTP_NEIGHBOR_PROP_DELAY_THR
#else
/* See IEEE802.1AS B.3 should be less than 800ns (cur: 100us). */
#define GPTP_NEIGHBOR_PROP_DELAY_THR 100000
#endif

/* Max number of ClockIdentities in pathTrace. */
#define GPTP_MAX_PATHTRACE_SIZE CONFIG_NET_GPTP_PATH_TRACE_ELEMENTS

/* Helpers to access gptp_domain fields. */
#define GPTP_PORT_START 1
#define GPTP_PORT_END (gptp_domain.default_ds.nb_ports + GPTP_PORT_START)

#define GPTP_PORT_INDEX (port - GPTP_PORT_START)

#define GPTP_GLOBAL_DS() (&gptp_domain.global_ds)
#define GPTP_DEFAULT_DS() (&gptp_domain.default_ds)
#define GPTP_CURRENT_DS() (&gptp_domain.current_ds)
#define GPTP_PARENT_DS() (&gptp_domain.parent_ds)
#define GPTP_PROPERTIES_DS() (&gptp_domain.properties_ds)
#define GPTP_STATE() (&gptp_domain.state)

#define GPTP_PORT_DS(port) \
	(&gptp_domain.port_ds[port - GPTP_PORT_START])
#define GPTP_PORT_STATE(port) \
	(&gptp_domain.port_state[port - GPTP_PORT_START])
#define GPTP_PORT_BMCA_DATA(port) \
	(&gptp_domain.port_bmca_data[port - GPTP_PORT_START])
#define GPTP_PORT_IFACE(port) \
	gptp_domain.iface[port - GPTP_PORT_START]

#if defined(CONFIG_NET_GPTP_STATISTICS)
#define GPTP_PORT_PARAM_DS(port)				\
	(&gptp_domain.port_param_ds[port - GPTP_PORT_START])
#endif

#define CLEAR_RESELECT(global_ds, port) \
	(global_ds->reselect_array &= (~(1 << (port - 1))))
#define SET_RESELECT(global_ds, port) \
	(global_ds->reselect_array |= (1 << (port - 1)))
#define CLEAR_SELECTED(global_ds, port) \
	(global_ds->selected_array &= (~(1 << (port - 1))))
#define SET_SELECTED(global_ds, port) \
	(global_ds->selected_array |= (1 << (port - 1)))
#define IS_SELECTED(global_ds, port) \
	((global_ds->selected_array >> (port - 1)) & 0x1)
#define IS_RESELECT(global_ds, port) \
	((global_ds->reselect_array >> (port - 1)) & 0x1)

/*
 * Global definition of the gPTP domain.
 * Note: Only one domain is supported for now.
 */
extern struct gptp_domain gptp_domain;

/*
 * Type of TLV message received.
 */
enum gptp_tlv_type {
	GPTP_TLV_MGNT = 0x0001,
	GPTP_TLV_MGNT_ERR_STATUS              = 0x0002,
	GPTP_TLV_ORGANIZATION_EXT             = 0x0003,
	GPTP_TLV_REQ_UNICAST_TX               = 0x0004,
	GPTP_TLV_GRANT_UNICAST_TX             = 0x0005,
	GPTP_TLV_CANCEL_UNICAST_TX            = 0x0006,
	GPTP_TLV_ACK_CANCEL_UNICAST_TX        = 0x0007,
	GPTP_TLV_PATH_TRACE                   = 0x0008,
	GPTP_TLV_ALT_TIME_OFFSET_INDICATOR    = 0x0009,
	GPTP_TLV_AUTH                         = 0x2000,
	GPTP_TLV_AUTH_CHALLENGE               = 0x2001,
	GPTP_TLV_SECURITY_ASSOC_UPDATE        = 0x2002,
	GPTP_TLV_CUM_FREQ_SCALE_FACTOR_OFFSET = 0x2003,
};

/*
 * Class of the local clock used for a port.
 * This is used when determining the Grand Master.
 */
enum gptp_clock_class {
	GPTP_CLASS_PRIMARY                 = 6,
	GPTP_CLASS_APP_SPECIFIC            = 13,
	GPTP_CLASS_APP_SPECIFIC_LOST       = 14,
	GPTP_CLASS_PRIMARY_DEGRADED_A      = 52,
	GPTP_CLASS_APP_SPECIFIC_DEGRADED_A = 58,
	GPTP_CLASS_PRIMARY_DEGRADED_B      = 187,
	GPTP_CLASS_APP_SPECIFIC_DEGRADED_B = 193,
	GPTP_CLASS_OTHER                   = 248,
	GPTP_CLASS_SLAVE_ONLY              = 255,
};

/*
 * For gPTP, only a subset are used.
 * - DisabledPort
 * - MasterPort
 * - PassivePort
 * - SlavePort
 */
enum gptp_port_state {
	GPTP_PORT_INITIALIZING,
	GPTP_PORT_FAULTY,
	GPTP_PORT_DISABLED,
	GPTP_PORT_LISTENING,
	GPTP_PORT_PRE_MASTER,
	GPTP_PORT_MASTER,
	GPTP_PORT_PASSIVE,
	GPTP_PORT_UNCALIBRATED,
	GPTP_PORT_SLAVE,
};

enum gptp_received_info {
	GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO,
	GPTP_RCVD_INFO_REPEATED_MASTER_INFO,
	GPTP_RCVD_INFO_INFERIOR_MASTER_INFO,
	GPTP_RCVD_INFO_OTHER_INFO,
};

/**
 * @brief Announce path trace retaining structure.
 */
struct gptp_path_trace {
	/** Length of the path trace. */
	uint16_t len;

	/** Path trace of the announce message. */
	uint8_t path_sequence[GPTP_MAX_PATHTRACE_SIZE][GPTP_CLOCK_ID_LEN];
};

/**
 * @brief Per-time-aware system global variables.
 *
 * Not all variables from the standard are defined yet.
 * The structure is to be enhanced with missing fields when those are needed.
 *
 * selectedRole is not defined here as it is a duplicate of the port_state
 * variable declared in gptp_port_ds.
 */
struct gptp_global_ds {
	/** Mean time interval between messages providing time-sync info. */
	uint64_t clk_master_sync_itv;

	/** Value if current time. */
	uint64_t sync_receipt_local_time;

	/** Fractional frequency offset of the Clock Source entity. */
	double clk_src_freq_offset;

	/** Last Grand Master Frequency Change. */
	double clk_src_last_gm_freq_change;

	/** Ratio of the frequency of the ClockSource to the LocalClock. */
	double gm_rate_ratio;

	/** Last Grand Master Frequency Change. */
	double last_gm_freq_change;

	/** The synchronized time computed by the ClockSlave entity. */
	struct net_ptp_extended_time sync_receipt_time;

	/** Last Grand Master Phase Change. */
	struct gptp_scaled_ns clk_src_last_gm_phase_change;

	/** Last Grand Master Phase Change. */
	struct gptp_scaled_ns last_gm_phase_change;

	/** Global flags. */
	struct gptp_flags global_flags;

	/** System current flags. */
	struct gptp_flags sys_flags;

	/** Path trace to be sent in announce message. */
	struct gptp_path_trace path_trace;

	/** Grand Master priority vector. */
	struct gptp_priority_vector gm_priority;

	/** Previous Grand Master priority vector. */
	struct gptp_priority_vector last_gm_priority;

	/** Value of current_time when the last invoke call was received. */
	struct gptp_uscaled_ns local_time;

	/** Time maintained by the ClockMaster entity. */
	struct net_ptp_extended_time master_time;

	/** Time provided by the ClockSource entity minus the sync time. */
	struct gptp_scaled_ns clk_src_phase_offset;

	/** Grand Master Time Base Indicator. */
	uint16_t gm_time_base_indicator;

	/** Reselect port bit array. */
	uint32_t reselect_array;

	/** Selected port bit array. */
	uint32_t selected_array;

	/** Steps removed from selected master. */
	uint16_t master_steps_removed;

	/** Current UTC offset. */
	int16_t current_utc_offset;

	/** System current UTC offset. */
	int16_t sys_current_utc_offset;

	/** Time Base Indicator. */
	uint16_t clk_src_time_base_indicator;

	/** Previous Time Base Indicator. */
	uint16_t clk_src_time_base_indicator_prev;

	/** Time source. */
	enum gptp_time_source time_source;

	/** System time source. */
	enum gptp_time_source sys_time_source;

	/** Selected port Roles. */
	enum gptp_port_state selected_role[CONFIG_NET_GPTP_NUM_PORTS + 1];

	/** A Grand Master is present in the domain. */
	bool gm_present;
};

/**
 * @brief Default Parameter Data Set.
 *
 * Data Set representing capabilities of the time-aware system.
 */
struct gptp_default_ds {
	/** Quality of the local clock. */
	struct gptp_clock_quality clk_quality;

	/* Source of time used by the Grand Master Clock. */
	enum gptp_time_source time_source;

	/** Clock Identity of the local clock. */
	uint8_t clk_id[GPTP_CLOCK_ID_LEN];

	/** System current flags. */
	struct gptp_flags flags;

	/** Current UTC offset. */
	uint16_t cur_utc_offset;

	/** Defines if this system is Grand Master capable. */
	bool gm_capable;

	/** Number of ports of the time-aware system. */
	uint8_t nb_ports;

	/** Primary priority of the time-aware system. */
	uint8_t priority1;

	/** Secondary priority of the time-aware system. */
	uint8_t priority2;
};

/**
 * @brief Current Parameter Data Set.
 *
 * Data Set representing information relative to the Grand Master.
 */
struct gptp_current_ds {
	/** Last Grand Master Phase change . */
	struct gptp_scaled_ns last_gm_phase_change;

	/** Time difference between a slave and the Grand Master. */
	int64_t offset_from_master;

	/** Last Grand Master Frequency change. */
	double last_gm_freq_change;

	/** Number of times a Grand Master has changed in the domain. */
	uint32_t gm_change_count;

	/** Time when the most recent Grand Master changed. */
	uint32_t last_gm_chg_evt_time;

	/** Time when the most recent Grand Master phase changed. */
	uint32_t last_gm_phase_chg_evt_time;

	/** Time when the most recent Grand Master frequency changed. */
	uint32_t last_gm_freq_chg_evt_time;

	/** Time Base Indicator of the current Grand Master. */
	uint16_t gm_timebase_indicator;

	/** Number of steps between the local clock and the Grand Master. */
	uint8_t steps_removed;
};

/**
 * @brief Parent Parameter Data Set.
 *
 * Data Set representing the parent capabilities.
 */
struct gptp_parent_ds {
	/** Ratio of the frequency of the GM with the local clock. */
	int32_t cumulative_rate_ratio;

	/** Clock Identity of the Grand Master clock. */
	uint8_t gm_id[GPTP_CLOCK_ID_LEN];

	/** Clock Class of the Grand Master clock. */
	struct gptp_clock_quality gm_clk_quality;

	/** Port Identity of the Master Port attached to this system. */
	struct gptp_port_identity port_id;

	/** Primary Priority of the Grand Master clock. */
	uint8_t gm_priority1;

	/** Secondary Priority of the Grand Master clock. */
	uint8_t gm_priority2;
};

/**
 * @brief Time Properties Parameter Data Set.
 *
 * Data Set representing Grand Master capabilities from the point of view
 * of this system.
 */
struct gptp_time_prop_ds {
	/** The time source of the Grand Master. */
	enum gptp_time_source time_source;

	/** Current UTC offset for the Grand Master. */
	uint16_t cur_utc_offset;

	/** Current UTC offset valid for the Grand Master. */
	bool cur_utc_offset_valid : 1;

	/** The Grand Master will have 59s at the end of the current UTC day.
	 */
	bool leap59 : 1;

	/** The Grand Master will have 61s at the end of the current UTC day.
	 */
	bool leap61 : 1;

	/** The current UTC offset of the GM is traceable to a primary ref. */
	bool time_traceable : 1;

	/** The frequency of the Grand Master is traceable to a primary ref. */
	bool freq_traceable : 1;
};

/**
 * @brief Port Parameter Data Set.
 *
 * Data Set representing port capabilities.
 */
struct gptp_port_ds {
	/** Maximum interval between sync messages. */
	uint64_t sync_receipt_timeout_time_itv;

	/** Asymmetry on the link relative to the grand master time base. */
	int64_t delay_asymmetry;

	/** One way propagation time on the link attached to this port. */
	double neighbor_prop_delay;

	/** Propagation time threshold for the link attached to this port. */
	double neighbor_prop_delay_thresh;

	/** Estimate of the ratio of the frequency with the peer. */
	double neighbor_rate_ratio;

	/** Port Identity of the port. */
	struct gptp_port_identity port_id;

	/** Sync event transmission interval for the port. */
	struct gptp_uscaled_ns half_sync_itv;

	/** Path Delay Request transmission interval for the port. */
	struct gptp_uscaled_ns pdelay_req_itv;

	/** Maximum number of Path Delay Requests without a response. */
	uint16_t allowed_lost_responses;

	/** Current Sync sequence id for this port. */
	uint16_t sync_seq_id;

	/** Current Path Delay Request sequence id for this port. */
	uint16_t pdelay_req_seq_id;

	/** Current Announce sequence id for this port. */
	uint16_t announce_seq_id;

	/** Current Signaling sequence id for this port. */
	uint16_t signaling_seq_id;

	/** Initial Announce Interval as a Logarithm to base 2. */
	int8_t ini_log_announce_itv;

	/** Current Announce Interval as a Logarithm to base 2. */
	int8_t cur_log_announce_itv;

	/** Time without receiving announce messages before running BMCA. */
	uint8_t announce_receipt_timeout;

	/** Initial Sync Interval as a Logarithm to base 2. */
	int8_t ini_log_half_sync_itv;

	/** Current Sync Interval as a Logarithm to base 2. */
	int8_t cur_log_half_sync_itv;

	/** Time without receiving sync messages before running BMCA. */
	uint8_t sync_receipt_timeout;

	/** Initial Path Delay Request Interval as a Logarithm to base 2. */
	int8_t ini_log_pdelay_req_itv;

	/** Current Path Delay Request Interval as a Logarithm to base 2. */
	int8_t cur_log_pdelay_req_itv;

	/** Version of PTP running on this port. */
	uint8_t version;

	/** Time synchronization and Best Master Selection enabled. */
	bool ptt_port_enabled : 1;

	/** Previous status of ptt_port_enabled. */
	bool prev_ptt_port_enabled : 1;

	/** The port is measuring the path delay. */
	bool is_measuring_delay : 1;

	/** The port is capable of running IEEE802.1AS. */
	bool as_capable : 1;

	/** Whether neighborRateRatio needs to be computed for this port. */
	bool compute_neighbor_rate_ratio : 1;

	/** Whether neighborPropDelay needs to be computed for this port. */
	bool compute_neighbor_prop_delay : 1;

	/** Whether neighbor rate ratio can be used to update clocks. */
	bool neighbor_rate_ratio_valid : 1;
};

/**
 * @brief Port Parameter Statistics.
 *
 * Data Set containing statistics associated with various events.
 */
struct gptp_port_param_ds {
	/** Number of Sync messages received. */
	uint32_t rx_sync_count;

	/** Number of Follow Up messages received. */
	uint32_t rx_fup_count;

	/** Number of Path Delay Requests messages received. */
	uint32_t rx_pdelay_req_count;

	/** Number of Path Delay Response messages received. */
	uint32_t rx_pdelay_resp_count;

	/** Number of Path Delay Follow Up messages received. */
	uint32_t rx_pdelay_resp_fup_count;

	/** Number of Announce messages received. */
	uint32_t rx_announce_count;

	/** Number of ptp messages discarded. */
	uint32_t rx_ptp_packet_discard_count;

	/** Number of Sync reception timeout. */
	uint32_t sync_receipt_timeout_count;

	/** Number of Announce reception timeout. */
	uint32_t announce_receipt_timeout_count;

	/** Number Path Delay Requests without a response. */
	uint32_t pdelay_allowed_lost_resp_exceed_count;

	/** Number of Sync messages sent. */
	uint32_t tx_sync_count;

	/** Number of Follow Up messages sent. */
	uint32_t tx_fup_count;

	/** Number of Path Delay Request messages sent. */
	uint32_t tx_pdelay_req_count;

	/** Number of Path Delay Response messages sent. */
	uint32_t tx_pdelay_resp_count;

	/** Number of Path Delay Response messages sent. */
	uint32_t tx_pdelay_resp_fup_count;

	/** Number of Announce messages sent. */
	uint32_t tx_announce_count;

	/** Neighbor propagation delay threshold exceeded. */
	uint32_t neighbor_prop_delay_exceeded;
};

/**
 * @brief gPTP domain.
 *
 * Data Set containing all the information necessary to represent
 * one time-aware system domain.
 */
struct gptp_domain {
	/** Global Data Set for this gPTP domain. */
	struct gptp_global_ds global_ds;

	/** Default Data Set for this gPTP domain. */
	struct gptp_default_ds default_ds;

	/** Current Data Set for this gPTP domain. */
	struct gptp_current_ds current_ds;

	/** Parent Data Set for this gPTP domain. */
	struct gptp_parent_ds parent_ds;

	/** Time Properties Data Set for this gPTP domain. */
	struct gptp_time_prop_ds properties_ds;

	/** Current State of the MI State Machines for this gPTP domain. */
	struct gptp_states state;

	/** Port Parameter Data Sets for this gPTP domain. */
	struct gptp_port_ds port_ds[CONFIG_NET_GPTP_NUM_PORTS];

#if defined(CONFIG_NET_GPTP_STATISTICS)
	/** Port Parameter Statistics Data Sets for this gPTP domain. */
	struct gptp_port_param_ds port_param_ds[CONFIG_NET_GPTP_NUM_PORTS];
#endif /* CONFIG_NET_GPTP_STATISTICS */

	/** Current States of the MD State Machines for this gPTP domain. */
	struct gptp_port_states port_state[CONFIG_NET_GPTP_NUM_PORTS];

	/** Shared data between BMCA State Machines for this gPTP domain. */
	struct gptp_port_bmca_data port_bmca_data[CONFIG_NET_GPTP_NUM_PORTS];

	/* Network interface linked to the PTP PORT. */
	struct net_if *iface[CONFIG_NET_GPTP_NUM_PORTS];
};

/**
 * @brief Get port specific data from gPTP domain.
 * @details This contains all the configuration / status of the gPTP domain.
 *
 * @param domain gPTP domain
 * @param port Port id
 * @param port_ds Port specific parameter data set (returned to caller)
 * @param port_param_ds Port parameter statistics data set (returned to caller)
 * @param port_state Port specific states data set (returned to caller)
 * @param port_bmca Port BMCA state machine specific data (returned to caller)
 * @param iface Port specific parameter data set (returned to caller)
 *
 * @return 0 if ok, < 0 if error
 */
int gptp_get_port_data(struct gptp_domain *domain, int port,
		       struct gptp_port_ds **port_ds,
		       struct gptp_port_param_ds **port_param_ds,
		       struct gptp_port_states **port_state,
		       struct gptp_port_bmca_data **port_bmca_data,
		       struct net_if **iface);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_NET_GPTP */

#endif /* __GPTP_DS_H */
