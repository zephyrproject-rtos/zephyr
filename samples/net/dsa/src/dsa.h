/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_SAMPLE__
#define __DSA_SAMPLE__

#if defined(CONFIG_NET_DSA_DEPRECATED)
#include <zephyr/net/dsa.h>
#else
#include <zephyr/net/dsa_core.h>
#endif
#include <zephyr/net/ethernet.h>

extern struct ud user_data;

/* User data for the interface callback */
struct ud {
	struct net_if *lan[CONFIG_NET_SAMPLE_DSA_MAX_USER_PORTS];
	struct net_if *conduit;
};

#endif
