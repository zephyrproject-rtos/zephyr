/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MAYFLY_H_
#define _MAYFLY_H_

struct mayfly {
	uint8_t volatile _req;
	uint8_t _ack;
	void *_link;
	void *param;
	void (*fp)(void *);
};

void mayfly_init(void);
void mayfly_enable(uint8_t caller_id, uint8_t callee_id, uint8_t enable);
uint32_t mayfly_enqueue(uint8_t caller_id, uint8_t callee_id, uint8_t chain,
			struct mayfly *m);
void mayfly_run(uint8_t callee_id);

extern void mayfly_enable_cb(uint8_t caller_id, uint8_t callee_id,
			     uint8_t enable);
extern uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id);
extern uint32_t mayfly_prio_is_equal(uint8_t caller_id, uint8_t callee_id);
extern void mayfly_pend(uint8_t caller_id, uint8_t callee_id);

#endif /* _MAYFLY_H_ */
