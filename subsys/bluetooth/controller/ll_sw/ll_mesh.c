/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <bluetooth/hci.h>

#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "ll.h"
#include "ll_mesh.h"

u8_t ll_mesh_advertise(u8_t handle, u8_t own_addr_type,
		       u8_t const *const rand_addr,
		       u8_t chan_map, u8_t tx_pwr,
		       u8_t min_tx_delay, u8_t max_tx_delay,
		       u8_t retry, u8_t interval,
		       u8_t scan_window, u8_t scan_delay, u8_t scan_filter,
		       u8_t data_len, u8_t const *const data)
{
	u32_t ticks_anchor;
	u8_t err;

	/* convert to 625 us units for internal use */
	interval = ((u32_t)interval + 1) * 10000U / 625;

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
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	ll_adv_data_set(handle, data_len, data);
#else
	ll_adv_data_set(data_len, data);
#endif

	/* TODO: scan filter */

	/* TODO: calculate random tx delay */
	ticks_anchor = ticker_ticks_now_get();
	ticks_anchor += HAL_TICKER_US_TO_TICKS(min_tx_delay * 10000U);

	/* Enable advertising instance */
	err = ll_adv_enable(handle, 1,
			    1, ticks_anchor, retry,
			    scan_window, scan_delay);

	return err;
}

u8_t ll_mesh_advertise_cancel(u8_t handle)
{
	u8_t err;

	/* TODO: multi-instance support */
	err = ll_adv_enable(handle, 0, 0, 0, 0, 0, 0);

	return err;
}
