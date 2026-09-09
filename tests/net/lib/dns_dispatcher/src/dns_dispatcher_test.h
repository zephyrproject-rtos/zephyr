/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DNS_DISPATCHER_TEST_H_
#define DNS_DISPATCHER_TEST_H_

/*
 * Select single- vs multi-interface test sources. prj_multi_iface.conf sets
 * both CONFIG_NET_IF_MAX_IPV4_COUNT and CONFIG_NET_IF_MAX_IPV6_COUNT to 2.
 */
#if (CONFIG_NET_IF_MAX_IPV6_COUNT >= 2) && (CONFIG_NET_IF_MAX_IPV4_COUNT >= 2)
#define DNS_DISPATCHER_MULTI_IFACE_TEST 1
#else
#define DNS_DISPATCHER_MULTI_IFACE_TEST 0
#endif

#endif /* DNS_DISPATCHER_TEST_H_ */
