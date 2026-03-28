/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "ull_adv_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "ull_sync_types.h"

#define BT_PER_ADV_SYNC_MAX 1

static struct ll_sync_set ll_sync[BT_PER_ADV_SYNC_MAX];

struct ll_sync_set *ull_sync_set_get(uint16_t handle)
{
	if (handle >= BT_PER_ADV_SYNC_MAX) {
		return NULL;
	}

	return &ll_sync[handle];
}

struct ll_sync_set *ull_sync_is_enabled_get(uint16_t handle)
{
	struct ll_sync_set *sync;

	sync = ull_sync_set_get(handle);
	if (!sync) {
		return NULL;
	}

	return sync;
}

uint16_t ull_sync_handle_get(struct ll_sync_set *sync)
{
	return 0;
}
