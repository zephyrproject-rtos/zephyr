/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf_802154_serialization_crit_sect.h"

#ifndef TEST
#include <irq.h>
#endif

void nrf_802154_serialization_crit_sect_enter(uint32_t *p_critical_section)
{
#ifndef TEST
	*p_critical_section = irq_lock();
#else
	(void)p_critical_section;
#endif
}

void nrf_802154_serialization_crit_sect_exit(uint32_t critical_section)
{
#ifndef TEST
	irq_unlock(critical_section);
#else
	(void)critical_section;
#endif
}
