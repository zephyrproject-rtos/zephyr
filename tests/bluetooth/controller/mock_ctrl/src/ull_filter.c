/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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
#include "lll_filter.h"

static uint8_t bt_addr[BDADDR_SIZE] = { 0, 0, 0, 0, 0, 0};

#define BT_CTLR_RL_SIZE 8

uint8_t ll_rl_size_get(void)
{
	return BT_CTLR_RL_SIZE;
}

uint8_t ull_filter_rl_find(uint8_t id_addr_type, uint8_t const *const id_addr,
			   uint8_t *const free_idx)
{
	return FILTER_IDX_NONE;
}

void ull_filter_rpa_update(bool timeout)
{

}

const uint8_t *ull_filter_tgta_get(uint8_t rl_idx)
{
	return bt_addr;
}
