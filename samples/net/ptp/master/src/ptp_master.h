/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <net/net_if.h>
#include <net/ptp_time.h>

/* Master PTP clock functions. */
int ptp_master_clk_set(struct net_ptp_time *net);
int ptp_master_clk_get(struct net_ptp_time *net);
int ptp_master_clk_rate_adjust(float rate);

/* Initializes PTP master, returns non-zero on error. */
int ptp_master_init(struct net_if *iface);

/* Starts PTP master, returns non-zero on error. */
int ptp_master_start(void);
