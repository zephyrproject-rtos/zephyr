/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MAYFLY_CALL_ID_0       0
#define MAYFLY_CALL_ID_1       1
#define MAYFLY_CALL_ID_2       2
#define MAYFLY_CALL_ID_PROGRAM 3
#define MAYFLY_CALLER_COUNT    4
#define MAYFLY_CALLEE_COUNT    4

struct mayfly {
	uint8_t volatile _req;
	uint8_t _ack;
	memq_link_t *_link;
	void *param;
	void (*fp)(void *);
};

void mayfly_init(void);
void mayfly_enable(uint8_t caller_id, uint8_t callee_id, uint8_t enable);
uint32_t mayfly_enqueue(uint8_t caller_id, uint8_t callee_id, uint8_t chain,
		     struct mayfly *m);
void mayfly_run(uint8_t callee_id);

extern void mayfly_enable_cb(uint8_t caller_id, uint8_t callee_id, uint8_t enable);
extern uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id);
extern uint32_t mayfly_prio_is_equal(uint8_t caller_id, uint8_t callee_id);
extern void mayfly_pend(uint8_t caller_id, uint8_t callee_id);
