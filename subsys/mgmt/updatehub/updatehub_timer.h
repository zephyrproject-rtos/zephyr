/*
 * Copyright (c) 2020 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_TIMER_H__
#define __UPDATEHUB_TIMER_H__

enum updatehub_blk_vars {
	UPDATEHUB_BLK_ATTEMPT,
	UPDATEHUB_BLK_INDEX,
	UPDATEHUB_BLK_TX_AVAILABLE,

	UPDATEHUB_BLK_MAX_VARS,
};

int updatehub_blk_get(enum updatehub_blk_vars var);
void updatehub_blk_inc(enum updatehub_blk_vars var);
void updatehub_blk_set(enum updatehub_blk_vars var, int val);

void updatehub_tmr_start(void);
void updatehub_tmr_stop(void);

#endif /* __UPDATEHUB_TIMER_H__ */
