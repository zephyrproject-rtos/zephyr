/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_TEST_APP)
#define SYS_LOG_DOMAIN "net-test/vlan"
#define NET_SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_LEVEL
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>

#include "common.h"

#if CONFIG_NET_VLAN_COUNT > 1
#define CREATE_MULTIPLE_TAGS
#endif

int setup_vlan(struct interfaces *interfaces)
{
	int ret;

	/* For SLIP technology, we create one VLAN interface */
#if !defined(CREATE_MULTIPLE_TAGS)
	ret = net_eth_vlan_enable(interfaces->non_vlan,
				  CONFIG_SAMPLE_VLAN_TAG_1);
	if (ret < 0) {
		NET_ERR("Cannot enable VLAN for tag %d (%d)",
			CONFIG_SAMPLE_VLAN_TAG_1, ret);
	}
#endif

#if defined(CREATE_MULTIPLE_TAGS)
	/* This sample has two VLANs. First the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = net_eth_vlan_enable(interfaces->first_vlan,
				  CONFIG_SAMPLE_VLAN_TAG_1);
	if (ret < 0) {
		NET_ERR("Cannot enable VLAN for tag %d (%d)",
			CONFIG_SAMPLE_VLAN_TAG_1, ret);
	}

	ret = net_eth_vlan_enable(interfaces->second_vlan,
				  CONFIG_SAMPLE_VLAN_TAG_2);
	if (ret < 0) {
		NET_ERR("Cannot enable VLAN for tag %d (%d)",
			CONFIG_SAMPLE_VLAN_TAG_2, ret);
	}

#endif

	return ret;
}
