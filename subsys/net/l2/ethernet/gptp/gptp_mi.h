/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPTP Media Independent interface
 *
 * This is not to be included by the application.
 */

#ifndef __GPTP_MI_H
#define __GPTP_MI_H

#include "gptp_md.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Media Independent Sync Information.
 *
 * This structure applies for MDSyncReceive as well as MDSyncSend.
 */
struct gptp_mi_port_sync_sync {
	/** Time at which the sync receipt timeout occurs. */
	uint64_t sync_receipt_timeout_time;

	/** Copy of the gptp_md_sync_info to be transmitted. */
	struct gptp_md_sync_info sync_info;

	/** Port to which the Sync Information belongs to. */
	uint16_t local_port_number;
};

/**
 * @brief Initialize all Media Independent State Machines.
 */
void gptp_mi_init_state_machine(void);

/**
 * @brief Run all Media Independent Port Sync State Machines.
 *
 * @param port Number of the port the State Machines needs to be run on.
 */
void gptp_mi_port_sync_state_machines(int port);

/**
 * @brief Run all Media Independent Port BMCA State Machines.
 *
 * @param port Number of the port the State Machines needs to be run on.
 */
void gptp_mi_port_bmca_state_machines(int port);

/**
 * @brief Run all Media Independent State Machines.
 */
void gptp_mi_state_machines(void);

/**
 * @brief Return current time in nanoseconds.
 *
 * @param port Port number of the clock to use.
 *
 * @return Current time in nanoseconds.
 */
uint64_t gptp_get_current_time_nanosecond(int port);

#ifdef __cplusplus
}
#endif

#endif /* __GPTP_MI_H */
