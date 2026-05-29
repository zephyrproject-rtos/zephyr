/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include "nrf_802154.h"
#include "nrf_802154_serialization.h"

static int serialization_init(void)
{
	/* On NET core we don't use Zephyr's shim layer so we have to call inits manually */
	nrf_802154_init();

	nrf_802154_serialization_init();

	return 0;
}

BUILD_ASSERT(CONFIG_NRF_802154_SER_RADIO_INIT_PRIO > CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	     "CONFIG_NRF_802154_SER_RADIO_INIT_PRIO must be higher than CONFIG_KERNEL_INIT_PRIORITY_DEVICE");
SYS_INIT(serialization_init, POST_KERNEL, CONFIG_NRF_802154_SER_RADIO_INIT_PRIO);
