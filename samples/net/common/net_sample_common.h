/*
 * Copyright (c) 2024 Nordic Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_CONNECTION_MANAGER)
void wait_for_network(void);
#else
static inline void wait_for_network(void) { }
#endif /* CONFIG_NET_CONNECTION_MANAGER */

#if defined(CONFIG_NET_VLAN)
int init_vlan(void);
#else
static inline int init_vlan(void)
{
	return 0;
}
#endif /* CONFIG_NET_VLAN */
