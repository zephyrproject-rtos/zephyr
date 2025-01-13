/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_SAMPLE__
#define __DSA_SAMPLE__

#include <zephyr/net/dsa.h>
#include <zephyr/net/ethernet.h>

extern struct ud user_data;

/* User data for the interface callback */
struct ud {
	struct net_if *lan[CONFIG_NET_SAMPLE_DSA_MAX_SLAVE_PORTS];
	struct net_if *master;
};

#endif
