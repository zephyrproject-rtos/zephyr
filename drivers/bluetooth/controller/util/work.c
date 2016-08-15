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

#include <stdint.h>
#include "hal_irq.h"

#include "work.h"

static struct work *_work_head;

#ifdef __GNUC__
static inline uint32_t __disable_irq(void)
{
	uint32_t result;

	__asm__ volatile ("MRS %0, PRIMASK\n\t CPSID i":"=r" (result));

	return (result & 0x01);
}

static inline void __enable_irq(void)
{
	__asm__ volatile ("CPSIE i");
}
#endif

void work_enable(uint8_t group)
{
	irq_enable(group);
}

void work_disable(uint8_t group)
{
	irq_disable(group);
}

uint8_t work_enabled(uint8_t group)
{
	return irq_enabled(group);
}

uint32_t work_schedule(struct work *w, uint8_t chain)
{
	int was_masked = __disable_irq();
	struct work *prev;
	struct work *curr;

	/* Dequeue expired work at head */
	while ((_work_head)
	       && (_work_head->ack == _work_head->req)
	    ) {
		_work_head = _work_head->next;
	}

	/* Dequeue expired in between list and find last node */
	curr = _work_head;
	prev = curr;
	while (curr) {
		/* delete expired work */
		if (curr->ack == curr->req) {
			prev->next = curr->next;
		} else {
			prev = curr;
		}

		curr = curr->next;
	}

	/* chain, if explicitly requested, or if work not at current level */
	chain = chain || (!irq_priority_equal(w->group))
	    || (!irq_enabled(w->group));

	/* Already in List */
	curr = _work_head;
	while (curr) {
		if (curr == w) {
			if (!chain) {
				break;
			}

			if (!was_masked) {
				__enable_irq();
			}

			return 1;
		}

		curr = curr->next;
	}

	/* handle work(s) that can be inline */
	if (!chain) {
		w->req = w->ack;

		if (!was_masked) {
			__enable_irq();
		}

		if (w->fp) {
			w->fp(w->params);
		}

		return 0;
	}

	/* New, add to the list */
	w->req = w->ack + 1;
	w->next = 0;
	if (prev == curr) {
		_work_head = w;
	} else {
		prev->next = w;
	}

	irq_pending_set(w->group);

	if (!was_masked) {
		__enable_irq();
	}

	return 0;
}

void work_run(uint8_t group)
{
	int was_masked = __disable_irq();
	struct work *curr = _work_head;

	while (curr) {
		if ((curr->group == group) && (curr->ack != curr->req)) {
			curr->ack = curr->req;

			if (curr->fp) {
				if (curr->next) {
					irq_pending_set(group);
				}

				if (!was_masked) {
					__enable_irq();
				}

				curr->fp(curr->params);

				return;
			}
		}

		curr = curr->next;
	}

	if (!was_masked) {
		__enable_irq();
	}
}
