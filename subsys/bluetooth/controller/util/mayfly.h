/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MAYFLY_H_
#define _MAYFLY_H_

#define MAYFLY_CALL_ID_0       0
#define MAYFLY_CALL_ID_1       1
#define MAYFLY_CALL_ID_2       2
#define MAYFLY_CALL_ID_PROGRAM 3
#define MAYFLY_CALLER_COUNT    4
#define MAYFLY_CALLEE_COUNT    4

struct mayfly {
	u8_t volatile _req;
	u8_t _ack;
	memq_link_t *_link;
	void *param;
	void (*fp)(void *);
};

void mayfly_init(void);
void mayfly_enable(u8_t caller_id, u8_t callee_id, u8_t enable);
u32_t mayfly_enqueue(u8_t caller_id, u8_t callee_id, u8_t chain,
		     struct mayfly *m);
void mayfly_run(u8_t callee_id);

extern void mayfly_enable_cb(u8_t caller_id, u8_t callee_id, u8_t enable);
extern u32_t mayfly_is_enabled(u8_t caller_id, u8_t callee_id);
extern u32_t mayfly_prio_is_equal(u8_t caller_id, u8_t callee_id);
extern void mayfly_pend(u8_t caller_id, u8_t callee_id);

#endif /* _MAYFLY_H_ */
