/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
uint32_t mayfly_enqueue(uint8_t caller_id, uint8_t callee_id, uint8_t chain,
			struct mayfly *m);
void mayfly_run(uint8_t callee_id);

extern void mayfly_enable(uint8_t caller_id, uint8_t callee_id, uint8_t enable);
extern uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id);
extern uint32_t mayfly_prio_is_equal(uint8_t caller_id, uint8_t callee_id);
extern void mayfly_pend(uint8_t caller_id, uint8_t callee_id);

#endif /* _MAYFLY_H_ */
