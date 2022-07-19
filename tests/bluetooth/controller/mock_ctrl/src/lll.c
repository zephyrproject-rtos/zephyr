/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include "kconfig.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"
#include "ll_feat.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"
#include "ull_conn_types.h"
#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

extern sys_slist_t ut_rx_q;

__attribute__((weak)) int lll_csrand_get(void *buf, size_t len)
{
	*(int *)buf = 0;
	return 0;
}

__attribute__((weak)) int lll_csrand_isr_get(void *buf, size_t len)
{
	*(int *)buf = 0;
	return 0;
}

uint32_t lll_radio_tx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return 0;
}

void lll_disable(void *param)
{
}
