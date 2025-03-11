/*
 * Copyright (c) 2025 Tailored Technology Ltd
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************/
/* Groupchat Feature : Common         */
/*  Shared by primary/mixer/secondary */
/**************************************/

#include <stddef.h>
#include <stdint.h>
#include "common.h"

uint8_t get_bis_idx(struct bt_iso_chan *chan)
{
	/* valid bis index values are 1 .. CONFIG_BT_ISO_MAX_CHAN*/
	if(chan->bis_idx > CONFIG_BT_ISO_MAX_CHAN) {
		return 0;
	}
	return chan->bis_idx;
}

