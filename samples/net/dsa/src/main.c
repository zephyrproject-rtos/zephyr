/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_sample, CONFIG_NET_DSA_LOG_LEVEL);

#include "dsa.h"

#if defined(CONFIG_NET_SAMPLE_DSA_LLDP)
#include "dsa_lldp.h"
#endif

struct ud user_data;

static void dsa_iface_find_cb(struct net_if *iface, void *user_data)
{

	struct ud *ifaces = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) {
		if (ifaces->master == NULL) {
			ifaces->master = iface;

			/* Get slave interfaces */
			for (int i = 0; i < ARRAY_SIZE(ifaces->lan); i++) {
				struct net_if *slave = dsa_get_slave_port(iface, i);

				if (slave == NULL) {
					continue;
				}
				LOG_INF("Slave interface %d found.", i);

				ifaces->lan[i] = slave;
			}
			return;
		}
	}
}

int main(void)
{
	/* Initialize interfaces - read them to user_data */
	(void)memset(&user_data, 0, sizeof(user_data));
	net_if_foreach(dsa_iface_find_cb, &user_data);

#if defined(CONFIG_NET_SAMPLE_DSA_LLDP)
	dsa_lldp(&user_data);
#endif
	LOG_INF("DSA ports init - OK");
	return 0;
}
