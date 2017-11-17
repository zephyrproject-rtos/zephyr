/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <bluetooth/hci.h>

#if defined(CONFIG_SOC_FAMILY_NRF)
#include "hal/nrf5/ticker.h"
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "ticker/ticker.h"

#include "ll.h"

#include "ll_mesh.h"

u8_t ll_mesh_advertise(u8_t handle, u8_t own_addr_type,
		       u8_t const *const rand_addr,
		       u8_t chan_map, u8_t tx_pwr,
		       u8_t retry, u8_t interval,
		       u8_t scan_window, u8_t scan_delay, u8_t scan_filter,
		       u8_t data_len, u8_t const *const data)
{
	u8_t err;

	/* convert to 625 us units for internal use */
	interval = ((u32_t)interval + 1) * 10000 / 625;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* Non-conn Non-Scan advertising */
	err = ll_adv_params_set(handle, 0, interval, 0x03, own_addr_type,
				0, NULL, chan_map, 0, NULL, 0, 0, 0, 0, 0);
#else
	err = ll_adv_params_set(interval, 0x03, own_addr_type, 0, NULL,
				chan_map, 0);
#endif
	if (err) {
		return err;
	}

	/* TODO: use the supplied random address instead of global random
	 * address.
	 */

	/* TODO: Tx power */

	/* TODO: multi-instance adv data support */
	ll_adv_data_set(data_len, data);

	/* TODO: scan filter */

	/* Enable advertising instance */
	/* TODO: multi-instance support */
	err = ll_adv_enable(handle, 1, retry, scan_window, scan_delay);

	return err;
}

u8_t ll_mesh_advertise_cancel(u8_t handle)
{
	u8_t err;

	/* TODO: multi-instance support */
	err = ll_adv_enable(handle, 0, 0, 0, 0);

	return err;
}
