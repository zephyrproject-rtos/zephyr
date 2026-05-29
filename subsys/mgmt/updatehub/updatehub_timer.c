/*
 * Copyright (c) 2020 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(updatehub, CONFIG_UPDATEHUB_LOG_LEVEL);

#include <zephyr/kernel.h>
#include "updatehub_timer.h"

static int blk_vars[UPDATEHUB_BLK_MAX_VARS];

static void timer_expire(struct k_timer *timer)
{
	LOG_DBG("tmr_expire");
	blk_vars[UPDATEHUB_BLK_TX_AVAILABLE] = 1;
}

K_TIMER_DEFINE(uhu_packet_down_tmr, timer_expire, NULL);

int updatehub_blk_get(enum updatehub_blk_vars var)
{
	LOG_DBG("blk_get[%d] = %d", var, blk_vars[var]);
	return blk_vars[var];
}

void updatehub_blk_inc(enum updatehub_blk_vars var)
{
	blk_vars[var]++;
	LOG_DBG("blk_inc[%d] = %d", var, blk_vars[var]);
}

void updatehub_blk_set(enum updatehub_blk_vars var, int val)
{
	LOG_DBG("blk_set[%d] = %d", var, val);
	blk_vars[var] = val;
}

void updatehub_tmr_start(void)
{
	LOG_DBG("tmr_start");
	k_timer_start(&uhu_packet_down_tmr,
		      K_SECONDS(CONFIG_UPDATEHUB_COAP_CONN_TIMEOUT),
		      K_NO_WAIT);
}

void updatehub_tmr_stop(void)
{
	LOG_DBG("tmr_stop");
	k_timer_stop(&uhu_packet_down_tmr);
}
