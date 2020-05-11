/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/types.h>

#include "util/util.h"

/* Initial channel map indicating Used and Unused data channels.
 * The HCI LE Set Host Channel Classification command allows the Host to
 * specify a channel classification for the data, secondary advertising,
 * periodic, and isochronous physical channels based on its local information.
 */
static uint8_t map[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
static uint8_t count = 37U;

int ull_chan_reset(void)
{
	/* initialise connection channel map */
	map[0] = 0xFF;
	map[1] = 0xFF;
	map[2] = 0xFF;
	map[3] = 0xFF;
	map[4] = 0x1F;
	count = 37U;

	return 0;
}

uint8_t ull_chan_map_get(uint8_t *const chan_map)
{
	memcpy(chan_map, map, sizeof(map));

	return count;
}

void ull_chan_map_set(uint8_t const *const chan_map)
{
	memcpy(map, chan_map, sizeof(map));
	count = util_ones_count_get(map, sizeof(map));
}
