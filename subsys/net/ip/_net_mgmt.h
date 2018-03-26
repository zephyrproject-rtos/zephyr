/*
 * Copyright (c) 2018 Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private network events header.
 *        Needed to break circular dependency:
 *        net_mgmt.h -> net_event.h -> wifi_mgmt.h -> net_mgmt.h
 */

#ifndef ___NET_MGMT_H__
#define ___NET_MGMT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_NET_MGMT_EVENT_INFO

#include <net/wifi_mgmt.h>

/* Maximum size of "struct net_event_ipv6_addr" or
 * "struct net_event_ipv6_nbr" or "struct net_event_ipv6_route"
 * or "sizeof(struct wifi_scan_result)"
 * NOTE: Update comments here and calculate which struct occupies max size.
 */
#define NET_EVENT_INFO_MAX_SIZE sizeof(struct wifi_scan_result)

#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ___NET_MGMT_H__ */
