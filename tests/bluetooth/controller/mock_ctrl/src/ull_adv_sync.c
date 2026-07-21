/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci_types.h>

#include "util/mem.h"
#include "util/memq.h"
#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"

struct ll_adv_sync_set *ull_adv_sync_get(uint8_t handle)
{
	return NULL;
}

uint16_t ull_adv_sync_handle_get(const struct ll_adv_sync_set *sync)
{
	return 0;
}
