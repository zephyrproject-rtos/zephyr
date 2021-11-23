/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <net/net_if.h>
#include <net/ptp_time.h>

/* Slave PTP clock functions. */
int ptp_slave_clk_get(struct net_ptp_time *net);

/* Initializes PTP slave, returns non-zero on error. */
int ptp_slave_init(struct net_if *iface, int delay_req_offset_ms);

/* Starts PTP slave, returns non-zero on error. */
int ptp_slave_start(void);
