/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

/* TODO: Remove weak attribute when refactored architecture replaces the old
 *       controller implementation. */
#include <toolchain.h>

u8_t __weak ll_tx_pwr_lvl_get(u16_t handle, u8_t type, s8_t *tx_pwr_lvl)
{
	/* TODO: check for active connection */

	/* TODO: check type here for current or maximum */

	/* TODO: Support TX Power Level other than 0dBm */
	*tx_pwr_lvl = 0;

	return 0;
}

void ll_tx_pwr_get(s8_t *min, s8_t *max)
{
	/* TODO: Support TX Power Level other than 0dBm */
	*min = 0;
	*max = 0;
}

