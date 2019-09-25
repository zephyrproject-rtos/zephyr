/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPTP Media Dependent interface for full duplex and point to point
 *
 * This is not to be included by the application.
 */

#ifndef __GPTP_MD_H
#define __GPTP_MD_H

#include <net/gptp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Media Dependent Sync Information.
 *
 * This structure applies for MDSyncReceive as well as MDSyncSend.
 */
struct gptp_md_sync_info {
	/* Time of the current grandmaster compared to the previous. */
	struct gptp_scaled_ns last_gm_phase_change;

	/** Most recent preciseOriginTimestamp from the PortSyncSync. */
	struct net_ptp_time precise_orig_ts;

	/** Most recent followupCorrectionField from the PortSyncSync. */
	s64_t follow_up_correction_field;

	/** Most recent upstreamTxTime from the PortSyncSync. */
	u64_t upstream_tx_time;

	/* Frequency of the current grandmaster compared to the previous. */
	double last_gm_freq_change;

	/** Most recent rateRatio from the PortSyncSync. */
	double rate_ratio;

	/** PortIdentity of this port. */
	struct gptp_port_identity src_port_id;

	/* Time Base Indicator of the current Grand Master. */
	u16_t gm_time_base_indicator;

	/** Current Log Sync Interval for this port. */
	s8_t log_msg_interval;
};

/**
 * @brief Initialize all Media Dependent State Machines.
 */
void gptp_md_init_state_machine(void);

/**
 * @brief Run all Media Dependent State Machines.
 *
 * @param port Number of the port the State Machines needs to be run on.
 */
void gptp_md_state_machines(int port);

#ifdef __cplusplus
}
#endif

#endif /* __GPTP_MD_H */
