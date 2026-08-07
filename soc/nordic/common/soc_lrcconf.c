/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc_lrcconf.h>
#include <zephyr/kernel.h>

static struct k_spinlock lock;
static sys_slist_t poweron_main_list;
static sys_slist_t poweron_active_list;

void soc_lrcconf_poweron_request(sys_snode_t *node, nrf_lrcconf_power_domain_mask_t domain)
{
	__ASSERT(is_power_of_two(domain), "Only one bit can be set for the domain parameter");

	sys_slist_t *poweron_list;

	if (domain == NRF_LRCCONF_POWER_MAIN) {
		poweron_list = &poweron_main_list;
	} else if (domain == NRF_LRCCONF_POWER_DOMAIN_0) {
		poweron_list = &poweron_active_list;
	} else {
		return;
	}

	K_SPINLOCK(&lock) {
		if (sys_slist_len(poweron_list) == 0) {
			nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, domain, true);
		}

		sys_slist_find_and_remove(poweron_list, node);
		sys_slist_append(poweron_list, node);
	}
}

void soc_lrcconf_poweron_release(sys_snode_t *node, nrf_lrcconf_power_domain_mask_t domain)
{
	__ASSERT(is_power_of_two(domain), "Only one bit can be set for the domain parameter");

	sys_slist_t *poweron_list;

	if (domain == NRF_LRCCONF_POWER_MAIN) {
		poweron_list = &poweron_main_list;
	} else if (domain == NRF_LRCCONF_POWER_DOMAIN_0) {
		poweron_list = &poweron_active_list;
	} else {
		return;
	}

	K_SPINLOCK(&lock) {
		if (!sys_slist_find_and_remove(poweron_list, node)) {
			K_SPINLOCK_BREAK;
		}

		if (sys_slist_len(poweron_list) > 0) {
			K_SPINLOCK_BREAK;
		}
		nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, domain, false);
	}
}
